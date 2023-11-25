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
}




#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static char *elf_file = NULL;
static int difftest_port = 1234;

#ifdef CONFIG_FTRACE
unsigned char *strbuf = NULL;
word_t strbuf_size = 0;
word_t func_num = 0; 
word_t *func_begin;
word_t *func_end;
word_t *func_name_index;



static void load_elf()
{
  if(elf_file == NULL) return;
  Assert(elf_file != NULL, "The elf_file name is an empty string!");
  FILE *fp = fopen(elf_file, "r");
  Assert(fp != NULL, "Failed to open '%s' elf file!", elf_file);

  
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  Log("The elf file is %s, size = %ld",elf_file, size);
  fseek(fp, 0, SEEK_SET);


  //Summon an elf_header
  Elf32_Ehdr elf_header;
  Assert(fread((void *)&elf_header, sizeof(Elf32_Ehdr), 1, fp) == 1, "Failed to read the elf header from elf file");
  //Some basic checks on the elf file
  Assert(elf_header.e_ident[EI_MAG0] == ELFMAG0,"Failed in checking the magic number");
  Assert(elf_header.e_ident[EI_MAG1] == ELFMAG1,"Failed in checking the magic number");
  Assert(elf_header.e_ident[EI_MAG2] == ELFMAG2,"Failed in checking the magic number" );
  Assert(elf_header.e_ident[EI_MAG3] == ELFMAG3,"Failed in checking the magic number" );

  Assert(elf_header.e_ident[EI_CLASS] == ELFCLASS32, "Wrong elf class!");
  Assert(elf_header.e_ident[EI_DATA] == ELFDATA2LSB, "Should be ELFDATA2LSB!");
  Assert(elf_header.e_ident[EI_VERSION] == EV_CURRENT, "Wrong on the elf version check");
  Log("Pass All the Elf file test!");
  Assert(fseek(fp, 0L, SEEK_SET) == 0, "Failed in fseek");

 //char strbuf[65536]; //save the ftrace strable
  Elf32_Off shdr_off = elf_header.e_shoff, shnum = elf_header.e_shnum, shsize = elf_header.e_shentsize;
  for (int i = 0 ; i < shnum ; i++)
  {
    Elf32_Off sh_offset = shdr_off + shsize * i;
    Assert(fseek(fp, 0, SEEK_SET) == 0, "Failed in fseek");
    Assert(fseek(fp, sh_offset, SEEK_CUR) == 0, "Failed in fseek");
    Elf32_Shdr shdr;
    Assert(fread((void *)&shdr, sizeof(Elf32_Shdr), 1, fp) == 1, "Failed to read a shdr");
    if(shdr.sh_type == SHT_STRTAB && (i != elf_header.e_shstrndx)) //Here we read the strtable
      {
    //    printf("index in shdr table %d, e_shstrndx is %d\n", i, elf_header.e_shstrndx);
        Elf32_Off strtab_off = shdr.sh_offset, strtab_size = shdr.sh_size;
        strbuf_size = strtab_size;
        Log("Find SHT_STRTABLE!shdr.sh_offset = 0x%x, shdr.sh_size = 0x%x", strtab_off, strtab_size);
        Assert(fseek(fp, 0, SEEK_SET) == 0, "Failed in fseek");
        Assert(fseek(fp, strtab_off, SEEK_CUR) == 0, "Failed in fseek");
        strbuf = (unsigned char *)malloc(strtab_size);
        Assert(fread(strbuf, (size_t)strtab_size, 1, fp) == 1, "Failed to load the strtab section"); 
        for (int pos = 0 ; pos <= strtab_size - 1 ; pos++)
          putchar(strbuf[pos]);
        putchar('\n');
      }
    /*
      Process the symtable, construct pc->function_name pair and save it in the ftrace table
      (begin, end) ---> function name(char * string)
      (func_begin, func_begin + func_size) ---> func_name
    */

    if(shdr.sh_type == SHT_SYMTAB) 
      {
        Elf32_Off symtab_off = shdr.sh_offset, symtab_size = shdr.sh_size;
        int symnum = symtab_size / sizeof(Elf32_Sym);
        Assert(fseek(fp, 0L, SEEK_SET) == 0, "Failed in fseek");
        Assert(fseek(fp, symtab_off, SEEK_CUR) == 0, "Failed in fseek");
        func_begin = (word_t *)malloc(sizeof(word_t) * symnum);
        func_end = (word_t *)malloc(sizeof(word_t) * symnum);     
        //Alloc symnum unsigned char * to prepare for the function name load
        func_name_index = (word_t *)malloc(sizeof(word_t) * symnum);
        printf("Sym number is %d\n", symnum);
        for (int j = 1 ; j <= symnum ; j++)
          {
            Elf32_Sym sym;
            Assert(fread((void *)&sym, sizeof(Elf32_Sym), 1, fp) == 1, "Failed to read sym");
            printf("sym.st_value is 0x%08x sym.st_info is 0x%08x sym.st_name is 0x%08x sym.st_size is 0x%08x\n", sym.st_value, sym.st_info, sym.st_name, sym.st_size);
            if(ELF32_ST_TYPE(sym.st_info) == STT_FUNC)
            {
              printf("Find func type!\n");
              func_begin[func_num] = sym.st_value;//begin address
              func_end[func_num] = sym.st_value + sym.st_size - 4; //end address
              func_name_index[func_num] = sym.st_name;
              func_num++;
            }
          }
      }
  }
  //Log("The strtab length: %ld, contents:\n %s", strlen(strtable), strtable);
}

char *find_func_by_index(word_t id)
{
  return (char *)(strbuf + id);
}

void display_all_function_info()
{
  printf("Function number is: %d", func_num);
  for (int i = 0 ; i < func_num ; i++)
      printf("Function: %s, Address: [0x%08x, 0x%08x]\n", find_func_by_index(func_name_index[i]), func_begin[i], func_end[i]);
}

static word_t stk_top = 0;
static word_t func_call_stack[65536];
static char *curr_run[65536];
static int jmp_id, ret_id;
static bool jmp_func, ret_func;

void detect_and_display_function_call(word_t _now_pc, word_t jal_addr, word_t ival)
{
  jmp_func = false;
  ret_func = false;
  jmp_id = -1;
  ret_id = -1;
  for (int i = 0 ; i < func_num ; i++)
    {
      if(jal_addr == func_begin[i]) //jal to func_begin must means call!
        {
          jmp_func = true;
          jmp_id = i;
        }
      if((_now_pc >= func_begin[i]) && (_now_pc <= func_end[i])) //means the tail will ret
          ret_id = i;
    }
  if(ival == 0x00008067) //ret instruction value;
    ret_func = true;

//  if((stk_top > 0) && (func_call_stack[stk_top] == jal_addr)) ret_func = true; //means will ret to the last recursion 
  if((jmp_func == false) && (ret_func == false)) return;//no ftrace display
  if((jmp_func == true) && (ret_func == false)) //just call the next function, recursion stack grows!
  { 
    stk_top++;
    printf("0x%08x:", _now_pc);
    func_call_stack[stk_top] = _now_pc + 4;//static next pc is the return address
    curr_run[stk_top] = find_func_by_index(func_name_index[jmp_id]);
    for (int j = 1 ; j < stk_top ; j++) //construct recursion display
        putchar(' ');
    printf("call [%s@0x%08x]\n", curr_run[stk_top], jal_addr);
    return;  
  }
  if((jmp_func == false) && (ret_func == true)) //just return from the current function
  {
    printf("0x%08x:", _now_pc);
    for (int j = 1 ; j < stk_top ; j++) //construct recursion display
        putchar(' ');
    printf("ret [%s]\n", curr_run[stk_top]);
    stk_top--;
    return;
  }
  if((jmp_func == true) && (ret_func == true)) //tail recursion!
  {
    //First print the ret infomation
    printf("0x%08x:", _now_pc);
    for (int j = 1 ; j < stk_top ; j++) //construct recursion display
        putchar(' ');
    printf("ret [%s]\n", curr_run[stk_top]);
    stk_top--;   
    //Second print the call information
    stk_top++;
    printf("0x%08x:", _now_pc);
    func_call_stack[stk_top] = _now_pc + 4;//static next pc is the return address
    curr_run[stk_top] = find_func_by_index(func_name_index[jmp_id]);
    for (int j = 1 ; j < stk_top ; j++) //construct recursion display
        putchar(' ');
    printf("call [%s@0x%08x]\n", curr_run[stk_top], jal_addr);
    return;  
  }
  Assert(0, "Shouldn' reach here!");
  return;//Not a function call;
}




#endif


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
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:", table, NULL)) != -1) {
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
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Set random seed. */
  init_rand();

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();
  
  /* Load the elf file for ftrace */
  #ifdef CONFIG_FTRACE
    load_elf();
    display_all_function_info();
  #endif


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
