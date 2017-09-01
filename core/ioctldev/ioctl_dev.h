#ifndef IOCTLDEV_H
#define IOCTLDEV_H

#include <linux/ioctl.h>

#define MAJOR_NUM 100

#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, char *)

#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *)

#define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int)

#define DEVICE_FILE_NAME "ioctl_dev"

#endif
