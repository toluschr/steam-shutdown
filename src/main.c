#include <stdbool.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>

#if defined(_WIN32)
  #include <windows.h>
#endif

#if defined(UNIX) || defined(__unix__)
  #include <sys/reboot.h>
  #include <unistd.h>
#endif

#define TIMEOUT 1000 * 1000 * 10

void quit (const char *message, int code) {
  printf("%s\n", message);
  exit(code);
}

bool sisdisit (const char *string, size_t l_string) {
  for (int i = 0; i < l_string; i++) {
    if (string[i] < '0' || string[i] > '9') return 0;
  }
  return 1;
}

void killall (const char *name, int sig) {
  DIR *proc = opendir("/proc");

  struct dirent *entry;
  while ((entry = readdir(proc)) != NULL) {
    if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) continue;
    if (!sisdisit(entry->d_name, strlen(entry->d_name))) continue;
    if (atoi(entry->d_name) == getpid()) continue;

    char status_path[4096] = {0};
    sprintf(status_path, "/proc/%s/status", entry->d_name);
    FILE *status = fopen(status_path, "r");

    char buffer[4096] = {0};
    char current;
    while ((current = fgetc(status)) != EOF && current != '\n') strncat(buffer, &current, 1);
    if (strstr(buffer, name) != NULL) kill(atoi(entry->d_name), sig);

    puts(buffer);
    fclose(status);
  }

  closedir(proc);
}

int poweroff () {
#if defined(UNIX) || defined(__unix__)
  return reboot(RB_POWER_OFF);
#endif

#if defined(_WIN32)
  return ExitWindowsEx(RB_POWER_OFF);
#endif
}

int main (int argc, char **argv) {
  if (argc != 2) quit("!> Missing Library Path", 1);
  if (geteuid() != 0) quit("!> Requires Root Permissions", 1);

  char downloading_path[4096] = {0};
  sprintf(downloading_path, "%s/downloading", argv[1]);

  DIR *downloading = opendir(downloading_path);
  int queued;

  do {
    queued = 0;
    struct dirent *entry;
    while ((entry = readdir(downloading)) != NULL) {
      if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) continue;
      queued += (entry->d_type == 4);
    }
    rewinddir(downloading);
    printf(":> Queued %i Download(s)\n", queued);
  } while (queued > 0 && usleep(TIMEOUT) == 0);

  closedir(downloading);

  printf(":> Killing Steam!\n");
  killall("steam", SIGQUIT);
  usleep(TIMEOUT);
  poweroff();
}

