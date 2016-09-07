#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm-generic/uaccess.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>


MODULE_AUTHOR("Rabin Ranabhat");
MODULE_LICENSE("GPL");

static char msg[128];
static int len = 0;
static int len_check = 1;

struct procinfo
{
 pid_t pid;
 pid_t ppid;
 u64 start_time;
 int num_sib;
};

struct procinfo myprocinfo;

static int procRead(struct file *sp_file,char __user *buf, size_t size, loff_t *offset)
{

  //need this to avoid infinite loop while writing to the proc file
  if (len_check)
    {
      len_check = 0;    
    }
  else
    {
      len_check = 1;
      return 0;
    }

    printk(KERN_INFO "LENGTH - proc called read %zu\n",len);
    printk(KERN_INFO "INFO - proc called read %d\n",myprocinfo.pid);
	//printk(KERN_INFO "test buff %s\n", testBuff);
	//memcpy(buf, myprocinfo.pid, sizeof myprocinfo.pid);
	//put_user(msg, strlen(msg));
	
    copy_to_user(buf, msg,len);
        
 	//strcpy(msg, "testing\n");

	//sprintf(buf, "%d\n", myprocinfo.ppid);
	//sprintf(buf + strlen(buf), "%s\n", "ab");
	//copy_to_user(buf, msg, strlen(msg));
	//put_user(msg, strlen(msg));
	
	return len;
}

static int procWrite(struct file *sp_file,const char __user *buf, size_t size, loff_t *offset)
{

	printk(KERN_INFO "proc called write %zu\n",size);
	len = size;
	//reading the PID from the userspace buffer to msg buffer in kernel space
	copy_from_user(msg,buf,len);

	//converting the string to int
	long pidValue;
	pidValue = simple_strtol(msg, NULL, 10);
	
	//declarations so that required values from sched.h could be read
	  struct task_struct *task;
	  struct task_struct *myParent;
	  int siblingsCount = 0;
	  struct list_head * p;
	  
	  struct pid *pid_struct;

	  //when pid valud is zero, set the task to current task
	 if(pidValue == 0)
	{
	  //process the current process
	  printk(KERN_ALERT "inside the current process\n");
	  task = current;
	}
	 //when the PID value is greater than zero, set the task to the input pid value
	 else if (pidValue > 0)
	{
	  //get task_struct of the input process
	  pid_struct = find_get_pid(pidValue);
	  task = pid_task(pid_struct, PIDTYPE_PID);
	}
	 //if pid value is negative, set the task to the parent of the current task
	 else
	{
	  //process the parent of the current process
	  task = current->parent;
	}

	 //getting the parent of the current task
	 myParent = task->parent;

	 //getting the pid of the current task and the parent
	 myprocinfo.pid = task->pid;
	 myprocinfo.ppid = myParent->pid;

	 printk(KERN_ALERT "Process ID: %d\n",myprocinfo.pid);

	 //this struct belongs to the parent of the current process. 
	 printk(KERN_ALERT "Parent ID: %d\n", myprocinfo.ppid);

	 //assess the children of my parent i.e. counting the siblings
	 list_for_each(p, &(myParent->children))
	{
	  siblingsCount++;
	}
	 //setting the siblings value to the struct
	 myprocinfo.num_sib = siblingsCount;

	 printk(KERN_ALERT "Siblings Count: %d\n", myprocinfo.num_sib);
	 
	 //getting the task start time
	 myprocinfo.start_time = task->real_start_time;
	 printk(KERN_ALERT "Start Time: %llu\n", myprocinfo.start_time);

	return len;
}

//for this program, the proc file opreations that are defined are - read and write
struct file_operations fops = {
  .read = procRead,
  .write = procWrite,
};

//creating the proc file and also defining the file operations for the proc file
static int __init procFileInit (void)
{
  printk(KERN_INFO "Proc: Initialize proc file.\n");

  if (! proc_create("get_proc_info",0666,NULL,&fops)) 
    {
      printk(KERN_INFO "Proc: Could not create proc file\n");
      //if proc file init fails, unregister the procfile
      remove_proc_entry("get_proc_info",NULL);
      return -1;
    }

  return 0;

}

//exit file sequence for proc file, remove proc entry and then declare in dmesg that proc exit is completed.
static void __exit procFileExit(void)
{
  remove_proc_entry("get_proc_info",NULL);
  printk(KERN_INFO "Proc: Exit proc file\n");
}

//defining the init and exit modules for proc file
module_init(procFileInit);
module_exit(procFileExit);
