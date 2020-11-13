CS330 PintOS project1
====================

----
### Day 1  (Sep 14th) : Commit #1 
#### Main issue : Timer  
#####    thread.h
>   Added
>   >   1.  struct thread-> endtime  

#####    thread.c
>   Added
>   >   1.  sleep_list : sleep 상태안 threads list
>   >   2.  thread_insert_sleep() : current_thread의 endtime 인스턴스를 설정 후 sleep_list에 ordered하여 추가 
>   >   3.  print debugging functions     
>   >   4.  less(a,b) : a의 sleep endtime이 b보다 클 경우 return true  

>   Modified
>   >   1.  thread_init() : sleep_list 를 initialize 하도록 추가  
>   >   2.  thread_tick() : while()로 tick 마다 sleep_list에 일어나야할 thread를 pop하여 unblock    

#####    timer.c    
>   Modified    
>    >  1.  timer_sleep():  ticks >0 일경우 sleep_list에 endtime을 설정한 thread를 추가하고 block    

----

### Day 2  (Sep 15th) : Commit #2, #3
#### Main issue : Timer(all alarm test) & Priority(change,fifo,preempt test)
#####    thread.h
>   Added

#####    thread.c
>   Added
>   >   1.  current_ticks : current_tick을 timer에서 가져와 계속 저장
>   >   2.	thread_wake()	:	while loop를 통해 sleep_list에 있는 모든 깨어나야하는 thread를 깨움
>		>		3.	less_priority(a,b,) : a의 priority가 b보다 클 경우 return true 
>		>		4.	thread_insert_ready(struct thread)	:	ready list에 thread를 추가

>   Modified
>   >   1.	thread_unblock()	:	list_push_back이 아닌 thread_insert_ready() 함수를 통해 ready list 에 추가
>		>		2.	thread_yield()	:	list_push_back이 아닌 thread_insert_ready() 함수를 통해 ready list 에 추가
>		>		3. 	thread_unblock()	: unblock시에 current thread의 priority가 최대가 아닌 경우 다시 yield를 하도록 추가함
>		>		4.	thread_set_priority()	:	 current thread의 priority가 최대가 아닌 경우 다시 yield를 하도록 추가함

#####    timer.c    
>   Modified    
>    >  1.  timer_interrupt():  thread_tick 마다 thread_wake()를 실행하여 깨울 Thread sleep_list에서 꺼내기

---
### Day 3  (Sep 16th) : Commit #4
#### Main issue : Priority(sema, donate-one)
#####    thread.h
>   Added
>		>		1. int real_priority : original priority of thread	(in struct thread)

#####    thread.c

>   Modified
>   >   1. init_priority() :  real_priority 

#####   	synch.c 
>   Modified    
>   >  	1.  sema_up()	: sema++ 하는 코드를 if stamement 이전으로 변경
>		>		2.	sema_down()	:	wating list 에 추가하는 방식을 insert_ordered를 사용하는 방식으로 변경
>		>		3.	lock_aquire()	:	semaphore가 down 되어 있는 경우 추가할때마다 waiting list에서 가장 큰 priority 값으로 holder값을 변경
>		>		4.	lock_release()	:	release할 경우 holder의 priority를 real값으로 변경, lock의 holder를 waiters의 첫번째 thread로 설정

---
### Day 4  (Sep 17th) : Commit #5
#### Main issue : Priority(fifo, sema, change)
#####    thread.c
>   Modified
>		>		1. thread_set_priority() : new_priority를 real_priority와 priority에 대해 비교하여 donated된 priority보다 높으면 new_priority를 set, 그렇지 않으면 new_priority를 real_priority에만 set 

---
### Day 5  (Sep 18th) : Commit #6,7
#### Main issue : Priority(nested, multiple donation), Condvar
#####    synch.h
>   Added
>		>		1. struct list_elem elem (in struct lock)

#####    thread.h
>   Added
>		>		1. struct lock * waiting_lock (in struct thread)
>   >   2. struct list lock_list  (in struct thread)

#####    thread.c
>   Added
>		>		1. bool less_lock(struct list_elem*, struct list_elem*, void*) : lock.semaphore.waiters의 제일 앞에 있는 elem의 priority를 비교
>   >   2. bool less_sema(struct list_elem*, struct list_elem*, struct thread*)

#####    thread.c
>   Modified
>   >   1. init_thread() : thread의 lock_list initialization 추가

#####    synch.c
>   Added
>   >   1. acquire_sync() : lock_acquire를 call한 thread의 priority가 현재 lock.holder의 priority보다 높을 때 priority donation이 재귀적으로 발생
>   >   2. release_sync() : thread가 holding하고 있던 lock들의 semaphore.waiters의 front thread중 가장 priority가 높은 것을 thread의 priority로 설정

>   Modified
>		>		1. sema_up() : sema.waiters sort를 sema.waiters중 첫번째 thread를 unblock하기 전에 추가
>   >   2. lock_acquire() : semaphore.value가 0일 시 current thread의 waiting lock에 그 lock 할당, acquire_sync()로 priority 동기화
>   >   3. lock_release() : lock을 lock_list에서 삭제하고 release_sync로 holding중인 lock들의 waiters중에서 가장 priority가 큰 것을 donated priority로 설정,lock의 holder를 waiters 가장 앞에 있던 thread로 설정
>   >   4. cond_wait() : cond_waiters에 priority 순서대로 추가
