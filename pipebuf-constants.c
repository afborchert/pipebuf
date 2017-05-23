#include <limits.h>
#include <stdio.h>
#include <unistd.h>

int main() {
#ifdef PIPE_BUF
   printf("PIPE_BUF = %lu\n", (unsigned long int) PIPE_BUF);
#else
   printf("PIPE_BUF is not defined\n");
#endif
   int pipefds[2];
   if (pipe(pipefds) >= 0) {
      printf("_PC_PIPE_BUF = %ld\n", fpathconf(pipefds[1], _PC_PIPE_BUF));
   }
}
