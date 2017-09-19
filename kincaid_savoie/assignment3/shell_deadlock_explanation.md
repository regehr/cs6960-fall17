# Explanation of shell deadlock when using pipes

It seems the shell deadlocks because it has a buffer `buf` with a maximum size of 100 in its `main` function. 
The shell stores commands provided by the user in this buffer using a call to `gets` with a maximum size 
of the buffer, 100 characters. Because of this, commands longer than 100 characters can get truncated, 
leading to deadlocks when a pipe is opened but a command is not executed to read from the pipe.

