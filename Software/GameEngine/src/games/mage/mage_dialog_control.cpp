#include "mage_dialog_control.h"

#include "mage_map.h"
#include "mage_portrait.h"
#include "screen_manager.h"
#include <utility>

const MageDialogAlignmentCoords alignments[ALIGNMENT_COUNT] = {
   { // BOTTOM_LEFT
      .text =     { 0, 8, 19, 6 },
      .label =    { 0, 6, 7, 3 },
      .portrait = { 0, 1, 6, 6 }
   },
   { // BOTTOM_RIGHT
      .text = { 0, 8, 19, 6 },
      .label = { 12, 6, 7, 3 },
      .portrait = { 13, 1, 6, 6 }
   },
   { // TOP_LEFT
      .text = { 0, 0, 19, 6 },
      .label = { 0, 5, 7, 3 },
      .portrait = { 0, 7, 6, 6 }
   },
   { // TOP_RIGHT
      .text = { 0, 0, 19, 6 },
      .label = { 12, 5, 7, 3 },
      .portrait = { 13, 7, 6, 6 }
   }
};

void MageDialogControl::load(uint16_t dialogId, std::string name)
{
   triggeringEntityName = name;

   currentScreenIndex = 0;
   currentResponseIndex = 0;
   currentDialog = ROM()->GetReadPointerByIndex<MageDialog>(dialogId);
   loadNextScreen();
   open = true;
}

void MageDialogControl::StartModalDialog(std::string messageString)
{
   if (currentDialog)
   {
      // Recycle all of the values set by the previous dialog to preserve look and feel
      // If there was no previous dialog... uhhhhhhh good luck with that?
      currentResponseIndex = 0;
      currentMessageIndex = 0;
      currentMessage = messageString;
      responses.clear();
      open = true;
   }
}

void MageDialogControl::loadNextScreen()
{
   if (currentScreenIndex >= currentDialog->ScreenCount)
   {
      open = false;
      return;
   }
   currentMessageIndex = 0;

   const auto& currentScreen = currentDialog->GetScreen(currentScreenIndex);
   loadCurrentScreenPortrait();

   currentMessage = currentScreen.GetMessage(stringLoader, currentMessageIndex, triggeringEntityName);
   currentFrameTilesetIndex = currentScreen.borderTilesetIndex;

   currentScreenIndex++;
}

std::optional<uint16_t> MageDialogControl::applyInput(const DeltaState& delta)
{
   if (!isOpen())
   {
      return std::nullopt;
   }

   const auto& currentScreen = currentDialog->GetScreen(currentScreenIndex);
   if (shouldShowResponses(currentScreen))
   {
      currentResponseIndex += currentScreen.responseCount;
      if (delta.Up()) { currentResponseIndex -= 1; }
      if (delta.Down()) { currentResponseIndex += 1; }
      currentResponseIndex %= currentScreen.responseCount;
      if (delta.Right())
      {
         open = false;
         return responses[currentResponseIndex].scriptIndex;
      }
   }

   const auto shouldAdvance = delta.AdvanceDialog()
      || MAGE_NO_MAP != mapControl->mapLoadId;

   if (shouldAdvance)
   {
      currentMessageIndex++;
      if (currentMessageIndex >= currentScreen.messageCount)
      {
         loadNextScreen();
      }
      else
      {
         currentMessage = currentScreen.GetMessage(stringLoader, currentMessageIndex, triggeringEntityName);
         //currentMessage = stringLoader->getString(currentMessageId, triggeringEntityName);
      }
   }
   return std::nullopt;
}

void MageDialogControl::update()
{
   if (!open)
   {
      return;
   }

   cursorPhase += IntegrationStepSize.count();
}

void MageDialogControl::Draw() const
{
   if (!open)
   {
      return;
   }

   const auto& currentScreen = currentDialog->GetScreen(currentScreenIndex);
   const auto coords = alignments[(uint8_t)currentScreen.alignment % ALIGNMENT_COUNT];
   const auto tileset = ROM()->GetReadPointerByIndex<MageTileset>(currentFrameTilesetIndex);

   const auto labelX = (uint16_t)((coords.label.origin.x * tileset->TileWidth) + (tileset->TileWidth / 2));
   const auto labelY = (uint16_t)((coords.label.origin.y * tileset->TileHeight) + (tileset->TileHeight / 2));
   const auto messageX = (uint16_t)((coords.text.origin.x * tileset->TileWidth) + (tileset->TileWidth / 2));
   const auto messageY = (uint16_t)((coords.text.origin.y * tileset->TileHeight) + (tileset->TileHeight / 2));

   drawBackground({ labelX, labelY, coords.label.w, coords.label.h });
   screenManager->DrawText(currentEntityName, COLOR_WHITE, labelX + tileset->TileWidth + 8, labelY + tileset->TileHeight - 2);
   drawBackground({ messageX, messageY, coords.text.w, coords.text.h });
   screenManager->DrawText(currentMessage, COLOR_WHITE, messageX + tileset->TileWidth + 8, messageY + tileset->TileHeight - 2);

   if (shouldShowResponses(currentScreen))
   {
      // render all of the response labels
      for (int responseIndex = 0; responseIndex < currentScreen.responseCount; ++responseIndex)
      {
         screenManager->DrawText(stringLoader->getString(responses[responseIndex].stringIndex, triggeringEntityName), COLOR_WHITE, messageX + tileset->TileWidth + 6, messageY);
      }
      screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, tileset->ImageId, messageX, messageY);
   }
   else
   {
      // bounce the arrow at the bottom when we don't have responses
      static const auto TAU = 6.283185307179586f;
      const auto tileset = ROM()->GetReadPointerByIndex<MageTileset>(currentFrameTilesetIndex);
      const auto bounce = cos(((float)cursorPhase / 1000.0f) * TAU) * 3;
      screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, tileset->ImageId,
         messageX + (coords.text.w - 2) * tileset->TileWidth,
         messageY + (coords.text.h - 2) * tileset->TileHeight + bounce);
   }

   if (currentPortraitId != DIALOG_SCREEN_NO_PORTRAIT)
   {
      screenManager->DrawTileScreenCoords(currentPortraitRenderableData.tilesetId, currentPortraitRenderableData.tileId, messageX + tileset->TileWidth, messageY + tileset->TileHeight, currentPortraitRenderableData.renderFlags);
   }
}

void MageDialogControl::drawBackground(const EntityRect& box) const
{
   const auto tileset = ROM()->GetReadPointerByIndex<MageTileset>(currentFrameTilesetIndex);
   for (auto x = box.w - 1; x >= 0; x--)
   {
      for (auto y = box.h - 1; y >= 0; y--)
      {
         const auto leftEdge = x == 0;
         const auto rightEdge = x == (box.w - 1);
         const auto topEdge = y == 0;
         const auto bottomEdge = y == (box.h - 1);

         const auto drawX = x * tileset->TileWidth + box.origin.x;
         const auto drawY = y * tileset->TileHeight + box.origin.y;

         if (leftEdge)
         {
            if (topEdge)
            {
               screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_TOP_LEFT, drawX, drawY);
            }
            else if (bottomEdge)
            {
               screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_BOTTOM_LEFT, drawX, drawY);
            }
            else
            {
               screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_LEFT_REPEAT, drawX, drawY);
            }
         }
         else if (rightEdge)
         {
            if (topEdge)
            {
               screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_TOP_RIGHT, drawX, drawY);
            }
            else if (bottomEdge)
            {
               screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_BOTTOM_RIGHT, drawX, drawY);
            }
            else
            {
               screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_RIGHT_REPEAT, drawX, drawY);
            }
         }
         else if (topEdge)
         {
            screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_TOP_REPEAT, drawX, drawY);
         }
         else if (bottomEdge)
         {
            screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_BOTTOM_REPEAT, drawX, drawY);
         }
         else //background
         {
            screenManager->DrawTileScreenCoords(currentFrameTilesetIndex, DIALOG_TILES_CENTER_REPEAT, drawX, drawY);
         }
      }
   }
}

void MageDialogControl::loadCurrentScreenPortrait()
{
   auto& currentScreen = currentDialog->GetScreen(currentScreenIndex);
   currentPortraitId = currentScreen.portraitIndex;
   // only try rendering when we have a portrait
   if (currentPortraitId != DIALOG_SCREEN_NO_PORTRAIT)
   {
      if (currentScreen.entityIndex != NO_PLAYER_INDEX)
      {
         auto& currentEntity = mapControl->Get<MageEntityData>(currentScreen.entityIndex);
         uint8_t sanitizedPrimaryType = currentEntity.primaryIdType % NUM_PRIMARY_ID_TYPES;
         if (sanitizedPrimaryType == ENTITY_TYPE)
         {
            currentPortraitId = ROM()->GetReadPointerByIndex<MageEntityType>(currentEntity.primaryId)->portraitId;
         }

         auto portrait = ROM()->GetReadPointerByIndex<MagePortrait>(currentPortraitId);
         auto animationDirection = portrait->getEmoteById(currentScreen.emoteIndex);
         currentEntity.flags = animationDirection->renderFlags;
         currentPortraitRenderableData.renderFlags = animationDirection->renderFlags | (currentEntity.flags & 0x80);
         // if the portrait is on the right side of the screen, flip the portrait on the X axis
         if (((uint8_t)currentScreen.alignment % 2))
         {
            currentPortraitRenderableData.renderFlags |= RENDER_FLAGS_FLIP_X;
         }
      }

   }

}