# Advanced OS

Student: Pradeep Kumar Beri <u1077774@utah.edu> u1077774    
Assignment: 2  

# Modify xv6 to have readyQueue implementation

I have various other bad solutions, finally coming to the right one as implemented by many other 
students of our class. Each proc will have nextReady proc member. Ptable will have head, and tail pointer
 to locate the start(for dequeue) and to add new runnable process respectively
 Add and remove takes O(1) steps, but if a runnable process is killed abruptly or somehow changes state
 other than scheduler dequeing it, then it takes O(n) steps to find the process in readyQueue and
 update the readyQueue accordingly.

