#include <asm-generic/errno-base.h>
#define _GNU_SOURCE

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
  int chk = mkdir("jail", 0700);
  if (errno != EEXIST && chk == -1) {
    perror("can't create jail directory\n");
    exit(EXIT_FAILURE);
  }

  if (chroot("./jail") == -1) {
    perror("chroot");
  }

  if (chdir("/") == -1) {
    perror("chdir");
  }

  /*create required directories in the jail - /usr/bin, /usr/lib, /lib64 */
  chk = mkdir("usr/bin", 0777);
  chk = mkdir("usr/lib", 0777);
  chk = mkdir("lib64", 0777);
  chk = mkdir("proc", 0777);
  if (errno != EEXIST && chk == -1) {
    perror("can't create required directories\n");
    exit(EXIT_FAILURE);
  }

  /*bind mount the required directories*/
  if ((mount("/usr/bin", "usr/bin", NULL, MS_BIND | MS_REC, NULL) == -1) ||
      (mount("/usr/lib", "usr/lib", NULL, MS_BIND | MS_REC, NULL)) == -1 ||
      (mount("/lib64", "lib64", NULL, MS_BIND | MS_REC, NULL)) == -1 ||
      (mount("proc", "/proc", "proc", 0, NULL)) == -1) {
    perror("can't perform bind mount of required directories");
    exit(EXIT_FAILURE);
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

  mkdir("proc", 0777);
  if (mount("proc", "/proc", "proc", 0, NULL) == -1) {
    perror("mount error");
  }
  printf("mounting procfs\n");

  // DIR *dp = opendir("usr/bin");
  // for (;;) {
  //   struct dirent *dent = readdir(dp);
  //   if (!dent)
  //     break;
  //
  //   if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0 || strcmp(dent->d_name, "sh") != 0)
  //     continue;
  //
  //   printf("here: %s\n", dent->d_name);
  // }

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
  printf("parent running: \n");
  struct utsname buf;
  uname(&buf);
  printf("nodename in parent: %s\n", buf.nodename);

  return 0;
}
