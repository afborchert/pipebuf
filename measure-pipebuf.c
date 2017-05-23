/* 
   Copyright (c) 2014, 2017 Andreas F. Borchert
   All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/* measure size of a pipe buffer */

#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void die(const char* msg) {
   perror(msg);
   exit(255);
}

#define MAXSIZE 20 /* we probe buflens up to 2^(MAXSIZE-1) */
#define MAXPROCESSES 32 /* should be power of two and > MAXSIZE and <= 128 */

/*
   empty the pipeline by reading up to expected bytes,
   return the actual number of bytes read
*/
static size_t suck_pipe(int fd, size_t expected) {
   char* buf = malloc(expected); if (!buf) die("malloc");
   size_t bytes_read = 0;
   while (bytes_read < expected) {
      ssize_t nbytes = read(fd, buf, expected - bytes_read);
      if (nbytes < 0) die("read from pipe");
      if (nbytes == 0) break;
      bytes_read += nbytes;
   }
   close(fd);
   free(buf);
   return bytes_read;
}

/*
   create a pipeline and fork off a process that writes
   nbytes into the writing end of this pipeline
   and exits i in case of success;
   returns the reading end of the pipeline
*/
static int pipe_and_fork(int i, size_t nbytes) {
   int fds[2];
   if (pipe(fds) < 0) die("pipe");
   pid_t pid = fork(); if (pid < 0) die("fork");
   if (pid == 0) {
      close(fds[0]);
      char* buf = malloc(nbytes);
      if (!buf) die("malloc");
      int fd = fds[1];
      int flags = fcntl(fd, F_GETFL) | O_NONBLOCK;
      fcntl(fd, F_SETFL, flags);
      ssize_t written = write(fd, buf, nbytes);
      if (written < nbytes) exit(255);
      exit(i);
   }
   close(fds[1]);
   return fds[0];
}

/*
   run and evaluate a series of tests where 'tests' is
   the number of tests and nbytes[i] contains the number
   of bytes to be written by the i-th process
*/
static size_t run_tests(size_t nbytes[], size_t tests) {
   int pipes[tests];
   for (int i = 0; i < tests; ++i) {
      pipes[i] = pipe_and_fork(i, nbytes[i]);
   }
   // wait for all the forked processes
   bool success[tests];
   for (size_t i = 0; i < tests; ++i) {
      success[i] = false;
   }
   int wstat;
   pid_t pid;
   while ((pid = wait(&wstat)) > 0) {
      if (WIFEXITED(wstat)) {
	 int status = WEXITSTATUS(wstat);
	 if (status != 255 && status < MAXPROCESSES) {
	    success[status] = true;
	 }
      }
   }
   // evaluate and confirm results
   size_t confirmed_len = 0;
   for (size_t i = 0; i < tests; ++i) {
      if (success[i]) {
	 confirmed_len = suck_pipe(pipes[i], nbytes[i]);
	 continue;
      }
      break;
   }
   return confirmed_len;
}

// check for buffer sizes that are powers of two
size_t test1() {
   size_t nbytes[MAXSIZE];
   for (int i = 0; i < MAXSIZE; ++i) {
      nbytes[i] = 1 << i;
   }
   return run_tests(nbytes, MAXSIZE);
}

// check for arbitrary buf sizes
size_t test2(size_t buflen,
      size_t increment, size_t tests) {
   size_t nbytes[tests];
   for (size_t i = 0; i < tests; ++i) {
      nbytes[i] = buflen + increment * i;
   }
   return run_tests(nbytes, tests);
}

int main() {
   // check for buf sizes that are powers of two
   size_t buflen = test1();
   if (!buflen) {
      printf("pipe buffer size is beyond %zu\n",
	 (size_t)1 << (MAXSIZE-1)); exit(1);
   }

   // check for buf sizes that are not powers of two
   size_t increment = buflen / MAXPROCESSES;
   size_t lastlen = 0;
   size_t tests = MAXPROCESSES;
   while (increment > 0) {
      size_t len = test2(buflen, increment, tests);
      if (!len) {
	 printf("pipe buffer len is possibly %zu "
	    "but that was not confirmed\n", buflen); exit(1);
      }
      lastlen = len;
      buflen = len;
      if (increment > MAXPROCESSES) {
	 tests = MAXPROCESSES;
	 increment /= MAXPROCESSES;
      } else if (increment > 1) {
	 tests = increment;
	 increment = 1;
      } else {
	 increment = 0;
      }
   }
   if (lastlen) {
      buflen = lastlen;
   }
   printf("pipe buffer size = %zu\n", buflen);
}
