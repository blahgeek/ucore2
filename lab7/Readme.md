# Reader-Writer Problem

## 使用信号量实现的Reader First算法

读优先，指一个读者试图进行读操作时，如果这时正有其他读者在进行操作，他可直接开始读操作，而不需要等待。读者比写者有更高的优先级。

使用信号量实现时，定义两个信号量分别用于读者互斥访问计数器和写者和读者互斥访问，分别为`r_mutex`和`rw_mutex`。读者读数据时，如果自己是第一个进入读的读者，则获取`rw_mutex`使之后的写者等待，并且读取完毕后只有当自己是最后一个读取完毕的读者才将`rw_mutex`释放，允许写者进入。其中判断自己是否为第一个进入和最后一个退出的读者使用一个计数器记录，访问该计数器需要通过`r_mutex`信号量来互斥访问。

代码如下：

```c
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
```

## 使用管程实现的Writer First算法

写优先，指一个读者试图进行读操作时,如果有其他写者在等待进行写操作或正在进行写操作,他要等待该写者完成写操作后才开始读操作。写者比读者有着更高的优先级。

使用管程实现时，在管程中定义了N+1个信号量，其中N为读者的数量。每个读者需要等待时睡在相应的信号量上，写者需要等待时睡在同一个信号量上。当一个读者需要读数据时，它首先判断是否有正在写数据的写者，如果没有的话（也说明没有正在等待的写者，因为它将会比读者优先进入）就开始读数据，否则睡在相应的信号量上。读取数据完毕后，如果自己是最后一个退出的读者，则向写者的信号量发送信号让下一个写者进入（如果有的话）。写者需要进入写数据时，首先检查是否有正在读或者正在写的人，如果有的话则睡在写者信号上；写完毕后判断是否还有正在等待的写者，若有则唤醒一个，否则唤醒一个读者。

代码：

```c
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
```
