#define _GNU_SOURCE

#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE 1024 * 64

static int child_proc(void *arg) {
  printf("child running: \n");
  struct utsname buf;
  char *new_name = "virt";
  sethostname(new_name, strlen(new_name));

  uname(&buf);
  printf("nodename in child: %s\n", buf.nodename);
  printf("namespace pid (child): %d\n", getpid());
  printf("namespace ppid: %d\n", getppid());

  if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) {
    perror("mount error");
  }

  mkdir("./proc2", 0555);
  if (mount("proc", "/proc", "proc", 0, NULL) == -1) {
    perror("mount error");
  }
  printf("mounting procfs\n");

  // if (chroot(".") == -1) {
  //   perror("chroot");
  // }

  // printf("here:");
  execlp("sh", "sh", (char *)NULL);

  perror("execlp");

  return 0;
}

int main(void) {
  char *stack = malloc(STACK_SIZE);
  pid_t pid;
  if ((pid = clone(child_proc, stack + STACK_SIZE, CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWPID | SIGCHLD,
                   NULL /*arg*/)) == -1) {
    perror("clone error");
  }

  printf("pid by clone: %d\n", pid);

  waitpid(pid, NULL, 0);
  // parent proc
  // sleep(500);
  printf("parent running: \n");
  struct utsname buf;
  uname(&buf);
  printf("nodename in parent: %s\n", buf.nodename);

  return 0;
}
