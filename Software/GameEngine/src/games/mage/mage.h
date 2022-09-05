#ifndef _MAGE_H
#define _MAGE_H

#include "mage_color_palette.h"
#include "mage_defines.h"

#include "EngineInput.h"
#include "EngineAudio.h"

class FrameBuffer;
class MageGameControl;
class MageCommandControl;
class MageHexEditor;
class MageDialogControl;
class MageScriptControl;
class MageScriptActions;
class EngineWindowFrame;

class MageGameEngine
{
public:
   MageGameEngine() noexcept;

   //this will handle any blocking delays at the end of the loop
   void handleBlockingDelay();

   //updates the state of all the things before rendering:
   void GameUpdate(uint32_t deltaTime);

   //This renders the game to the screen based on the loop's updated state.
   void GameRender();

   void EngineMainGameLoop();
   void onSerialStart();
   void onSerialCommand(char* commandString);

   //this runs the actual game, performing initial setup and then
   //running the game loop indefinitely until the game is exited.
   void Run();

   std::unique_ptr<AudioPlayer> audioPlayer;
   std::unique_ptr<EngineWindowFrame> windowFrame;

   std::shared_ptr<MageGameEngine> self{ this };
   std::shared_ptr<EngineROM> ROM;
   std::shared_ptr<MageGameControl> gameControl;
   std::shared_ptr<MageHexEditor> hexEditor;
   std::shared_ptr<MageScriptControl> scriptControl;
   std::shared_ptr<MageScriptActions> scriptActions;
   std::shared_ptr<MageCommandControl> commandControl;
   std::shared_ptr<MageDialogControl> dialogControl;
   std::shared_ptr<MageEntity> hackableDataAddress;
   std::shared_ptr<EngineInput> inputHandler;
   std::shared_ptr<FrameBuffer> frameBuffer;
private:

   bool engineIsInitialized{ false };

   uint32_t lastTime;
   uint32_t now;
   uint32_t deltaTime;
   uint32_t lastLoopTime;

};

#endif //_MAGE_H