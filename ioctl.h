/*
** This file uses
*/
#ifndef _RDO_IOCTL_H_
#define _RDO_IOCTL_H_

#include <linux/ioctl.h>

/** Defines all driver ioctls */
#define IOC_MAGIC 'k'
#define IOC_MAXNR 1 /* Must be kept in sync with DRV_IOCTLS below */
#define DRV_IOCTLS(_)				\
  _(0, RESET, __)				\
  _(1, INFO,  R,  int)


/** Defines all ioctl constants */
/* Shouldn't need modification past this */
#define _IO___(m, x, ...) _IO(m, x)
#define _IO_R(m, x, ...) _IOR(m, x, __VA_ARGS__)
#define _IO_W(m, x, ...) _IOW(m, x, __VA_ARGS__)
#define _IO_RW(m, x, ...) _IOR(m, x, __VA_ARGS__)
#define IOCTL_DEFS(n, name, prop, ...)		\
  IOC_##name = _IO_##prop(IOC_MAGIC, n, __VA_ARGS__),
enum ioctl_enum_t {
DRV_IOCTLS(IOCTL_DEFS)
};
#undef IOCTL_DEFS
#undef _IO___
#undef _IO_R
#undef _IO_W
#undef _IO_RW

#endif /* _RDO_IOCTL_H_ */
