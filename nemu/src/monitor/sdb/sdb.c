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
  nemu_state.state = NEMU_QUIT; //Return elegantly
  return -1;
}


static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  uint64_t steps = (args != NULL) ? (uint64_t)atoi(arg) : 1;
  cpu_exec(steps);
  return 0;
}

static int cmd_info(char *args)
{
  char *arg = strtok(NULL, " ");
  if(arg == NULL) Log("The info arg should be r or w, not NULL\n");
  else if(arg != NULL && strcmp(arg, "r") == 0) isa_reg_display();
  else if(arg != NULL && strcmp(arg, "w") == 0) display_all_wp();
  else Assert(0, "Should not use any option except for r or w in info command\n");
//  if(arg == NULL || (*arg != 'r') || (*arg != 'w'))  Log("The info only supports register state observation or watchpoint observation, available usage is info r or info w");
//  else if (*arg == 'r') isa_reg_display();
  return 0;
}


word_t vaddr_read(vaddr_t addr, int len);

static int cmd_x(char *args)
{
  char *num = strtok(NULL, " ");
  if(num == NULL) 
  {
    Log("x pass NULL as num of bytes to scan, please use like x N <expr>");
    return 0;
  }
  char *expr = strtok(NULL, " ");
  if(expr == NULL)
  {
    Log("Please specify the expression of the address to scan, usage: x N <expr>");
    return 0;
  }
  Log("Reading %d byte(s) start in %s addr: ", atoi(num), expr);
  uint32_t nums = (uint32_t)atoi(num);
  uint32_t vaddr_base = (uint32_t)strtoul(expr, NULL, 16);

  for (uint32_t i = 0 ; i <= nums ; i++)
    {
      uint32_t val = (uint32_t)vaddr_read(vaddr_base + (i << 2), 4);
      printf("0x%.8"PRIx32":  0x%.8"PRIx32"\n", vaddr_base + (i << 2), val); //use 0x.8 to display the leading zeros
    }
  return 0;
}
//debug: fault expr (1 + 2 * 3) -( 4 + 6 / 2) 


static int cmd_p(char *args)
{
  bool suc = false;
  unsigned int calc_result = 0;
  if(args == NULL) Log("Invalid NULL expression, please specify the expression!");
  else calc_result = expr(args, &suc);
  if(suc) 
  {
    
    printf("Decimal Unsigned Value:%"PRIu32"  Hexadecimal Value: 0x%.8x \n", calc_result, calc_result);
    return 0;
  }
  else 
  {
    Log("Token extraction fault!");
    return -1;
  }
}



static int cmd_w(char *args)
{
  Assert(args != NULL, "Invalid watchpoint expression!");
  WP *new_node = new_wp(args);
  Assert(new_node != NULL, "Failed to alloc a watchpoint! WP pool is overflow!");
  Log("New watchpoint NO: %d, on expression %s", new_node->NO, new_node->watch_expr);
  return 0;
}
static int cmd_d(char *args)
{
  int no = atoi(args);
  free_wp(find_no_WP(no));
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
  { "si", "Execute single step instructions", cmd_si},
  { "info", "Print program running status, e.g: using info r to display register state, and using info w to display watchpoint information", cmd_info},
  { "x", "Usage: x N <expr>, Scan the memory begin with address calculated from <expr> and print successive N byte contents in hexadecimal.", cmd_x},
  { "p", "Usage: p <expr>, e.g: p $eax+1. calculate the expression <expr> and print", cmd_p},
  { "w", "Usage: w <expr>, setting watchpoint on <expr>, pause the procedure and print the expr whenever the expr varies value.", cmd_w},
  { "d", "Usage: d N, delete the watchpoint with id N", cmd_d},
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
        if (cmd_table[i].handler(args) < 0) { return; } //exec the cmd_q and gets -1, return the sdb_mainloop.
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
