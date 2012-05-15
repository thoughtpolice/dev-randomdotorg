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
#include <linux/uaccess.h>
#include <linux/dns_resolver.h>
#include <net/sock.h>

#include "rdo.h"
#include "ioctl.h"
#include "config.h"

#include "http_parser.h"


/** Module parameters and other top-level definitions */

static uint nbufsz = 8192;
module_param(nbufsz, uint, S_IRUGO);

static int rdo_major = 0;
static struct rdo_dev* rdo_device;

static char* randomdotorg_ip = NULL;

/** Networking */

static unsigned int
inet_addr(char* ip)
{
  int a, b, c, d;
  char addr[4];

  // TODO: terrible
  sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d);
  addr[0] = a;
  addr[1] = b;
  addr[2] = c;
  addr[3] = d;
  
  return *(unsigned int *)addr;
}


static struct socket*
connect_host(char* ip)
{
  struct socket* sk = NULL;
  int result;

  result = sock_create(AF_INET, SOCK_STREAM, 0, &sk);
  if (result < 0) {
    belch(KERN_ERR, "Couldn't sock_create()");
    return NULL;
  }

  struct sockaddr_in addr = { 0, };
  addr.sin_family         = AF_INET;
  addr.sin_port           = htons(80);
  addr.sin_addr.s_addr    = inet_addr(ip);

  result = sk->ops->connect(sk, (struct sockaddr*)&addr, sizeof(struct sockaddr_in), 0);
  if (result < 0) {
    belch(KERN_ERR, "couldn't kconnect(\"%s:80\")!", ip);
    goto err;
  }
  
  belch(KERN_INFO, "connected to %s:80", ip);
  return sk;

 err:
  if (sk) {
    sk->ops->release(sk);
    sock_release(sk);
  }

  return NULL;
}

static int
send_host(struct socket* sk, char* buf)
{
  size_t len = strlen(buf)-1;

  struct msghdr msg;
  struct iovec iov;

  iov.iov_base = (void*)buf;
  iov.iov_len = (__kernel_size_t)len;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  return sock_sendmsg(sk, &msg, len);
}

static unsigned int
recv_host(struct socket* sk, char** buf, size_t count)
{
  char* kbuf = kmalloc(count, GFP_KERNEL);
  if(!kbuf)
    return -ENOMEM;

  memset(kbuf, 0x41, count);

  *buf = kbuf;
  return count;
}

static void
disconnect_host(struct socket* sk)
{
  if (sk) {
    belch(KERN_INFO, "disconnecting...");
    sk->ops->release(sk);
    sock_release(sk);
  }
}

static char*
get_randomdotorg_bytes(size_t num)
{
  int result;
  char* kbuf = NULL;

  // Connect
  struct socket* sk = connect_host(randomdotorg_ip);
  if (!sk) {
    belch(KERN_ERR, "could not connect to host");
    return NULL;
  }

  // Send GET request
  result = send_host(sk, "GET /cgi-bin/randbyte?nbytes=10&format=f\r\n");
  if (result < 0) {
    belch(KERN_ERR, "couldn't send GET request!");
    goto end;
  }

  // Get response
  result = recv_host(sk, &kbuf, num);
  if (result < 0) {
    belch(KERN_ERR, "couldn't receive %lu bytes!", num);
    kbuf = NULL;
    goto end;
  }

  // Done
 end:
  disconnect_host(sk);
  return kbuf;
}

static char*
resolve_dns_ip(char* dnsname)
{
  char* ipaddr = NULL;

  belch(KERN_INFO, "resolving IP for %s...", dnsname);

  int result = dns_query(NULL, dnsname, strlen(dnsname), NULL, &ipaddr, NULL);
  if(result < 0) {
    belch(KERN_ERR, "could not dns_query(\"%s\")!", dnsname);
    kfree(ipaddr);
    return NULL;
  }

  int count = result;
  char* buf = kmalloc(count+1, GFP_KERNEL);
  if(!buf) {
    belch(KERN_ERR, "kmalloc for resolve_dns_ip failed");
    kfree(ipaddr);
    return NULL;
  }

  memset(buf, 0, count+1);
  memcpy(buf, ipaddr, count);
  kfree(ipaddr);

  return buf;
}

/** File operations */

static ssize_t
rdo_read(struct file* filp, char __user* buff, size_t count, loff_t *offp)
{
  int ret = -EFAULT;

  belch(KERN_INFO, "rdo_read: reading %lu bytes", count);
  char* kbuf = get_randomdotorg_bytes(count);
  if(!kbuf) {
    ret = -EHOSTDOWN;
    goto end;
  }

  /* Make sure copy_to_user returns 0, i.e. '0 bytes remain
     to be copied' */
  if(copy_to_user(buff, kbuf, count) != 0) {
    ret = -EFAULT;
    goto err_1;
  }

  *offp += count;
  ret    = count;

 err_1:
  kfree(kbuf);

 end:
  return ret;
}

static long
rdo_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
  int ret = 0;

  if(_IOC_TYPE(cmd) != IOC_MAGIC) return -ENOTTY;
  if(_IOC_NR(cmd) > IOC_MAXNR) return -ENOTTY;

  switch (cmd) {
  case IOC_RESET:
    belch("ioctl(IOC_RESET)");
    break;

  case IOC_INFO:
    belch("ioctl(IOC_INFO)");
    break;

  default:
    return -ENOTTY;
  }

  return ret;
}

static int
rdo_open(struct inode* inode, struct file* filp)
{
  struct rdo_dev* dev;

  dev = container_of(inode->i_cdev, struct rdo_dev, cdev);
  filp->private_data = dev;

  return 0;
}

static int
rdo_release(struct inode* inode, struct file* filp)
{
  return 0;
}


struct file_operations rdo_fops = {
  .owner          = THIS_MODULE,
  .read           = rdo_read,
  .unlocked_ioctl = rdo_ioctl,
  .open           = rdo_open,
  .release        = rdo_release,
};


/** Initialization/teardown, and module descriptions/frontmatter */

static int __init
init_drv(void)
{
  int ret = -ENODEV;
  int result;
  dev_t dev;

  belch(KERN_INFO, "initializing");

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

  /* Allocation and initialization of character device */
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

  /* Register device callbacks via cdev_{init,add} */
  cdev_init(&rdo_device->cdev, &rdo_fops);
  rdo_device->cdev.owner = THIS_MODULE;
  rdo_device->cdev.ops   = &rdo_fops;
  result = cdev_add(&rdo_device->cdev, MKDEV(rdo_major, 0), 1);
  if(result < 0) {
    belch(KERN_ERR, "cdev_add() for rdo_dev failed in init_drv()");
    goto err_2;
  }

  /* Resolve random.org IP address initially */
  char* ip = resolve_dns_ip("random.org");
  if(!ip) {
    ret = -EHOSTDOWN;
    belch(KERN_ERR, "resolve_dns_ip() failed");
    goto err_3;
  }
  randomdotorg_ip = ip;
  belch(KERN_INFO, "resolved random.org ip (= %s)", randomdotorg_ip);

  /* Connect once */
  char* bytes = get_randomdotorg_bytes(nbufsz);
  if (!bytes) {
    ret = -EHOSTDOWN;
    belch(KERN_ERR, "Couldn't connect the first time!");
    goto err_3;
  }

  /* Done loading */
  belch(KERN_INFO, "loaded; nbufsz = %u", nbufsz);
  return 0;

  /* Error paths */
 err_3:
  kfree(randomdotorg_ip);
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
  if (randomdotorg_ip)
    kfree(randomdotorg_ip);

  if (rdo_device) {
    cdev_del(&rdo_device->cdev);
    kfree(rdo_device);
  }

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
