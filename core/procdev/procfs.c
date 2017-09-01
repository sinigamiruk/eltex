#include <linux/module.h> /* Specifically, a module */
#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/proc_fs.h>  /* Necessary because we use the proc fs */
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#define MAX_LEN_MSG 10
#define procfs_name "helloworld"

char msg[MAX_LEN_MSG];
struct proc_dir_entry *m_proc_dir;

ssize_t procfile_read(struct file *flip, char *buffer, 
                      size_t buffer_length, loff_t *offset);

ssize_t procfile_write(struct file *flip, const char *buffer,
                       size_t buffer_length, loff_t *offset);

void m_proc_cleanup(void); 
int m_proc_init(void);

struct file_operations proc_dev_fops = {
  read: procfile_read,
  write: procfile_write
};

ssize_t
procfile_read(struct file *flip, char *buffer, 
              size_t buffer_length, loff_t *offset)
{
  static int f_read = 0;

  copy_to_user(buffer, msg, MAX_LEN_MSG);
  printk(KERN_INFO "procfile_read read %d bytes", MAX_LEN_MSG);
  
  if (f_read == 1) {
    f_read = 0;
    return 0;
  }

  f_read = 1;
  return strlen(buffer);
}

ssize_t
procfile_write(struct file *flip, const char *buffer, 
               size_t buffer_length, loff_t *offset)
{
  int count_read = 0;
  static int f_write = 0;
  
  if (f_write == 1) {
    return strlen(buffer);
  }

  count_read = strncpy_from_user(msg, buffer, MAX_LEN_MSG - 1);
  printk(KERN_INFO "procfile_write write to kernel %d bytes\nstrlenmsg = %d", 
        count_read, (int) strlen(msg));

  f_write = 1;
  return count_read;
}

int m_proc_init()
{
  m_proc_dir = proc_create(procfs_name, 0644, NULL, &proc_dev_fops);
  
  if (m_proc_dir == NULL) {
    remove_proc_entry(procfs_name, NULL);
    printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
                     procfs_name);
    return -ENOMEM;
  }
  
  printk(KERN_INFO "/proc/%s created\n", procfs_name);  
  return 0; 
}

void m_proc_cleanup() 
{
  remove_proc_entry(procfs_name, NULL);
  printk(KERN_INFO "/proc/%s removed\n", procfs_name);
}

MODULE_LICENSE("GPL"); 
module_init(m_proc_init);
module_exit(m_proc_cleanup);
