#include "FrameBuffer.h"

#include "main.h"
#include "games/mage/mage.h"
#include "utility.h"
#include "games/mage/mage_defines.h"
#include "games/mage/mage_color_palette.h"
#include "convert_endian.h"
#include "modules/sd.h"
#include "config/custom_board.h"
#include "EnginePanic.h"
#include "EngineROM.h"

#include "adafruit/gfxfont.h"

#include <algorithm>

#ifndef DC801_EMBEDDED
#include <SDL.h>
#include "shim_timer.h"
#include "shim_err.h"
#include "EngineWindowFrame.h"
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define MAX_ROM_CONTINUOUS_COLOR_DATA_READ_LENGTH 64

//Cursor coordinates
static int16_t m_cursor_x = 0;
static int16_t m_cursor_y = 0;
static area_t m_cursor_area = { 0, 0, WIDTH, HEIGHT };
static uint16_t m_color = COLOR_WHITE;
static bool m_wrap = true;
static volatile bool m_stop = false;


void FrameBuffer::clearScreen(uint16_t color)
{
   for (uint32_t i = 0; i < FRAMEBUFFER_SIZE; ++i)
   {
      frame[i] = SCREEN_ENDIAN_U2_VALUE(color);
   }
}

void FrameBuffer::drawChunkWithFlags(
   uint32_t address,
   const MageColorPalette* colorPalette,
   int32_t screen_x, // top-left corner of screen coordinates to draw at
   int32_t screen_y, // top-left corner of screen coordinates to draw at
   uint16_t tile_width,
   uint16_t tile_height,
   uint16_t source_x, // top-left corner of source image coordinates to READ FROM
   uint16_t source_y, // top-left corner of source image coordinates to READ FROM
   uint16_t pitch, // The width of the source image in pixels
   uint8_t flags
)
{
   const auto colors = (fadeFraction != 0) ?
      &( MageColorPalette{
         this,
         colorPalette,
         fadeColor,
         fadeFraction
      })
      : colorPalette;

   //MageColorPalette* colorPalette = colorPaletteOriginal;
   RenderFlags renderFlags{ flags };
   bool flip_x = renderFlags.horizontal;
   bool flip_y = renderFlags.vertical;
   bool flip_diag = renderFlags.diagonal;
   bool glitched = renderFlags.glitched;

   if (glitched)
   {
      screen_x += tile_width * 0.125;
      tile_width *= 0.75;
   }

   if (screen_x + tile_width < 0 || screen_x >= WIDTH
      || screen_y + tile_height < 0 || screen_y >= HEIGHT)
   {
      return;
   }

   auto pixelOffset = address + ((source_y * pitch) + source_x);
   const uint8_t* pixels;
   gameEngine->ROM->GetPointerTo(pixels, pixelOffset);

   for (auto row = 0; row != tile_height && row < HEIGHT; row++)
   {
      for (auto col = 0; col != tile_width && col < WIDTH; col++)
      {
         auto pixelRow = renderFlags.vertical ? tile_height - row : row;
         auto pixelCol = renderFlags.horizontal ? tile_width - col : col;
         auto pixelIndex = (pixelRow * tile_width) + pixelCol;

         uint8_t colorIndex = pixels[pixelIndex];
         auto color = colors->colorAt(colorIndex);
         if (color != TRANSPARENCY_COLOR)
         {
            drawPixel(screen_x + col, screen_y + row, color);
         }
      }
   }
}

void FrameBuffer::fillRect(int x, int y, int w, int h, uint16_t color)
{
   if ((x >= WIDTH) || (y >= HEIGHT))
   {
      return;
   }

   // Clip to screen
   auto right = x + w;
   if (right >= WIDTH) { right = WIDTH - 1; }
   auto bottom = y + h;
   if (bottom >= HEIGHT) { bottom = HEIGHT - 1; }
   // X
   for (int i = x; i < right; i++)
   {
      // Y
      for (int j = y; j < bottom; j++)
      {
         drawPixel(i, j, color);
      }
   }
}

void FrameBuffer::drawRect(int x, int y, int w, int h, uint16_t color)
{
   drawLine(x, y, x + w, y, color);
   drawLine(x, y + h, x + w, y, color);
   drawLine(x, y, x, y + h, color);
   drawLine(x + w, x, y, y + h, color);
}

uint16_t FrameBuffer::applyFadeColor(uint16_t color) const
{
   if (fadeFraction > 0.0f)
   {
      auto fadeColorUnion = ColorUnion{ fadeColor };
      auto colorUnion = ColorUnion{ color };
      colorUnion.c.r = Util::lerp(colorUnion.c.r, fadeColorUnion.c.r, fadeFraction);
      colorUnion.c.g = Util::lerp(colorUnion.c.g, fadeColorUnion.c.g, fadeFraction);
      colorUnion.c.b = Util::lerp(colorUnion.c.b, fadeColorUnion.c.b, fadeFraction);
      colorUnion.c.alpha = fadeFraction > 0.5f ? fadeColorUnion.c.alpha : colorUnion.c.alpha;
      color = colorUnion.i;
   }
   return color;
}

void FrameBuffer::drawLine(int x1, int y1, int x2, int y2, uint16_t color)
{
   if (x2 < x1)
   {
      std::swap(x1, x2);
      std::swap(y1, y2);
   }

   // optimization for vertical lines
   if (x1 == x2)
   {
      if (y2 < y1) { std::swap(y1, y2); }
      while (y1++ != y2)
      {
         drawPixel(x1, y1, color);
      }
   }
   // optimization for horizontal lines
   else if (y1 == y2)
   {
      while (x1++ != x2)
      {
         drawPixel(x1, y1, color);
      }
   }
   // Bresenham’s Line Generation Algorithm
   // with ability to draw downwards
   else
   {
      auto drawDownward = false;
      if (y2 < y1)
      {
         std::swap(y1, y2);
         drawDownward = true;
      }

      auto dx = x2 - x1;
      auto dy = y2 - y1;
      for (int p = 2 * dy - dx; x1 < x2; x1++)
      {
         if (p >= 0)
         {
            auto y = drawDownward ? HEIGHT - y1 : y1;
            drawPixel(x1, y, color);
            y1++;
            p = p + 2 * dy - 2 * dx;
         }
         else
         {
            auto y = drawDownward ? HEIGHT - y1 : y1;
            drawPixel(x1, y, color);
            p = p + 2 * dy;
         }
      }
   }

}


void FrameBuffer::drawPoint(int x, int y, uint8_t size, uint16_t color)
{
   drawLine(x - size, y - size,
            x + size, y + size,
            color);
   drawLine(x - size, y + size,
            x + size, y - size,
            color);
}

void FrameBuffer::__draw_char(
   int16_t x,
   int16_t y,
   unsigned char c,
   uint16_t color,
   uint16_t bg,
   GFXfont font
)
{
   // Character is assumed previously filtered by write() to eliminate
   // newlines, returns, non-printable characters, etc.  Calling drawChar()
   // directly with 'bad' characters of font may cause mayhem!

   c -= font.first;
   GFXglyph* glyph = &(font.glyph[c]);
   uint8_t* bitmap = font.bitmap;

   uint16_t bo = glyph->bitmapOffset;
   uint8_t w = glyph->width;
   uint8_t h = glyph->height;

   int8_t xo = glyph->xOffset;
   int8_t yo = glyph->yOffset;
   uint8_t xx, yy, bits = 0, bit = 0;

   // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
   // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
   // has typically been used with the 'classic' font to overwrite old
   // screen contents with new data.  This ONLY works because the
   // characters are a uniform size; it's not a sensible thing to do with
   // proportionally-spaced fonts with glyphs of varying sizes (and that
   // may overlap).  To replace previously-drawn text when using a custom
   // font, use the getTextBounds() function to determine the smallest
   // rectangle encompassing a string, erase the area with fillRect(),
   // then draw new text.  This WILL infortunately 'blink' the text, but
   // is unavoidable.  Drawing 'background' pixels will NOT fix this,
   // only creates a new set of problems.  Have an idea to work around
   // this (a canvas object type for MCUs that can afford the RAM and
   // displays supporting setAddrWindow() and pushC(olors()), but haven't
   // implemented this yet.

   int16_t yyy;
   for (yy = 0; yy < h; yy++)
   {
      for (xx = 0; xx < w; xx++)
      {
         if (!(bit++ & 7))
         {
            bits = bitmap[bo++];
         }
         if (bits & 0x80)
         {
            yyy = y + yo + yy;
            if (yyy >= m_cursor_area.ys && yyy <= m_cursor_area.ye)
            {
               drawPixel(x + xo + xx, y + yo + yy, color);
            }
         }
         bits <<= 1;
      }
   }
}

void FrameBuffer::write_char(uint8_t c, GFXfont font)
{
   //If newline, move down a row
   if (c == '\n')
   {
      m_cursor_x = m_cursor_area.xs;
      m_cursor_y += font.yAdvance;
   }
   //Otherwise, print the character (ignoring carriage return)
   else if (c != '\r')
   {
      uint8_t first = font.first;

      //Valid char?
      if ((c >= first) && (c <= font.last))
      {
         uint8_t c2 = c - first;
         GFXglyph* glyph = &(font.glyph[c2]);

         uint8_t w = glyph->width;
         uint8_t h = glyph->height;

         if ((w > 0) && (h > 0))
         { // Is there an associated bitmap?
            int16_t xo = glyph->xOffset;

            if ((m_cursor_x + (xo + w)) >= m_cursor_area.xe && m_wrap)
            {
               // Drawing character would go off right edge; wrap to new line
               m_cursor_x = m_cursor_area.xs;
               m_cursor_y += font.yAdvance;
            }

            __draw_char(m_cursor_x, m_cursor_y, c, m_color, COLOR_BLACK, font);
         }
         m_cursor_x += glyph->xAdvance;
      }
   }
}

void FrameBuffer::printMessage(const char* text, GFXfont font, uint16_t color, int x, int y)
{
   m_color = SCREEN_ENDIAN_U2_VALUE(color);
   m_cursor_area.xs = x;
   m_cursor_x = m_cursor_area.xs;
   m_cursor_y = y + (font.yAdvance / 2);

   // this prevents crashing if the first character of the string is null
   if (text[0] != '\0')
   {
      for (uint16_t i = 0; i < strlen(text); i++)
      {
         write_char(text[i], font);
      }
   }
   m_cursor_area.xs = 0;
}

void FrameBuffer::setTextArea(area_t* area)
{
   m_cursor_area = *area;
}

void FrameBuffer::getTextBounds(GFXfont font, const char* text, int16_t x, int16_t y, bounds_t* bounds)
{
   getTextBounds(font, text, x, y, NULL, bounds);
}

void FrameBuffer::getTextBounds(GFXfont font, const char* text, int16_t x, int16_t y, area_t* near, bounds_t* bounds)
{
   bounds->width = 0;
   bounds->height = 0;

   if (near != NULL)
   {
      m_cursor_area = *near;
   }

   GFXglyph glyph;

   uint8_t c;
   uint8_t first = font.first;
   uint8_t last = font.last;

   uint8_t gw, gh, xa;
   int8_t xo, yo;

   int16_t minx = WIDTH;
   int16_t miny = HEIGHT;

   int16_t maxx = -1;
   int16_t maxy = -1;

   int16_t gx1, gy1, gx2, gy2;
   int16_t ya = font.yAdvance;

   // Walk the string
   while ((c = *text++))
   {
      if ((c != '\n') && (c != '\r'))
      {
         if ((c >= first) && (c <= last))
         {
            c -= first;
            glyph = font.glyph[c];

            gw = glyph.width;
            gh = glyph.height;

            xa = glyph.xAdvance;
            xo = glyph.xOffset;
            yo = glyph.yOffset;

            if (m_wrap && ((x + ((int16_t)xo + gw)) >= WIDTH))
            {
               // Line wrap
               x = 0;
               y += ya;
            }

            gx1 = x + xo;
            gy1 = y + yo;
            gx2 = gx1 + gw - 1;
            gy2 = gy1 + gh - 1;

            if (gx1 < minx)
            {
               minx = gx1;
            }

            if (gy1 < miny)
            {
               miny = gy1;
            }

            if (gx2 > maxx)
            {
               maxx = gx2;
            }

            if (gy2 > maxy)
            {
               maxy = gy2;
            }

            x += xa;
         }
      }
      else if (c == '\n')
      {
         x = 0;
         y += ya;
      }
   }

   if (maxx >= minx)
   {
      bounds->width = maxx - minx + 1;
   }

   if (maxy >= miny)
   {
      bounds->height = maxy - miny + 1;
   }
}

void draw_raw_async(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* p_raw)
{
   //clip
   if ((x < 0) || (x > WIDTH - w) || (y < 0) || (y > HEIGHT - h))
   {
      return;
   }
#ifdef DC801_EMBEDDED
   while (ili9341_is_busy())
   {
      //wait for previous transfer to complete before starting a new one
   }
   ili9341_set_addr(x, y, x + w - 1, y + h - 1);
   uint32_t bytecount = w * h * 2;
   ili9341_push_colors((uint8_t*)p_raw, bytecount);
#endif
   /*
   //Blast data to TFT
   while (bytecount > 0) {

      uint32_t count = MIN(320*80*2, bytecount);

      ili9341_push_colors((uint8_t*)p_raw, count);

      p_raw += count / 2; //convert to uint16_t count
      bytecount -= count;
   }
   //don't wait for it to finish
   */
}

void FrameBuffer::blt()
{
#ifdef DC801_EMBEDDED
   draw_raw_async(0, 0, WIDTH, HEIGHT, frame);
#else
   gameEngine->windowFrame->GameBlt(frame.data());
#endif
}