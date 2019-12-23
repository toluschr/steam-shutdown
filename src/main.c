#include <stdbool.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>

#if defined(_WIN32)
#include <sys/stat.h>
#include <windows.h>
#include <process.h>
#include <tlhelp32.h>
#include <winbase.h>

#define PATH_DELIMETER '\\'
#endif

#if defined(UNIX) || defined(__unix__)
#include <sys/reboot.h>
#include <unistd.h>

#define PATH_DELIMETER '/'
#endif

#define TIMEOUT 1000 * 10

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


int __getpid (void) {
#if defined(UNIX) || defined(__unix__)
  return getpid();
#endif
#if defined(_WIN32)
  return GetCurrentProcessId();
#endif
}

#if defined(LINUX) || defined(__linux__)
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
#endif
#if defined(_WIN32)
void killall (const char *name, int sig) {
  HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
  PROCESSENTRY32 pEntry;
  pEntry.dwSize = sizeof (pEntry);
  BOOL hRes = Process32First(hSnapShot, &pEntry);
  do {
    if (strstr(pEntry.szExeFile, name) == NULL) continue;
    if (pEntry.th32ProcessID == __getpid()) continue;

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)pEntry.th32ProcessID);
    if (hProcess == NULL)  continue;
    
    TerminateProcess(hProcess, sig);
    CloseHandle(hProcess);
  } while ((hRes = Process32Next(hSnapShot, &pEntry)));
  CloseHandle(hSnapShot);
}
#endif

int poweroff () {
#if defined(UNIX) || defined(__unix__)
  return reboot(RB_POWER_OFF);
#endif

#if defined(_WIN32)
  return ExitWindowsEx(EWX_POWEROFF, SHTDN_REASON_FLAG_PLANNED);
#endif
}

int msleep (int millis) {
#if defined(UNIX) || defined(__unix__)
  return usleep(millis * 1000);
#endif

#if defined(_WIN32)
  Sleep(millis);
  return 0;
#endif
}

int main (int argc, char **argv) {
  if (argc != 2) quit("!> Missing Library Path", 1);
#if defined(UNIX) || defined(__unix__)
  if (geteuid() != 0) quit("!> Requires Root Permissions", 1);
#endif

  char downloading_path[4096] = {0};

  sprintf(downloading_path, "%s%cdownloading", argv[1], PATH_DELIMETER);

  DIR *downloading = opendir(downloading_path);
  int queued;

  do {
    queued = 0;
    struct dirent *entry;
    while ((entry = readdir(downloading)) != NULL) {
      if (strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".") == 0) continue;
#if defined(_WIN32)
      struct _stat st;
      char buffer[4096] = {0};
      sprintf(buffer, "%s%c%s", downloading_path, PATH_DELIMETER, entry->d_name);
      _stat(buffer, &st);
      queued += (st.st_mode & S_IFDIR != 0);
#else
      queued += (entry->d_type == 4);
#endif
    }
    rewinddir(downloading);
    printf(":> Queued %i Download(s)\n", queued);
  } while (queued > 0 && msleep(TIMEOUT) == 0);

  closedir(downloading);

  printf(":> Killing Steam!\n");
#if defined(_WIN32)
  killall("steam", SIGTERM);
#else
  killall("steam", SIGQUIT);
#endif
  msleep(TIMEOUT);
  poweroff();
}

