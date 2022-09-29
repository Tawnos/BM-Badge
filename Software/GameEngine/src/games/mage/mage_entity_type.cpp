#include "mage_entity_type.h"
#include "FrameBuffer.h"
#include "convert_endian.h"

MageEntityTypeAnimationDirection::MageEntityTypeAnimationDirection(std::shared_ptr<EngineROM> ROM, uint32_t& offset)
{
   ROM->Read(&typeId, offset);
   // Endianness conversion
   typeId = ROM_ENDIAN_U2_VALUE(typeId);

   // Increment offset
   offset += sizeof(typeId);

   // Read count
   ROM->Read(&type, offset);

   // Increment offset
   offset += sizeof(type);

   // Read count
   ROM->Read(&renderFlags, offset);
}


MageEntityType::MageEntityType(std::shared_ptr<EngineROM> ROM, uint32_t& address)
{
   address += 32; // skip over reading the name, no need to hold that in ram
   address += sizeof(uint8_t) + sizeof(uint8_t); // paddingA + paddingB

   // Read portraitId
   ROM->Read(&portraitId, address);
   address += sizeof(portraitId);

   auto animationCount = uint16_t{ 0 };
   // Read animationCount
   ROM->Read(&animationCount, address);
   address += sizeof(animationCount);

   // Construct array
   entityTypeAnimations = std::vector<MageAnimation>{ animationCount };

   //increment through animations to fill entityTypeAnimations array:
   for (uint32_t animationIndex = 0; animationIndex < animationCount; animationIndex++)
   {
      entityTypeAnimations[animationIndex] = MageAnimation{ ROM, address };
      address += sizeof(MageAnimation);
   }
}

MageEntity::MageEntity(std::shared_ptr<EngineROM> ROM, uint32_t& address)
{
   uint32_t size = 0;

   //Read Name
   ROM->Read(name, address);

   //increment address
   address += MAGE_ENTITY_NAME_LENGTH;

   // Read x
   ROM->Read(&x, address);

   // Endianness conversion
   x = ROM_ENDIAN_U2_VALUE(x);

   //increment address
   address += sizeof(x);

   // Read y
   ROM->Read(&y, address);

   // Endianness conversion
   y = ROM_ENDIAN_U2_VALUE(y);

   //increment address
   address += sizeof(y);

   // Read onInteractScriptId
   ROM->Read(&onInteractScriptId, address);

   // Endianness conversion
   onInteractScriptId = ROM_ENDIAN_U2_VALUE(onInteractScriptId);

   //increment address
   address += sizeof(onInteractScriptId);

   // Read onTickScript
   ROM->Read(&onTickScriptId, address);

   // Endianness conversion
   onTickScriptId = ROM_ENDIAN_U2_VALUE(onTickScriptId);

   //increment address
   address += sizeof(onTickScriptId);

   // Read primaryId
   ROM->Read(&primaryId, address);

   // Endianness conversion
   primaryId = ROM_ENDIAN_U2_VALUE(primaryId);

   //increment address
   address += sizeof(primaryId);

   // Read secondaryId
   ROM->Read(&secondaryId, address);

   // Endianness conversion
   secondaryId = ROM_ENDIAN_U2_VALUE(secondaryId);

   //increment address
   address += sizeof(secondaryId);

   // Read primaryIdType
   ROM->Read(&primaryIdType, address);

   //increment address
   address += sizeof(primaryIdType);

   // Read currentAnimation
   ROM->Read(&currentAnimation, address);

   //increment address
   address += sizeof(currentAnimation);

   // Read currentFrameIndex
   ROM->Read(&currentFrameIndex, address);

   //increment address
   address += sizeof(currentFrameIndex);

   // Read direction
   ROM->Read(&direction, address);

   //increment address
   address += sizeof(direction);

   // Read hackableStateA
   ROM->Read(&hackableStateA, address);
   //increment address
   address += sizeof(hackableStateA);

   // Read hackableStateB
   ROM->Read(&hackableStateB, address);
   //increment address
   address += sizeof(hackableStateB);

   // Read hackableStateC
   ROM->Read(&hackableStateC, address);
   //increment address
   address += sizeof(hackableStateC);

   // Read hackableStateD
   ROM->Read(&hackableStateD, address);
   //increment address
   address += sizeof(hackableStateD);
}

void MageEntity::updateRenderableData(MageGameControl* gameControl)
{
   //make a local copy of the entity so the hacked values remain unchanged:
   //MageEntity entity = *entityPointer;

   //ensure the primaryIdType is valid
   primaryIdType =  (MageEntityPrimaryIdType)(primaryIdType % NUM_PRIMARY_ID_TYPES);

   //then get valid tileset renderableData based on primaryId type:
   if (primaryIdType == MageEntityPrimaryIdType::TILESET)
   {
      //ensure the tilesetId (in this scenario, the entity's primaryId) is valid.
      renderableData.tilesetId = primaryId;
      renderableData.tileId = secondaryId;
      renderableData.duration = 0; //unused
      renderableData.frameCount = 0; //unused
      renderableData.renderFlags = direction; //no need to check, it shouldn't cause a crash.
   }
   else if (primaryIdType == MageEntityPrimaryIdType::ANIMATION)
   {
      //ensure the animationId (in this scenario, the entity's primaryId) is valid.
      //uint16_t animationId = primaryId;
      //MageAnimation* animation = &animations[animationId];
      //MageAnimation::Frame currentFrame = animation->AnimationFrame(currentFrame);
      auto animation = gameControl->getAnimation(primaryId);
      renderableData.tilesetId = animation->TilesetId();
      renderableData.tileId = animation->TileId();
      renderableData.duration = animation->AnimationFrame(currentFrameIndex).duration; //no need to check, it shouldn't cause a crash.
      renderableData.frameCount = animation->FrameCount(); //no need to check, it shouldn't cause a crash.
      renderableData.renderFlags = direction; //no need to check, it shouldn't cause a crash.
   }
   else if (primaryIdType == MageEntityPrimaryIdType::ENTITY_TYPE)
   {
      //ensure the entityType (in this scenario, the entity's primaryId) is valid.
      uint16_t entityTypeId = gameControl->getValidEntityTypeId(primaryId);

      //If the entity has no animations defined, return default:
      if ((gameControl->entityTypes[entityTypeId].AnimationCount()) == 0)
      {
         //the entity has no animations, so return default values and give up.
#ifndef DC801_EMBEDDED
         fprintf(stderr, "An entityType with no animations exists. Using fallback values.");
#endif
         renderableData.tilesetId = MAGE_TILESET_FAILOVER_ID;
         renderableData.tileId = MAGE_TILE_FAILOVER_ID;
         renderableData.duration = MAGE_ANIMATION_DURATION_FAILOVER_VALUE;
         renderableData.frameCount = MAGE_FRAME_COUNT_FAILOVER_VALUE;
         renderableData.renderFlags = MAGE_RENDER_FLAGS_FAILOVER_VALUE;
      }

      //get a valid entity type animation ID:
      //note that entityType was already validated above.
      uint8_t entityTypeAnimationId = currentAnimation;

      //make a local copy of the current entity type animation:
      MageAnimation currentAnimation = gameControl->entityTypes[entityTypeId].EntityTypeAnimation(entityTypeAnimationId);

      //get a valid direction for the animation:
      uint8_t direction = direction;

      //create a directedAnimation entity based on direction:
      //auto directedAnimation = getAnimation;
      //if (direction == MageEntityAnimationDirection::NORTH)
      //{
      //   directedAnimation = currentAnimation.North();
      //}
      //else if (direction == MageEntityAnimationDirection::EAST)
      //{
      //   directedAnimation = currentAnimation.East();
      //}
      //else if (direction == MageEntityAnimationDirection::SOUTH)
      //{
      //   directedAnimation = currentAnimation.South();
      //}
      //else if (direction == MageEntityAnimationDirection::WEST)
      //{
      //   directedAnimation = currentAnimation.West();
      //}
      //renderableData.getRenderableState(entityPointer, currentAnimation);

   }

   auto tileset = gameControl->GetTileset(renderableData.tilesetId);
   Point oldCenter = { renderableData.center.x, renderableData.center.y };
   // accounting for possible change in tile size due to hacking;
   // adjust entity position so that the center will not change
   // from the previous tileset to the new tileset.

   if (renderableData.lastTilesetId != renderableData.tilesetId)
   {
      //get the difference between entity centers:
      x += oldCenter.x - renderableData.center.x;
      y += oldCenter.y - renderableData.center.y;
   }
   renderableData.updateRenderableBoxes(this, tileset);
   renderableData.lastTilesetId = renderableData.tilesetId;
}

void MageEntity::RenderableData::updateRenderableBoxes(const MageEntity* entity, const MageTileset* tileset)
{
   uint16_t width = tileset->TileWidth();
   uint16_t height = tileset->TileHeight();
   uint16_t halfWidth = width / 2;
   uint16_t halfHeight = height / 2;
   hitBox.x = entity->x + (halfWidth / 2);
   hitBox.y = entity->y + (halfHeight)-height;
   hitBox.w = halfWidth;
   hitBox.h = halfHeight;
   center.x = hitBox.x + (hitBox.w / 2);
   center.y = hitBox.y + (hitBox.h / 2);
}

void MageEntity::RenderableData::getRenderableState(MageGameControl* gameControl, const MageEntity* entity, const MageEntityTypeAnimationDirection* animationDirection)
{
   //based on animationDirection.Type(), you can get two different outcomes:
   //Scenario A: Type is 0, TypeID is an animation ID:
   //Scenario B: Type is not 0, so Type is a tileset(you will need to subtract 1 to get it 0-indexed), and TypeId is the tileId.
   if (animationDirection->Type() == 0)
   {
      auto animation = gameControl->getAnimation(animationDirection->TypeId());

      MageAnimation::Frame currentFrame = animation->AnimationFrame(entity->currentFrameIndex);
      tilesetId = animation->TilesetId();
      tileId = currentFrame.tileId;

      duration = currentFrame.duration; //no need to check, it shouldn't cause a crash.
      frameCount = animation->FrameCount(); //no need to check, it shouldn't cause a crash.
      renderFlags = animationDirection->RenderFlags(); //no need to check, it shouldn't cause a crash.
      renderFlags += entity->direction & 0x80;
   }
   else
   {
      tilesetId = animationDirection->Type() - 1;
      tileId = animationDirection->TypeId();
      duration = 0; //does not animate;
      frameCount = 0; //does not animate
      renderFlags = entity->direction; //no need to check, it shouldn't cause a crash.
   }
}

