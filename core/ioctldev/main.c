#include "ioctl_dev.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h> 

void ioctl_set_msg(int file_desc, char *message) {
  int ret_val;

  ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);

  if (ret_val < 0) {
    printf("ioctl_set_msg failed:%d\n", ret_val);
    exit(-1);
  }

  printf("set_msg message: %s\n", message);
}

void ioctl_get_msg(int file_desc) {
  int ret_val;
  char message[100];

  ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

  if (ret_val < 0) {
    printf("ioctl_get_msg failed:%d\n", ret_val);
    exit(-1);
  }

  printf("get_msg message:%s\n", message);
}

void ioctl_get_nth_byte(int file_desc) {
  int i;
  char c;

  printf("get_nth_byte message:");

  i = 0;
  do {
    c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);
    if (c < 0) {
      printf("ioctl_get_nth_byte failed at the %d'th byte:\n", i);
      exit(-1);
    }

    putchar(c);
  } while (c != 0);
  putchar('\n');
}

int main() {
  int file_desc, ret_val;
  char *msg = "HELLO IOCTL\n";

  file_desc = open(DEVICE_FILE_NAME, 0);
  if (file_desc < 0) {
    printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
    exit(-1);
  }

  ioctl_get_nth_byte(file_desc);
  ioctl_get_msg(file_desc);
  ioctl_set_msg(file_desc, msg);

  close(file_desc);
  exit(EXIT_SUCCESS);
}
