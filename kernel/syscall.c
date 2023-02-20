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
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "proc_file.h"

#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // reclaim the current process, and reschedule. added @lab3_1
  free_process( current );
  schedule();
  return 0;
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  /*alloc a page of 4kb to va*/
  uint64 va = g_ufree_page;
  g_ufree_page += PGSIZE;
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork() {
  sprint("User call fork.\n");
  return do_fork( current );
}

//
// kerenl entry point of yield. added @lab3_2
//
ssize_t sys_user_yield() {
  // TODO (lab3_2): implment the syscall of yield.
  // hint: the functionality of yield is to give up the processor. therefore,
  // we should set the status of currently running process to READY, insert it in
  // the rear of ready queue, and finally, schedule a READY process to run.

  // sprint("current->pid=%d\n",current->pid);
  // sprint("current->queue_next->pid=%d\n",current->queue_next->pid);



  /*set the status of currently running process to READY*/
  current->status=READY;
  /*insert current process in the rear of ready queue*/
  insert_to_ready_queue(current);
  /*schedule a READY process to run*/
  schedule();

  return 0;
}

//
// open file
//

void fetch_absolute_path(char* relative_path,char* absolute_path)
{
  // current directory
  struct dentry* p=current->pfiles->cwd;
  if(relative_path[0]=='.' && relative_path[1]=='.')       // cd ../###
    p=p->parent;
  while(p)
  {
    char t[MAX_DENTRY_NAME_LEN];
    memset(t,'\0',MAX_DENTRY_NAME_LEN);
    memcpy(t,absolute_path,strlen(absolute_path));
    memset(absolute_path,'\0',MAX_DENTRY_NAME_LEN);
    memcpy(absolute_path,p->name,strlen(p->name));
    if(p->parent!=NULL)
    {
      absolute_path[strlen(p->name)]='/';
      absolute_path[strlen(p->name)+1]='\0';
    }
    strcat(absolute_path,t);
    p=p->parent;
  }
  if(relative_path[0]=='.' && relative_path[1]=='.')       // cd ../###
    strcat(absolute_path,relative_path+3);
  else if(relative_path[0]=='.' && relative_path[1]!='.')
    strcat(absolute_path,relative_path+2);
  else strcat(absolute_path,relative_path);
}


ssize_t sys_user_open(char *pathva, int flags) {
  char* pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  
  if(pathpa[0]=='.')
  {
    char absolute_path[MAX_DENTRY_NAME_LEN];
    memset(absolute_path,'\0',MAX_DENTRY_NAME_LEN); 
    fetch_absolute_path(pathpa,absolute_path);
    return do_open(absolute_path,flags);
  }
  return do_open(pathpa, flags);
}

//
// read file
//
ssize_t sys_user_read(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = lookup_pa((pagetable_t)current->pagetable, addr);
    uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
    uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
    uint64 r = do_read(fd, (char *)pa + off, len);
    i += r; if (r < len) return i;
  }
  return count;
}

//
// write file
//
ssize_t sys_user_write(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = lookup_pa((pagetable_t)current->pagetable, addr);
    uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
    uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
    uint64 r = do_write(fd, (char *)pa + off, len);
    i += r; if (r < len) return i;
  }
  return count;
}

//
// lseek file
//
ssize_t sys_user_lseek(int fd, int offset, int whence) {
  return do_lseek(fd, offset, whence);
}

//
// read vinode
//
ssize_t sys_user_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current->pagetable), istat);
  return do_stat(fd, pistat);
}

//
// read disk inode
//
ssize_t sys_user_disk_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current->pagetable), istat);
  return do_disk_stat(fd, pistat);
}

//
// close file
//
ssize_t sys_user_close(int fd) {
  return do_close(fd);
}

//
// lib call to opendir
//
ssize_t sys_user_opendir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  return do_opendir(pathpa);
}

//
// lib call to readdir
//
ssize_t sys_user_readdir(int fd, struct dir *vdir){
  struct dir * pdir = (struct dir *)user_va_to_pa((pagetable_t)(current->pagetable), vdir);
  return do_readdir(fd, pdir);
}

//
// lib call to mkdir
//
ssize_t sys_user_mkdir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  return do_mkdir(pathpa);
}

//
// lib call to closedir
//
ssize_t sys_user_closedir(int fd){
  return do_closedir(fd);
}

//
// lib call to link
//
ssize_t sys_user_link(char * vfn1, char * vfn2){
  char * pfn1 = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn1);
  char * pfn2 = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn2);
  return do_link(pfn1, pfn2);
}

//
// lib call to unlink
//
ssize_t sys_user_unlink(char * vfn){
  char * pfn = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)vfn);
  return do_unlink(pfn);
}

//
//  print the directory of the process
//
void sys_user_pwd(char* pathva)
{
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)pathva);
  struct dentry* p=current->pfiles->cwd;
  if(p->parent==NULL)
  {
    strcpy(pathpa,"/");
  }
  else{
    while(p)
    {
      char t[MAX_DENTRY_NAME_LEN];
      memset(t,'\0',MAX_DENTRY_NAME_LEN);
      memcpy(t,pathpa,strlen(pathpa));
      memset(pathpa,'\0',MAX_DENTRY_NAME_LEN);
      memcpy(pathpa,p->name,strlen(p->name));
      if(p->parent!=NULL)
      {
        pathpa[strlen(p->name)]='/';
        pathpa[strlen(p->name)+1]='\0';
      }
      strcat(pathpa,t);
      p=p->parent;
    }
    pathpa[strlen(pathpa)-1]='\0';
  }

  // sprint("\n##################ls the open files###################\n");
  // sprint("the number of opened files:%d\n",current->pfiles->nfiles);
  // struct file* filep=current->pfiles->opened_files;
  // for(int i=0;i<current->pfiles->nfiles-MAX_FILES;i++)
  // {
  //   sprint("fd:%d name:%s\n",i,current->pfiles->opened_files[i].f_dentry->name);
  // }
  // sprint("\n##################ls the open files done##############\n");
}



//
//  checkout the directory to path
//
// 改路径一定存在
void sys_user_cd(char* pathva)
{
  char* pathpa=(char*)user_va_to_pa((pagetable_t)(current->pagetable), pathva);
  char new_path[MAX_DEVICE_NAME_LEN];
  memset(new_path,'\0',MAX_DENTRY_NAME_LEN);
  struct dentry* current_directory=current->pfiles->cwd;
  // fetch pathname of new directory
  fetch_absolute_path(pathpa,new_path);


  // open the new directory
  int fd=do_opendir(new_path);
  // change the current directory to new path
  current->pfiles->cwd=current->pfiles->opened_files[fd].f_dentry;
  do_closedir(fd);
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page();
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    case SYS_user_fork:
      return sys_user_fork();
    case SYS_user_yield:
      return sys_user_yield();
    // added @lab4_1
    case SYS_user_open:
      return sys_user_open((char *)a1, a2);
    case SYS_user_read:
      return sys_user_read(a1, (char *)a2, a3);
    case SYS_user_write:
      return sys_user_write(a1, (char *)a2, a3);
    case SYS_user_lseek:
      return sys_user_lseek(a1, a2, a3);
    case SYS_user_stat:
      return sys_user_stat(a1, (struct istat *)a2);
    case SYS_user_disk_stat:
      return sys_user_disk_stat(a1, (struct istat *)a2);
    case SYS_user_close:
      return sys_user_close(a1);
    // added @lab4_2
    case SYS_user_opendir:
      return sys_user_opendir((char *)a1);
    case SYS_user_readdir:
      return sys_user_readdir(a1, (struct dir *)a2);
    case SYS_user_mkdir:
      return sys_user_mkdir((char *)a1);
    case SYS_user_closedir:
      return sys_user_closedir(a1);
    // added @lab4_3
    case SYS_user_link:
      return sys_user_link((char *)a1, (char *)a2);
    case SYS_user_unlink:
      return sys_user_unlink((char *)a1);
    // added @lab4_challenge_1
    case SYS_user_pwd:
      sys_user_pwd((char*)a1);
      return 0;
    case SYS_user_cd:
      sys_user_cd((char*)a1);
      return 0;
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
