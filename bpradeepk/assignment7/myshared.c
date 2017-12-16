//I Have used Alex and Dong's Code as reference, and tried to understand their imlplementation.
//I have also referenced other implementation to understand different approaches others took.

#include "types.h"
#include "user.h"

//Define the ring size of 8 pages 
#define RINGSZ ((8*4096)-(2*sizeof(int)))

//Create the ring structure
struct ring {
     int roff;
     int woff;
     char data[RINGSZ];
} ;

//Creating a ring
static struct ring *ring;

static int min(int a, int b)
{
    return a < b ? a : b;
}

// C atomic intrsutcion for loading the ptr
static int load(int *ptr)
{
   return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

//NOT Complete
