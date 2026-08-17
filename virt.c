#define _GNU_SOURCE

#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
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

void virt_init() {
  if ((mount(NULL, "/", NULL, MS_PRIVATE, NULL)) == -1) {
    perror("can't change mount propagation of the root");
    exit(EXIT_FAILURE);
  }

  int chk = mkdir("jail", 0777);
  if (errno != EEXIST && chk == -1) {
    perror("can't create jail directory\n");
    exit(EXIT_FAILURE);
  }

  /*create required directories in the jail - /usr/bin, /usr/lib, /lib64 */
  chk = mkdir("jail/usr", 0777);
  chk = mkdir("jail/usr/bin", 0777);
  chk = mkdir("jail/usr/lib", 0777);
  chk = mkdir("jail/lib64", 0777);
  chk = mkdir("jail/proc", 0777);
  chk = mkdir("jail/lib", 0777);
  if (errno != EEXIST && chk == -1) {
    perror("can't create required directories\n");
    exit(EXIT_FAILURE);
  }

  /*bind mount the required directories*/
  if ((mount("/usr/bin", "jail/usr/bin", NULL, MS_BIND | MS_REC, NULL) == -1) ||
      (mount("/usr/lib", "jail/usr/lib", NULL, MS_BIND | MS_REC, NULL)) == -1 ||
      (mount("/lib64", "jail/lib64", NULL, MS_BIND | MS_REC, NULL)) == -1 ||
      (mount("/lib", "jail/lib", NULL, MS_BIND | MS_REC, NULL)) == -1 ||
      (mount("/proc", "jail/proc", "proc", 0, NULL)) == -1) {
    perror("can't perform bind mount of required directories");
    exit(EXIT_FAILURE);
  }

  if (chroot("./jail") == -1) {
    perror("chroot");
  }

  if (chdir("/") == -1) {
    perror("chdir");
  }

  char *const _argv[] = {"/usr/bin/sh", NULL};
  char path[1024];
  snprintf(path, 1024, "PATH=%s", getenv("PATH"));
  char term[1024];
  snprintf(term, 1024, "TERM=%s", getenv("TERM"));
  char *const _env[] = {path, term, NULL};
  execve("/usr/bin/sh", _argv, _env);

  perror("execve");
}

static int child_proc(void *arg) {
  virt_init();

  return 0;
}

int main(void) {
  char *stack = malloc(STACK_SIZE);
  pid_t pid;
  if ((pid = clone(child_proc, stack + STACK_SIZE, CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWPID | SIGCHLD, NULL)) == -1) {
    perror("clone error");
  }

  waitpid(pid, NULL, 0);
  return 0;
}
