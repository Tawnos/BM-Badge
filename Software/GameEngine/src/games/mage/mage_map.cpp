#include "mage_map.h"
#include "EnginePanic.h"
#include <algorithm>
#include <numeric>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>
#include "mage.h"
#include "shim_err.h"

#ifdef DC801_EMBEDDED
#include "modules/drv_ili9341.h"
#endif

void MapControl::Load()
{
   auto offset = ROM()->GetOffsetByIndex<MapData>(mapLoadId);
   currentMap.emplace(offset);

   for (auto i = 0; i < currentMap->scriptCount; i++)
   {
      scripts.push_back(ROM()->GetReadPointerByIndex<MageScript>(currentMap->scriptGlobalIDs[i]));
   }

   for (auto i = 0; i < currentMap->entityCount; i++)
   {
      // populate the MapControl's copy of the Entity Data
      auto& entityData = Get<MageEntityData>(i);
      auto entityAddress = ROM()->GetOffsetByIndex<MageEntityData>(currentMap->entityGlobalIDs[i]);
      ROM()->Read<MageEntityData>(entityData, entityAddress);

      // convert entity coordinates into map coordinates (0 at bottom -> 0 at top)
      entityData.targetPosition.y -= ROM()->GetReadPointerByIndex<MageTileset>(entityData.primaryId)->TileHeight;

      // fill renderable data from the entity
      Get<RenderableData>(i).UpdateFrom(entityData);
   }

   onTick = MageScriptState{ currentMap->onTickScriptId, false };

   auto player = getPlayerEntityData();
   if (player)
   {
      player->SetName(ROM()->GetCurrentSave().name);
   }
   mapLoadId = MAGE_NO_MAP;
}

void MapControl::OnTick(MageScriptControl* scriptControl)
{
   //the map's onTick script will run every tick, restarting from the beginning as it completes
   onTick.scriptIsRunning = true;
   scriptControl->processScript(onTick, MAGE_MAP_ENTITY);
}

MapData::MapData(uint32_t& offset)
{
   auto layerCount = uint8_t{ 0 };
   ROM()->Read(name, offset, MapNameLength);
   ROM()->Read(tileWidth, offset);
   ROM()->Read(tileHeight, offset);
   ROM()->Read(cols, offset);
   ROM()->Read(rows, offset);
   ROM()->Read(onLoadScriptId, offset);
   ROM()->Read(onTickScriptId, offset);
   ROM()->Read(onLookScriptId, offset);
   ROM()->Read(layerCount, offset);
   ROM()->Read(playerEntityIndex, offset);
   ROM()->Read(entityCount, offset);
   ROM()->Read(geometryCount, offset);
   ROM()->Read(scriptCount, offset);
   ROM()->Read(goDirectionsCount, offset);

   if (entityCount > MAX_ENTITIES_PER_MAP)
   {
      ENGINE_PANIC("Error: Game is attempting to load more than %d entities on one map", MAX_ENTITIES_PER_MAP);
   }

   offset += sizeof(uint8_t); // padding for 4-byte alignment

   entityGlobalIDs = ROM()->GetViewOf<uint16_t>(offset, entityCount);
   geometryGlobalIDs = ROM()->GetViewOf<uint16_t>(offset, geometryCount);
   scriptGlobalIDs = ROM()->GetViewOf<uint16_t>(offset, scriptCount);
   goDirections = ROM()->GetViewOf<GoDirection>(offset, goDirectionsCount);

   //padding to align with uint32_t memory spacing:
   if ((entityCount + geometryCount + scriptCount) % 2)
   {
      offset += sizeof(uint16_t);
   }

   layers = MapLayers(offset, ROM()->GetViewOf<MapTile>(offset, (uint16_t)(layerCount * rows * cols)), (uint16_t)(rows * cols));
}

void MapControl::DrawEntities() const
{
   //sort entity indices by the entity y values:
   std::vector<size_t> entityDrawOrder(currentMap->entityCount);
   std::iota(entityDrawOrder.begin(), entityDrawOrder.end(), 0);
   const auto sortByY = [&](size_t i1, size_t i2) {
      return Get<MageEntityData>(i1).targetPosition.y < Get<MageEntityData>(i2).targetPosition.y;
   };
    std::stable_sort(entityDrawOrder.begin(), entityDrawOrder.end(), sortByY);

   //now that we've got a sorted array with the lowest y values first,
   //iterate through it and draw the entities one by one:
   for (auto& entityIndex : entityDrawOrder)
   {
      Get<RenderableData>(entityIndex).Draw(frameBuffer);
   }
}

void MapControl::DrawLayer(uint8_t layer) const
{
   //               map height - screen height
   // *********************************************************** 
   // *|             ____________________________              |*
   // *|                                                       |*
   // *|                                                       |*
   // *|             ____________________________screen width  |* 
   // *|             |##########################|              |*
   // *|             |##########################|              |*
   // *|             |##########################|              |* screen width + map width
   // *|             |##########################|              |* 
   // *|             |##########################|              |*
   // *|             |##########################|              |*
   // *|             ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ              |*
   // *|             screen height                             |*
   // *|                                                       |*
   // *|                                                       |*
   // *|             ŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻŻ              |*
   // ***********************************************************
   //                screen height + map height


   // identify start and stop tiles to draw
   const auto startTileX = std::max(0, (frameBuffer->camera.position.x - DrawWidth) / currentMap->tileWidth);
   const auto startTileY = std::max(0, (frameBuffer->camera.position.y - DrawHeight) / currentMap->tileHeight);

   const auto endTileX = std::min(int{ currentMap->cols - 1 }, (startTileX + DrawWidth) / currentMap->tileWidth);
   const auto endTileY = std::min(int{ currentMap->rows - 1 }, (startTileY + DrawHeight) / currentMap->tileHeight + 1);

   for (auto mapTileRow = startTileY; mapTileRow <= endTileY; mapTileRow++)
   {
      for (auto mapTileCol = startTileX; mapTileCol <= endTileX; mapTileCol++)
      {
         auto tileIndex = mapTileCol + (mapTileRow * currentMap->cols);
         auto currentTile = &currentMap->layers[layer][tileIndex];

         if (!currentTile->tileId) { continue; }

         auto tileDrawX = currentMap->tileWidth * mapTileCol;
         auto tileDrawY = currentMap->tileHeight * mapTileRow;

         frameBuffer->DrawTileWorldCoords(currentTile->tilesetId, currentTile->tileId - 1, tileDrawX, tileDrawY, currentTile->flags);
      }
   }
}

void MapControl::Draw() const
{
   // always draw layer 0 
   DrawLayer(0);

   //draw remaining map layers except the last one before drawing entities.
   for (uint8_t layerIndex = 1; layerIndex < LayerCount() - 1; layerIndex++)
   {
      DrawLayer(layerIndex);
   }

   DrawEntities();

   //draw the final layer above the entities only when there is more than one layer
   if (LayerCount() > 1)
   {
      DrawLayer(LayerCount() - 1);
   }

   // draw any geometry or overlay
   frameBuffer->DrawRectWorldCoords(getInteractBox());
}

std::optional<uint16_t> MapControl::Update()
{
   auto playerEntityData = getPlayerEntityData();
   // require a player on the map to move/interact
   if (!playerEntityData) { return std::nullopt; }

   const auto playerRenderableData = getPlayerRenderableData();
   handleCollision(playerRenderableData, playerEntityData);

   const auto interactBox = getInteractBox();
   std::optional<uint16_t> interactionId = std::nullopt;
   for (auto i = 0; i < currentMap->entityCount; i++)
   {
      auto& entity = Get<MageEntityData>(i);
      auto& renderableData = Get<RenderableData>(i);

      if (&entity != playerEntityData && interactBox.Overlaps(renderableData.hitBox))
      {
         interactionId.emplace(i);
      }

      renderableData.UpdateFrom(entity);
   }

   return interactionId;
}

EntityRect MapControl::getInteractBox() const
{
   const auto playerRenderableData = getPlayerRenderableData();
   static const uint16_t interactLength = playerRenderableData->hitBox.h / 2 + playerRenderableData->hitBox.w / 2;
   const auto direction = static_cast<MageEntityAnimationDirection>(getPlayerEntityData()->flags & RENDER_FLAGS_ENTITY_DIRECTION_MASK);
   return EntityRect{
         {
            uint16_t{   
               playerRenderableData->hitBox.origin.x 
               - playerRenderableData->hitBox.w / 2
               - (direction == MageEntityAnimationDirection::WEST ? interactLength : 0u)
               + (direction == MageEntityAnimationDirection::EAST ? interactLength : 0u)
            },
            uint16_t{
               playerRenderableData->hitBox.origin.y 
               //- playerRenderableData->hitBox.h / 2
               - (direction == MageEntityAnimationDirection::NORTH ? interactLength : 0u)
               + (direction == MageEntityAnimationDirection::SOUTH ? interactLength : 0u)
            }
      }, 
      uint16_t(playerRenderableData->hitBox.w * 2), 
      uint16_t(playerRenderableData->hitBox.h * 2) 
   };
}

void MapControl::handleCollision(RenderableData* const playerRenderableData, MageEntityData* playerEntityData)
{

   const auto oldPosition = playerRenderableData->origin;

   const auto topLeft = oldPosition;
   const auto topRight = oldPosition + EntityPoint{ playerRenderableData->hitBox.w, 0 };
   const auto botLeft = oldPosition + EntityPoint{ 0, playerRenderableData->hitBox.h };
   const auto botRight = oldPosition + EntityPoint{ playerRenderableData->hitBox.w, playerRenderableData->hitBox.h };

   std::vector<EntityPoint> hitboxPointsToCheck{};
   if (playerEntityData->targetPosition.x < playerRenderableData->origin.x)
   {
      playerEntityData->flags = (playerEntityData->flags & 0x80) | static_cast<uint8_t>(MageEntityAnimationDirection::WEST);
      hitboxPointsToCheck.push_back(topLeft);
      hitboxPointsToCheck.push_back(botLeft);
   }
   else if (playerEntityData->targetPosition.x > playerRenderableData->origin.x)
   {
      playerEntityData->flags = (playerEntityData->flags & 0x80) | static_cast<uint8_t>(MageEntityAnimationDirection::EAST);
      hitboxPointsToCheck.push_back(topRight);
      hitboxPointsToCheck.push_back(botRight);
   }

   if (playerEntityData->targetPosition.y < playerRenderableData->origin.y)
   {
      playerEntityData->flags = (playerEntityData->flags & 0x80) |  static_cast<uint8_t>(MageEntityAnimationDirection::NORTH);
      hitboxPointsToCheck.push_back(topLeft);
      hitboxPointsToCheck.push_back(topRight);
   }
   else if (playerEntityData->targetPosition.y > playerRenderableData->origin.y)
   {
      playerEntityData->flags = (playerEntityData->flags & 0x80) | static_cast<uint8_t>(MageEntityAnimationDirection::SOUTH);
      hitboxPointsToCheck.push_back(botLeft);
      hitboxPointsToCheck.push_back(botRight);
   }

   for (auto& hitboxPoint : hitboxPointsToCheck)
   {
      const auto column = hitboxPoint.x / currentMap->tileWidth;
      const auto row = hitboxPoint.y / currentMap->tileHeight;
      const auto tileOffsetPoint = EntityPoint{ (uint16_t)(currentMap->tileWidth * column), (uint16_t)(currentMap->tileHeight * row) };
      const auto tileId = column + currentMap->cols * row;

      // ignore checks that are outside the bounds of the map
      if (tileId < 0 || tileId >= currentMap->layers[0].size()) { continue; }

      const auto tileset = ROM()->GetReadPointerByIndex<MageTileset>(currentMap->layers[0][tileId].tilesetId);

      // check for world collision:
      const auto tileGeometryId = currentMap->layers[0][tileId].tileId;
      auto geometry = tileset->GetGeometryForTile(tileGeometryId);
      if (geometry && geometry->IsPointInside(hitboxPoint, tileOffsetPoint))
      {
         // TODO: bring back the intersection-offset algorithm
         playerEntityData->targetPosition = oldPosition;
         break;
      }
   }
}
