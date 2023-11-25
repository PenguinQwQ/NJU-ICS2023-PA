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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
word_t vaddr_read(vaddr_t addr, int len);

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_DEC, TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_LEFTP, TK_RIGHTP, 
  TK_NEG, TK_HEX, TK_REG, TK_NEQ, TK_LOGIC_AND, TK_DEREF,
  /* TODO: Add more token types */

};


static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces, actually same as \s+, one or more space
  {"\\+", TK_ADD},         // plus
  {"\\-", TK_SUB},
  {"\\*", TK_MUL}, 
  {"\\/", TK_DIV},
  {"0x[0-9a-fA-F]+", TK_HEX},
  {"(0|[1-9][0-9]*)", TK_DEC}, //avoid hex disturbing and leading zeros in dec is illegal
  {"\\(", TK_LEFTP},
  {"\\)", TK_RIGHTP},
  {"==", TK_EQ},        // equal
  //Further expression format

  {"\\$[prsgta\\$][apc0-9]", TK_REG},
  {"\\!=", TK_NEQ},
  {"&&", TK_LOGIC_AND},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[256];
} Token;

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

     //   Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
     //       i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        int pos = 0;
        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          case TK_ADD: tokens[nr_token++].type = TK_ADD; break;
          case TK_SUB: 
            if(nr_token == 0 || ((nr_token > 0) && ((tokens[nr_token - 1].type != TK_RIGHTP) && (tokens[nr_token - 1].type != TK_DEC) && (tokens[nr_token - 1].type != TK_HEX) && (tokens[nr_token - 1].type != TK_REG)))) tokens[nr_token++].type = TK_NEG;
            else tokens[nr_token++].type = TK_SUB; 
            break;
          case TK_MUL: 
            if(nr_token == 0 || ((nr_token > 0) && ((tokens[nr_token - 1].type != TK_RIGHTP) && (tokens[nr_token - 1].type != TK_DEC) && (tokens[nr_token - 1].type != TK_HEX) && (tokens[nr_token - 1].type != TK_REG)))) tokens[nr_token++].type = TK_DEREF;
            else tokens[nr_token++].type = TK_MUL; 
            break;
          case TK_DIV: tokens[nr_token++].type = TK_DIV; break;
          case TK_LEFTP: tokens[nr_token++].type = TK_LEFTP; break;
          case TK_RIGHTP: tokens[nr_token++].type = TK_RIGHTP; break;
          case TK_EQ: tokens[nr_token++].type = TK_EQ; break;
          case TK_DEC: 
            tokens[nr_token].type = TK_DEC; 
            pos = 0;
            if(*substr_start == '0') //simple 0
              {
                tokens[nr_token].str[pos] = '0';
                tokens[nr_token].str[pos + 1] = '\0';
                nr_token++;
                break;
              }
            while(pos <= 31 && (*substr_start) >= '0' && (*substr_start) <= '9')
            {
              tokens[nr_token].str[pos] = *substr_start;
              pos++; substr_start++;
            }
            tokens[nr_token].str[pos] = '\0';
            nr_token++;
            break;
          case TK_HEX:
            tokens[nr_token].type = TK_HEX;
            pos = 0;
            substr_start += 2;
            while(pos <= 31 && ((((*substr_start) >= '0' && (*substr_start) <= '9')) || ((*substr_start) >= 'a' && (*substr_start) <= 'f')))
            {
              tokens[nr_token].str[pos] = *substr_start;
              pos++; substr_start++;
            }
            tokens[nr_token].str[pos] = '\0';
            nr_token++;
            break;       
          case TK_LOGIC_AND: tokens[nr_token++].type = TK_LOGIC_AND; break;
          case TK_NEQ: tokens[nr_token++].type = TK_NEQ; break;
          case TK_REG: 
            tokens[nr_token].type = TK_REG;
            //save the reg name into str
            tokens[nr_token].str[0] = *substr_start++;
            tokens[nr_token].str[1] = *substr_start++;
            tokens[nr_token].str[2] = *substr_start;
            tokens[nr_token].str[3] = '\0';
            nr_token++;
            break;
          default: Log("Pattern match fault!"); break;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int l, int r) //this function is used to check if the farmost parenthese matches
{
  if((tokens[l].type != TK_LEFTP) || (tokens[r].type != TK_RIGHTP))
      return false;
  int stop = 0;
  for (int pos = l + 1 ; pos <= r - 1 ; pos++)
    {
      if(tokens[pos].type == TK_LEFTP)
        stop++;
      if(tokens[pos].type == TK_RIGHTP)
        stop--;
      if(stop < 0)
        return false;//not match as ')' is more than '('
    }
  if(stop != 0)
    return false; //not match as '(' != ')'
  return true;

  /*
  //use stack for left parenthese count
  int lpcnt = 0;
  for (int i = l ; i <= r ; i++)
    {
      if(tokens[i].type == TK_LEFTP) 
      {
        lpcnt++;
        continue;
      }
      if(tokens[i].type == TK_RIGHTP) 
      {
        lpcnt--;
        continue;
      }
      if(lpcnt == 0) return false; //Can't take off the parenthese
      if(lpcnt < 0)
      {
        Assert(lpcnt >= 0, "The left and right parenthese is not match!");
        return false;
      }
    }
  if(lpcnt == 0)
    return true;
  else
  {
    Assert(lpcnt >= 0, "The left and right parenthese is not match!");
    return false;
  }
  */
}

//debug: fault expr (1 + 2 * 3) -( 4 + 6 / 2) 
word_t eval(int l, int r) //calculate expr begin in l and end in r, so the solution for the whole expression is eval(0, nr_token-1)
{
  if(l > r)
    {
      Log("The left boundary exceed the right boundary!");
      Assert(0, "Bad Expression");
      return -1;
    }
  if (l == r)
  {
    if(tokens[l].type == TK_DEC) //Decimal Process
      return (word_t)strtoul(tokens[l].str, NULL, 10);
    if(tokens[l].type == TK_HEX)
      return (word_t)strtoul(tokens[l].str, NULL, 16);
    if(tokens[l].type == TK_REG)
      {
        bool suc;
        word_t val = isa_reg_str2val(tokens[l].str, &suc);
        if(suc) return val;
      //  else Assert(0, "Reg analysis failure!");
      }
  //  Assert(0, "Should not reach here");
    return 0;
  }
  //else, check if the parenthese can be taken off
  if(check_parentheses(l, r))
    return eval(l + 1, r - 1);
  //else it's a simlpe expression, we should scan the op tokens to decide how to solve it
  int inp = 0;
  int main_op = -1;
  int prior = 5;
  for (int i = l ; i <= r ; i++)
    {
      if(tokens[i].type == TK_LEFTP) 
      {
          inp++;
          continue;
      }
      if(tokens[i].type == TK_RIGHTP)
      {
          inp--;
          continue;
      }
      if(inp > 0 || tokens[i].type == TK_DEC || tokens[i].type == TK_HEX || tokens[i].type == TK_REG) continue; //in a parenthese, or simple decimal number
      if(inp == 0 && (tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ || tokens[i].type == TK_LOGIC_AND) && (prior >= 1))
      {
        main_op = i;
        prior = 1;
        continue;
      }
      if(inp == 0 && (tokens[i].type == TK_ADD || tokens[i].type == TK_SUB) && (prior >= 2))
      {
        main_op = i;
        prior = 2;
        continue;
      }
      if(inp == 0 && (tokens[i].type == TK_MUL || tokens[i].type == TK_DIV) && (prior >= 3))
      {
        main_op = i;
        prior = 3;
        continue;
      }
      if(inp == 0 && (tokens[i].type == TK_NEG || tokens[i].type == TK_DEREF) && (prior >= 4))
      {
        if(prior > 4) //if it is the first neg, we should place it as a main operator, else use the left.
          main_op = i;
        prior = 4;
        continue;//shouldn't use the rightmost as the main operator!
      }
    }
//  Log("main_op is %d", tokens[main_op].type);
  Assert((main_op >= l) && (main_op <= r), "Can't find valid main operator, expression invalid!");
  if(prior <= 3) //double operator number
  {
  word_t left = eval(l, main_op - 1);
  word_t right = eval(main_op + 1, r);
  switch(tokens[main_op].type)
  {
    case TK_ADD: return left + right;
    case TK_SUB: return left - right;
    case TK_MUL: return left * right;
    case TK_DIV: return left / right;
    case TK_EQ: return left == right;
    case TK_NEQ: return left != right;
    case TK_LOGIC_AND: return left && right;
    default: Assert(0, "Invalid main operator type!"); return -1;
  }
  }
  if(prior == 4) //its single number operation
  {
    Assert(tokens[main_op].type == TK_NEG || tokens[main_op].type == TK_DEREF, "The single operator isn't neg, so its invalid!");
   // int left = eval(l, main_op - 1), right = eval(main_op + 1, r);
    Assert(main_op == l, "Neg should be the same as l");
    if(tokens[main_op].type == TK_NEG)
    {
      word_t rev = -1;
      return rev * eval(main_op + 1, r);
    }
    else //deref, memory deref
      return (word_t)vaddr_read(eval(main_op + 1, r), 4);
  }
  Assert(0, "Shouldn't reach here!");
  return -1;
}




word_t expr(char *e, bool *success) {
  nr_token = 0;
  memset(tokens, 0, sizeof(tokens));
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  //Test marco for token extraction.
  /*
  for (int i = 0 ; i < nr_token ; i++)
    {
        switch (tokens[i].type) {
          case TK_ADD: Log("+"); break;
          case TK_SUB: case TK_NEG:
              Log("-"); break;
          case TK_MUL: Log("*"); break;
          case TK_DIV: Log("/"); break;
          case TK_LEFTP: Log("("); break;
          case TK_RIGHTP: Log(")"); break;
          case TK_EQ: Log("=="); break;
          case TK_DEC: 
              Log("%s", tokens[i].str);
              break;
          default: 
              Log("Invalid token!");
              *success = false;
              return 0;
        }
    }
    */
  word_t val = eval(0, nr_token - 1);
  *success = true;
  return val;
}