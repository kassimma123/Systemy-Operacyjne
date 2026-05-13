#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#define ROZM_BLOKU 1024
int main(int argc, char **argv) { // POSIX
  int fd = shm_open("./nazwa_pam", O_CREAT | O_RDWR, 0660);
  ftruncate(fd, ROZM_BLOKU);

  char *wsk =
      (char *)mmap(NULL, ROZM_BLOKU, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  printf("odczytano z pam. wspolnej: %s\n", wsk);
  munmap(wsk, ROZM_BLOKU);
  shm_unlink("./nazwa_pam");
  return 0;
}
