#include <stdio.h>
#include <proc.h>
#include <sem.h>
#include <monitor.h>
#include <assert.h>
// Part 1/4: Common Definitions
#define N1 5
#define N2 3
#define TIMES 3
#define SLEEP_TIME 1
char randc_r(unsigned long* next) {  // POSIX.1-2001 example
    *next = *next * 1103515245 + 12345;
    return 'A' + ((unsigned)(*next/65536) % 26);
}
typedef struct buffer {
    char* buf;
    unsigned bufN;  // must be buffer size + 1
    unsigned h, t;
} buffer_t;
void buf_init_alloc(buffer_t* buf, unsigned size) {
    buf->bufN = size + 1;
    buf->buf = kmalloc(buf->bufN);
    buf->h = 0;
    buf->t = 0;
}
void buf_init(buffer_t* buf, char* mem, unsigned memN) {
    buf->buf = mem;
    buf->bufN = memN;
    buf->h = 0;
    buf->t = 0;
}
void buf_put(buffer_t* buf, char c) {
    buf->buf[buf->t++] = c;
    buf->t = buf->t % buf->bufN;
}
char buf_get(buffer_t* buf) {
    char c = buf->buf[buf->h++];
    buf->h = buf->h % buf->bufN;
    return c;
}
unsigned buf_count(buffer_t* buf) {
    return (buf->t + buf->bufN - buf->h) % buf->bufN;
}
// Part 2/4: Semaphore
semaphore_t mutex1, empty1, full1, mutex2, empty2, full2;
buffer_t buf1_sema, buf2_sema;
int input_sema(void* arg) {
    int iter = 0;
    unsigned long seed = 1;
    while (iter++ < TIMES) {
        do_sleep(SLEEP_TIME);
        char c = randc_r(&seed);
        cprintf("input_sema(%d): %c\n", iter, c);
        down(&empty1);
        down(&mutex1);
        buf_put(&buf1_sema, c);
        up(&mutex1);
        up(&full1);
    }
    return 0;
}
int calc_sema(void* arg) {
    int iter = 0;
    while (iter++ < TIMES) {
        down(&full1);
        down(&mutex1);
        char c = buf_get(&buf1_sema);
        up(&mutex1);
        up(&empty1);
        cprintf("                    calc_sema(%d): %c -> ?\n", iter, c);
        c += -'A'+'a';
        do_sleep(SLEEP_TIME*3);
        cprintf("                    calc_sema(%d): %c -> %c\n", iter, c-'a'+'A', c);
        down(&empty2);
        down(&mutex2);
        buf_put(&buf2_sema, c);
        up(&mutex2);
        up(&full2);
    }
    return 0;
}
int output_sema(void* arg) {
    int iter = 0;
    while (iter++ < TIMES) {
        down(&full2);
        down(&mutex2);
        char c = buf_get(&buf2_sema);
        up(&mutex2);
        up(&empty2);
        cprintf("                                             output_sema(%d): %c\n", iter, c);
        do_sleep(SLEEP_TIME);
    }
    return 0;
}
// char mem1_sema[N1+1], mem2_sema[N2+1];
void check_sema(void) {
    sem_init(&mutex1, 1);
    sem_init(&empty1, N1);
    sem_init(&full1, 0);
    sem_init(&mutex2, 1);
    sem_init(&empty2, N2);
    sem_init(&full2, 0);
    // buf_init(&buf1_sema, mem1_sema, N1+1);
    // buf_init(&buf2_sema, mem2_sema, N2+1);
    buf_init_alloc(&buf1_sema, N1);
    buf_init_alloc(&buf2_sema, N2);
    if (kernel_thread(input_sema, NULL, 0) <= 0)
        panic("create input_sema failed.\n");
    if (kernel_thread(calc_sema, NULL, 0) <= 0)
        panic("create calc_sema failed.\n");
    if (kernel_thread(output_sema, NULL, 0) <= 0)
        panic("create output_sema failed.\n");
}
// Part 3/4: Monitor & Conditional Variable
monitor_t mt1, mt2;
buffer_t buf1_condvar, buf2_condvar;
#define CV_EMPTY 0
#define CV_FULL 1
int input_condvar(void* arg) {
     cprintf("create input_condvar succeeded!\n");
   int iter = 0;
    unsigned long seed = 2;
    while (iter++ < TIMES) {
        do_sleep(SLEEP_TIME);
        char c = randc_r(&seed);
        cprintf("input_condvar(%d): %c\n", iter, c);
        down(&(mt1.mutex));
        if (buf_count(&buf1_condvar) == N1)  // ucore is Hoare-style by `next`
            cond_wait(&(mt1.cv[CV_EMPTY]));
        buf_put(&buf1_condvar, c);
        cond_signal(&(mt1.cv[CV_FULL]));
        if (mt1.next_count > 0)
            up(&(mt1.next));
        else
            up(&(mt1.mutex));
    }
    return 0;
}
int calc_condvar(void* arg) {
     cprintf("create calc_condvar succeeded!\n");
    int iter = 0;
    while (iter++ < TIMES) {
        down(&(mt1.mutex));
        if (buf_count(&buf1_condvar) == 0)
            cond_wait(&(mt1.cv[CV_FULL]));
        char c = buf_get(&buf1_condvar);
        cond_signal(&(mt1.cv[CV_EMPTY]));
        if (mt1.next_count > 0)
            up(&(mt1.next));
        else
            up(&(mt1.mutex));
        cprintf("                    calc_condvar(%d): %c -> ?\n", iter, c);
        c += -'A'+'a';
        do_sleep(SLEEP_TIME*3);
        cprintf("                    calc_condvar(%d): %c -> %c\n", iter, c-'a'+'A', c);
        down(&(mt2.mutex));
        if (buf_count(&buf2_condvar) == N2)  // ucore is Hoare-style by `next`
            cond_wait(&(mt2.cv[CV_EMPTY]));
        buf_put(&buf2_condvar, c);
        cond_signal(&(mt2.cv[CV_FULL]));
        if (mt2.next_count > 0)
            up(&(mt2.next));
        else
            up(&(mt2.mutex));
    }
    return 0;
}
int output_condvar(void* arg) {
     cprintf("create output_condvar succeeded!\n");
    int iter = 0;
    while (iter++ < TIMES) {
        down(&(mt2.mutex));
        if (buf_count(&buf2_condvar) == 0)
            cond_wait(&mt2.cv[CV_FULL]);
        char c = buf_get(&buf2_condvar);
        cond_signal(&(mt2.cv[CV_EMPTY]));
        if (mt2.next_count > 0)
            up(&(mt2.next));
        else
            up(&(mt2.mutex));
        cprintf("                                             output_condvar(%d): %c\n", iter, c);
        do_sleep(SLEEP_TIME);
    }
    return 0;
}
void check_condvar(void) {
    monitor_init(&mt1, 2);
    monitor_init(&mt2, 2);
    buf_init_alloc(&buf1_condvar, N1);
    buf_init_alloc(&buf2_condvar, N2);
    if (kernel_thread(input_condvar, NULL, 0) <= 0)
        panic("create input_condvar failed.\n");
    if (kernel_thread(calc_condvar, NULL, 0) <= 0)
        panic("create calc_condvar failed.\n");
    if (kernel_thread(output_condvar, NULL, 0) <= 0)
        panic("create output_condvar failed.\n");
}
// Part 4/4: Entry Point
void check_sync(void) {
//    check_sema();
    check_condvar();
}
