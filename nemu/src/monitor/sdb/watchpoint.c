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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char EXPR[32];
  uint32_t old_value;
  bool isused;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
	wp_pool[i].isused = false;
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
void wp_display()
{
   int i;
   int flag = true;
   printf("NO\tEXPR\tOld Value\n");
   for(i = 0; i < NR_WP; i++)
	   if (wp_pool[i].isused)
    	 {
	       printf("%d\t%s\t%d\n", wp_pool[i].NO, wp_pool[i].EXPR, wp_pool[i].old_value);
		   flag = false;
		 }
   if (flag) printf("No watchpoint now\n");
}

WP* new_wp()
{
  if(free_ == NULL) assert(0);
  else 
  {
    head = free_;
    free_ = free_->next;
  }
  head->isused = true;
  return head;
}

void free_wp(WP* wp)
{ 
  wp->isused = false;	
  if (head == wp)
  {
     head = NULL;
     wp->next = free_;
     free_ = wp;
  }
  else 
  {
    WP* last = head;
    for (WP* i = head->next; i != NULL; i = i->next, last = last->next)
    {
      if (i == wp)
      {
         last->next = i->next;
         wp->next = free_;
         free_ = wp;
      }
    }
  }
}

void check_watchpoints()
{
  for (int i = 0; i < NR_WP; i++)
  {
	if (wp_pool[i].isused)
    {
      bool success = false;
      int new_val = expr(wp_pool[i].EXPR, &success);
      if (success)
      {
        if (wp_pool[i].old_value != new_val)
        {
           printf("Old Value: %d\n", wp_pool[i].old_value);
		   printf("New Value: %d\n", new_val);
           wp_pool[i].old_value = new_val;
           nemu_state.state = NEMU_STOP;
        }
      }
      else printf("EXPR error\n");
	}
  }
}

WP* get_watchpoint(char* EXPR)
{
  WP* new_watchpoint = new_wp();
  strcpy(new_watchpoint->EXPR, EXPR);
  bool success = false;
  uint32_t new_val = expr(new_watchpoint->EXPR, &success);
  if (success) new_watchpoint->old_value = new_val;
  else
  {
    printf("EXPR errorr\n");
   assert(0);
  }
  return NULL;
}

void delete_watchpoint(int NO)
{
  int i;
  for(i = 0; i < NR_WP; i++)
  {
    if (wp_pool[i].NO == NO)
    {
	   free_wp(&wp_pool[i]);
       break;
	}

  }
  if (i == NR_WP) assert(0);
}
