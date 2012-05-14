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

#include "config.h"

/** Macros, module parameters and top-level definitions */

#define belch(lvl, ...) printk(lvl DRIVER_NAME ": " __VA_ARGS__)

/* How many bytes to read from random.org at a time */
static uint nbufsz = 16384;
module_param(nbufsz, uint, S_IRUGO);


static int rdo_major = 0;

/** Initialization/teardown, and module descriptions/frontmatter */

static int __init
init_drv(void)
{
  int ret = -ENODEV;
  int result;

  dev_t rdo_dev;

  if (nbufsz == 0) {
    belch(KERN_ERR, "nbufsz must be > 0");
    ret = -EINVAL;
    goto err;
  }

  result = alloc_chrdev_region(&rdo_dev, 0, 1, "randomdotorg");
  if (result < 0) {
    belch("alloc_chrdev_region() failed");
    goto err;
  }

  rdo_major = MAJOR(rdo_dev);

  belch(KERN_INFO, "loaded; nbufsz = %u", nbufsz);
  return 0;

 err:
  return ret;
}

static void __exit
exit_drv(void)
{
  if (rdo_major)
    unregister_chrdev_region(MKDEV(rdo_major,0), 1);

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
