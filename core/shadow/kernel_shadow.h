#include <linux/module.h> /* Specifically, a module */
#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/proc_fs.h>  /* Necessary because we use the proc fs */
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>

#define MAX_LEN_MSG sizeof(int) + 1
#define PROCFS_NAME "mshadow"

static ssize_t 
procfile_write(struct file *flip, const char *buffer,
                       size_t buffer_length, loff_t *offset);

static int __init m_init(void);
static void __exit m_cleanup(void); 

static struct file_operations proc_dev_fops = {
  write: procfile_write
};
