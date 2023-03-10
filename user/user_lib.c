/*
 * The supporting library for applications.
 * Actually, supporting routines for applications are catalogued as the user 
 * library. we don't do that in PKE to make the relationship between application 
 * and user library more straightforward.
 */

#include "user_lib.h"
#include "util/types.h"
#include "util/snprintf.h"
#include "kernel/syscall.h"
int do_user_call(uint64 sysnum, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6,
                 uint64 a7) {
  int ret;
  //sysnum 为调用的函数类型 是printu还是exit
  // before invoking the syscall, arguments of do_user_call are already loaded into the argument
  // registers (a0-a7) of our (emulated) risc-v machine.
  asm volatile(
    //汇编语句模板
      "ecall\n"
      "sw a0, %0"  // returns a 32-bit value 将功能号压栈
      //输出部分
      : "=m"(ret) //"=m"表示写到内存
      //输入部分
      :
      //
      : "memory");

  return ret;
}

//
// printu() supports user/lab1_1_helloworld.c
//
int printu(const char* s, ...) {
  va_list vl;
  va_start(vl, s);//vl指向第一个可变参数

  char out[256];  // fixed buffer size.
  int res = vsnprintf(out, sizeof(out), s, vl);//res=字符个数 并将字符串拷贝到out中
  va_end(vl);
  const char* buf = out;
  size_t n = res < sizeof(out) ? res : sizeof(out);//字符个数

  // make a syscall to implement the required functionality.
  //功能号 要输出的字符串首地址 字符个数
  return do_user_call(SYS_user_print, (uint64)buf, n, 0, 0, 0, 0, 0);
}

//
// applications need to call exit to quit execution.
//
int exit(int code) {
  return do_user_call(SYS_user_exit, code, 0, 0, 0, 0, 0, 0); 
}


int print_backtrace(int backtrace_num)
{
  return do_user_call(SYS_user_print_backtrace, backtrace_num, 0, 0, 0, 0, 0, 0); 
}
