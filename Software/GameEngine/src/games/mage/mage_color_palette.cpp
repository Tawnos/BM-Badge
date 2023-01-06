#include "mage_color_palette.h"
#include "modules/sd.h"
#include "FrameBuffer.h"
#include "EnginePanic.h"
#include "convert_endian.h"
#include "utility.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif // EMSCRIPTEN

MageColorPalette::MageColorPalette(uint32_t& address) noexcept
{
#ifndef DC801_EMBEDDED
   // Read name only if we're on Desktop,
   // Embedded don't got RAM for that
   ROM->Read(name, address, COLOR_PALETTE_NAME_LENGTH);
#else
   // Regardless of reading/storing it, ALWAYS increment past it
   address += COLOR_PALETTE_NAME_LENGTH;
#endif

   uint8_t colorCount;
   // Read colorCount
   ROM->Read(colorCount, address);
   address += 1; // padding

   ROM->InitializeCollectionOf(colors, address, colorCount);

#ifndef DC801_EMBEDDED
   for (int i = 0; i < colorCount; ++i)
   {
      sprintf(colorIntegrityString + (7 * i), " 0x%04x", colors[i]);
   }
#endif //DC801_DESKTOP
}

MageColorPalette::MageColorPalette(const FrameBuffer* frameBuffer, const MageColorPalette* sourcePalette, uint16_t fadeColor, float fadeFraction)
{
   for (int i = 0; i < sourcePalette->colors.size(); ++i)
   {
      auto sourceColor = sourcePalette->colors[i];
      if (sourceColor == TRANSPARENCY_COLOR)
      {
         colors[i] = sourceColor;
      }
      if (fadeFraction >= 1.0f)
      {
         colors[i] = fadeColor;
      }
      else if (fadeFraction > 0.0f)
      {
         auto fadeColorUnion = ColorUnion{ fadeColor };
         auto colorUnion = ColorUnion{ colors[i] };
         colorUnion.c.r = Util::lerp(colorUnion.c.r, fadeColorUnion.c.r, fadeFraction);
         colorUnion.c.g = Util::lerp(colorUnion.c.g, fadeColorUnion.c.g, fadeFraction);
         colorUnion.c.b = Util::lerp(colorUnion.c.b, fadeColorUnion.c.b, fadeFraction);
         colorUnion.c.alpha = fadeFraction > 0.5f ? fadeColorUnion.c.alpha : colorUnion.c.alpha;
         colors[i] = colorUnion.i;
      }
   }
   
}