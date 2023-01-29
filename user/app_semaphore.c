/*
* This app create two child process.
* Use semaphores to control the order of
* the main process and two child processes print info. 
*/
#include "user/user_lib.h"
#include "util/types.h"

int main(void) {
    int main_sem, child_sem[2];
    //��0���źŵ�
    main_sem = sem_new(1); 
    //��1 2���źŵ�
    for (int i = 0; i < 2; i++) child_sem[i] = sem_new(0);
    int pid = fork();
    /*child process*/
    if (pid == 0) {
        pid = fork();
        for (int i = 0; i < 10; i++) {
            sem_P(child_sem[pid == 0]);
            printu("Child%d print %d\n", pid == 0, i);
            /*child0 process*/
            if (pid != 0) sem_V(child_sem[1]); 
            /*child1 process*/
            else sem_V(main_sem);
        }
    }
    
    /*parent process*/
    else {
        for (int i = 0; i < 10; i++) {
            sem_P(main_sem);
            printu("Parent print %d\n", i);
            sem_V(child_sem[0]);
        }
    }
    exit(0);
    return 0;
}
