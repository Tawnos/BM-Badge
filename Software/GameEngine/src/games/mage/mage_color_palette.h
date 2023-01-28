#ifndef SOFTWARE_MAGE_COLOR_PALETTE_H
#define SOFTWARE_MAGE_COLOR_PALETTE_H

#include <memory>
#include "mage_rom.h"
#include "utility.h"

class FrameBuffer;
class MageGameEngine;

#define COLOR_PALETTE_INTEGRITY_STRING_LENGTH 2048
#define COLOR_PALETTE_NAME_LENGTH 31
#define COLOR_PALETTE_NAME_SIZE COLOR_PALETTE_NAME_LENGTH + 1

//this is the color that will appear transparent when drawing tiles:
#define TRANSPARENCY_COLOR	0x0020


#ifdef IS_BIG_ENDIAN
struct Color_565
{
   uint8_t r : 5;
   uint8_t g : 5;
   uint8_t a : 1;
   uint8_t b : 5;
};
#else
struct Color_565
{
   Color_565(uint16_t color) noexcept 
      : r((color & 0b0000000011111000) >> 3),
        g((color & 0b1100000000000000) >> 14 | (color & 0b111) << 2),
        b((color & 0b0001111100000000) >> 8),
        a((color & 0b0010000000000000) >> 13)
   {}

   operator uint16_t() const { return r << 14 | g << 9 | a << 5 | b; }
   uint8_t b : 5;
   uint8_t a : 1;
   uint8_t g : 5;
   uint8_t r : 5;
};
#endif

class MageColorPalette
{
public:
   MageColorPalette() noexcept = default;
   uint16_t colorAt(uint8_t colorIndex, uint16_t fadeColorValue, float fadeFraction = 0.0f) const
   {
      auto colorData = (const uint16_t*)(&colorCount + 2);
      auto sourceColor = colorData[colorIndex % colorCount];

      auto color = Color_565{ sourceColor };

      if (fadeFraction >= 1.0f)
      {
         return fadeColorValue;
      }
      else if (fadeFraction > 0.0f)
      {
         auto fadeColor = Color_565{ fadeColorValue };
         color.r = Util::lerp(color.r, fadeColor.r, fadeFraction);
         color.g = Util::lerp(color.g, fadeColor.g, fadeFraction);
         color.b = Util::lerp(color.b, fadeColor.b, fadeFraction);
         color.a = fadeFraction > 0.5f ? fadeColor.a : color.a;
      }
      return color;
   }

private:
   char name[COLOR_PALETTE_NAME_SIZE]{ 0 };
   uint8_t colorCount{ 0 };
};

#endif //SOFTWARE_MAGE_COLOR_PALETTE_H
