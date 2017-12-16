# Advanced OS

Student: Pradeep Kumar Beri <u1077774@utah.edu> u1077774    
Assignment: 5  

# Eventually xv6 time-slices away from CPU bound processes

Timer Vector interrupt will eventually rise timer interrupts after some fixed time interval. This Code
can be seen at lapicint() function which is called from main.c. So, atleast we are sure,
timer interrupt gets risen once every few ticks. The interrupt arrives at IRQ_0.
And when, this interrupt occurs, we can see from a call to scheduler 
occurs in scheduler() in proc.c

In this function, we can see that, which basically schedules the next runnable process. At line 3424 we see that, timer 
interrupt wakes up all processes sleeping on tick, and checks if the process is set to be killed, if not, it makes
it runnable and switches to scheduler().

Now, this argument stands only if interrupts are not disabled and timer interrupts is processed eventually.
We can note that, while handling of any interrupts, all other interrupts are disabled. The other places where interrupts
are disabled are the starting points while booting and making the system ready. Other than that, interrupts are always enabled.

A timer interrupt can never be pre-empted withough being handled. So, we can be sure that, timer interrupt will be generated,
  it can't be pre-empted and it will never remain always unhandled, and when handled, it switches to scheduler to pick 
  up the first next Runnable process. So, eventually xv6 time-slices away from CPU- bound processes.


