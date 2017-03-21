#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#include "launcher_commands.h"

#define SW_ADDRESS  0x41240000
#define FRM_BUFF1	0x10000000

#define HEIGHT 1080
#define WIDTH 1920
#define CENTER_TOLERANCE 20
#define FRAME_LEN (HEIGHT * WIDTH)

#define TARGET_COLOR 0xBEEF
#define WHITE_COLOR 0xFFFF
#define COLOR_TOLERANCE 0x00FF

static int close_to(uint16_t pixel, uint16_t target) {
  return (pixel <= target + COLOR_TOLERANCE && pixel >= TARGET_COLOR - COLOR_TOLERANCE);
}

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
  char *dev = LAUNCHER_NODE;
  unsigned int duration = 1000;
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
  
  SWs = mmap(NULL, 1, PROT_READ, MAP_SHARED, dev_mem_ptr, SW_ADDRESS);
  IMAGE_BUFF1 = mmap(NULL, FRAME_LEN * sizeof(*IMAGE_BUFF1), PROT_READ | PROT_WRITE, MAP_SHARED, dev_mem_ptr, FRM_BUFF1);
  
  printf("offset page size = %lu\n", sysconf(_SC_PAGE_SIZE));
  if(IMAGE_BUFF1 == MAP_FAILED){
	  perror("Failed to map the buffer. %m");
	  exit(1);
  }
  
  printf("made it past memory mapping\nEntering search and destroy loop\n");

  int i;
  int last_col, last_row;
  int num_vert_changes , num_hor_changes;
  int farthest_left;
  int curr_col, curr_row;
  int cmd, on_target = 0;
  
  //Switch 1 exits the loop
  while( !(*SWs & 1) ){
    num_hor_changes = num_vert_changes = 0;
    farthest_left = WIDTH - 1;
    curr_col = -1, curr_row = -1;

    printf("Searching...\n");
    // If the previously found location is still TARGET_COLOR,
    // assume that we are still on target and continue with centering the turret.
    while(on_target && !(*SWs & 1)) {

      // Default to stopped. 
      cmd = LAUNCHER_STOP;

      // If Up go up, else, go down.
      // HOLD...
      if (last_row < HEIGHT/2 - CENTER_TOLERANCE)
        cmd |= LAUNCHER_UP;
      else if (last_row > HEIGHT/2 + CENTER_TOLERANCE)
        cmd |= LAUNCHER_DOWN;

      //If Right , go right, else, go left.
      // HOLD....
      if (last_col < WIDTH/2 - CENTER_TOLERANCE)
        cmd |= LAUNCHER_LEFT;
      else if (last_col > WIDTH/2 + CENTER_TOLERANCE)
        cmd |= LAUNCHER_RIGHT;

      // IF cmd still is equal to LAUNCHER_STOP. We are centered.
      // LOOOSSSEEEE!!!!!
      launcher_cmd(fd, cmd);

      // If we just fired a shot. Let's take a break and find the target again.
      if (cmd == LAUNCHER_FIRE) {
        // stop the turret on default.
        launcher_cmd(fd, LAUNCHER_STOP);
        break;
      }

      // Identify if we are still on target.
      on_target = close_to(IMAGE_BUFF1[(last_row * WIDTH) + last_col], TARGET_COLOR);
    }



    // Look in each row for the first TARGET_COLOR
    // This value is hopefully the top of the target
    while(curr_col < 0 && !(*SWs & 1)) {
      for (i = 0; i < FRAME_LEN; ++i) {
        if (close_to(IMAGE_BUFF1[i], (uint16_t)TARGET_COLOR)) {
          curr_col = i % WIDTH;
          break;
        }
      }
    }
    
    // Scan down the curr_col to verify that we are looking at a target
    for (i = curr_col; i < FRAME_LEN; i += WIDTH) {
      uint16_t target = (num_vert_changes % 2) ? TARGET_COLOR : WHITE_COLOR;
      if (close_to(IMAGE_BUFF1[i], target)) {
        num_vert_changes++;
      }

      if (num_vert_changes >= 5)
        break;
    }

    // If we did not find something close enough to the image, start over.
    if (num_vert_changes < 5)
      continue;

    // Find the farthest left pixel of red that we can.
    // This pixel is hopefully the Left of the target.
    for (i = 0; i < FRAME_LEN; ++i) {
      if (close_to(IMAGE_BUFF1[i], TARGET_COLOR)) {
        if ((i % WIDTH) < farthest_left) {
          farthest_left = (i % WIDTH);
        }
      }
    }

    curr_row = farthest_left;
    // Scan down the curr_row to verify that we are looking at a target
    for (i = curr_col; i < FRAME_LEN; i += WIDTH) {
      uint16_t target = (num_hor_changes % 2) ? TARGET_COLOR : WHITE_COLOR;
      if (close_to(IMAGE_BUFF1[i], target)) {
        num_hor_changes++;
      }

      if (num_hor_changes >= 5)
        break;
    }

    if (num_hor_changes < 5) 
      continue;

    on_target = 1;
    last_col = curr_col;
    last_row = curr_row;
    printf("Target Acquired...\n");

  }
	
  
  munmap(SWs, 1);
  munmap(IMAGE_BUFF1, (HEIGHT * WIDTH * sizeof(*IMAGE_BUFF1)));
  close(dev_mem_ptr);
  close(fd);
  return EXIT_SUCCESS;
}

