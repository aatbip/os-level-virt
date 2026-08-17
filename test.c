#define _GNU_SOURCE

#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  int pid = fork();
  if (pid == 0) {
    printf("child: %d\n", getpid());
    sleep(30);
    if (unshare(CLONE_NEWNS) == -1)
      perror("unshare");
    printf("child mount unshared\n");
    sleep(30);
    printf("Child mounting tmpfs...\n");
    mount("tmp", "tmp", "tmpfs", MS_REC | MS_PRIVATE, NULL);
    sleep(20);
    printf("Child opening file in tmp\n");
    int fd = open("tmp/child_file", O_CREAT | O_APPEND | O_RDWR, 0700);
    printf("fd: %d\n", fd);
    sleep(1);
    printf("Child writing file\n");
    const char buf[] = "child file";
    if (write(fd, buf, strlen(buf)) == -1)
      perror("writeC");
    sleep(300);
  } else {
    printf("parent: %d\n", getpid());
    sleep(80);
    printf("Parent mounting tmpfs...\n");
    mount("tmp", "tmp", "tmpfs", 0, NULL);
    sleep(20);
    printf("Parent opening file in tmp\n");
    int fd = open("tmp/parent_file", O_CREAT | O_APPEND | O_RDWR, 0700);
    printf("fd: %d\n", fd);
    sleep(1);
    printf("Parent writing file\n");
    const char buf[] = "parent file";
    if (write(fd, buf, strlen(buf)) == -1)
      perror("writeP");
    sleep(300);
  }
  return 0;
}
