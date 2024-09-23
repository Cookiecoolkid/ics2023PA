#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;
static struct timeval init_time;


uint32_t NDL_GetTicks() {
  struct timeval now;
  gettimeofday(&now, NULL);
  uint32_t ret = (now.tv_sec - init_time.tv_sec) * 1000 + (now.tv_usec - init_time.tv_usec) / 1000;
  return ret;
}

int NDL_PollEvent(char *buf, int len) {
  memset(buf, 0, len);
  int fd = open("/dev/events", 0);
  read(fd, buf, len);
  // printf("Event: %s\n", buf);
  int ret = strlen(buf);
  close(fd);
  return ret;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
  else
  {
	 int fd = open("/proc/dispinfo", 0);
	 char buf[64];
	 read(fd, buf, sizeof(buf));
	 close(fd);
	 int idx = 0;
	 int width = 0, height = 0;
	 while(buf[idx] != '\n')
	 {
		idx++;
		if (buf[idx] >= '0' && buf[idx] <= '9')
			width = width * 10 + buf[idx] - '0';
	 }
	 idx++;
	 while(buf[idx] != '\n')
	 {
		 idx++;
		 if(buf[idx] >= '0' && buf[idx] <= '9')
			 height = height * 10 + buf[idx] - '0';
	 }
	 screen_w = width, screen_h = height;
	 if (*w == 0 && *h ==0) 
	 {
		 canvas_w = screen_w;
		 canvas_h = screen_h;
		 *w = screen_w;
		 *h = screen_h;
	 }
	 else
	 {
		 canvas_w = *w;
		 canvas_h = *h;
	 }
	 printf("Get Canvas_w = %d, Canvas_h = %d\n", canvas_w, canvas_h);
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
	int fd = open("/dev/fb", 0);
    for(int i = 0; i < h && y + i < canvas_h; i++)
	{
		//x = (screen_w - w) / 2, y = (screen_h - h) / 2;
		size_t offset = (y + i) * screen_w + x;
		size_t pix_offset = i * w;
		lseek(fd, offset, SEEK_SET); 
		size_t len = w < canvas_w - x ? w : canvas_w - x;
		write(fd, pixels + pix_offset, len);	
	}
	close(fd);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  gettimeofday(&init_time, NULL);
  return 0;
}

void NDL_Quit() {
}
