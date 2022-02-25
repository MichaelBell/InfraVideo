#pragma once

#ifdef __cplusplus
extern "C" {
#endif
// Display 320x240 8bpp RGB 332 buffer using core 1
// Note this changes clock frequency so peripherals should be initialized
// after calling display_start
void display_start(uint8_t* frame);
#ifdef __cplusplus
}
#endif

