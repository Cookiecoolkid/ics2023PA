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

  /* TODO: Add more token types */
  TK_NUM, DEREF, TK_LOGIC_AND, TK_NEQ, TK_HEX, TK_REG, TK_LOGIC_OR, TK_NEG,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", '-'},          // sub
  {"\\/", '/'},          // divide
  {"\\*", '*'},         // multiply
  {"\\(", '('},          // left parentheses
  {"\\)", ')'},          // right parenthese
  {"0[xX][a-fA-F0-9]{1,8}", TK_HEX }, // hexadecimal
  {"0|[1-9][0-9]*", TK_NUM},          // number
  {"&&", TK_LOGIC_AND},  // logic and
  {"\\|\\|", TK_LOGIC_OR},    // logic or
  {"!=", TK_NEQ},        // not equal
  {"\\$[a-zA-Z]+[0-9]*|\\$pc|\\$\\$0",TK_REG}, // $ + reg_name
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
  char str[32];
} Token;

//static Token tokens[256] __attribute__((used)) = {};

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

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
    	if (substr_len >= 32) assert(0);

        Token recognized_token;
        //recognized_token.type = rules[i].token_type;
    	//strncpy(recognized_token.str, e + position - substr_len, substr_len);
        switch (rules[i].token_type) {
          case TK_NOTYPE:break; 

          case TK_NUM:{
              tokens[nr_token].type = TK_NUM;
              strncpy(tokens[nr_token++].str, e + position - substr_len, substr_len); break;} 

          case DEREF: {
              tokens[nr_token].type = DEREF;
              strncpy(tokens[nr_token++].str, e + position - substr_len, substr_len); break;}
    
          case TK_HEX:{
              tokens[nr_token].type = TK_HEX;
              strncpy(tokens[nr_token++].str, e + position - substr_len, substr_len); break;}
          case TK_REG:{
              tokens[nr_token].type = TK_REG;
              strncpy(tokens[nr_token++].str, e + position - substr_len + 1, substr_len - 1); break;}
    	  default:{ 
              recognized_token.type = rules[i].token_type;
              tokens[nr_token++] = recognized_token;
              }
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
static bool check_parentheses(int st, int ed)
{
  if (tokens[st].type == '(' && tokens[ed].type == ')')
  {
    int delta = 0;
    for(int i = st; i <= ed; i++)
    {
      if (tokens[i].type == '(') delta++;
      else if(tokens[i].type == ')') delta--;
      if(delta < 0 || (i != ed && delta == 0 )|| (i == ed && delta!=0)) return false;
    }
    return true;
  }
  else return false;
}

static int map(int ch)
{  
  if (ch == TK_LOGIC_AND || ch == TK_LOGIC_OR) return 0;
  else if (ch == TK_NEQ || ch == TK_EQ) return 1; 
  else if (ch == '+' || ch == '-') return 2;
  else if (ch == '*' || ch == '/') return 3;
  // else return 4;
  else if (ch == '(' || ch == ')') return 4;
  else assert(0);
}

bool isop(int ch)
{
	if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch =='(' ||
            ch == ')' || ch == TK_LOGIC_AND || ch == TK_LOGIC_OR || 
            ch == TK_EQ || ch == TK_NEQ)
		return true;
	else return false;
}

bool check_bug(int st, int ed)
{
	if (tokens[st].type == '(' && tokens[ed].type == ')') return true;
	else if (st == ed) return true;
	else
	{
	for(int i = st; i <= ed; i++)
		if (tokens[i].type != '(' && tokens[i].type != ')' && isop(tokens[i].type))
			return true;
	}
    return false;
}

static uint32_t eval(int st, int ed)
{
  int mainOp = -1;
  if(st > ed) assert(0);
  else if (tokens[st].type == DEREF) return eval(st + 1, ed);
  else if (tokens[st].type == TK_NEG) return -eval(st + 1, ed);
  else if (st == ed)
  {
      if (tokens[st].type == TK_NUM)
      {
		uint32_t ret = atoi(tokens[st].str);
		if (st != 0 && tokens[st - 1].type == DEREF) return vaddr_read(ret, 4);  
		return ret;
      }
      else if (tokens[st].type == TK_HEX)
      {    
          if (st != 0 && tokens[st - 1].type == DEREF) 
          {
            uint32_t hex;
            sscanf(tokens[st].str + 2, "%x", &hex);
            return vaddr_read(hex, 4);
          }
          else
           return (uint32_t)strtoul(tokens[st].str + 2, NULL, 16);
      }
      else if (tokens[st].type == TK_REG)
      { 
         bool success = false;
         uint32_t tmp = isa_reg_str2val(tokens[st].str, &success);
         if (!success) assert(0);
		 // for *$pc
         if (st != 0 && tokens[st - 1].type == DEREF)
		 {
            return vaddr_read(tmp, 4);
		 }
		 else
  		    return tmp;
      }    
  }
    // need to solve "-1-1 = 0" or simply use (-1)-1 instead of -1-1
  else if (check_parentheses(st, ed) == true) return eval(st + 1, ed - 1);
  else
  {
	// int mainOp = -1;
	//int flag = 0;
    for (int i = st; i <= ed; i++)
    {
      if (isop(tokens[i].type))
      {
		 int delta = 1;
         if (tokens[i].type == '(')
		 {
            while(delta && i != ed)
			 {
				 i++;
				 if (tokens[i].type == '(') delta++;
				 if (tokens[i].type == ')') delta--;
			 }
		 }
         else
         {
			//flag = 1;
            if (mainOp == -1 || map(tokens[mainOp].type) >= map(tokens[i].type))
                mainOp = i;
         }
      }
    }
	//assert(flag == 1);
    uint32_t val1 = eval(st, mainOp - 1);
    uint32_t val2 = eval(mainOp + 1, ed);

    switch (tokens[mainOp].type)
    {
        case '+': return val1 + val2;
        case '-': return val1 - val2;
        case '*': return val1 * val2;
        case '/': return val1 / val2;
        case TK_LOGIC_AND: return val1 && val2;
        case TK_LOGIC_OR: return val1 || val2;
        case TK_EQ: return val1 == val2;
        case TK_NEQ: return val1 != val2;
        default: assert(0);
    }
  }
  return 0;
}

void intToString(uint32_t value, char* str) {
	 int i = 0;
     int isNegative = 0;
    
    if (value < 0) {
        isNegative = 1;
        value = -value;
    }
    
    do {
        str[i++] = value % 10 + '0';
        value /= 10;
	    } while (value > 0);
    if(isNegative) str[i++] = '-';
    int length = i;
    for(int j = 0; j < length / 2; j++) {
	  char temp = str[j];
	  str[j] = str[length - j - 1];
      str[length - j - 1] = temp;
    }
    str[i] = '\0';
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {

    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  *success = true;

  for(int i = nr_token - 1; i >= 0; i--)
  {
    if(tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_NUM && tokens[i - 1].type != ')' && tokens[i - 1].type != TK_REG && tokens[i - 1].type != TK_HEX)))
        {
		    tokens[i].type = DEREF;
			/*
		    tokens[i+1].type = TK_NUM;
			uint32_t hexa;
			sscanf(tokens[i + 1].str + 2, "%x", &hexa);
			uint32_t num1 = vaddr_read(hexa, 4);
            // transfer reg to char and put into tokens[i].str
			char tmp[32];
			intToString(num1, tmp);
			strncpy(tokens[i + 1].str, tmp, 31);
			tokens[i + 1].str[31] = '\0';
			for (int j = i; j < nr_token - 1; j++)
				tokens[j] = tokens[j + 1];
			nr_token--;
			*/
		}
    if(tokens[i].type == '-' && (i == 0 || tokens[i - 1].type == TK_NEG ||(tokens[i - 1].type != TK_NUM && tokens[i - 1].type != ')' && tokens[i - 1].type != TK_REG && tokens[i - 1].type != TK_HEX)))
       { 
		  tokens[i].type = TK_NEG;
		  /*
		  if (tokens[i + 1].str[0] != '-')
	      {		  
            for(int j = 31 ; j > 0 ; j --)
			   tokens[i+1].str[j] = tokens[i+1].str[j-1];
		    tokens[i+1].str[0] = '-';
		    for(int j = i; j < nr_token - 1;j ++)
			    tokens[j] = tokens[j + 1];
	        nr_token--;
		  }
		  else
		  {
            for(int j = 0; j < 31; j++)
				tokens[i + 1].str[j] = tokens[i + 1].str[j + 1];
			tokens[i + 1].str[31] = '\0';
			for (int j = i; j < nr_token - 1; j++)
                tokens[j] = tokens[j + 1];
			nr_token--;

		  }
		  */
	   }
  }
  uint32_t ret = eval(0, nr_token - 1);
  printf("%u\n", ret);
  
  for (int i = 0; i < nr_token; i++)
  {
	  memset(tokens[i].str,'\0', sizeof(tokens[i].str));
  }
  return ret;
}
