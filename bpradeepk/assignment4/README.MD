# Advanced OS

Student: Pradeep Kumar Beri <u1077774@utah.edu> u1077774    
Assignment: 4  

#  Modify xv6 to add 4 system calls to create, acquire, release and destroy your own mutex

Created a Mutex containing 2 mutexes, so basically mutexes will be identified as 0 or 1 and would be 
locked by pid. It has a spinlock to be acquired before you can modify any contents of the struct, but
the lock itself is acquired through sleeping rather than spinlocking as suggested by the assignment.

