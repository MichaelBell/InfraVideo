#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

#include "display.h"

#define MLX_I2C_ADDR 0x33

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

static uint8_t display[FRAME_WIDTH * FRAME_HEIGHT];

static paramsMLX90640 mlx90640;
static uint16_t eeMLX90640[832];
static uint16_t frame[834];
static float mlx90640Frame[768];

void get_frame()
{
  float eTa;
  float emissivity = 1;
  int retries = 10;
  int subpage;
  uint8_t subpages[2] = {0,0};

  while (retries-- && (!subpages[0] || !subpages[1]))
  {
    MLX90640_GetFrameData(MLX_I2C_ADDR, frame);
    subpage = MLX90640_GetSubPageNumber(frame);
    printf("Got data for page %d\n", subpage);

    subpages[subpage] = 1;

    eTa = MLX90640_GetTa(frame, &mlx90640);
    MLX90640_CalculateTo(frame, &mlx90640, emissivity, eTa, mlx90640Frame);

    MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, mlx90640Frame, 1, &mlx90640);
    MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, mlx90640Frame, 1, &mlx90640);
  }

  printf("Got frame\n");
}

uint8_t colour_from_value(float v)
{
  // Simple blue/red
  //return (v > 30.f) ? 0x80 : 0x01;
  
  // Range -6 to 54 degrees
  //float hue = (54.f - v) * 0.1f;

  // Range 16 to 34 degrees
  float hue = (34.f - v) * 0.3333333f;
  if (hue < 0.f || hue >= 6.f) hue = 0.f;
  
  int i = (int)floorf(hue);
  uint8_t t = (uint8_t)floorf(8.f * (hue - i));
  uint8_t q = 7 - t;

  switch(i) {
    case 0:
      return 0xe0 | (t << 2);
    case 1:
      return (q << 5) | 0x1c;
    case 2:
      return 0x1c | (t >> 1);
    case 3:
      return (q << 2) | 0x03;
    case 4:
      return (t << 5) | 0x03;
    case 5:
    default:
      return 0xe0 | (q >> 1);
  }
}

int main()
{
    memset(display, 0x00, FRAME_WIDTH * FRAME_HEIGHT);
    display_start(display);

    stdio_init_all();

    printf("Starting\n");
    MLX90640_I2CInit();
    MLX90640_I2CFreqSet(400);

    if (MLX90640_SetDeviceMode(MLX_I2C_ADDR, 0) != 0)
    {
      printf("Failed setting device mode\n");
      return 1;
    }
    printf("Device mode set\n");

    if (MLX90640_SetSubPageRepeat(MLX_I2C_ADDR, 0) != 0)
    {
      printf("Failed setting subpage repeat\n");
      return 1;
    }
    if (MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b100) != 0)
    {
      printf("Failed setting refresh rate\n");
      return 1;
    }
    if (MLX90640_SetChessMode(MLX_I2C_ADDR) != 0)
    {
      printf("Failed setting chess mode\n");
      return 1;
    }
    if (MLX90640_DumpEE(MLX_I2C_ADDR, eeMLX90640) != 0)
    {
      printf("Failed dumping EEPROM\n");
      return 1;
    }
    printf("EEPROM dumped\n");
    if (MLX90640_ExtractParameters(eeMLX90640, &mlx90640) != 0)
    {
      printf("Failed extracting parametes\n");
      return 1;
    }

    printf("Completed setup\n");

    while (1)
    {
      get_frame();
#if 0
      for (int i = 0; i < 24; i += 2)
      {
        for (int j = 0; j < 32; j += 4)
        {
          printf("%.1f ", mlx90640Frame[i*32 + j]);
        }
        printf("\n");
      }
#endif
      for (int i = 0; i < 32; ++i)
      {
        for (int j = 0; j < 24; ++j)
        {
          uint8_t col = colour_from_value(mlx90640Frame[(23-j)*32 + i]);
          display[FRAME_WIDTH * (i + 1) * 4 + (j + 1) * 4] = col;
          display[FRAME_WIDTH * (i + 1) * 4 + (j + 1) * 4 + 1] = col;
          display[FRAME_WIDTH * ((i + 1) * 4 + 1) + (j + 1) * 4] = col;
          display[FRAME_WIDTH * ((i + 1) * 4 + 1) + (j + 1) * 4 + 1] = col;
        }
      }

      //sleep_ms(1000);
    }
}
