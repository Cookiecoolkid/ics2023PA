#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

#define KEYDOWN_MASK 0x8000

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  yield();
  for(int i = 0; i < len; i++) putch(*((char*)buf + i));
  return len;
}


size_t events_read(void *buf, size_t offset, size_t len) {
  yield();
  AM_INPUT_KEYBRD_T keybrd_event = io_read(AM_INPUT_KEYBRD);
  int scancode = keybrd_event.keycode;
  bool keydown = keybrd_event.keydown;
  // int scancode = keycode | (keydown ? KEYDOWN_MASK : 0);
  ((char*)buf)[0] = '\0';
  if (!strcmp(keyname[scancode], "NONE")) 
	  return 0;
  else if(keydown)
  {
	  //printf("keydown: %s\n", keyname[scancode]);
	  //printf("scancode: %d, keyname: %s\n", scancode, keyname[scancode]);
	  int ret = sprintf((char*)buf, "kd %s", keyname[scancode]);
	  //printf("buf: %s\n", buf);
	  //printf("%d\n", ret);
	  return ret;
  }
  else
  {	  
	  //printf("keyup: %s\n", keyname[scancode]);
	  int ret = sprintf((char*)buf, "ku %s", keyname[scancode]);
	  //printf("buf: %s\n", buf);
	  //printf("%d\n", ret);
	  return ret;
  }
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T gpu_config = io_read(AM_GPU_CONFIG);
  int width = gpu_config.width;
  int height = gpu_config.height;
  buf += sprintf(buf, "Width : %d\n", width);
  buf += sprintf(buf, "Height:%d\n", height);
  return 0;
}

size_t fb_write(void *buf, size_t offset, size_t len) {
  // printf("fb_write: offset = %d, len = %d\n", offset, len);
  yield();
  AM_GPU_CONFIG_T gpu_config = io_read(AM_GPU_CONFIG);
  int screen_width = gpu_config.width;
  int x = offset % screen_width;
  int y = offset / screen_width;
   
  io_write(AM_GPU_FBDRAW, x, y, buf, len, 1, true);
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
