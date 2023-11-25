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



static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char *expression)
{
  if(free_ == NULL)
    {
      Log("There is no available watchpoint! Failed to alloc a new watchpoint!");
      return NULL;
    }
  WP* new_node = free_;
  free_ = free_->next; //Minus one
  strcpy(new_node->watch_expr, expression);
  bool suc = false;
  unsigned res = expr(expression, &suc);
  if(suc) new_node->past_value = res;
  else Log("Failed to calculate the expression of watchpoint!");
  //update the watchpoint list, using the head insert
  new_node->next = head;
  head = new_node;
  return new_node;
}



void display_all_wp()
{
  WP* cur = head;
  while(cur != NULL)
  {
    Log("Watchpoint NO: %d, watch_expr: %s, latest value: 0x%.8x", cur->NO, cur->watch_expr, cur->past_value);
    cur = cur->next;
  }
}

WP* find_no_WP(int id)
{
  WP* cur = head;
  while(cur != NULL)
    {
      if(cur->NO == id) return cur;
      else cur = cur->next;
    }
  Log("Failed to find NO:%d watchpoint in the pool! There are these watchpoint available:", id);
  display_all_wp();
  return NULL;
}

void free_wp(WP *wp)
{
  Assert(wp != NULL, "The watchpoint to be freed shouldn't be NULL!");
  memset(wp->watch_expr, 0, sizeof(wp->watch_expr)); //erase the watch expr
  wp->past_value = 0;
  if(head == wp) //the to release watchpoint is on the head
  {
    head = head->next;
    wp->next = free_;
    free_ = wp;
    Log("Free watchpoint on head successfully!");
    return;
  }
  else
  {
    WP *prev = head;
    while(prev->next != wp) prev = prev->next;
    WP *nxt = wp->next;
    if(nxt == NULL) //wp is the tail of the list
      {
        prev->next = NULL;
        wp->next = free_;
        free_ = wp;
        Log("Free watchpoint on tail successfully!");
        return;
      }
    else //wp is the mid  
      {
        prev->next = nxt;
        wp->next = free_;
        free_ = wp;
        Log("Free watchpoint in mid successfully!");
        return;
      }
  }
  Assert(0, "Should not reach here!!!");
}

void scan_all_watchpoints(bool *change)
{
  WP* cur = head;

  while(cur != NULL)
  {
    bool suc = false;
    unsigned now_val = expr(cur->watch_expr, &suc);
    Assert(suc, "Failed to calculate the expression!");
    if(now_val != cur->past_value)
    {
        *change = true;
        Log("Watchpoint NO: %d, watch_expr: %s, past value: 0x%.8x, latest value: 0x%.8x", cur->NO, cur->watch_expr, cur->past_value, now_val);
        cur->past_value = now_val;
    }
    cur = cur->next;
  }
}
