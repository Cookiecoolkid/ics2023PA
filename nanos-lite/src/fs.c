#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

static int open_offset[64];
extern size_t serial_write(const void *buf, size_t offset, size_t len);
extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);
extern size_t events_read(void *buf, size_t offset, size_t len);
extern size_t dispinfo_read(void *buf, size_t offset, size_t len);
extern size_t fb_write(const void *buf, size_t offset, size_t len);
enum {FD_STDIN, FD_STDOUT, FD_STDERR, EVENTS, DISPINFO, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [EVENTS] = {"/dev/events", 0, 0, events_read, invalid_write},
  [DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
  [FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write},
#include "files.h"
};
/*
int fileTableSize()
{
  int ret = 0;
  while(file_table[ret].name[0] != '\0')
  {
	  printf("file_table[%d].name = %s\n", ret, file_table[ret].name);
	  ret++;
  }
  return ret;
}
*/
int fs_open(const char *pathname, int flags, int mode)
{
  // int ft_size = fileTableSize();
  int ft_size = 100;
  for(int i = 3; i < ft_size; i++)
	 if(!strcmp(file_table[i].name, pathname))
	 {
		  // printf("Succssfully Match Pathname\n");
		  return i;
	 }
  assert(0); // Should not reach here
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len)
{
  size_t offset = file_table[fd].disk_offset + open_offset[fd];
  // printf("open_offset + len = %d + %d = %d, Total_size = %d\n", open_offset[fd], len, open_offset[fd] + len, file_table[fd].size);
  if(open_offset[fd] + len > file_table[fd].size)
	  len = file_table[fd].size - open_offset[fd];
  assert(open_offset[fd] + len <= file_table[fd].size);
  if(file_table[fd].read != NULL) file_table[fd].read(buf, offset, len);
  else ramdisk_read(buf, offset, len);
  open_offset[fd] += len;
  return len;
}

size_t fs_close(int fd)
{
  open_offset[fd] = 0;
  return 0;
}

size_t fs_write(int fd, const void *buf, size_t len)
{
  size_t offset = file_table[fd].disk_offset + open_offset[fd];
  if(file_table[fd].write == NULL && open_offset[fd] + len > file_table[fd].size)
	  len = file_table[fd].size - open_offset[fd];
  // assert(open_offset[fd] + len <= file_table[fd].size);
  // Default: Call ramdisk_write()
  if (file_table[fd].write != NULL) 
	  file_table[fd].write(buf, offset, len);
  else ramdisk_write(buf, offset, len);
  open_offset[fd] += len;
  return len;
}

size_t fs_lseek(int fd, size_t offset, int whence)
{
  if(whence == SEEK_SET) open_offset[fd] = offset;
  else if (whence == SEEK_CUR) open_offset[fd] += offset;
  else if (whence == SEEK_END) open_offset[fd] = file_table[fd].size + offset;
  return open_offset[fd];
}


void init_fs() {
  // TODO: initialize the size of /dev/fb
  AM_GPU_CONFIG_T gpu_config = io_read(AM_GPU_CONFIG);
  int width = gpu_config.width;
  int height = gpu_config.height;
  file_table[FD_FB].size = width * height * 4;
}
