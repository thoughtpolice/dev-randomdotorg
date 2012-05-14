#ifndef _RDO_H_
#define _RDO_H_

/** Macros & types */
#define belch(lvl, ...) printk(lvl DRIVER_NAME ": " __VA_ARGS__)

struct rdo_dev {
  struct semaphore sem;
  struct cdev cdev;
};

#endif /* _RDO_H_ */
