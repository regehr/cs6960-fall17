
#include "types.h"
#include "user.h"

int main(void)
{
  char *msg = "abcdefghijklmnopqrstuvwxyz";
  char rbuf[128];
  int rlen;
  int i;

  printf(1, "buftest\n");

  printf(1, "checking setup()\n");
  if (buf_setup()) {
    printf(2, "error: buf_setup()\n");
    exit(); 
  }

  printf(1, "checking put()/get()\n");
  for (i = 0; i < 2000; i++) {
    if (buf_put(msg, strlen(msg)+1)) {
      printf(2, "error: buf_put() failed\n");
      exit();
    }

    if (buf_get(rbuf, &rlen, sizeof(rbuf))) {
      printf(2, "error: buf_get() failed\n");
      exit();
    }

    if (rlen != strlen(msg)+1) {
      printf(2, "error: buf_get() returned wrong size. got %d. expected %d\n", rlen, strlen(rbuf)+1);
      exit(); 
    }

    if (strcmp(msg, rbuf)) {
      printf(2, "error: buf_get() message read != message written");
      exit();
    }
  }

  printf(1, "checking N puts followed by N gets\n");
  for (i = 0; i < 100; i++) {
    if (buf_put(msg, strlen(msg)+1)) {
      printf(2, "error: buf_put() failed\n");
      exit();
    }
  }
  for (i = 0; i < 100; i++) {
    if (buf_get(rbuf, &rlen, sizeof(rbuf))) {
      printf(2, "error: buf_get() failed\n");
      exit();
    }

    if (rlen != strlen(msg)+1) {
      printf(2, "error: buf_get() returned wrong size. got %d. expected %d\n", rlen, strlen(rbuf)+1);
      exit(); 
    }

    if (strcmp(msg, rbuf)) {
      printf(2, "error: buf_get() message read != message written");
      exit();
    }

    memset(rbuf, 0, sizeof(rbuf));
  }

  printf(1, "checking get() fails without previous put()\n"); 
  if (!buf_get(rbuf, &rlen, sizeof(rbuf))) {
      printf(2, "error: buf_get() succeeded without put\n");
      exit();
  }

  if (buf_put(msg, strlen(msg)+1)) {
    printf(2, "error: buf_put() failed\n");
    exit();
  }
  if (!buf_get(rbuf, &rlen, 5)) {
    printf(2, "error: buf_get() succeeded with maxlen < msg length\n");
    exit();
  }

  for (i = 0; i < 2000; i++) {
    if (buf_put(msg, strlen(msg)+1)) {
      goto got_error;
    }
  }
  printf(2, "error: writing around buffer did not return error\n");
  exit();

got_error:

  if (!buf_setup()) {
    printf(2, "error: buf_setup() successfully called twice\n");
  }

  printf(1, "PASS\n");

  exit();
}
