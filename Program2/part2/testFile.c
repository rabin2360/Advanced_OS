#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <errno.h>
#include <linux/types.h>


struct procinfo
{
  pid_t pid;
  pid_t ppid;
  int num_sib;
  __u64  start_time;
};

struct procinfo info;

int main()
{
  int rc;
  int value = 2584;
  info.pid = 10;

  rc = syscall(134, value, &info);
  
  printf("pid %d\n", info.pid);
  printf("father %d\n", info.ppid);
  printf("siblings %d\n", info.num_sib);
  printf("start time %llu\n", info.start_time);
  return 0;
}
