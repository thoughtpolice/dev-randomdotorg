/*
 * Copyright (c) 2012 Austin Seipp
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/types.h>

#include "config.h"

/** Macros & types */

#define belch(lvl, ...) printk(lvl DRIVER_NAME ": " __VA_ARGS__)

struct rdo_dev {
  struct semaphore sem;
  struct cdev cdev;
};


/** Module parameters and other top-level definitions */

static uint nbufsz = 8192;
module_param(nbufsz, uint, S_IRUGO);

static int rdo_major = 0;
static struct rdo_dev* rdo_device;


/** File operations */

int
rdo_open(struct inode* inode, struct file* filp)
{
  struct rdo_dev* dev;

  dev = container_of(inode->i_cdev, struct rdo_dev, cdev);
  filp->private_data = dev;

  belch(KERN_INFO, "/dev/randomdotorg opened");

  return 0;
}

int
rdo_release(struct inode* inode, struct file* filp)
{
  return 0;
}

struct file_operations rdo_fops = {
  .owner     = THIS_MODULE,
  .open      = rdo_open,
  .release   = rdo_release,
};


/** Initialization/teardown, and module descriptions/frontmatter */

static int __init
init_drv(void)
{
  int ret = -ENODEV;
  int result;

  dev_t dev;

  if (nbufsz == 0) {
    belch(KERN_ERR, "nbufsz must be > 0");
    ret = -EINVAL;
    goto err;
  }
  if (nbufsz > 16384) {
    belch(KERN_ERR, "nbufsz must be < 16384");
    ret = -EINVAL;
    goto err;
  }

  /* Allocation of character device & initialization */
  result = alloc_chrdev_region(&dev, 0, 1, "randomdotorg");
  rdo_major = MAJOR(dev);
  if (result < 0) {
    belch("alloc_chrdev_region() failed");
    goto err;
  }

  rdo_device = kmalloc(sizeof(struct rdo_dev), GFP_KERNEL);
  if (!rdo_device) {
    belch(KERN_ERR, "kmalloc() for rdo_dev failed in init_drv()");
    ret = -ENOMEM;
    goto err_1;
  }
  memset(rdo_device, 0, sizeof(struct rdo_dev));
  sema_init(&rdo_device->sem, 1);

  /* cdev setup */
  cdev_init(&rdo_device->cdev, &rdo_fops);
  rdo_device->cdev.owner = THIS_MODULE;
  rdo_device->cdev.ops   = &rdo_fops;
  result = cdev_add(&rdo_device->cdev, MKDEV(rdo_major, 0), 1);
  if(result < 0) {
    belch(KERN_ERR, "cdev_add() for rdo_dev failed in init_drv()");
    goto err_2;
  }

  /* Done */
  belch(KERN_INFO, "loaded; nbufsz = %u", nbufsz);
  return 0;

  /* Error paths */
 err_2:
  kfree(rdo_device);
 err_1:
  unregister_chrdev_region(MKDEV(rdo_major, 0), 1);
 err:
  return ret;
}

static void __exit
exit_drv(void)
{
  if (rdo_device)
    kfree(rdo_device);

  if (rdo_major)
    unregister_chrdev_region(MKDEV(rdo_major, 0), 1);

  belch(KERN_INFO, "unloaded");
}


MODULE_AUTHOR("Austin Seipp <mad.one@gmail.com>");
/* Actually MIT, but I don't want the kernel to bitch... */
MODULE_LICENSE("Dual MIT/GPL"); 
MODULE_DESCRIPTION("Provides a /dev/randomdotorg device "
		   "for atmospheric random data");
MODULE_VERSION(DRIVER_VERSION);
module_init(init_drv);
module_exit(exit_drv);
