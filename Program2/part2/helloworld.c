#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/version.h>

//////////////////////added
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <asm/pgtable.h>
#include <asm/asm-offsets.h>

#define RET_OPOCDE	'\xc3'


static pte_t *pte;
static unsigned long **sysCallTable;
static unsigned long *sys_ni_syscall_ptr;

static size_t no_syscall_len;
/////////////////////////////end


MODULE_AUTHOR("Rabin Ranabhat");
MODULE_LICENSE("GPL");

//struct for process information
struct procinfo
{
 pid_t pid;
 pid_t ppid;
 int num_sib;
 u64 start_time;
};


asmlinkage void sys_get_proc_info(int pid, struct procinfo *info)
{
  //user entered pid value
  pid_t inputPid = pid;
  //user entered task
  struct task_struct *task;
  //parent of the user entered task
  struct task_struct *myParent;
  //head of the list
  struct list_head * head;
  
  struct procinfo myprocinfo;
  int siblingsCount = 0;

  struct pid *pid_struct;

  if(inputPid == 0)
   {
     //process the current process
     task = current;
   }
  else if (inputPid > 0)
   {
     //get task_struct of the input process
     pid_struct = find_get_pid(inputPid);
     task = pid_task(pid_struct, PIDTYPE_PID);
   }
  else
   {
     //process the parent of the current process
     task = current->parent;
   }

  //getting the parent
  myParent = task->parent;


  myprocinfo.pid = task->pid;
  myprocinfo.ppid = myParent->pid;

  printk(KERN_ALERT "my id is %d\n",myprocinfo.pid);
  info->pid = myprocinfo.pid;

  //this struct belongs to the parent of the current process. 
  printk(KERN_ALERT "my parent id is %d\n", myprocinfo.ppid);
  info->ppid = myprocinfo.ppid;

  //counting all the children linked to the parent to get the siblings
  list_for_each(head, &(myParent->children))
    {
      siblingsCount++;
    }

  myprocinfo.num_sib = siblingsCount;

  printk(KERN_ALERT "siblings %d\n", myprocinfo.num_sib);
  info->num_sib = siblingsCount;

  myprocinfo.start_time = task->real_start_time;
  printk(KERN_ALERT "Start time %llu\n", myprocinfo.start_time);
  info->start_time = myprocinfo.start_time;
}

/////////////////////from this point on 

static asmlinkage long no_syscall(void)
{
	return -ENOSYS;
}

static inline void set_no_syscall_len(void){
	int i = 0;
	
	/* figure out the size of function */
	while(((char *)no_syscall)[i] != RET_OPOCDE)
		i++;
	
	no_syscall_len = (i + 1);
}

/* Restore kernel memory page protection */
static inline void protect_memory(void){
	set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
}

/* Unprotected kernel memory page containing for writing */
static inline void unprotect_memory(void){
	set_pte_atomic(pte, pte_mkwrite(*pte));
}

//search for sys call table using __NR_close
//if page table is found, it returns the address of it
//otherwise, NULL is returned
static inline unsigned long **findSyscallTable(void){
	unsigned long **sys_table;
	unsigned long offset = PAGE_OFFSET;
	
	while(offset < ULONG_MAX){
		sys_table = (unsigned long **)offset;
		
		if(sys_table[__NR_close] == (unsigned long *)sys_close)
			return sys_table;
		
		offset += sizeof(void *);
	}
	
	return NULL;
}

//finding free position within the page table
static inline int find_free_position(void){
	int i;
	int pos = -1;
	
	//determines the index in the sys call table that is empty
	for(i = 0; i <= __NR_syscall_max; i++){
		if(memcmp(sysCallTable[i], no_syscall, no_syscall_len) == 0){
			pos = i;
			break;
		}
	}
	
	return pos;
}

//register the system call dynamically
static int register_syscall(void *systemCall){
	
  int sysCallNumber;
  spinlock_t my_lock;
	
  spin_lock_init(&my_lock);
	
  //lock
  spin_lock(&my_lock);
	
  if((sysCallNumber = find_free_position()) < 0)
    {
      return -1;
    }
	
	
  unprotect_memory();
  sysCallTable[sysCallNumber] = systemCall;
  protect_memory();
	
  //unlock
  spin_unlock(&my_lock);
	
 return sysCallNumber;
}

static void unregister_syscall(void *systemCall){
  
  int sysCallNumber = 0;
	
   while(sysCallNumber <= __NR_syscall_max)
     {
     if(sysCallTable[sysCallNumber] == systemCall)
       {
	 break;
       }	
       
     sysCallNumber++;
     }

   unprotect_memory();
   //restoring the value in the syscall table
   sysCallTable[sysCallNumber] = sys_ni_syscall_ptr;
   protect_memory();
}


static int dynamicSystemcallAddInit(void){
  
  unsigned int level;
  int i = 0;
  sys_ni_syscall_ptr = NULL;
	
  sysCallTable = findSyscallTable();
	
  if(!sysCallTable)
    {
     printk(KERN_INFO "System Call table not found\n");
     return 3;
    }
	
  if(!(pte = lookup_address((unsigned long)sysCallTable, &level)))
    {
      return 3;    
    }

  set_no_syscall_len();
	
  if((i = find_free_position()) < 0)
    {
      return 3;
    }
	
  /* retrieves the original pointer to sys_ni_syscall */
  sys_ni_syscall_ptr = sysCallTable[i];
	
      return 2;
}

///////////////////end

int init_module(void)
{
  //printk(KERN_ALERT "Hello world: I am %s speaking from the Kernel.\n", mystring);
   int systableInitialize;
   int sysNum;

   //find the system table and the index of an empty spot
   systableInitialize = dynamicSystemcallAddInit();
   
   //either system table initialization failed or no empty spots
   if(systableInitialize == 3)
     {
        printk(KERN_ALERT "System table NOT found.\n");
       return -1;
     }

   //attempt to register my sys call
   sysNum = register_syscall(sys_get_proc_info);

   //registration of sys call failed
   if(sysNum < 0)
     {
       printk(KERN_ALERT "System call was NOT registered.\n");
       return -1;
     }

   //all the above steps passed and the sys call was registered dynamically
   printk(KERN_ALERT "System call was registered. Syscall # %d\n", sysNum);
  return 0;
}

void cleanup_module(void)
{
  //replce the registered sys call with sys_ni_syscall pointer
  unregister_syscall(sys_get_proc_info);
  //notifying the user about cleanup
  printk(KERN_ALERT "Bye now :)\n");

  //printk(KERN_ALERT "Goodbye from %s, I am exiting the Kernel.\n", mystring);
}

//module_init(init_module);
//module_exit(cleanup_module);
