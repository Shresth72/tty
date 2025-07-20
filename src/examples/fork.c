#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int fds[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    perror("socketpair");
    exit(EXIT_FAILURE);
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    // --- Child Process ---
    close(fds[0]); // Close parent end

    const char *msg = "Hello from child\n";
    write(fds[1], msg, strlen(msg)); // Send to parent

    char buf[128];
    ssize_t n = read(fds[1], buf, sizeof(buf) - 1); // Receive from parent
    if (n > 0) {
      buf[n] = '\0';
      printf("Child got: %s", buf);
    }

    exit(EXIT_SUCCESS);
  }

  // --- Parent Process ---
  close(fds[1]); // Close child end

  bool running = true;
  fd_set readfds;
  while (running) {
    FD_ZERO(&readfds);
    FD_SET(fds[0], &readfds);

    if (select(fds[0] + 1, &readfds, NULL, NULL, NULL) == -1) {
      perror("select");
      break;
    }

    if (FD_ISSET(fds[0], &readfds)) {
      char buf[128];
      ssize_t n = read(fds[0], buf, sizeof(buf) - 1);
      if (n > 0) {
        buf[n] = '\0';
        printf("Parent got: %s", buf);

        // Respond back
        const char *reply = "Hello back from parent\n";
        write(fds[0], reply, strlen(reply));
        running = false;
      }
    }
  }

  return 0;
}
