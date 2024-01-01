#include "mage.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <typeinfo>

#include "mage_defines.h"
#include "mage_script_control.h"
#include "shim_timer.h"
#include "utility.h"

#ifndef DC801_EMBEDDED
#include <SDL.h>
#endif

#ifdef EMSCRIPTEN
#include "emscripten.h"
#endif

void MageGameEngine::Run()
{
   mapControl->mapLoadId = ROM()->GetCurrentSave().currentMapId;
   LoadMap();

   // TODO: fix/move emscripten startup
#ifdef EMSCRIPTEN
   emscripten_set_main_loop(gameLoop, 24, 1);
#else

   //update timing information at the start of every game loop
   auto lastTime = GameClock::now();
   auto totalTime = GameClock::duration{ 0 };
   auto accumulator = GameClock::duration{ 0 };
   auto fps = 0;

   while (inputHandler->KeepRunning())
   {
      if (inputHandler->Reset())
      {
         mapControl->mapLoadId = ROM()->GetCurrentSave().currentMapId;
      }

      if (mapControl->mapLoadId != MAGE_NO_MAP)
      {
         LoadMap();
      }


      // if the map is about to change, don't bother updating entities since they're about to be reloaded
      if (mapControl->mapLoadId != MAGE_NO_MAP) { return; }

      const auto loopStart = GameClock::now();
      const auto deltaTime = loopStart - lastTime;
      lastTime = loopStart;
      accumulator += deltaTime;

      const auto deltaState = inputHandler->GetDeltaState();
      applyUniversalInputs(deltaState);

      // using a fixed frame rate targeting the compile-time FPS
      // update the game state  
      while (accumulator >= MinTimeBetweenRenders)
      {
         // hack mode
         if (hexEditor->isHexEditorOn()
            && !(dialogControl->isOpen()
               || !playerHasControl
               || !hexEditor->playerHasHexEditorControl
               || hexEditor->IsMovementDisabled()))
         {
            hexEditor->applyHexModeInputs(deltaState);
         }
         // dialog mode
         else if (dialogControl->isOpen())
         {
            scriptControl->jumpScriptId = dialogControl->update(deltaState);
         }
         // gameplay mode
         else
         {
            applyGameModeInputs(deltaState);
         }
         // update the entities based on the current state of their (hackable) data array.
         mapControl->UpdateEntities();
         camera.applyEffects();
         accumulator -= IntegrationStepSize;
         totalTime += IntegrationStepSize;
      }

      // don't run scripts when hex editor is on
      if (!hexEditor->isHexEditorOn())
      {
         scriptControl->tickScripts();
      }

      // commands are only allowed to be sent once per frame, so they are outside the update loop
      commandControl->sendBufferedOutput();

      gameRender();

      lastTime = loopStart;

      // TODO: shouldn't this only pause the input updates, not the whole game loop?
      // if a blocking delay was added by any actions, pause before returning to the game loop:
      if (inputHandler->blockingDelayTime)
      {
         debug_print("Pretend to block for %x ms", inputHandler->blockingDelayTime);
         // TODO: after looking into it, we should avoid `nrf_delay_ms` entirely - it just loops nops:
         //delay for the right amount of time
         //nrf_delay_ms(inputHandler->blockingDelayTime);
         //reset delay time when done so we don't do this every loop.
         inputHandler->blockingDelayTime = 0;
      }
   }
#endif
}

void MageGameEngine::LoadMap()
{
   // clear any scripts that might trigger
   scriptControl->jumpScriptId = MAGE_NO_SCRIPT;

   // reset any fading that might be in progress
   frameBuffer->ResetFade();

   // close any open dialogs/hex editor overlays
   dialogControl->close();
   hexEditor->setHexEditorOn(false);

   commandControl->reset();

   // Load the map and trigger its OnLoad script
   // This is the only place OnLoad should be called
   mapControl->Load();
   auto onLoad = MageScriptState{ mapControl->currentMap->onLoadScriptId, true };
   scriptControl->processScript(onLoad, MAGE_MAP_ENTITY);

   // ensure the player has control
   playerHasControl = true;
}

void MageGameEngine::applyUniversalInputs(const DeltaState& delta)
{
   //make sure any delta.Buttons handling in this function can be processed in ANY game mode.
   //that includes the game mode, hex editor mode, any menus, maps, etc.
   ledSet(LED_PAGE, delta.Buttons.IsPressed(KeyPress::Page) ? 0xFF : 0x00);
   if (delta.Buttons.IsPressed(KeyPress::Xor)) { hexEditor->setHexOp(HEX_OPS_XOR); }
   if (delta.Buttons.IsPressed(KeyPress::Add)) { hexEditor->setHexOp(HEX_OPS_ADD); }
   if (delta.Buttons.IsPressed(KeyPress::Sub)) { hexEditor->setHexOp(HEX_OPS_SUB); }
   if (delta.Buttons.IsPressed(KeyPress::Bit128)) { hexEditor->runHex(0b10000000); }
   if (delta.Buttons.IsPressed(KeyPress::Bit64)) { hexEditor->runHex(0b01000000); }
   if (delta.Buttons.IsPressed(KeyPress::Bit32)) { hexEditor->runHex(0b00100000); }
   if (delta.Buttons.IsPressed(KeyPress::Bit16)) { hexEditor->runHex(0b00010000); }
   if (delta.Buttons.IsPressed(KeyPress::Bit8)) { hexEditor->runHex(0b00001000); }
   if (delta.Buttons.IsPressed(KeyPress::Bit4)) { hexEditor->runHex(0b00000100); }
   if (delta.Buttons.IsPressed(KeyPress::Bit2)) { hexEditor->runHex(0b00000010); }
   if (delta.Buttons.IsPressed(KeyPress::Bit1)) { hexEditor->runHex(0b00000001); }

   // only trigger on the first release of XOR MEM0, regardless of order it is pressed
   if (delta.Buttons.IsPressed(KeyPress::Xor) && delta.ActivatedButtons.IsPressed(KeyPress::Mem0)
      || delta.ActivatedButtons.IsPressed(KeyPress::Xor) && delta.Buttons.IsPressed(KeyPress::Mem0))
   {
      tileManager->ToggleDrawGeometry();
   }
}

void MageGameEngine::applyGameModeInputs(const DeltaState& delta)
{
   auto playerEntity = mapControl->getPlayerEntityData();

   const auto buttons = delta.Buttons;

   //set mage speed based on if the right pad down is being pressed:
   const auto moveAmount = buttons.IsPressed(KeyPress::Rjoy_down) ? RunSpeed : WalkSpeed;

   // clip player to [0,uint16_t max]
   if (delta.Left())
   {
      playerEntity->position.x = int(playerEntity->position.x) - moveAmount < 0 ? 0 : playerEntity->position.x - moveAmount;
   }
   else if (delta.Right())
   {
      playerEntity->position.x = int(playerEntity->position.x) + moveAmount > std::numeric_limits<uint16_t>::max() ? std::numeric_limits<uint16_t>::max() : playerEntity->position.x + moveAmount;
   }

   if (delta.Up())
   {
      playerEntity->position.y = int(playerEntity->position.y) - moveAmount < 0 ? 0 : playerEntity->position.y - moveAmount;
   }
   else if (delta.Down())
   {
      playerEntity->position.y = int(playerEntity->position.y) + moveAmount > std::numeric_limits<uint16_t>::max() ? std::numeric_limits<uint16_t>::max() : playerEntity->position.y + moveAmount;
   }

   auto playerEntityTypeId = playerEntity->primaryIdType % NUM_PRIMARY_ID_TYPES;
   auto hasEntityType = playerEntityTypeId == ENTITY_TYPE;
   auto entityType = hasEntityType ? ROM()->GetReadPointerByIndex<MageEntityType>(playerEntityTypeId) : nullptr;

   //check to see if the mage is pressing the action buttons, or currently in the middle of an action animation.
   if (playerHasControl && !delta.PlayerIsActioning())
   {
      //if not actioning or resetting, handle all remaining inputs:
      auto entityInteractId = mapControl->TryMovePlayer(delta);
      if (entityInteractId)
      {
         if (delta.Hack() && hexEditor->playerHasHexEditorControl)
         {
            hexEditor->disableMovementUntilRJoyUpRelease();
            hexEditor->openToEntity(*entityInteractId);
         }
         else if (!delta.Hack())
         {
            const auto scriptId = mapControl->Get<MageEntityData>(*entityInteractId).onInteractScriptId;
            auto& scriptState = mapControl->Get<MapControl::OnInteractScript>(scriptId);
            scriptState.script = mapControl->scripts[scriptId];
            scriptState.scriptIsRunning = true;
            scriptControl->processScript(scriptState, *entityInteractId);
         }
         else
         {

         }
      }
   }

   auto playerRenderableData = mapControl->getPlayerRenderableData();
   //handle animation assignment for the player:
   //Scenario 1 - perform action:
   if (delta.PlayerIsActioning() && hasEntityType
      && entityType->animationCount >= MAGE_ACTION_ANIMATION_INDEX)
   {
      playerRenderableData->SetAnimation(MAGE_ACTION_ANIMATION_INDEX);
   }
   //Scenario 2 - show walk animation:
   else if (mapControl->playerIsMoving && hasEntityType
      && entityType->animationCount >= MAGE_WALK_ANIMATION_INDEX)
   {
      playerRenderableData->SetAnimation(MAGE_WALK_ANIMATION_INDEX);
   }
   //Scenario 3 - show idle animation:
   else if (playerHasControl)
   {
      playerRenderableData->SetAnimation(MAGE_IDLE_ANIMATION_INDEX);
   }

   //this checks to see if the player is currently animating, and if the animation is the last frame of the animation:
   bool isPlayingActionButShouldReturnControlToPlayer = hasEntityType
      && (playerRenderableData->currentAnimation == MAGE_ACTION_ANIMATION_INDEX)
      && (playerRenderableData->currentFrameIndex == playerRenderableData->frameCount - 1)
      && (std::chrono::milliseconds{ playerRenderableData->currentFrameMs } >= playerRenderableData->duration);

   //if the above bool is true, set the player back to their idle animation:
   if (isPlayingActionButShouldReturnControlToPlayer)
   {
      playerRenderableData->SetAnimation(MAGE_IDLE_ANIMATION_INDEX);
   }


   if (!hexEditor->playerHasHexEditorControl || !playerHasControl)
   {
      return;
   }

   //opening the hex editor is the only button press that will lag actual gameplay by one frame
   //this is to allow entity scripts to check the hex editor state before it opens to run scripts
   if (delta.ActivatedButtons.IsPressed(KeyPress::Hax))
   {
      hexEditor->setHexEditorOn(true);
   }
   hexEditor->applyMemRecallInputs();
}

void MageGameEngine::gameRender()
{
   if (hexEditor->isHexEditorOn())
   {
      hexEditor->Draw();
   }
   else
   {
      mapControl->Draw();

      // dialogs are drawn after/on top of the map
      dialogControl->Draw();
   }

   //update the state of the LEDs
   // drawButtonStates(inputHandler->GetButtonState());
   // drawLEDStates();
   updateHexLights();

   // write changes to the framebuffer to the output screen
   frameBuffer->blt();
}


void MageGameEngine::updateHexLights() const
{
   const auto entityDataPointer = mapControl->GetEntityDataPointer();
   const auto hexCursorOffset = hexEditor->GetCursorOffset();
   const auto currentByte = *(entityDataPointer + hexCursorOffset);
   ledSet(LED_BIT128, ((currentByte >> 7) & 0x01) ? 0xFF : 0x00);
   ledSet(LED_BIT64, ((currentByte >> 6) & 0x01) ? 0xFF : 0x00);
   ledSet(LED_BIT32, ((currentByte >> 5) & 0x01) ? 0xFF : 0x00);
   ledSet(LED_BIT16, ((currentByte >> 4) & 0x01) ? 0xFF : 0x00);
   ledSet(LED_BIT8, ((currentByte >> 3) & 0x01) ? 0xFF : 0x00);
   ledSet(LED_BIT4, ((currentByte >> 2) & 0x01) ? 0xFF : 0x00);
   ledSet(LED_BIT2, ((currentByte >> 1) & 0x01) ? 0xFF : 0x00);
   ledSet(LED_BIT1, ((currentByte >> 0) & 0x01) ? 0xFF : 0x00);

   const auto entityRelativeMemOffset = hexCursorOffset % sizeof(MageEntityData);
   ledSet(LED_MEM0, (entityRelativeMemOffset == hexEditor->memOffsets[0]) ? 0xFF : 0x00);
   ledSet(LED_MEM1, (entityRelativeMemOffset == hexEditor->memOffsets[1]) ? 0xFF : 0x00);
   ledSet(LED_MEM2, (entityRelativeMemOffset == hexEditor->memOffsets[2]) ? 0xFF : 0x00);
   ledSet(LED_MEM3, (entityRelativeMemOffset == hexEditor->memOffsets[3]) ? 0xFF : 0x00);
}