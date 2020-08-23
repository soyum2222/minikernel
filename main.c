#include <stdio.h>
#include <string.h>

typedef long long int64;
typedef unsigned long long uint64;

int64 getrip() {
    register int64 ip;
    asm volatile (
    "movq 0x10(%%rsp), %0\n\t"
    :"=r"(ip)
    );
    return ip;
}

#define MAX_TASK_NUM      10
#define KERNEL_STACK_SIZE   1024 // unsigned long

struct thread {
    uint64 sp;
    uint64 ip;
    uint64 bp;
};


struct pcb {
    unsigned int pid;
    struct thread t;
    uint64 stack[KERNEL_STACK_SIZE];
    struct pcb *next
};

struct pcb *my_current_task = NULL;

struct pcb task[MAX_TASK_NUM];

int need_schedule;

void schedule(void);

void process(void);

void start_kernel() {
    int pid = 0;
    int i;
    /* Initialize process 0*/
    task[pid].pid = pid;
    task[pid].t.ip = (uint64) process;
    task[pid].t.sp = (uint64) &task[pid].stack[KERNEL_STACK_SIZE - 1];
    printf("stack low address is: %d \n", &task[pid].stack[0]);
    printf("stack address is: %d \n", &task[pid].stack[KERNEL_STACK_SIZE - 1]);
    task[pid].next = &task[pid];

    for (i = 1; i < MAX_TASK_NUM; i++) {
        memcpy(&task[i], &task[0], sizeof(struct pcb));
        task[i].pid = i;
        task[i].t.sp = (uint64) (&task[i].stack[KERNEL_STACK_SIZE - 1]);
        task[i].next = task[i - 1].next;
        task[i - 1].next = &task[i];
    }

    pid = 0;
    my_current_task = &task[pid];
    asm volatile(
    "movq %1,%%rsp\n\t"    // set task[pid].thread.sp to sp
    "pushq %%rsp\n\t"         // push bp
    "movq %%rsp,%%rbp\n\t"
    "pushq %0\n\t"            // push task[pid].thread.ip
    "retq\n\t"                // pop task[pid].thread.ip to ip
    :
    : "m" (task[pid].t.ip), "m" (task[pid].t.sp)
    );
}


void process(void) {

    int i;
    while (1) {
        printf("PID %d  running\n", my_current_task->pid);
        i++;
        if (i % 10000 == 0) {
            need_schedule = 1;
            schedule();
        }

    }


}


void schedule() {
    struct pcb *curr;
    struct pcb *next;

    curr = my_current_task;
    curr->t.ip = getrip();

    if (need_schedule) {

        next = curr->next;


        printf("begin schedule pid : %d \n", curr->pid);
        printf("pid : %d rip : %x rsp : %x rbp : %x ", curr->pid, curr->t.ip, curr->t.sp, curr->t.bp);

        need_schedule = 0;
        my_current_task = next;
        asm volatile(
        "movq %%rbp ,%2\n\t"   // save bp
        "movq %%rsp,%0\n\t"    // save sp
        "movq %3,%%rsp\n\t"    //restore  sp
        "movq %5,%%rbp\n\t"
        "pushq %4\n\t"
        "retq\n\t"              //restore  ip
        :"=m"(curr->t.sp), "=m"(curr->t.ip), "=m"(curr->t.bp)
        : "m"(next->t.sp), "m"(next->t.ip), "m"(next->t.bp)
        );
    }

}

int main() {
    printf("begin\n");
    start_kernel();
}



