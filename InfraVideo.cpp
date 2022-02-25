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
static uint16_t mlxFrameRaw[834];

#define IR_FRAME_WIDTH 24
#define IR_FRAME_HEIGHT 32
#define IR_FRAME_SIZE (IR_FRAME_WIDTH * IR_FRAME_HEIGHT)
static float irFrame[IR_FRAME_SIZE];
static float irFrameAvg[IR_FRAME_SIZE];

void get_frame()
{
  const float trShift = 8.f;
  float eTa;
  float emissivity = 1;
  int retries = 10;
  int subpage;
  uint8_t subpages[2] = {0,0};

  while (retries-- && (!subpages[0] || !subpages[1]))
  {
    MLX90640_GetFrameData(MLX_I2C_ADDR, mlxFrameRaw);
    subpage = MLX90640_GetSubPageNumber(mlxFrameRaw);
    printf("Got data for page %d\n", subpage);

    subpages[subpage] = 1;

    eTa = MLX90640_GetTa(mlxFrameRaw, &mlx90640) - trShift;
    MLX90640_CalculateTo(mlxFrameRaw, &mlx90640, emissivity, eTa, irFrame);

    MLX90640_BadPixelsCorrection((&mlx90640)->brokenPixels, irFrame, 1, &mlx90640);
    MLX90640_BadPixelsCorrection((&mlx90640)->outlierPixels, irFrame, 1, &mlx90640);
  }

  printf("Got frame\n");
}

void average_frame()
{
  for (int i = 0; i < IR_FRAME_SIZE; ++i)
  {
    irFrameAvg[i] = (irFrameAvg[i] + irFrame[i]) * 0.5f;
  }
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

    for (int i = 0; i < IR_FRAME_SIZE; ++i)
      irFrameAvg[i] = 20.f;

    display_start(display);

    stdio_init_all();

    printf("Starting\n");
    MLX90640_I2CInit();
    MLX90640_I2CFreqSet(1000);

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
    if (MLX90640_SetRefreshRate(MLX_I2C_ADDR, 0b101) != 0)
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

    // Sometimes get bad data on first frame, so fetch a frame before the loop
    get_frame();

    while (1)
    {
      get_frame();
      average_frame();
#if 0
      for (int i = 0; i < 24; i += 2)
      {
        for (int j = 0; j < 32; j += 4)
        {
          printf("%.1f ", irFrame[i*32 + j]);
        }
        printf("\n");
      }
#endif
      for (int i = 0; i < IR_FRAME_HEIGHT - 1; ++i)
      {
        for (int j = 0; j < IR_FRAME_WIDTH - 1; ++j)
        {
          float tl = irFrameAvg[(23-j)*32 + i];
          float tr = irFrameAvg[(22-j)*32 + i];
          float bl = irFrameAvg[(23-j)*32 + i + 1];
          float br = irFrameAvg[(22-j)*32 + i + 1];
          for (int k = 0; k < 5; ++k)
          {
            float l = (tl * (5-k) + bl * k) * 0.2f;
            float r = (tr * (5-k) + br * k) * 0.2f;
            for (int m = 0; m < 5; ++m)
            {
              float val = (l * (5-m) + r * m) * 0.2f;
              uint8_t col = colour_from_value(val);
              display[FRAME_WIDTH * ((i + 1) * 5 + k) + (j + 1) * 5 + m] = col;
            }
          }
        }
      }

      //sleep_ms(1000);
    }
}
