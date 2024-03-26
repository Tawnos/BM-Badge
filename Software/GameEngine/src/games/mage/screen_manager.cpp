#include "screen_manager.h"
#include "mage_portrait.h"
#include "EnginePanic.h"
#include <algorithm>
#include <ranges>
#include <array>

#ifdef DC801_EMBEDDED
#include "modules/drv_ili9341.h"
#endif
#include <fonts/Monaco9.h>

void ScreenManager::drawTile(uint16_t tilesetId, uint16_t tileId, int32_t tileDrawX, int32_t tileDrawY, uint8_t flags) const
{
   auto tileset = ROM()->GetReadPointerByIndex<MageTileset>(tilesetId);
   auto colorPalette = ROM()->GetReadPointerByIndex<MageColorPalette>(tilesetId);

   auto ySourceMin = int32_t{ 0 };
   auto ySourceMax = int32_t{ tileset->TileHeight };
   auto xSourceMin = int32_t{ 0 };
   auto xSourceMax = int32_t{ tileset->TileWidth };
   auto iteratorX = int16_t{ 1 };
   auto iteratorY = int16_t{ 1 };

   if (flags & RENDER_FLAGS_FLIP_X || flags & RENDER_FLAGS_FLIP_DIAG)
   {
      xSourceMin = tileset->TileWidth - 1;
      xSourceMax = -1;
      iteratorX = -1;
   }

   if (flags & RENDER_FLAGS_FLIP_Y || flags & RENDER_FLAGS_FLIP_DIAG)
   {
      ySourceMin = tileset->TileHeight - 1;
      ySourceMax = -1;
      iteratorY = -1;
   }

   //if (flags & RENDER_FLAGS_IS_GLITCHED)
   //{
   //    target.origin.x += target.w * 0.125;
   //    target.w *= 0.75;
   //}

   // offset to the start address of the tile
   const auto tiles = ROM()->GetReadPointerByIndex<MagePixel>(tilesetId);
   const auto tilePixels = std::span<const MagePixel>(&tiles[tileId * tileset->TileWidth * tileset->TileHeight], tileset->TileWidth * tileset->TileHeight);

   for (auto yTarget = tileDrawY;
      ySourceMin != ySourceMax;
      ySourceMin += iteratorY, yTarget++)
   {
      auto sourceRowPtr = &tilePixels[ySourceMin * tileset->TileWidth];

      if (yTarget < 0 || yTarget >= DrawHeight)
      {
         continue;
      }

      for (auto xSource = xSourceMin, xTarget = tileDrawX;
         xSource != xSourceMax;
         xSource += iteratorX, xTarget++)
      {
         if (xTarget < 0 || xTarget >= DrawWidth)
         {
            continue;
         }

         const auto& sourceColorIndex = sourceRowPtr[xSource];
         const auto color = colorPalette->get(sourceColorIndex);

         frameBuffer->setPixel(xTarget, yTarget, color);
      }
   }

   // we can fit up to 254 bytes in a single transfer window, each color is 2 bytes, so we can do a max of 127 pixels/transfer
   // if (tileset->ImageWidth * target.h * sizeof(uint16_t) <= 254)
   // {
   //    std::array<uint16_t, 254 / sizeof(uint16_t)> tileBuffer{0};
   //    for (auto i = 0; i < tileBuffer.size(); i++)
   //    {
   //       auto& color = colorPalette->get(sourceTilePtr[i]);
   //       if (color != TRANSPARENCY_COLOR)
   //       {
   //          tileBuffer[i] = color;
   //       }
   //    }
   // }

   if (drawGeometry)
   {
      const auto tileDrawPoint = Vector2T{ tileDrawX, tileDrawY };
      //frameBuffer->drawRect(EntityRect{ tileDrawPoint, tileset->TileWidth, tileset->TileHeight }, COLOR_RED);
      auto geometry = tileset->GetGeometryForTile(tileId);
      if (geometry)
      {
         auto geometryPoints = geometry->FlipByFlags(flags, tileset->TileWidth, tileset->TileHeight);
         for (auto i = 0; i < geometryPoints.size(); i++)
         {
            const auto tileLinePointA = geometryPoints[i] + tileDrawPoint;
            const auto tileLinePointB = geometryPoints[(i + 1) % geometryPoints.size()] + tileDrawPoint;

            frameBuffer->drawLine(tileLinePointA, tileLinePointB, COLOR_GREEN);
         }
      }
   }
}


void ScreenManager::DrawText(const std::string_view& text, uint16_t color, int x, int y) const
{
   frameBuffer->printMessage(text, Monaco9, color, x, y);
}