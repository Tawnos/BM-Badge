#ifndef _MAGE_GAME_CONTROL
#define _MAGE_GAME_CONTROL

#include "EngineROM.h"
#include "EngineInput.h"
#include "FrameBuffer.h"

#include "mage_animation.h"
#include "mage_entity_type.h"
#include "mage_camera.h"
#include "mage_defines.h"
#include "mage_header.h"
#include "mage_tileset.h"
#include <vector>

#define MAGE_COLLISION_SPOKE_COUNT 6

struct MageSaveGame
{
   MageSaveGame() noexcept : name{ DEFAULT_PLAYER_NAME } {};
   const char* identifier = EngineROM::SaveIdentifierString;
   uint32_t engineVersion{ ENGINE_VERSION };
   uint32_t scenarioDataCRC32{ 0 };
   uint32_t saveDataLength{ sizeof(MageSaveGame) };
   std::string name{ MAGE_ENTITY_NAME_LENGTH, 0 }; // bob's club
   //this stores the byte offsets for the hex memory buttons:
   uint8_t memOffsets[MAGE_NUM_MEM_BUTTONS]{
      MageEntityField::x,
      MageEntityField::y,
      MageEntityField::primaryId, // entityType
      MageEntityField::direction,
   };
   uint16_t currentMapId{ DEFAULT_MAP };
   uint16_t warpState{ MAGE_NO_WARP_STATE };
   uint8_t clipboard[1]{ 0 };
   uint8_t clipboardLength{ 0 };
   uint8_t paddingA{ 0 };
   uint8_t paddingB{ 0 };
   uint8_t paddingC{ 0 };
   uint8_t saveFlags[MAGE_SAVE_FLAG_BYTE_COUNT] = { 0 };
   uint16_t scriptVariables[MAGE_SCRIPT_VARIABLE_COUNT] = { 0 };
};

class StringLoader
{
public:
   StringLoader(const MageGameEngine* gameEngine, const uint16_t* scriptVariables, const MageHeader& stringHeader) noexcept
      : gameEngine(gameEngine), scriptVariables(scriptVariables), stringHeader(stringHeader) {}

   std::string getString(uint16_t stringId, int16_t mapLocalEntityId) const;
private:
   const MageGameEngine* gameEngine;
   const uint16_t* scriptVariables;
   MageHeader stringHeader;
};

class MageGameControl
{
   friend class MageGameEngine;
   friend class MageScriptActions;
   friend class MageCommandControl;
   friend class MageScriptControl;
   friend class MageDialogControl;
   friend class MageMap;
   friend class MageHexEditor;
   friend class MageEntity;
public:
   MageGameControl(MageGameEngine* gameEngine);

   uint8_t currentSaveIndex{ 0 };

   //used to verify whether a save is compatible with game data
   uint32_t engineVersion;
   uint32_t scenarioDataCRC32;
   uint32_t scenarioDataLength;

   MageSaveGame currentSave{};

   //this lets us make it so that inputs stop working for the player
   bool playerHasControl{ false };
   bool playerHasHexEditorControl{ false };
   bool playerHasHexEditorControlClipboard{ false };
   bool isCollisionDebugOn{ false };
   bool isEntityDebugOn{ false };
   MageCamera camera;

   void setCurrentSaveToFreshState();
   void readSaveFromRomIntoRam(bool silenceErrors = false);
   void saveGameSlotSave();
   void saveGameSlotErase(uint8_t slotIndex);
   void saveGameSlotLoad(uint8_t slotIndex);

   //this will return the current map object.
   auto Map() { return map; }

   //this handles inputs that apply in ALL game states. That includes when
   //the hex editor is open, when it is closed, when in any menus, etc.
   void applyUniversalInputs();

   //this takes input information and moves the playerEntity around
   //If there is no playerEntity, it just moves the camera freely.
   void applyGameModeInputs(uint32_t deltaTime);
   void applyCameraEffects(uint32_t deltaTime);

   //this will check in the direction the player entity is facing and start
   //an on_interact script for an entity if any qualify.
   void handleEntityInteract(bool hack);

   //this will load a map to be the current map.
   void LoadMap(uint16_t index);

   //this will render the map onto the screen.
   void DrawMap(uint8_t layer);

   uint16_t getValidEntityTypeId(uint16_t entityTypeId) const
   {
      //always return a valid entity type for the entityTypeId submitted.
      return entityTypeId % entityTypes.size();
   }

   inline const MageEntityType* MageGameControl::GetEntityType(uint16_t entityTypeId) const
   {
      //always return a valid entity type for the entityTypeId submitted.
      return entityTypes[getValidEntityTypeId(entityTypeId)];
   }

   inline uint8_t MageGameControl::getValidEntityTypeAnimationId(uint8_t entityTypeAnimationId, uint16_t entityTypeId) const
   {
      //use failover animation if an invalid animationId is submitted to the function.
      //There's a good chance if that happens, it will break things.
      entityTypeId = entityTypeId % entityTypes.size();

      auto animationCount = entityTypes[entityTypeId]->AnimationCount();
      //always return a valid entity type animation ID for the entityTypeAnimationId submitted.
      return entityTypeAnimationId % animationCount;
   }

   inline uint8_t MageGameControl::updateDirectionAndPreserveFlags(uint8_t desired, uint8_t previous) const
   {
      return (direction & RENDER_FLAGS_DIRECTION_MASK) 
         | (previous & (RENDER_FLAGS_IS_DEBUG | RENDER_FLAGS_IS_GLITCHED));
   }

   const MageGeometry* getGeometry(uint16_t globalGeometryId) const;
   const MageEntity* getEntityByMapLocalId(uint8_t mapLocalEntityId) const;
   MageEntity* getEntityByMapLocalId(uint8_t mapLocalEntityId);

   std::string getString(uint16_t stringId, int16_t mapLocalEntityId = NO_PLAYER) const;

   //this returns the address offset for a specific script Id:
   uint32_t getScriptAddressFromGlobalScriptId(uint32_t scriptId);

   Point getPushBackFromTilesThatCollideWithPlayer();

#ifndef DC801_EMBEDDED
   void verifyAllColorPalettes(const char* errorTriggerDescription);
#endif

   void updateEntityRenderableData(uint8_t mapLocalEntityId, bool skipTilesetCheck = true);
private:

   MageGameEngine* gameEngine;
   std::unique_ptr<MageDialogControl> dialogControl;

   std::unique_ptr<StringLoader> stringLoader;
   //this is where the current map data from the ROM is stored.
   std::shared_ptr<MageMap> map;
   std::shared_ptr<TileManager> tileManager;

   //this is an array of the entity types on the ROM
   //each entry is an indexed entity type.
   std::vector<const MageEntityType*> entityTypes;

   //a couple of state variables for tracking player movement:
   float mageSpeed{ 0.0f };
   bool isMoving{ false };

   Point playerVelocity = { 0,0 };
}; //class MageGameControl



#endif //_MAGE_GAME_CONTROL
