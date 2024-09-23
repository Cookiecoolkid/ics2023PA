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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "../../../include/memory/vaddr.h"
static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  nemu_state.state = NEMU_END;
  return -1;
}

static int cmd_si(char* args){
  char* arg = strtok(NULL, " ");
  int n;
  if(arg == NULL) n = 1;
  else n = atoi(arg);
  cpu_exec(n);
 // printf("%d", n);

  return 0; 
}

static int cmd_info(char* args){
  char* arg = strtok(NULL, " ");
  if(arg == NULL) printf("No argument provided\n");
  else if(strcmp(arg, "r") == 0)
    isa_reg_display();
  else if (strcmp(arg, "w") == 0)
    wp_display();
   
  return 0;
}

static int cmd_x(char* args){
  char* arg1 = strtok(NULL, " ");
  char* arg2 = strtok(NULL, " ");
  if (arg1 == NULL || arg2 == NULL) printf("invalid arguments\n");
  else
  {
	int n = atoi(arg1);
	unsigned int EXPR;
	sscanf(arg2, "%x", &EXPR);
       //	vaddr_read(EXPR, n);
	for(int i = 1; i <= n; i++)
	{
	    printf("%08x\n", vaddr_read(EXPR, 4));
	    EXPR += 4;
	}
       
  }
  return 0;
}

static int cmd_p(char* args){
  char* EXPR = args;
  if (EXPR == NULL) printf("invalid arguments\n");
  bool success = true;
  expr(EXPR, &success);
  return 0;
}

static int cmd_w(char* args){
  char* EXPR = strtok(NULL, " ");
  if (EXPR == NULL) 
  {
     printf("No argument");
     assert(0);
  }
  get_watchpoint(EXPR);
  return 0;
}

static int cmd_d(char*args){
  char* number = strtok(NULL, " ");
  if (number == NULL)
  {
     printf("No argument");
     assert(0);
  }
  int No = atoi(number);
  delete_watchpoint(No);
  return 0;
}



static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  {"si", "Make the program execute N instructions and then pause,defaulting to 1 if N is not provided.", cmd_si},
  {"info", "When SUBCMD is 'r', then print register status, and for 'w', then print watchpoint infomation", cmd_info},
  {"x", "This cmd has argument 'N' and 'EXPR', Calculate the value of expression EXPR and use the result as the starting memory address. Output N consecutive 4-byte values in hexadecimal format.", cmd_x},
  {"p", "This cmd has argument 'EXPR', Calculate the value of expression EXPR", cmd_p},
  {"w", "This cmd has argument 'EXPR', Pause program execution when the value of expression EXPR changes.", cmd_w},
  {"d", "This cmd has argument 'N', Delete watchpoint with index N", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
