# Advanced OS

Student: Pradeep Kumar Beri <u1077774@utah.edu> u1077774    
Assignment: 6  

# Implement a sensible Ctrl-C implementation

Modified Console.c to capture the keySequence and then called makeKillable() function from proc.c
which basically lists the curproc and all its children( direct and indirect) as killed.
Since Console.c already checks for if a process is killed or not, so marking it killed did the job.
Initially I was just marking the job kill through a different implementation logic, and the scheduler 
while picking up next process was checking for the killflag and killing that particular process,
but carefully analysing the code gave me this more accurate implementation.
