#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/vreg.h"
#include "pico/sem.h"

#include "dvi.h"
#include "dvi_serialiser.h"
#include "common_dvi_pin_configs.h"

// TMDS bit clock 252 MHz
// DVDD 1.2V (1.1V seems ok too)
#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240
#define VREG_VSEL VREG_VOLTAGE_1_20
#define DVI_TIMING dvi_timing_640x480p_60hz

struct dvi_inst dvi0;
static uint8_t* framebuf;

void core1_main() {
  dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
  while (queue_is_empty(&dvi0.q_colour_valid))
    __wfe();
  dvi_start(&dvi0);
  dvi_scanbuf_main_8bpp(&dvi0);
  __builtin_unreachable();
}

void core1_scanline_callback() {
  // Discard any scanline pointers passed back
  uint8_t *bufptr;
  while (queue_try_remove_u32(&dvi0.q_colour_free, &bufptr))
    ;
  // Note first two scanlines are pushed before DVI start
  static uint scanline = 2;
  bufptr = &framebuf[FRAME_WIDTH * scanline];
  queue_add_blocking_u32(&dvi0.q_colour_valid, &bufptr);
  scanline = (scanline + 1) % FRAME_HEIGHT;
}

void display_start(uint8_t* frame_ptr)
{
  framebuf = frame_ptr;
  vreg_set_voltage(VREG_VSEL);
  sleep_ms(10);
  set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

  dvi0.timing = &DVI_TIMING;
  dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
  dvi0.scanline_callback = core1_scanline_callback;
  dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

  // Once we've given core 1 the framebuffer, it will just keep on displaying
  // it without any intervention from core 0
  queue_add_blocking_u32(&dvi0.q_colour_valid, &frame_ptr);
  frame_ptr += FRAME_WIDTH;
  queue_add_blocking_u32(&dvi0.q_colour_valid, &frame_ptr);

  multicore_launch_core1(core1_main);
}
