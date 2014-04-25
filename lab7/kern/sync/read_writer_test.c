/* 
* @Author: BlahGeek
* @Date:   2014-04-23
* @Last Modified by:   BlahGeek
* @Last Modified time: 2014-04-25
*/

#include <stdio.h>
#include <proc.h>
#include <sem.h>
#include <monitor.h>
#include <assert.h>

#define READER_COUNT 5
#define WRITER_COUNT 5

#define READ_TIME 200
#define READ_SLEEPTIME 100
#define WRITE_TIME 100
#define WRITE_SLEEPTIME 200

#define TIMES 3

static int now_reading_count; // how many people are reading now
static int now_writing_count; // how many people are writing now, shoule be 0 or 1 obviously

static void print_info() {
    cprintf("Now %d reading, %d writing.\n", now_reading_count, now_writing_count);
}

static void do_read(int me) {
    cprintf("No.%d is reading now...", me);
    print_info();
    do_sleep(READ_TIME);
}

static void do_write(int me) {
    cprintf("No.%d is writing now...", me);
    print_info();
    do_sleep(WRITE_TIME);
}


// ----- semaphore ------
static semaphore_t r_mutex, rw_mutex;

// ----- Reader First using semaphore ---------
static int reader_first_using_semaphore_reader(void * arg) {
    int me = (int)arg;
    int i;
    for(i = 0 ; i < TIMES ; i += 1){
        do_sleep(READ_SLEEPTIME);
        cprintf("I'm reader No.%d\n", me);
        down(&r_mutex);
        now_reading_count += 1;
        if(now_reading_count == 1) // now one is reading
            down(&rw_mutex); // Do not allow writing while reading
        up(&r_mutex);
        do_read(me);
        down(&r_mutex);
        now_reading_count --;
        if(now_reading_count == 0)
            up(&rw_mutex);
        up(&r_mutex);
    }
    return 0;
}


static int reader_first_using_semaphore_writer(void * arg) {
    int me = (int) arg;
    int i;
    for(i = 0 ; i < TIMES ; i += 1){
        do_sleep(WRITE_SLEEPTIME);
        cprintf("I'm writer No.%d\n", me);
        down(&rw_mutex);
        now_writing_count += 1;
        do_write(me);
        now_writing_count -= 1;
        up(&rw_mutex);
    }
    return 0;
}

void reader_first_using_semaphore() {
    sem_init(&r_mutex, 1);
    sem_init(&rw_mutex, 1);
    now_reading_count = now_writing_count = 0;
    int i;
    for(i = 0 ; i < READER_COUNT ; i += 1)
        kernel_thread(reader_first_using_semaphore_reader,
                      (void *)i, 0);
    for(i = 0 ; i < WRITER_COUNT ; i += 1)
        kernel_thread(reader_first_using_semaphore_writer,
                      (void *)i, 0);
}
// --- end -- Reader First using semaphore ------


// Writer first using Monitor ----

static int waiting_writer_count;
static monitor_t mt, *mtp=&mt;

static int writer_first_using_monitor_writer (void * arg) {
    int me = (int) arg;
    int t;
    for(t = 0 ; t < TIMES ; t += 1){
        do_sleep(WRITE_SLEEPTIME);
        cprintf("I'm writer No.%d\n", me);
        down(&(mtp->mutex));
        if(now_reading_count != 0 || now_writing_count != 0){
            waiting_writer_count += 1;
            cond_wait(&(mtp->cv[READER_COUNT]));
            waiting_writer_count -= 1;
        }
        now_writing_count += 1;
        if(mtp->next_count>0) up(&(mtp->next));
        else up(&(mtp->mutex));
        do_write(me);
        down(&(mtp->mutex));
        now_writing_count -= 1;
        if(waiting_writer_count != 0)
            cond_signal(&(mtp->cv[READER_COUNT]));
        else {
            int i = 0 ;
            for(; i < READER_COUNT ; i += 1)
                cond_signal(&(mtp->cv[i]));
        }
        if(mtp->next_count>0) up(&(mtp->next));
        else up(&(mtp->mutex));
    }
    return 0;
}

static int writer_first_using_monitor_reader (void * arg) {
    int me = (int) arg;
    int t;
    for(t = 0 ; t < TIMES ; t += 1){
        do_sleep(READ_SLEEPTIME);
        cprintf("I'm reader No.%d\n", me);
        down(&(mtp->mutex));
        if(now_writing_count != 0)
            cond_wait(&(mtp->cv[me]));
        now_reading_count += 1;
        if(mtp->next_count>0) up(&(mtp->next));
        else up(&(mtp->mutex));
        do_read(me);
        down(&(mtp->mutex));
        now_reading_count -= 1;
        if(now_reading_count == 0)
            cond_signal(&(mtp->cv[READER_COUNT]));
        if(mtp->next_count>0) up(&(mtp->next));
        else up(&(mtp->mutex));
    }
    return 0;
}

void writer_first_using_monitor () {
    monitor_init(mtp, READER_COUNT+1);
    now_reading_count = now_writing_count = 0;
    waiting_writer_count = 0;
    int i;
    for(i = 0 ; i < WRITER_COUNT ; i += 1)
        kernel_thread(writer_first_using_monitor_writer,
                      (void *)i, 0);
    for(i = 0 ; i < READER_COUNT ; i += 1)
        kernel_thread(writer_first_using_monitor_reader,
                      (void *)i, 0);

}
