#include "types.h"
#include "user.h"

//Test program for 1 simple test on myShared Buffer
int main()
{
    char *str = "testing";//Data being written
   
    //Setup the buffer memory
    if(buf_setup()) {
       printf(1," shared memory Not Allocated\n");
                exit();
                  }
       uint len = strlen(str);
       
       //Pass on the data to the allocated buffer above
       if(!buf_put(str, len)){
          printf(1, "Wrote '%s' to shared memory\n", str);
                  }

       //Create speace for the data to be retrieved
       char *newstr = malloc(150);
       uint nlen;
       int res;

       //Get the Shared Message from the buffer memory
       if(!(res=buf_get(newstr, &nlen, 150))){
         if(strcmp(newstr, str) != 0){
              printf(1,"Strings Don't Match\n");
              exit();
            }
          printf(1, "NewStr[%s]: Len[%d]\n", newstr, nlen);
          } else {
            printf(1, "Error Ret Code[%d]\n",res);
          }
        printf(1,"Test Passed!\n");
     exit();
}
