#include <proc.h>
#include <elf.h>
#include "../include/fs.h"
#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif
extern size_t fs_read(int fd, void *buf, size_t len);
extern size_t ramdisk_read(void* buf, size_t offset, size_t len);
extern int fs_open(const char *pathname, int flags, int mode);
extern size_t fs_lseek(int fd, size_t offset, int whence);
extern size_t fs_close(int fd);

uintptr_t loader(PCB *pcb, const char *filename) {
  printf("Begin fs_open\n");
  int fd = fs_open(filename, 0 ,0);
  Elf_Ehdr elf_ehdr;
  printf("Finish fs_open, Begin fs_read\n");
  // printf("before ramdisk_read()\n");

  size_t ret = fs_read(fd, (void *)&elf_ehdr, sizeof(Elf_Ehdr));
  // printf("ret = %d\n", ret);
  assert(ret == sizeof(Elf_Ehdr));  
  // printf("pass assert\n");
  assert(elf_ehdr.e_ident[0] == 0x7f && elf_ehdr.e_ident[1] == 'E' && elf_ehdr.e_ident[2] == 'L' && elf_ehdr.e_ident[3] == 'F');
  printf("Get Valid ELF\n");
  uint16_t phnum = elf_ehdr.e_phnum;
 //  uint16_t ph_size = elf_ehdr->e_phentsize;
  
  Elf_Phdr elf_phdr[phnum];
  printf("Reading ramdisk\n");

  fs_lseek(fd, elf_ehdr.e_phoff, SEEK_SET);
  ret = fs_read(fd, elf_phdr, sizeof(Elf_Phdr) * phnum);
  printf("Finish read\n", ret);
  int i;
  for(i = 0; i < phnum; i++)
  {
	  if(elf_phdr[i].p_type == PT_LOAD)
	  {
      	size_t vaddr = elf_phdr[i].p_vaddr;
	  	size_t filesz = elf_phdr[i].p_filesz;
	  	size_t memsz = elf_phdr[i].p_memsz;
		printf("Segment: i = %d, vaddr = %d, filesz = %d, memsz = %d\n", i, vaddr, filesz,memsz);
		fs_lseek(fd, elf_phdr[i].p_offset, SEEK_SET);
	  	fs_read(fd, (void*)vaddr, memsz);
	  	if(filesz < memsz) memset((void*)(vaddr + filesz), 0, memsz - filesz);
	  }
	  	  
  }
  fs_close(fd);
  return elf_ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  printf("Loading file: %s\n", filename);
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

