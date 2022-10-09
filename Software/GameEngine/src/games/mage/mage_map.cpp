#include "mage_map.h"
#include "EngineROM.h"
#include "EnginePanic.h"
#include "convert_endian.h"
#include "shim_err.h"
#include <algorithm>
#include "mage.h"

void MageMap::Load(uint16_t index)
{
   auto address = mapHeader.offset(index);
   auto layerCount = uint8_t{ 0 };
   auto entityCount = uint16_t{ 0 };
   auto geometryCount = uint16_t{ 0 };
   auto scriptCount = uint16_t{ 0 };
   auto goDirectionsCount = uint8_t{ 0 };
   auto mapLayerOffsetsCount = uint16_t{ 0 };

   ROM->Read(name, address, MapNameLength);
   ROM->Read(&tileWidth, address);
   ROM->Read(&tileHeight, address);
   ROM->Read(&cols, address);
   ROM->Read(&rows, address);
   ROM->Read(&onLoad, address);
   ROM->Read(&onTick, address);
   ROM->Read(&onLook, address);
   ROM->Read(&layerCount, address);
   ROM->Read(&playerEntityIndex, address);
   ROM->Read(&entityCount, address);
   ROM->Read(&geometryCount, address);
   ROM->Read(&scriptCount, address);
   ROM->Read(&goDirectionsCount, address);

   address += sizeof(uint8_t); // padding

   //entityGlobalIds = std::vector<uint16_t>{ entityCount ,};
   //geometryGlobalIds = std::vector<uint16_t>{ geometryCount };
   //scriptGlobalIds = std::vector<uint16_t>{ scriptCount };
   //goDirections = std::vector<GoDirection>{ goDirectionsCount };

   entityGlobalIds = ROM->InitializeCollectionOf<uint16_t>(address, entityCount);
   geometryGlobalIds = ROM->InitializeCollectionOf<uint16_t>(address, geometryCount);
   scriptGlobalIds = ROM->InitializeCollectionOf<uint16_t>(address, scriptCount);
   goDirections = ROM->InitializeCollectionOf<GoDirection>(address, goDirectionsCount);

   //padding to align with uint32_t memory spacing:
   if ((entityCount + geometryCount + scriptCount) % 2)
   {
      address += sizeof(uint16_t); // Padding
   }

   mapLayerOffsets = std::vector<uint32_t>{ layerCount };
   for (uint32_t i = 0; i < layerCount; i++)
   {
      mapLayerOffsets.push_back(address);
      address += (rows * cols) * (sizeof(uint16_t) + (2 * sizeof(uint8_t)));
   }

   if (entityCount > MAX_ENTITIES_PER_MAP)
   {
      ENGINE_PANIC("Error: Game is attempting to load more than %d entities on one map",MAX_ENTITIES_PER_MAP);
   }

   for (uint8_t i = 0; i < entityCount; i++)
   {
      auto offset = entityHeader.offset(getGlobalEntityId(i));
      //fill in entity data from gameEngine->ROM:
      auto entity = MageEntity{ ROM, offset };

      if (isEntityDebugOn)
      { // show all entities, we're in debug mode
         filteredMapLocalEntityIds[i] = i;
         mapLocalEntityIds[i] = i;
         entities[i] = std::move(entity);
         filteredEntityCountOnThisMap++;
      }
      else
      { // show only filtered entities
         bool isEntityMeantForDebugModeOnly = entity.direction & RENDER_FLAGS_IS_DEBUG;
         if (!isEntityMeantForDebugModeOnly)
         {
            filteredMapLocalEntityIds[i] = filteredEntityCountOnThisMap;
            mapLocalEntityIds[filteredEntityCountOnThisMap] = i;
            entities[filteredEntityCountOnThisMap] = std::move(entity);
            filteredEntityCountOnThisMap++;
         }
      }
   }
}

void MageMap::DrawEntities(MageGameEngine* gameEngine)
{
   int32_t cameraX = gameEngine->gameControl->camera.adjustedCameraPosition.x;
   int32_t cameraY = gameEngine->gameControl->camera.adjustedCameraPosition.y;
   //first sort entities by their y values:
   std::sort(
      entities.begin(),
      entities.end(),
      [](const MageEntity& entity1, const MageEntity& entity2) { return entity1.y < entity2.y; }
   );

   uint8_t filteredPlayerEntityIndex = getFilteredEntityId(playerEntityIndex);

   //now that we've got a sorted array with the lowest y values first,
   //iterate through it and draw the entities one by one:
   for (uint8_t i = 0; i < FilteredEntityCount(); i++)
   {
      MageEntity* entity = &entities[i];
      int32_t x = entity->x - cameraX;
      int32_t y = entity->y - cameraY - tileHeight;

      auto renderableData = entity->getRenderableData();
      MageTileset* tileset = &gameEngine->gameControl->tilesets[renderableData->tilesetId];
      uint16_t imageId = tileset->ImageId();
      uint16_t tileWidth = tileset->TileWidth();
      uint16_t tileHeight = tileset->TileHeight();
      uint16_t cols = tileset->Cols();
      uint16_t tileId = renderableData->tileId;
      uint32_t address = gameEngine->gameControl->imageHeader->offset(imageId);
      uint16_t source_x = (tileId % cols) * tileWidth;
      uint16_t source_y = (tileId / cols) * tileHeight;
      gameEngine->frameBuffer->drawChunkWithFlags(
         address,
         gameEngine->gameControl->getValidColorPalette(imageId),
         x,
         y,
         tileWidth,
         tileHeight,
         source_x,
         source_y,
         tileset->ImageWidth(),
         TRANSPARENCY_COLOR,
         renderableData->renderFlags
      );

      if (gameEngine->gameControl->isCollisionDebugOn)
      {
         gameEngine->frameBuffer->drawRect(
            x, y,
            tileWidth,
            tileHeight,
            COLOR_LIGHTGREY
         );
         gameEngine->frameBuffer->drawRect(
            renderableData->hitBox.x - cameraX,
            renderableData->hitBox.y - cameraY,
            renderableData->hitBox.w,
            renderableData->hitBox.h,
            renderableData->isInteracting
            ? COLOR_RED
            : COLOR_GREEN
         );
         gameEngine->frameBuffer->drawPoint(
            renderableData->center.x - cameraX,
            renderableData->center.y - cameraY,
            5,
            COLOR_BLUE
         );
         if (getPlayerEntityIndex() == filteredPlayerEntityIndex)
         {
            gameEngine->frameBuffer->drawRect(
               renderableData->interactBox.x - cameraX,
               renderableData->interactBox.y - cameraY,
               renderableData->interactBox.w,
               renderableData->interactBox.h,
               renderableData->isInteracting ? COLOR_BLUE : COLOR_YELLOW
            );
         }
      }
   }
}
