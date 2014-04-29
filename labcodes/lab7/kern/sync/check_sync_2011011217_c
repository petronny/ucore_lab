#include <stdio.h>
#include <stdlib.h>
#include <proc.h>
#include <sem.h>
#include <monitor.h>
#include <assert.h>

#define SLEEP_TIME 10
#define MONKEY_N 20
#define THISSIDE (i%2)
#define OTHERSIDE (1-(i%2))
#define WAITING 0
#define PASSING 1
#define PASSED 2

int SmonkeyCount;
int NmonkeyCount;
struct proc_struct *monkey_proc_condvar[MONKEY_N];
int state_condvar[2];/* WAITING, PASSING, PASSED */
monitor_t mt, *mtp = &mt;

semaphore_t monkey_mutex; /* 临界区互斥 */
semaphore_t monkey_Smutex; /* 临界区互斥 */
semaphore_t monkey_Nmutex; /* 临界区互斥 */

int south_monkey_using_sepaphore(void *arg)
{
        int i = (int) arg;
        cprintf("I am No.%d monkey from south.\n", i);
        down(&monkey_Smutex);
        if (SmonkeyCount == 0)
                down(&monkey_mutex);
        SmonkeyCount++;
        up(&monkey_Smutex);
        cprintf("No.%d monkey from south is passing the cordage.\n", i);
        do_sleep(SLEEP_TIME);
        cprintf("No.%d monkey from south already passed.\n", i);
        down(&monkey_Smutex);
        SmonkeyCount--;
        if (SmonkeyCount == 0)
                up(&monkey_mutex);
        up(&monkey_Smutex);
        return 0;
}
int north_monkey_using_sepaphore(void *arg)
{
        int i = (int) arg;
        cprintf("I am No.%d monkey from north.\n", i);
        down(&monkey_Nmutex);
        if (NmonkeyCount == 0)
                down(&monkey_mutex);
        NmonkeyCount++;
        up(&monkey_Nmutex);
        cprintf("No.%d monkey from north is passing the cordage.\n", i);
        do_sleep(SLEEP_TIME);
        cprintf("No.%d monkey from north already passed.\n", i);
        down(&monkey_Nmutex);
        NmonkeyCount--;
        if (NmonkeyCount == 0){
                up(&monkey_mutex);
        }
        up(&monkey_Nmutex);
        return 0;
}
void monkey_test_condvar(int i){
	if(state_condvar[THISSIDE]==WAITING && state_condvar[OTHERSIDE]!=PASSING){
		cprintf("monkey_test_condvar: state_condvar[%d] will be passing\n", i);
		state_condvar[THISSIDE] = PASSING;
		cprintf("monkey_test_condvar: signal_cv[%d]\n", i);
		cond_signal(&mtp->cv[THISSIDE]);
	}
}

void monkey_hold_condvar(int i){
	down(&(mtp->mutex)); /* Be waiting and try to pass the bridge */
	state_condvar[THISSIDE] = WAITING;
	monkey_test_condvar(i);
	if(mtp->next_count > 0)
		up(&(mtp->next));
	else
	up(&(mtp->mutex));
}
void monkey_drop_condvar(int i){
	down(&(mtp->mutex)); /* Passed through and test the otherside */
	state_condvar[THISSIDE] = PASSED;
	monkey_test_condvar(i);
	if(mtp->next_count > 0)
		up(&(mtp->next));
	else
	up(&(mtp->mutex));
}
int monkey_using_condvar(void *arg){ /* arg is the No. of monkey 0~N-1 */
	int i = (int)arg;
	cprintf("I'm No.%d monkey from the south.\n", i);
	monkey_hold_condvar(i);
	cprintf("No.%d monkey from the %s is passing the cordage.\n", i, (i%2==0)?"south":"north");
	do_sleep(SLEEP_TIME);
	cprintf("No.%d monkey from the %s has already passed.\n", i, (i%2==0)?"south":"north");
	monkey_drop_condvar(i);
	monkey_test_condvar(i);
	return 0;
}

void check_sync(void){
	int i,SgenMonkeyCount = 0, NgenMonkeyCount = 0;
	//check monkey
	sem_init( &monkey_mutex, 1);
	sem_init( &monkey_Smutex, 1);
	sem_init( &monkey_Nmutex, 1);
	SmonkeyCount = 0;
	NmonkeyCount = 0;
	while(SgenMonkeyCount+NgenMonkeyCount<MONKEY_N*2){
		if(rand()%2){
			if(SgenMonkeyCount<MONKEY_N){
				int pid = kernel_thread(south_monkey_using_sepaphore, (void *)SgenMonkeyCount, 0);
				if (pid <= 0)
					panic("create No.%d monkey failed.\n");
				set_proc_name(find_proc(pid), "monkey_proc");
				SgenMonkeyCount++;
			}
		}else{
			if(NgenMonkeyCount<MONKEY_N){
				int pid = kernel_thread(north_monkey_using_sepaphore, (void *)NgenMonkeyCount, 0);
				if (pid <= 0)
					panic("create No.%d monkey failed.\n");
				set_proc_name(find_proc(pid), "monkey_proc");
				NgenMonkeyCount++;
			}
		}
	}
}
