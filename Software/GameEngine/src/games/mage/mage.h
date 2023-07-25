#ifndef _MAGE_H
#define _MAGE_H

#include <memory>
#include <vector>

#include "Header.h"
#include "EngineAudio.h"
#include "EngineInput.h"
#include "EngineSerial.h"
#include "EngineWindowFrame.h"
#include "mage_app_timers.h"
#include "mage_camera.h"
#include "mage_color_palette.h"
#include "mage_command_control.h"
#include "mage_dialog_control.h"
#include "mage_script_control.h"
#include "mage_map.h"
#include "mage_hex.h"
#include "utility.h"
#include "mage_rom.h"

#ifndef DC801_EMBEDDED
#include "shim_timer.h"
#endif

class MageGameEngine
{
public:

   MageGameEngine(std::shared_ptr<EngineInput> inputHandler, std::shared_ptr<FrameBuffer> frameBuffer)
      : inputHandler(inputHandler), frameBuffer(frameBuffer)
   {
      audioPlayer = std::make_unique<AudioPlayer>();
      tileManager = std::make_shared<TileManager>(frameBuffer);
      mapControl = std::make_shared<MapControl>(tileManager);
      hexEditor = std::make_shared<MageHexEditor>(frameBuffer, inputHandler, mapControl, ROM()->GetCurrentSave().memOffsets);
      stringLoader = std::make_shared<StringLoader>(scriptControl, mapControl, ROM()->GetCurrentSave().scriptVariables);
      dialogControl = std::make_unique<MageDialogControl>(frameBuffer, inputHandler, tileManager, stringLoader, mapControl);
      camera = MageCamera{ mapControl };

      auto scriptActions = std::make_unique<MageScriptActions>(frameBuffer, inputHandler, camera, mapControl, dialogControl, commandControl, hexEditor, stringLoader);
      scriptControl = std::make_shared<MageScriptControl>(mapControl, hexEditor, std::move(scriptActions));
      commandControl = std::make_shared<MageCommandControl>(mapControl, tileManager, scriptControl, stringLoader);
   }
   //this will load a map to be the current map.
   void LoadMap(uint16_t index);

   //this runs the actual game, performing initial setup and then
   //running the game loop indefinitely until the game is exited.
   void Run();

private:
   void getInputSate();

   //updates the state of all the things before rendering:
   void gameUpdate(const DeltaState& delta);

   //This renders the game to the frame buffer based on the loop's updated state.
   void gameRender();

   //this will handle any blocking delays at the end of the loop
   void handleBlockingDelay();

   //this takes input information and moves the playerEntity around
   //If there is no playerEntity, it just moves the camera freely.
   void applyGameModeInputs(const DeltaState& delta);

   //this handles inputs that apply in ALL game states. That includes when
   //the hex editor is open, when it is closed, when in any menus, etc.
   void applyUniversalInputs(const DeltaState& delta);
   void handleEntityInteract(bool hack);

   bool engineIsInitialized{ false };

   uint8_t currentSaveIndex{ 0 };

   //this lets us make it so that inputs stop working for the player
   bool playerHasControl{ false };
   bool playerHasHexEditorControl{ false };
   
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