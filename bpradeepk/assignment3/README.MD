# Advanced OS

Student: Ryan Stutsman <u1077774@utah.edu> u1077774    
Assignment: 3  

# Part I: Modify xv6 so that main calls exit when returns

Modified unlib.c and Makefile, so that any c program gets called using exit_main from ulib.c instead of the main included

#Part II: Why can't we have multiple pipeline commands on xv6 console

It looks like, xv6 shell sh.c has a buffer array of 100 chars (main() in sh.c) and 
its breaking up long commands into shorter commands. So, when we write long commands like
'ls -l | wc | wc.....|wc..|wc', then its being split into two commands. If we end up having 'wc' as the first command when split,
when it will just wait on input which appears like a deadlock for us.
