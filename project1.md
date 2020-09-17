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
>		>		2.	thread_yeild()	:	list_push_back이 아닌 thread_insert_ready() 함수를 통해 ready list 에 추가
>		>		3. 	thread_unblock()	: unblock시에 current thread의 priority가 최대가 아닌 경우 다시 yeild를 하도록 추가함
>		>		4.	thread_set_priority()	:	 current thread의 priority가 최대가 아닌 경우 다시 yeild를 하도록 추가함

#####    timer.c    
>   Modified    
>    >  1.  timer_interrupt():  thread_tick 마다 thread_wake()를 실행하여 깨울 Thread sleep_list에서 꺼내기

---
### Day 3  (Sep 16th) : Commit 
#### Main issue : Priority(sema, donate-one)
#####    thread.h
>   Added
>		>		1. int real_priority : original priority of thread	

#####    thread.c

>   Modified
>   >   1. init_priority() :  real_priority 

#####   	synch.c    
>   Modified    
>   >  	1.  sema_up()	: sema++ 하는 코드를 if stamement 이전으로 변경
>		>		2.	sema_down()	:	wating list 에 추가하는 방식을 insert_ordered를 사용하는 방식으로 변경
>		>		3.	lock_aquire()	:	semaphore가 down 되어 있는 경우 추가할때마다 waiting list에서 가장 큰 priority 값으로 holder값을 변경
>		>		4.	lock_release()	:	release할 경우 holder의 priority를 real값으로 변경, lock의 holder를 waiters의 첫번째 thread로 설정

