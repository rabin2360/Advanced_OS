#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/version.h>


MODULE_AUTHOR("Rabin Ranabhat");
MODULE_LICENSE("GPL");

//the default for mystring is "Rabin"
static char *mystring = "Rabin";

//declaring the input parameter
module_param(mystring, charp, 0);

int init_module(void)
{
  printk(KERN_ALERT "Hello world: I am %s speaking from the Kernel.\n", mystring);
  return 0;
}

void cleanup_module(void)
{
  printk(KERN_ALERT "Goodbye from %s, I am exiting the Kernel.\n", mystring);
}

