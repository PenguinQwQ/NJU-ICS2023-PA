#include <am.h>
#include <nemu.h> 

#define SYNC_ADDR (VGACTL_ADDR + 4)
//static AM_GPU_CONFIG_T gpu;
static uint32_t wid, hei;
void __am_gpu_init() {
  uint32_t wh = inl(VGACTL_ADDR);
  hei = wh & 0xFFFF;
  wid = (wh >> 16) & 0xFFFF;
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = wid, .height = hei,
    .vmemsz = wid * hei * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {//implement the gpu fbdraw function in software level
  int x = ctl->x, y = ctl->y, w = ctl->w, h = ctl->h;
  uint32_t *color = ((uint32_t *)ctl->pixels);
  for (int Y = y ; Y <= y + h - 1 ; Y++)
    for (int X = x ; X <= x + w - 1 ; X++)
      {
        uint32_t *vmem_ptr = (uint32_t *)(FB_ADDR + Y * wid * sizeof(uint32_t) + X * sizeof(uint32_t));
        outl((uint32_t)vmem_ptr, *color); //write *color data to the vmem_ptr
        color++;
      }
  if (ctl->sync) {
  //if the software requires to sync, it write SYNC_ADDR with 1
  //SYNC_ADDR is 1 means it will display the vmem to the VGA screen
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}