## Examining one of the issues of the built in shell in xv6

The fewest number of pipes I could replecate this behavior on was 33. An example program is:

 >ls|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc|wc

The issue seems to be that sh.c holds command strings in a char buffer of size 100, meaning anything beyond that can get cut off and result in odd behavior like the results hanging or not returning. Since it's the only running program on xv6, the hanging can easily be confused with something like deadlock.
