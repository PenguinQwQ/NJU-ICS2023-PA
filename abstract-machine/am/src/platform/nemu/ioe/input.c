#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000
#define KEYCODE_MASK 0x000000FF
//16 * 16 = 256

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t keystate = inl(KBD_ADDR);
  kbd->keydown = (keystate & KEYDOWN_MASK) ? true : false;
  kbd->keycode = keystate & KEYCODE_MASK;
}