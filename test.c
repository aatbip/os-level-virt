#define _GNU_SOURCE

#include <dirent.h>
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
    if (unshare(CLONE_NEWNS) == -1)
      perror("unshare");
    printf("child mount unshared\n");
    sleep(1);
    printf("Child mounting tmpfs...\n");
    mount("tmp", "tmp", "tmpfs", MS_REC | MS_PRIVATE, NULL);
    sleep(1);
    printf("Child opening file in tmp\n");
    int fd = open("tmp/child_file", O_CREAT | O_APPEND | O_RDWR, 0700);
    printf("fd: %d\n", fd);
    sleep(1);
    printf("Child writing file\n");
    const char buf[] = "child file";
    if (write(fd, buf, strlen(buf)) == -1)
      perror("writeC");
    DIR *d = opendir("tmp");
    for (;;) {
      struct dirent *dir = readdir(d);
      if (!dir)
        break;
      printf("C files: %s\n", dir->d_name);
    }
    sleep(5);
  } else {
    printf("parent: %d\n", getpid());
    sleep(5);
    printf("Parent mounting tmpfs...\n");
    mount("tmp", "tmp", "tmpfs", 0, NULL);
    sleep(1);
    printf("Parent opening file in tmp\n");
    int fd = open("tmp/parent_file", O_CREAT | O_APPEND | O_RDWR, 0700);
    printf("fd: %d\n", fd);
    sleep(1);
    printf("Parent writing file\n");
    const char buf[] = "parent file";
    if (write(fd, buf, strlen(buf)) == -1)
      perror("writeP");
    DIR *d = opendir("tmp");
    for (;;) {
      struct dirent *dir = readdir(d);
      if (!dir)
        break;
      printf("P files: %s\n", dir->d_name);
    }
    sleep(5);
  }
  return 0;
}
