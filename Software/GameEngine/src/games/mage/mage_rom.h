﻿#ifndef MAGE_ROM_H_
#define MAGE_ROM_H_

#include "mage_rom.h"
#include <stdint.h>
#include "EngineROM.h"
#include "EngineInput.h"

//this is the path to the game.dat file on the SD card.
//if an SD card is inserted with game.dat in this location
//and its header hash is different from the one in the ROM chip
//it will automatically be loaded.
#define MAGE_GAME_DAT_PATH "MAGE/game.dat"

static const inline auto MAGE_SAVE_FLAG_COUNT= 2048;
static const inline auto MAGE_SAVE_FLAG_BYTE_COUNT= (MAGE_SAVE_FLAG_COUNT / 8);
static const inline auto MAGE_SCRIPT_VARIABLE_COUNT= 256;
static const inline auto MAGE_ENTITY_NAME_LENGTH = 12;
static const inline auto MAGE_NUM_MEM_BUTTONS = 4;
static const inline auto DEFAULT_MAP = 0;
static const inline auto MAGE_NO_WARP_STATE = ((uint16_t)-1);

enum struct MageEntityFieldOffset: uint8_t
{
   x = 12,
   y = 14,
   onInteractScriptId = 16,
   onTickScriptId = 18,
   primaryId = 20,
   secondaryId = 22,
   primaryIdType = 24,
   currentAnimation = 25,
   currentFrame = 26,
   direction = 27,
   hackableStateA = 28,
   hackableStateB = 29,
   hackableStateC = 30,
   hackableStateD = 31
};

struct MageSaveGame
{
   MageSaveGame() noexcept = default;
   MageSaveGame(const MageSaveGame&) noexcept = default;
   MageSaveGame& operator=(const MageSaveGame&) = default;
   
   char identifier[8]{ 'M', 'A', 'G', 'E', 'S', 'A', 'V', 'E' };
   uint32_t engineVersion{ ENGINE_VERSION };
   uint32_t scenarioDataCRC32{ 0 };
   uint32_t saveDataLength{ sizeof(MageSaveGame) };
   char name[MAGE_ENTITY_NAME_LENGTH]{ "Bub" };

   //this stores the byte offsets for the hex memory buttons:
   std::array<uint8_t, MAGE_NUM_MEM_BUTTONS> memOffsets{
      static_cast<uint8_t>(MageEntityFieldOffset::x),
      static_cast<uint8_t>(MageEntityFieldOffset::y),
      static_cast<uint8_t>(MageEntityFieldOffset::primaryId), // entityType
      static_cast<uint8_t>(MageEntityFieldOffset::direction),
   };
   uint16_t currentMapId{ DEFAULT_MAP };
   uint16_t warpState{ MAGE_NO_WARP_STATE };
   uint8_t paddingA{ 0 };
   uint8_t paddingB{ 0 };
   uint8_t paddingC{ 0 };
   uint8_t saveFlags[MAGE_SAVE_FLAG_BYTE_COUNT]{ 0 };
   uint16_t scriptVariables[MAGE_SCRIPT_VARIABLE_COUNT]{ 0 };
};

class MapData;
class MageAnimation;
class MageEntityType;
class MageEntityData;
class MageGeometry;
struct MageScript;
class MageDialog;
class MageSerialDialog;
class MageColorPalette;
struct MageTileset;



struct AnimationDirection
{
   uint16_t typeId{ 0 };
   uint8_t type{ 0 };
   uint8_t renderFlags{ 0 };
};

struct MagePortrait
{
   char portrait[32]{ 0 };
   char padding[3]{ 0 };
   uint8_t emoteCount{ 0 };

   const AnimationDirection* getEmoteById(uint8_t emoteId) const
   {
      auto animationPtr = (const AnimationDirection*)((uint8_t*)&emoteCount + sizeof(uint8_t));
      return &animationPtr[emoteId % emoteCount];
   }
};

template<typename T, typename Tag>
struct TaggedType : T {};

using MageStringValue = TaggedType<char, struct stringTag>;
using MageVariableValue = TaggedType<char, struct variableTag>;
using MagePixel = uint8_t;

typedef EngineROM<MageSaveGame,
   MapData,
   MageTileset,
   MageAnimation,
   MageEntityType,
   MageEntityData,
   MageGeometry,
   MageScript,
   MagePortrait,
   MageDialog,
   MageSerialDialog,
   MageColorPalette,
   MageStringValue,
   MageSaveGame,
   MageVariableValue,
   MagePixel> MageROM;

const MageROM* ROM();

struct MageTileset
{
   static inline const auto TilesetNameLength = 16;
   constexpr uint16_t TileCount() const { return Rows * Cols; }

   const MageGeometry* GetGeometryForTile(uint16_t tileIndex) const
   {
      auto geometriesPtr = (uint16_t*)((uint8_t*)&Rows + sizeof(uint16_t));

      if (tileIndex >= Cols * Rows || !geometriesPtr[tileIndex]) { return nullptr; }
      auto geometryIndex = geometriesPtr[tileIndex - 1];
      if (geometryIndex)
      {
         return ROM()->GetReadPointerByIndex<MageGeometry>(geometryIndex);
      }
      else
      {
         return nullptr;
      }
   }

   const char     Name[TilesetNameLength]{ 0 };
   const uint16_t ImageId{ 0 };
   const uint16_t ImageWidth{ 0 };
   const uint16_t ImageHeight{ 0 };
   const uint16_t TileWidth{ 0 };
   const uint16_t TileHeight{ 0 };
   const uint16_t Cols{ 0 };
   const uint16_t Rows{ 0 };
};
#endif
   