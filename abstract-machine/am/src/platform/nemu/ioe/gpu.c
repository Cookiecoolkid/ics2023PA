#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)
#define VGA_WINDOW_MASK 0xffff0000
void __am_gpu_init() {
	int i;
	int w = io_read(AM_GPU_CONFIG).width;
	int h = io_read(AM_GPU_CONFIG).height;
	uint32_t* fb = (uint32_t*)(uintptr_t)FB_ADDR;
	for (i = 0; i < w * h; i++) fb[i] = 0;
		
	outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) { //通过 inl 完成 io设备与内存数据的交互，这个步骤进一步被封装为__am_device_...()函数，调用函数即完成设备的写入
     uint32_t vga_window = inl(VGACTL_ADDR); //写入完成后即可通过 io_read io_write 直接读取这个设备的结构体，从而得到对应数据
	 uint16_t w = (uint16_t)(((uint32_t)(vga_window & VGA_WINDOW_MASK)) >> 16);
	 uint16_t h = (uint16_t)(vga_window & ~VGA_WINDOW_MASK);
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = w, .height = h,
    .vmemsz = w * h * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t * fb = (uint32_t*)(uintptr_t)FB_ADDR;
  uint32_t * pixels = (uint32_t*)ctl->pixels;
  uint16_t W = (uint16_t)(((uint32_t)(inl(VGACTL_ADDR) & VGA_WINDOW_MASK)) >> 16);
  for (int i = 0; i < ctl->w; i++)
	  for(int j = 0; j < ctl->h; j++)
	  	 fb[(j + ctl->y) * W + (i + ctl->x)] = pixels[j * ctl->w + i]; // similar to VGA lab (ctl->x, ctl->y) -> (ctl->x + ctl->w, ctl->y + ctl->h)
  
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
