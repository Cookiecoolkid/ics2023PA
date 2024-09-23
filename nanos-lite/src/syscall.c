#include <common.h>
#include "syscall.h"
#include <sys/time.h>
#include <proc.h>
// #define CONFIG_STRACE

void halt(int code);
extern int fs_open(const char *pathname, int flags, int mode);
extern size_t fs_read(int fd, void *buf, size_t len);
extern size_t fs_close(int fd);
extern size_t fs_write(int fd, const void *buf, size_t len);
extern size_t fs_lseek(int fd, size_t offset, int whence);
extern void naive_uload(PCB *pcb, const char *filename);

int sys_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	uint64_t us = io_read(AM_TIMER_UPTIME).us;
	tv->tv_sec = us / 1000000;
	tv->tv_usec = us % 1000000;
	return 0;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2, a[2] = c->GPR3, a[3] = c->GPR4; // Syscall arg1 arg2 arg3
  // printf("Syscall arg: %d, %d, %d, %d\n", a[0], a[1], a[2], a[3]);
#ifdef CONFIG_STRACE
  int type = c->GPR1;
  if (type == SYS_yield) printf("Syscall: SYS_yield() = 0\n");
  else if (type == SYS_exit) printf("Syscall: SYS_exit() - halt(%d)\n", a[1]);
  else if (type == SYS_write) printf("Syscall: SYS_write(fd = %d, buf = %d, count = %d) = %d\n", a[1], a[2], a[3], a[3]);
  else if (type == SYS_brk) printf("Syscall: SYS_brk(new _end = %d) = %d\n", a[1], 0);
  else if (type == SYS_open) printf("Syscall: SYS_open(path = %s, flags = %d, mode = %d)\n", (char*)a[1], a[2], a[3]);
  else if (type == SYS_read) printf("Syscall: SYS_read(fd = %d, buf = %d, count = %d) = %d\n", a[1], a[2], a[3], a[3]);
  else if (type == SYS_close) printf("Syscall: SYS_close(fd = %d) = %d\n", a[1], 0);
  else if (type == SYS_lseek) printf("Syscall: SYS_lseek(fd = %d, offset = %d, whence = %d)\n", a[1], a[2], a[3]);
  else if (type == SYS_gettimeofday) printf("Syscall: SYS_gettimeofday(tv = %d, tz = %d) = %d\n", a[1], a[2], 0);
  else if (type == SYS_execve) printf("Syscall: SYS_exxecve(filename = %s)\n", a[1]);
#endif
  switch (a[0]) {
	case SYS_yield: yield(); c->GPRx = 0; break;
	case SYS_exit: halt(a[1]); break;
	case SYS_write:
	{
		uintptr_t fd = a[1], buf = a[2], count = a[3];
		/*
		if (fd == 1 || fd == 2)
			for(int i = 0; i < count; i++)
				putch(*((char*)buf + i));
		else if (fd >= 3)
			fs_write(fd, (void*)buf, count);
		*/
		fs_write(fd, (void*)buf, count);
		c->GPRx = count;
		break;
	}
	case SYS_brk:
	{
		// malloc(a[1]);
		c->GPRx = 0; // Modifiied
		break;
	}
	case SYS_open:
	{
		char* pathname = (char*)a[1];
		int flags = (int)a[2];
		int mode = (int)a[3];
		size_t ret = fs_open(pathname, flags, mode);
		c->GPRx = ret;
		break;
	}
	case SYS_read:
	{
		uintptr_t fd = a[1], buf = a[2], len = a[3];
		size_t ret = fs_read(fd, (void*)buf, len);
		c->GPRx = ret;
		break; 
	}
	case SYS_close:
	{
		uintptr_t fd = a[1];
		size_t ret = fs_close(fd);
		c->GPRx = ret;
		break;
	}
	case SYS_lseek:
	{
		uintptr_t fd = a[1], offset = a[2], whence = a[3];
		size_t ret = fs_lseek(fd, (size_t)offset, (int)whence);
		c->GPRx = ret;
		break;
	}
	case SYS_gettimeofday:
	{
		struct timeval *tv = (struct timeval*)a[1];
		struct timezone *tz = (struct timezone*)a[2];
		sys_gettimeofday(tv, tz);
		c->GPRx = 0;
		break;
	}
	case SYS_execve:
	{
		const char* filename = (const char*)a[1];
		naive_uload(NULL, filename);
		c->GPRx = 0;
		break;
	}
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
