#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include "launcher_commands.h"

// from device tree
#define BTN_ADDRESS 0x41200000
#define SW_ADDRESS  0x41240000
#define FRM_BUFF1	0x10000000

#define HEIGHT 1080
#define WIDTH 1920
#define FRAME_LEN (HEIGHT * WIDTH)

static void launcher_cmd(int fd, int cmd) {
  int retval = 0;
  
  retval = write(fd, &cmd, 1);
  while (retval != 1) {
    if (retval < 0) {
      fprintf(stderr, "Could not send command to %s (error %d)\n", LAUNCHER_NODE, retval);
    } 

    else if (retval == 0) {
      fprintf(stdout, "Command busy, waiting...\n");
    }
  }

  if (cmd == LAUNCHER_FIRE) {
    usleep(5000000);
  }

}

int main() {
  char c;
  int fd;
  int cmd = LAUNCHER_STOP;
  char *dev = LAUNCHER_NODE;
  unsigned int duration = 1000;
  int* BTNs = NULL;
  int* SWs = NULL;
  
  uint16_t *IMAGE_BUFF1 = NULL;


  
  fd = open(dev, O_RDWR);
  if (fd == -1) {
    perror("Couldn't open file: %m");
    exit(1);
  }
  
  int dev_mem_ptr = open("/dev/mem", O_RDWR);
  if (dev_mem_ptr == -1) {
    perror("Couldn't open file: %m");
    exit(1);
  }
  
  BTNs = mmap(NULL, 1, PROT_READ, MAP_SHARED, dev_mem_ptr, BTN_ADDRESS);
  SWs = mmap(NULL, 1, PROT_READ, MAP_SHARED, dev_mem_ptr, SW_ADDRESS);
  IMAGE_BUFF1 = mmap(NULL, FRAME_LEN * sizeof(*IMAGE_BUFF1), PROT_READ | PROT_WRITE, MAP_SHARED, dev_mem_ptr, FRM_BUFF1);
  
  if(IMAGE_BUFF1 == MAP_FAILED){
	  perror("Failed to map the buffer. %m");
	  exit(1);
  }
  
  printf("made it past memory mapping\n");
  sleep(2);
  //Switch 1 exits the loop
  while( !(*SWs & 1) ){							  
    fflush(stdout);
  	usleep(duration);
  	static int i = 0;

    printf("%d\t%d\n",i, IMAGE_BUFF1[FRAME_LEN/2]);
    i++;
  }
  
  munmap(BTNs, 1);
  munmap(SWs, 1);
  munmap(IMAGE_BUFF1, (HEIGHT * WIDTH * sizeof(*IMAGE_BUFF1)));
  close(dev_mem_ptr);
  close(fd);
  return EXIT_SUCCESS;
}

