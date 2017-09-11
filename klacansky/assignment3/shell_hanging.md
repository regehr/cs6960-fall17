Builtin shell would hang on the last command of the following sequence.

```
ls | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc
ls | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc | wc
```

Hypothesis is that the last wc has no input and just waits for it. Inspecting the source code
of `sh.c` revealed the command can be at most hundred characters long and thus the last `wc`
gets executed as separate command and waits for input.
Note the line will end like `... wc |` and parser will parse the missing right side of pipe
as command with no arguments which will not be executed in `runcmd` and call `exit`.
