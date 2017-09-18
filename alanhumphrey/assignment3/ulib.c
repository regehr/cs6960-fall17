#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"


/*
----------------------------------------------------------------------------
assignment3 - outline of approach
----------------------------------------------------------------------------

1.) Based on Canvas discussion: review Makefile, see where entry point (main)
    is specified, use info (below) from the following link.


NOTES: https://sourceware.org/binutils/docs/ld/Entry-Point.html#Entry-Point


There are several ways to set the entry point. The linker will set the entry
point by trying each of the following methods in order, and stopping when one
of them succeeds:

the ‘-e’ entry command-line option;
the ENTRY(symbol) command in a linker script;
the value of a target specific symbol, if it is defined; For many targets this is start, but PE and BeOS based systems for example check a list of possible entry symbols, matching the first one found.
the address of the first byte of the ‘.text’ section, if present;
The address 0.


2.) Modified the following in Makefile (line 142)

_%: %.o $(ULIB)
  $(LD) $(LDFLAGS) -N -e ulib_main -Ttext 0 -o $@ $^
                        ^^^^^^^^^^^
  $(OBJDUMP) -S $@ > $*.asm
  $(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym



3.) Add extern decl for main (here) and wrap the call to main in our
    ulib_main below. Call exit so main (from user programs) exits cleanly.

----------------------------------------------------------------------------
*/


// ----------------------------------------------------------------------------
// For program entry - Makefile has been modified to define entry point to be
// this wrapper, which calls main, then exit() - so we get a clean termination.
//    signature is the same as main()

extern int
main(int argc, char** argv);

int
ulib_main(int argc, char** argv)
{
  main(argc, argv);
  exit();
}

// ----------------------------------------------------------------------------



char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}
