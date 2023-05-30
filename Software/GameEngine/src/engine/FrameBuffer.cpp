#include "FrameBuffer.h"
#include "EnginePanic.h"

#include <algorithm>

#ifdef DC801_EMBEDDED

#include "drv_ili9341.h"

#else

#include <SDL.h>
#include "shim_timer.h"
#include "shim_err.h"

#endif

#define MAX_ROM_CONTINUOUS_COLOR_DATA_READ_LENGTH 64

//Cursor coordinates
static inline int16_t m_cursor_x = 0;
static inline int16_t m_cursor_y = 0;
static inline Rect m_cursor_area = { 0, 0, DrawWidth, DrawHeight };
static inline uint16_t m_color = COLOR_WHITE;
static inline bool m_wrap = true;
static inline volatile bool m_stop = false;

void FrameBuffer::clearScreen(uint16_t color)
{
   frame.fill(color);
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
            auto y = drawDownward ? DrawHeight - y1 : y1;
            drawPixel(x1, y, color);
            y1++;
            p = p + 2 * dy - 2 * dx;
         }
         else
         {
            auto y = drawDownward ? DrawHeight - y1 : y1;
            drawPixel(x1, y, color);
            p = p + 2 * dy;
         }
      }
   }
}

void FrameBuffer::__draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, GFXfont font)
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
   // THIS IS ON PURPOSE AND BY DESIGN. 

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
            if (yyy >= m_cursor_area.origin.y && yyy <= m_cursor_area.w)
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
      m_cursor_x = m_cursor_area.origin.x;
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

            if ((m_cursor_x + (xo + w)) >= m_cursor_area.w && m_wrap)
            {
               // Drawing character would go off right edge; wrap to new line
               m_cursor_x = m_cursor_area.origin.x;
               m_cursor_y += font.yAdvance;
            }

            __draw_char(m_cursor_x, m_cursor_y, c, m_color, COLOR_BLACK, font);
         }
         m_cursor_x += glyph->xAdvance;
      }
   }
}

void FrameBuffer::printMessage(std::string text, GFXfont font, uint16_t color, int x, int y)
{
   m_color = color;
   m_cursor_area.origin.x = x;
   m_cursor_x = m_cursor_area.origin.x;
   m_cursor_y = y + (font.yAdvance / 2);

   // this prevents crashing if the first character of the string is null
   if (text[0] != '\0')
   {
      for (uint16_t i = 0; i < text.length(); i++)
      {
         write_char(text[i], font);
      }
   }
   m_cursor_area.origin.x = 0;
   blt();
}

void FrameBuffer::blt()
{
   // TODO FIXME: Moved this out of color palette lookup, think it might belong here, right before blitting the data
   // if (fadeFraction >= 1.0f)
   // {
   //    return fadeColorValue;
   // }
   // else if (fadeFraction > 0.0f)
   // {
   //    auto fadeColor = Color_565{ fadeColorValue };
   //    color.r = Util::lerp(color.r, fadeColor.r, fadeFraction);
   //    color.g = Util::lerp(color.g, fadeColor.g, fadeFraction);
   //    color.b = Util::lerp(color.b, fadeColor.b, fadeFraction);
   //    color.a = fadeFraction > 0.5f ? fadeColor.a : color.a;
   // }
      
#ifdef DC801_EMBEDDED 
   while (ili9341_is_busy())
   {
      //wait for previous transfer to complete before starting a new one
   }
   ili9341_set_addr(0, 0, DrawWidth - 1, DrawHeight - 1);
   uint32_t bytecount = DrawWidth * DrawHeight * 2;
   ili9341_push_colors((uint8_t*)frame.data(), bytecount);
#else
   windowFrame->GameBlt(frame.data());
#endif
}