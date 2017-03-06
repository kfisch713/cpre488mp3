#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "launcher_commands.h"

// from device tree
#define BTN_ADDRESS 0x41240000
#define SW_ADDRESS  0x41200000

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
  unsigned int duration = 500;
  char* BTNs = NULL;
  char* SWs = NULL;

  
  fd = open(dev, O_RDWR);
  if (fd == -1) {
    perror("Couldn't open file: %m");
    exit(1);
  }
  
  int dev_mem_ptr = open("/dev/mem", O_RDONLY);
  BTNs = mmap(NULL, 1, PROT_READ, MAP_SHARED, dev_mem_ptr, BTN_ADDRESS);
  SWs = mmap(NULL, 1, PROT_READ, MAP_SHARED, dev_mem_ptr, SW_ADDRESS);
  
  launcher_cmd(fd, LAUNCHER_FIRE);
  
  while (1){
	  printf("buttons: %x\t", *BTNs);
	  printf("switches: %x\n", *SWs);
	  
	  sleep(1);
  }
  
  
  
  //Switch 1 exits the loop
  while( !(*SWs & 1) ){
    printf("BTNs: %x\n", *BTNs);
    
    switch(*BTNs & 0b00011111){
	  //Button up
  	  case 1:
	    cmd = LAUNCHER_UP;
		break;
	  
	  //Button down
	  case 2:
	    cmd = LAUNCHER_DOWN;
		break;
	  
	  //Button left
	  case 4:
	    cmd = LAUNCHER_LEFT;
		break;
	  
	  //Button right
	  case 8:
	    cmd = LAUNCHER_RIGHT;
		break;
	  
	  //Button center
	  case 16: 
		cmd = LAUNCHER_FIRE;
		break;
	
	  //Default
	  default:
		cmd = LAUNCHER_STOP;
		break;
    }
    usleep(200);
    launcher_cmd(fd, cmd);
    //usleep(duration * 1000);
    //launcher_cmd(fd, LAUNCHER_STOP);
  }
  
  munmap(BTNs, 1);
  munmap(SWs, 1);
  close(dev_mem_ptr);
  close(fd);
  return EXIT_SUCCESS;
}