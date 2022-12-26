/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "elf.h"
#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}


ssize_t sys_user_print_backtrace(trapframe* tf,int backtrace_num) {
  //s0 �ú�����һ��������ջ��ָ����print_backtrace-f8-f7-f6
  sprint("sys_user_print_backtrace begin\n");
  sprint("**************************************************\n");
  uint64 ra=tf->regs.ra;
  uint64 sp=tf->regs.sp;
  uint64 s0=tf->regs.s0;
  sp+=32;
  for(int i=0;i<backtrace_num;i++)
  {
    ra=*(uint64*)(sp+8);
    s0=*(uint64*)(sp);
    sp+=16;
    sprint("return address is %llx\n",ra);
  }
#define LOAD_SECTION


#ifdef LOAD_SECTION

  elf_ctx elfloader;
  elf_info info;
  info.f = spike_file_open("./obj/app_print_backtrace", O_RDONLY, 0);
  info.p = current;
  /*��ȡelf header��elfloader.ehdr��*/
  if(elf_init(&elfloader, &info)!=EL_OK)
    sprint("elf init error\n");
  sprint("elf init done\n");
  sprint("shoff is %llx\n",(uint64)elfloader.ehdr.shoff);
  sprint("shentsize is %llx\n",(uint64)elfloader.ehdr.shentsize);
  sprint("ehsize is %llx\n",(uint64)elfloader.ehdr.ehsize);
  sprint("shnum is %llx\n",(uint64)elfloader.ehdr.shnum);
  sprint("shstrndx is %llx\n",(uint64)elfloader.ehdr.shstrndx);
  sprint("load elf header into elfloader.ehdr done\n");
  
  elf_sect_header sh_addr;
  int i;
  int off;/*��ǰ��ȡ��section header�ĵ�ַ*/
  for(i=0,off=elfloader.ehdr.shoff;i<elfloader.ehdr.shnum;i++,off+=sizeof(sh_addr))
  {
    sprint("-----------------------------------\n");
    /*��ȡ section header*/
    if(elf_fpread(&elfloader,(void*)&sh_addr,sizeof(sh_addr),off)!=sizeof(sh_addr))
      panic("load section header into elfloader fail\n");
    sprint("load section header %d done\n",i);
    sprint("sh_type is %llx\n",sh_addr.sh_type);
    sprint("sh_addr is %llx\n",sh_addr.sh_addr);
    sprint("sh_offset is %llx\n",sh_addr.sh_offset);
    sprint("sh_size is %llx\n",sh_addr.sh_size);
    sprint("sh_entsize is %llx\n",sh_addr.sh_entsize);
    continue;
    /*Ϊload section����memory*/
    void* dest=elf_alloc_mb(&elfloader,sh_addr.sh_offset,sh_addr.sh_offset,sh_addr.sh_size);
    sprint("alloc memory for section done\n");
    if (elf_fpread(&elfloader, dest, sh_addr.sh_size, sh_addr.sh_offset) != sh_addr.sh_size)
      panic("load section fail");
    sprint("load setion %d done\n",i);
  }

#endif

  uint64 num_section_header=(uint64)elfloader.ehdr.shnum;//section header������
  uint64 address_the_first_section_header=(uint64)elfloader.ehdr.shoff;//��1��section header�ĵ�ַ
  uint64 size_section_header=(uint64)elfloader.ehdr.shentsize;//section header�Ĵ�С
  uint64 index_string_table=(uint64)elfloader.ehdr.shstrndx;//string table������ֵ



  //section header symtab�ĵ�ַ symtab�洢����������
  uint64 section_header_symtab_address=address_the_first_section_header+15*size_section_header;

  //section header strtab�ĵ�ַ
  uint64 section_header_strtab_address=address_the_first_section_header+16*size_section_header;

  //section header shstrtab�ĵ�ַ
  uint64 section_header_shstrtab_address=address_the_first_section_header+17*size_section_header;

  elf_sect_header symtab_header;
  elf_sect_header strtab_header;
  elf_sect_header shstrtab_header;
  /*��ȡsymtab*/
  if(elf_fpread(&elfloader,(void*)&symtab_header,sizeof(symtab_header),section_header_symtab_address)!=sizeof(symtab_header))
    panic("load section header fail");
  sprint("symtab_header.sh_name is %x\n",symtab_header.sh_name);
  sprint("symtab_header.sh_offset is %llx\n",symtab_header.sh_offset);
  sprint("symtab_header.sh_size is %llx\n",symtab_header.sh_size);
  int num_symbol=symtab_header.sh_size/sizeof(Elf64_Sym);
  Elf64_Sym symbol[num_symbol];
  if(elf_fpread(&elfloader,(void*)&symbol,symtab_header.sh_size,symtab_header.sh_offset)!=symtab_header.sh_size)
    panic("load section symtab fail");
  for(int i=0;i<num_symbol;i++)
    sprint("value=%llx size=%llx name=%llx\n",symbol[i].st_value,symbol[i].st_size,symbol[i].st_name);
  
  
  
  /*��ȡstrtab*/
  if(elf_fpread(&elfloader,(void*)&strtab_header,sizeof(strtab_header),section_header_strtab_address)!=sizeof(strtab_header))
    panic("load section header fail");
  sprint("strtab_header.sh_name is %x\n",strtab_header.sh_name);
  sprint("strtab_header.sh_offset is %llx\n",strtab_header.sh_offset);
  sprint("strtab_header.sh_size is %llx\n",strtab_header.sh_size);
  char strtab[strtab_header.sh_size];
  if(elf_fpread(&elfloader,(void*)&strtab,strtab_header.sh_size,strtab_header.sh_offset)!=strtab_header.sh_size)
    panic("load section strtab fail");

  /*��ȡshstrtab*/
  if(elf_fpread(&elfloader,(void*)&shstrtab_header,sizeof(shstrtab_header),section_header_shstrtab_address)!=sizeof(shstrtab_header))
    panic("load section header fail");
  sprint("shstrtab_header.sh_name is %x\n",shstrtab_header.sh_name);
  sprint("shstrtab_header.sh_offset is %llx\n",shstrtab_header.sh_offset);
  sprint("shstrtab_header.sh_size is %llx\n",shstrtab_header.sh_size);
  char shstrtab[shstrtab_header.sh_size];
  if(elf_fpread(&elfloader,(void*)&shstrtab,shstrtab_header.sh_size,shstrtab_header.sh_offset)!=shstrtab_header.sh_size)
    panic("load section shstrtab fail");
  spike_file_close( info.f );
  /**********************************************************************/
  sprint("**************************************************\n");
  sprint("sys_user_print_backtrace end\n");
  return 0;
}
//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(trapframe* tf,long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    case SYS_user_print_backtrace:
      //a1Ҫ��ӡ�ĵ���ջ�Ĳ��� 
      return sys_user_print_backtrace(tf,a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
