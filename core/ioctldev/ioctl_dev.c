#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/module.h> /* Specifically, a module */
#include <linux/fs.h>
#include <asm/uaccess.h>  /* for get_user and put_user */

#include "ioctl_dev.h"
#define SUCCESS 0
#define DEVICE_NAME "hello_dev"
#define BUF_LEN 80
static int Device_Open = 0;

static char msg[BUF_LEN];
static char *msg_ptr;

static int device_open(struct inode *inode, struct file *file)
{
  if (Device_Open)
    return -EBUSY;

  Device_Open++;

  msg_ptr = msg;
  return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
  Device_Open--;

  return SUCCESS;
}

static ssize_t 
device_read(struct file *file, char __user * buffer, 
            size_t length, loff_t * offset)
{
  int bytes_read = 0;
  if (*msg_ptr == 0)
    return 0;
  
  while (length && *msg_ptr) {
    put_user(*(msg_ptr++), buffer++);
    length--;
    bytes_read++;
  }

  return bytes_read;
}

static ssize_t
device_write(struct file *file, const char __user * buffer, 
             size_t length, loff_t * offset)
{
  int i;

  i = strncpy_from_user(msg, buffer, BUF_LEN);
  msg_ptr = msg;

  return i;
}

long device_ioctl(struct file *file, unsigned int ioctl_num, 
                 unsigned long ioctl_param)
{
  int i;
  char ch;

  switch (ioctl_num) {
    case IOCTL_SET_MSG:
      i = strncpy_from_user(msg, (char*)ioctl_param, BUF_LEN);
      device_write(file, (char *)ioctl_param, i, 0);
      break;
  
    case IOCTL_GET_MSG:
      i = device_read(file, (char *)ioctl_param, 99, 0);
  
      put_user('\0', (char *)ioctl_param + i);
      break;
  
    case IOCTL_GET_NTH_BYTE:
      return msg[ioctl_param];
      break;
    }

  return SUCCESS;
}

struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .unlocked_ioctl = device_ioctl,
  .open = device_open,
  .release = device_release,  /* a.k.a. close */
};

int m_init_ioctl(void)
{
  int ret_val;
 
  ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);

  if (ret_val < 0) {
      printk(KERN_ALERT "%s failed with %d\n",
                     "Sorry, registering the character device ", ret_val);
      return ret_val;
    }

  printk(KERN_INFO "%s The major device number is %d.\n",
               "Registeration is a success", MAJOR_NUM);
  printk(KERN_INFO "If you want to talk to the device driver,\n");
  printk(KERN_INFO "you'll have to create a device file. \n");
  printk(KERN_INFO "We suggest you use:\n");
  printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
  printk(KERN_INFO "The device file name is important, because\n");
  printk(KERN_INFO "the ioctl program assumes that's the\n");
  printk(KERN_INFO "file you'll use.\n"); 

  return 0;
}

void m_cleanup_ioctl(void) 
{
  unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

MODULE_LICENSE("GPL");  
module_init(m_init_ioctl); 
module_exit(m_cleanup_ioctl);
