#ifndef _MAGE_H
#define _MAGE_H

#include "Header.h"
#include "mage_rom.h"

#include "EngineAudio.h"
#include "EngineInput.h"
#include "EngineSerial.h"
#include "EngineWindowFrame.h"
#include "mage_camera.h"
#include "mage_color_palette.h"
#include "mage_command_control.h"
#include "mage_dialog_control.h"
#include "mage_script_control.h"
#include "mage_hex.h"
#include "utility.h"
#include <memory>
#include <vector>

#ifndef DC801_EMBEDDED
#include "shim_timer.h"
#endif

class AudioPlayer;
class EngineInput;
class EngineWindowFrame;
class FrameBuffer;
class MageHexEditor;
class MageScriptControl;
class MageCommandControl;
class MageScriptActions;
class StringLoader;

class MageGameEngine
{
public:

   MageGameEngine()
   {
      audioPlayer = std::make_unique<AudioPlayer>();
      inputHandler = std::make_shared<EngineInput>();

      frameBuffer = std::make_shared<FrameBuffer>();
      
      //skip 'MAGEGAME' at front of .dat file
      uint32_t offset = ENGINE_ROM_IDENTIFIER_STRING_LENGTH;

      ROM()->Read(engineVersion, offset);

      if (engineVersion != ENGINE_VERSION)
      {
         throw std::runtime_error{ "game.dat is incompatible with Engine" };// \n\nEngine version : % d\ngame.dat version : % d", ENGINE_VERSION, engineVersion };
      }

      ROM()->Read(scenarioDataCRC32, offset);
      ROM()->Read(scenarioDataLength, offset);

      auto newSave = new MageSaveGame{};
      newSave->scenarioDataCRC32 = scenarioDataCRC32;
      currentSave.reset(newSave);

      tileManager = std::make_shared<TileManager>(frameBuffer);
      mapControl = std::make_unique<MapControl>(frameBuffer, tileManager);
      hexEditor = std::make_shared<MageHexEditor>(frameBuffer, inputHandler, mapControl, currentSave->memOffsets);
      stringLoader = std::make_shared<StringLoader>(scriptControl, mapControl, currentSave->scriptVariables);
      dialogControl = std::make_unique<MageDialogControl>(frameBuffer, inputHandler, tileManager, stringLoader, scriptControl, mapControl);
      camera = MageCamera{ mapControl };

      scriptControl = std::make_shared<MageScriptControl>(mapControl, hexEditor, std::make_unique<MageScriptActions>(frameBuffer, inputHandler, camera, mapControl, dialogControl, scriptControl, commandControl, hexEditor, stringLoader));
      commandControl = std::make_shared<MageCommandControl>(mapControl, tileManager, scriptControl);
   }

   //updates the state of all the things before rendering:
   void GameUpdate(uint32_t deltaTime);

   //This renders the game to the screen based on the loop's updated state.
   void GameRender();

   void EngineMainGameLoop();
   void onSerialStart();
   void onSerialCommand(char* commandString);

   //this will load a map to be the current map.
   void LoadMap(uint16_t index);

   //this runs the actual game, performing initial setup and then
   //running the game loop indefinitely until the game is exited.
   void Run();

private:
   //this will handle any blocking delays at the end of the loop
   void handleBlockingDelay();

   //this takes input information and moves the playerEntity around
   //If there is no playerEntity, it just moves the camera freely.
   void applyGameModeInputs(uint32_t deltaTime);

   //this handles inputs that apply in ALL game states. That includes when
   //the hex editor is open, when it is closed, when in any menus, etc.
   void applyUniversalInputs();
   void handleEntityInteract(bool hack);
   Point getPushBackFromTilesThatCollideWithPlayer();

   bool engineIsInitialized{ false };

   //used to verify whether a save is compatible with game data
   uint32_t engineVersion;
   uint32_t scenarioDataCRC32;
   uint32_t scenarioDataLength;
   
   uint32_t lastTime{ millis() };
   uint32_t now{ 0 };
   uint32_t deltaTime{ 0 };
   uint32_t lastLoopTime{ millis() };

   float mageSpeed{ 0.0f };
   bool isMoving{ false };
   Point playerVelocity = { 0,0 };

   uint8_t currentSaveIndex{ 0 };
   std::shared_ptr<MageSaveGame> currentSave{};

   //this lets us make it so that inputs stop working for the player
   bool playerHasControl{ false };
   bool playerHasHexEditorControl{ false };
   bool isCollisionDebugOn { false };
   
   std::unique_ptr<AudioPlayer> audioPlayer;
   std::shared_ptr<EngineInput> inputHandler;
   std::shared_ptr<FrameBuffer> frameBuffer;
   std::shared_ptr<MageHexEditor> hexEditor;
   std::shared_ptr<MageScriptControl> scriptControl;
   std::shared_ptr<MageCommandControl> commandControl;
   std::shared_ptr<TileManager> tileManager;
   std::shared_ptr<MageDialogControl> dialogControl;
   std::shared_ptr<MapControl> mapControl;
   std::shared_ptr<StringLoader> stringLoader;

   MageCamera camera;
};

#endif //_MAGE_H