CS330 PintOS project1
====================

----
### 1. Day 1  (Sep 15th) : Commit #1 
#### Main issue : Timer Problem.  
#####    thread.h
>   Added
>   >   1.  struct thread-> endtime   
>   >   2.  less() & Debugging print함수들 추가  
>   >   3.  thread_insert_sleep() : sleep_list 에 thread 추가   
#####    thread.c
>   Added
>   >   1.  sleep_list : sleep 상태안 threads list
>   >   2.  thread_insert_sleep() : current_thread의 endtime 인스턴스를 설정 후 sleep_list에 ordered되게 추가 
>   >   3.  print debugging functions      

>   Modified
>   >   1.  less(a,b) : a의 sleep endtime이 b보다 클 경우 return true  
>   >   2.  thread_init() : sleep_list 를 initialize 하도록 추가  
>   >   3.  thread_tick() : while()로 tick 마다 sleep_list에 일어나야할 thread를 pop하여 unblock    
#####    timer.c    
>   Modified    
>    >  1.  timer_sleep():  ticks >0 일경우 sleep_list에 endtime을 설정한 thread를 추가하고 block    

----
