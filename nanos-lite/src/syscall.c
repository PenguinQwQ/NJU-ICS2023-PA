#include <common.h>
#include "syscall.h"


enum {
  _SYS_exit,
  _SYS_yield,
  _SYS_open,
  _SYS_read,
  _SYS_write,
  _SYS_kill,
  _SYS_getpid,
  _SYS_close,
  _SYS_lseek,
  _SYS_brk,
  _SYS_fstat,
  _SYS_time,
  _SYS_signal,
  _SYS_execve,
  _SYS_fork,
  _SYS_link,
  _SYS_unlink,
  _SYS_wait,
  _SYS_times,
  _SYS_gettimeofday
};


void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  switch (a[0]) {
    case _SYS_yield:
      printf("Calling Syscall yield!\n");
      yield(); 
      c->GPRx = 0; //return 0 as yield return value
      break;
    case _SYS_exit:
      halt(a[0]);
      c->GPRx = 0;
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
