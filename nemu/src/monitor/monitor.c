/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/paddr.h>
#include <elf.h>
void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm(const char *triple);

static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
 // assert(0);
}

#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int difftest_port = 1234;
static char* elf_file = NULL;
static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}
#ifdef CONFIG_FTRACE
#include<common.h>
/*
typedef struct{
  char func_name[64];
  Elf32_Addr start;
  uint32_t size;
}func_table;
*/
func_table funcTable[256];

static void init_ftrace() {
	if (elf_file == NULL){
		Log("No elf is given.");
		return;  // if no return
	}
	//printf("%s\n", elf_file);
	FILE *fp = fopen(elf_file, "rb");
	
	Assert(fp, "Can not open '%s'", elf_file);
	
	
	fseek(fp, 0, SEEK_SET);
	Elf32_Ehdr elf_ehdr;
	Elf32_Shdr elf_shdr;
	//Elf32_Sym elf_symtab;
	//Elf32_Shdr elf_sym;
	Elf32_Shdr elf_strtab;
	
	uint16_t ret;
	//fseek(fp, 0, SEEK_END);
	//uint16_t eh_size = ftell(fp);
	ret = fread(&elf_ehdr, sizeof(Elf32_Ehdr), 1, fp);

	//uint16_t eh_size = elf_ehdr->e_ehsize;
	uint16_t shent_size = elf_ehdr.e_shentsize;
	
	Elf32_Addr shstr_addr = elf_ehdr.e_shoff + elf_ehdr.e_shentsize * elf_ehdr.e_shstrndx;
	//fread(elf_ehdr, 1, eh_size, fp);
	

	int sh_num = elf_ehdr.e_shnum;
	fseek(fp, elf_ehdr.e_shoff, SEEK_SET);
	Elf32_Addr str_addr;
	int i;
	for (i = 1; i <= sh_num; i++)
	{
		ret = fread(&elf_strtab, 1, shent_size, fp);
		assert(ret == shent_size);
		str_addr = elf_strtab.sh_offset;

		if(elf_strtab.sh_type == SHT_STRTAB && str_addr != shstr_addr)
			break;
	}
	assert(i <= sh_num);
	str_addr = elf_strtab.sh_offset;   // Why in gdb optimized out???;

	fseek(fp, elf_ehdr.e_shoff, SEEK_SET);
	for(int i = 0; i < sh_num; i++)
	{
		ret = fread(&elf_shdr, 1, shent_size, fp);
		assert(ret == shent_size);

		if(elf_shdr.sh_type == SHT_SYMTAB) break;
	}
	fseek(fp, elf_shdr.sh_offset, SEEK_SET);
	uint32_t sh_size = elf_shdr.sh_size;
	uint32_t sh_entsize = elf_shdr.sh_entsize;
	//elf_symtab = (Elf32_Sym*)malloc(sh_size);   //??
	uint32_t sym_num = sh_size / sh_entsize;

	Elf32_Sym elf_symtab[sym_num];

	// current fp is at section Symbol
	//ret = fread(&elf_symtab, sh_entsize, sym_num, fp);
	//assert(ret == sh_entsize * sym_num);
	for(int i = 0; i < sym_num; i++)
	{
		ret = fread(&elf_symtab[i], 1, sh_entsize, fp);
		assert(ret == sh_entsize);
	}

	// init funcTable
	int index = 0;
	
	for(int i = 0; i < sym_num; i++)
	{
		if(ELF32_ST_TYPE(elf_symtab[i].st_info) == STT_FUNC)
		{
			funcTable[index].size = elf_symtab[i].st_size;
			funcTable[index].start = elf_symtab[i].st_value;
			//printf("%x  %x\n", funcTable[index].start, elf_symtab[i].st_value);
			fseek(fp, elf_strtab.sh_offset + elf_symtab[i].st_name, SEEK_SET);
			ret = fscanf(fp, "%20s", funcTable[index].func_name);
			//printf("%x  %x\n", funcTable[index].start, elf_symtab[i].st_value);
			index++;
		}
	}
	printf("func_name:\t\t\tstart:\t\t\tsize:\n");
	for(int i = 0; i < index; i++)
		printf("%-20s\t\t%8x\t\t%d\n", funcTable[i].func_name, funcTable[i].start, funcTable[i].size);
	
	fclose(fp);
	
	
}
#endif
static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
	{"elf"      , required_argument, NULL, 'e'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:e:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
	  case 'e': elf_file = optarg; break;
      case 1: img_file = optarg; return 0;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
		printf("\t-e,--elf=FILE           get ELF FILE");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */
  
  cpu.mstatus = 0x1800;

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Set random seed. */
  init_rand();

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize ftrace*/
  IFDEF(CONFIG_FTRACE, init_ftrace());
  
  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize the simple debugger. */
  init_sdb();

#ifndef CONFIG_ISA_loongarch32r
  IFDEF(CONFIG_ITRACE, init_disasm(
    MUXDEF(CONFIG_ISA_x86,     "i686",
    MUXDEF(CONFIG_ISA_mips32,  "mipsel",
    MUXDEF(CONFIG_ISA_riscv,
      MUXDEF(CONFIG_RV64,      "riscv64",
                               "riscv32"),
                               "bad"))) "-pc-linux-gnu"
  ));
#endif

  /* Display welcome message. */
  welcome();
}
#else // CONFIG_TARGET_AM
static long load_img() {
  extern char bin_start, bin_end;
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}

void am_init_monitor() {
  init_rand();
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif
