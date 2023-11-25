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

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char watch_expr[65536]; //save the expression in a char[] string
  unsigned past_value; //save the past value of expr attained from expr
} WP;

word_t expr(char *e, bool *success);
void init_wp_pool();
WP* new_wp(char *expression);
void display_all_wp();
WP* find_no_WP(int id);
void free_wp(WP *wp);

void scan_all_watchpoints(bool *change);

word_t expr(char *e, bool *success);


#endif
