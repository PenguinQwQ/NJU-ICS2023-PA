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
extern word_t csr_reg[CSR_REGNUM];
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''. 
   * Then return the address of the interrupt/exception vector.
   */
  //no is the exception number to be written to the csr reg
  csr_reg[MCAUSE_CSR_ID] = NO;
  csr_reg[MEPC_CSR_ID] = epc;
  csr_reg[MSTATUS_CSR_ID] = 0x1800; //set status csr reg to 0x1800 for difftest match
  return csr_reg[MTVAL_CSR_ID];
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
