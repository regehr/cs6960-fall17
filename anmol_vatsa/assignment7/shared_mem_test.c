#include "types.h"
#include "user.h"

int main()
{
  char *cdata = "testdata";
  if(buf_setup()) {
    printf(1,"Could not get shared memory\n");
    exit();
  }
  uint clen = strlen(cdata);
  if(!buf_put(cdata, clen)){
    printf(1, "Wrote '%s' to shared memory\n", cdata);
  }
  char *data = malloc(150);
  uint len;
  int res;
  if(!(res=buf_get(data, &len, 150))){
    if(strcmp(data, cdata) != 0){
      printf(1,"Received wrong msg\n");
      exit();
    }
    printf(1, "Message recvd[%s] of len[%d]\n", data, len);
  } else {
    printf(1, "No message received/or mesage too long. Ret Code[%d]\n",res);
  }
  printf(1,"Passed!\n");
  exit();
}
