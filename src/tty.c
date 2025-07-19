#include <limits.h>
#include <pty.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "tty.h"

static int32_t master_fd;

size_t readfrompty(void) {
  static char buf[SHRT_MAX];
  static uint32_t buflen = 0;

  uint32_t nbytes = read(master_fd, buf + buflen, sizeof(buf) - buflen);
  buflen += nbytes;

  uint32_t iter = 0;
  while (iter < buflen) {
    uint32_t codepoint;
    int32_t len = utf8decode(&buf[iter], &codepoint);
    if (len == -1 || len > (int32_t)buflen)
      break;
    iter += len;

    printf("%u\n", codepoint);
  }

  if (iter < buflen) {
    memmove(buf, buf + iter, buflen - iter);
  }
  buflen -= iter;

  return nbytes;
}

int main() {

  if (forkpty(&master_fd, NULL, NULL, NULL) == 0) {
    execlp("/usr/bin/bash", "bash", NULL);
    perror("execlp");
    exit(1);
  }

  bool running = true;

  fd_set read_fdset;
  while (running) {
    FD_ZERO(&read_fdset);
    FD_SET(master_fd, &read_fdset);

    // Listen to reads in fdset
    select(master_fd + 1, &read_fdset, NULL, NULL, NULL);
    if (FD_ISSET(master_fd, &read_fdset)) {
      readfrompty();
    }
  }

  return EXIT_SUCCESS;
}
