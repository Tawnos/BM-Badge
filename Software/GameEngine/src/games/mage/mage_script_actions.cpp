#include "mage_script_actions.h"

#include "EngineInput.h"
#include "mage_camera.h"
#include "mage_command_control.h"
#include "mage_dialog_control.h"
#include "mage_entity_type.h"
#include "mage_script_control.h"
#include "mage_script_state.h"

#include "mage_geometry.h"
#include "mage_hex.h"
#include "utility.h"

void MageScriptActions::setResumeStatePointsAndEntityDirection(MageScriptState* resumeStateStruct, MageEntity* entity, const MageGeometry* geometry)
{
   auto entityCenterPoint = -entity->getRenderableData()->center - entity->location;
   resumeStateStruct->pointA = entityCenterPoint + geometry->GetPoint(resumeStateStruct->currentSegmentIndex);
   resumeStateStruct->pointB = entityCenterPoint + geometry->GetPoint(resumeStateStruct->currentSegmentIndex + 1);
   auto relativeDirection = resumeStateStruct->pointA.getRelativeDirection(resumeStateStruct->pointB);
   entity->renderFlags.updateDirectionAndPreserveFlags(relativeDirection);
}


void MageScriptActions::action_null_action(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t paddingA;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionNullAction;
   //nullAction does nothing.
}

void MageScriptActions::action_check_entity_name(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t stringId;
      uint8_t entityId;
      uint8_t expectedBoolValue;
      uint8_t paddingG;
   } ActionCheckEntityName;
   auto argStruct = (ActionCheckEntityName*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      std::string romString = stringLoader->getString(argStruct->stringId, scriptControl->currentEntityId);
      std::string entityName = mapControl->getEntityByMapLocalId(entityIndex)->name;

      int compare = strcmp(entityName.c_str(), romString.c_str());
      bool identical = compare == 0;
      if (identical == (bool)argStruct->expectedBoolValue)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_x(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedValue;
      uint8_t entityId;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityX;
   auto argStruct = (ActionCheckEntityX*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->location.x == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_y(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedValue;
      uint8_t entityId;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityY;
   auto argStruct = (ActionCheckEntityY*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->location.y == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_interact_script(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedScript;
      uint8_t entityId;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityInteractScript;
   auto argStruct = (ActionCheckEntityInteractScript*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->onInteractScriptId == argStruct->expectedScript);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_tick_script(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedScript;
      uint8_t entityId;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityTickScript;
   auto argStruct = (ActionCheckEntityTickScript*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->onTickScriptId == argStruct->expectedScript);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_type(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t entityTypeId;
      uint8_t entityId;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityType;
   auto argStruct = (ActionCheckEntityType*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = entity->primaryId == argStruct->entityTypeId && entity->primaryIdType == ENTITY_TYPE;

      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_primary_id(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedValue;
      uint8_t entityId;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityPrimaryId;
   auto argStruct = (ActionCheckEntityPrimaryId*)args;
   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      uint16_t sizeLimit{ 1 };
      uint8_t sanitizedPrimaryType = entity->primaryIdType % NUM_PRIMARY_ID_TYPES;
      
      if (sanitizedPrimaryType == MageEntityPrimaryIdType::ENTITY_TYPE) { sizeLimit = ROM->GetCount<MageEntityType>(); }
      else if (sanitizedPrimaryType == MageEntityPrimaryIdType::ANIMATION) { sizeLimit = ROM->GetCount<MageAnimation>(); }
      else if (sanitizedPrimaryType == MageEntityPrimaryIdType::TILESET) { sizeLimit = ROM->GetCount<MageTileset>(); }
      else { throw std::runtime_error{ "Sanitized Primary Type Unknown" }; }

      bool identical = ((entity->primaryId % sizeLimit) == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_secondary_id(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedValue;
      uint8_t entityId;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntitySecondaryId;
   auto argStruct = (ActionCheckEntitySecondaryId*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      uint16_t sizeLimit = 1;
      uint8_t sanitizedPrimaryType = entity->primaryIdType % NUM_PRIMARY_ID_TYPES;
      if (sanitizedPrimaryType == MageEntityPrimaryIdType::ENTITY_TYPE) { sizeLimit = 1; }
      if (sanitizedPrimaryType == MageEntityPrimaryIdType::ANIMATION) { sizeLimit = 1; }
      if (sanitizedPrimaryType == MageEntityPrimaryIdType::TILESET)
      {
         auto tileset = ROM->Get<MageTileset>(entity->primaryId);
         sizeLimit = tileset->Tiles();
      }
      bool identical = ((entity->secondaryId % sizeLimit) == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_primary_id_type(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedValue;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityPrimaryIdType;
   auto argStruct = (ActionCheckEntityPrimaryIdType*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      uint8_t sanitizedPrimaryType = entity->primaryIdType % NUM_PRIMARY_ID_TYPES;
      bool identical = (sanitizedPrimaryType == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_current_animation(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedValue;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityCurrentAnimation;
   auto argStruct = (ActionCheckEntityCurrentAnimation*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->currentAnimation == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_current_frame(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedValue;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityCurrentFrame;
   auto argStruct = (ActionCheckEntityCurrentFrame*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->currentFrameIndex == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_direction(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedValue;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityDirection;
   auto argStruct = (ActionCheckEntityDirection*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->renderFlags == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_glitched(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedBool;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCheckEntityGlitched;
   auto argStruct = (ActionCheckEntityGlitched*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      if (entity->renderFlags & RENDER_FLAGS_IS_GLITCHED)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_hackable_state_a(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedValue;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityHackableStateA;
   auto argStruct = (ActionCheckEntityHackableStateA*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->hackableStateA == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_hackable_state_b(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedValue;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityHackableStateB;
   auto argStruct = (ActionCheckEntityHackableStateB*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->hackableStateB == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_hackable_state_c(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedValue;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityHackableStateC;
   auto argStruct = (ActionCheckEntityHackableStateC*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->hackableStateC == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_hackable_state_d(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t entityId;
      uint8_t expectedValue;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckEntityHackableStateD;
   auto argStruct = (ActionCheckEntityHackableStateD*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      bool identical = (entity->hackableStateD == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_hackable_state_a_u2(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedValue;
      uint8_t entityId;
      uint8_t expectedBool;
   } ActionCheckEntityHackableStateAU2;
   auto argStruct = (ActionCheckEntityHackableStateAU2*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      uint16_t u2_value = *(uint16_t*)((uint8_t*)&entity->hackableStateA);
      bool identical = (u2_value == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_hackable_state_c_u2(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedValue;
      uint8_t entityId;
      uint8_t expectedBool;
   } ActionCheckEntityHackableStateCU2;
   auto argStruct = (ActionCheckEntityHackableStateCU2*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      uint16_t u2_value = *(uint16_t*)((uint8_t*)&entity->hackableStateC);
      bool identical = (u2_value == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_hackable_state_a_u4(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t expectedValue;
      uint16_t successScriptId;
      uint8_t entityId;
   } ActionCheckEntityHackableStateAU4;
   auto argStruct = (ActionCheckEntityHackableStateAU4*)args;
   argStruct->expectedValue = argStruct->expectedValue;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      uint32_t u4_value = ROM_ENDIAN_U4_VALUE(
         *(uint32_t*)((uint8_t*)&entity->hackableStateA));
      if (u4_value == argStruct->expectedValue)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_entity_path(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t expectedValue;
      uint8_t entityId;
      uint8_t expectedBool;
   } ActionCheckEntityPath;
   auto argStruct = (ActionCheckEntityPath*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      uint16_t pathId = *(uint16_t*)((uint8_t*)&entity->hackableStateA);
      bool identical = (pathId == argStruct->expectedValue);
      if (identical == (bool)argStruct->expectedBool)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_save_flag(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t saveFlagOffset;
      uint8_t expectedBoolValue;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCheckSaveFlag;
   auto argStruct = (ActionCheckSaveFlag*)args;
   auto currentSave = ROM->GetCurrentSave();
   uint16_t byteOffset = argStruct->saveFlagOffset / 8;
   uint8_t bitOffset = argStruct->saveFlagOffset % 8;
   uint8_t currentByteValue = currentSave->saveFlags[byteOffset];
   bool bitValue = (currentByteValue >> bitOffset) & 0x01u;

   if (bitValue == (bool)argStruct->expectedBoolValue)
   {
      scriptControl->jumpScriptId = argStruct->successScriptId;
   }
}

void MageScriptActions::action_check_if_entity_is_in_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t geometryId;
      uint8_t entityId;
      uint8_t expectedBoolValue;
      uint8_t paddingG;
   } ActionCheckifEntityIsInGeometry;
   auto argStruct = (ActionCheckifEntityIsInGeometry*)args;
   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto geometry = ROM->Get<MageGeometry>(argStruct->geometryId);

      bool colliding = geometry->isPointInGeometry(entity->getRenderableData()->center);
      if (colliding == (bool)argStruct->expectedBoolValue)
      {
         scriptControl->jumpScriptId = argStruct->successScriptId;
      }
   }
}

void MageScriptActions::action_check_for_button_press(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t buttonId; //KEYBOARD_KEY enum value
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCheckForButtonPress;
   auto argStruct = (ActionCheckForButtonPress*)args;

   auto activeButton = inputHandler->GetButtonActivatedState();
   if (activeButton.IsPressed((KeyPress)argStruct->buttonId))
   {
      scriptControl->jumpScriptId = argStruct->successScriptId;
   }
}

void MageScriptActions::action_check_for_button_state(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t buttonId; //KEYBOARD_KEY enum value
      uint8_t expectedBoolValue;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCheckForButtonState;
   auto argStruct = (ActionCheckForButtonState*)args;
   auto button = inputHandler->GetButtonState();
   if ((bool)(argStruct->expectedBoolValue) == button.IsPressed((KeyPress)argStruct->buttonId))
   {
      scriptControl->jumpScriptId = argStruct->successScriptId;
   }
}

void MageScriptActions::action_check_warp_state(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t stringId;
      uint8_t expectedBoolValue;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCheckWarpState;
   auto argStruct = (ActionCheckWarpState*)args;
   auto currentSave = ROM->GetCurrentSave();

   bool doesWarpStateMatch = currentSave->warpState == argStruct->stringId;
   if (doesWarpStateMatch == (bool)(argStruct->expectedBoolValue))
   {
      scriptControl->jumpScriptId = argStruct->successScriptId;
   }
}

void MageScriptActions::action_run_script(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t scriptId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionRunScript;
   auto argStruct = (ActionRunScript*)args;

   scriptControl->jumpScriptId = argStruct->scriptId;
}

void MageScriptActions::action_blocking_delay(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionBlockingDelay;
   auto argStruct = (ActionBlockingDelay*)args;
   argStruct->duration = argStruct->duration;

   //If there's already a total number of loops to next action set, a delay is currently in progress:
   if (resumeStateStruct->totalLoopsToNextAction != 0)
   {
      //decrement the number of loops to the end of the delay:
      resumeStateStruct->loopsToNextAction--;
      //if we've reached the end:
      if (resumeStateStruct->loopsToNextAction <= 0)
      {
         //reset the variables and return, the delay is complete.
         resumeStateStruct->totalLoopsToNextAction = 0;
         resumeStateStruct->loopsToNextAction = 0;
         return;
      }
   }
   //a delay is not active, so we should start one:
   else
   {
      //always a single loop for a blocking delay. On the next action call, (after rendering all current changes) it will continue.
      uint16_t totalDelayLoops = 1;
      //also set the blocking delay time to the larger of the current blockingDelayTime, or argStruct->duration:
      scriptControl->blockingDelayTime = (scriptControl->blockingDelayTime < argStruct->duration)
         ? argStruct->duration
         : scriptControl->blockingDelayTime;
      //now set the resumeStateStruct variables:
      resumeStateStruct->totalLoopsToNextAction = totalDelayLoops;
      resumeStateStruct->loopsToNextAction = totalDelayLoops;
   }
}

void MageScriptActions::action_non_blocking_delay(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionNonBlockingDelay;
   auto argStruct = (ActionNonBlockingDelay*)args;
   argStruct->duration = argStruct->duration;

   manageProgressOfAction(
      resumeStateStruct,
      argStruct->duration);
}

void MageScriptActions::action_set_entity_name(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t stringId;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityName;
   auto argStruct = (ActionSetEntityName*)args;

   //get the string from the stringId:
   std::string romString = stringLoader->getString(argStruct->stringId, scriptControl->currentEntityId);
   //Get the entity:
   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      //simple loop to set the name:
      for (int i = 0; i < MAGE_ENTITY_NAME_LENGTH; i++)
      {
         entity->name[i] = romString[i];
         if (romString[i] == 00)
         {
            // if we have hit one null, fill in the remainder with null too
            for (int j = i + 1; j < MAGE_ENTITY_NAME_LENGTH; j++)
            {
               entity->name[j] = 00;
            }
            break;
         }
      }
   }
}

void MageScriptActions::action_set_entity_x(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t newValue;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityX;
   auto argStruct = (ActionSetEntityX*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->location.x = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_y(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t newValue;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityY;
   auto argStruct = (ActionSetEntityY*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->location.y = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_interact_script(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t scriptId;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityInteractScript;
   auto argStruct = (ActionSetEntityInteractScript*)args;
   auto entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   scriptControl->setEntityScript(argStruct->scriptId, entityIndex, ON_INTERACT);
}

void MageScriptActions::action_set_entity_tick_script(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t scriptId;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityTickScript;
   auto argStruct = (ActionSetEntityTickScript*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   scriptControl->setEntityScript(argStruct->scriptId, entityIndex, ON_TICK);
}

void MageScriptActions::action_set_entity_type(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t entityTypeId;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityType;
   auto argStruct = (ActionSetEntityType*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->primaryId = argStruct->entityTypeId;
      entity->primaryIdType = ENTITY_TYPE;
   }
}

void MageScriptActions::action_set_entity_primary_id(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t newValue;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityPrimaryId;
   auto argStruct = (ActionSetEntityPrimaryId*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->primaryId = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_secondary_id(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t newValue;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntitySecondaryId;
   auto argStruct = (ActionSetEntitySecondaryId*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->secondaryId = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_primary_id_type(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      MageEntityPrimaryIdType newValue;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityPrimaryIdType;
   auto argStruct = (ActionSetEntityPrimaryIdType*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->primaryIdType = (MageEntityPrimaryIdType)(argStruct->newValue % NUM_PRIMARY_ID_TYPES);
   }
}

void MageScriptActions::action_set_entity_current_animation(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t newValue;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityCurrentAnimation;
   auto argStruct = (ActionSetEntityCurrentAnimation*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto renderable = entity->getRenderableData();
      entity->currentAnimation = argStruct->newValue;
      entity->currentFrameIndex = 0;
      renderable->currentFrameTicks = 0;
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_set_entity_current_frame(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t newValue;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityCurrentFrame;
   auto argStruct = (ActionSetEntityCurrentFrame*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto renderable = entity->getRenderableData()->currentFrameTicks = 0;
      entity->currentFrameIndex = argStruct->newValue;
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_set_entity_direction(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      MageEntityAnimationDirection direction;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityDirection;
   auto argStruct = (ActionSetEntityDirection*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->renderFlags.updateDirectionAndPreserveFlags(argStruct->direction);
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_set_entity_direction_relative(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      int8_t relativeDirection;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityDirectionRelative;
   auto argStruct = (ActionSetEntityDirectionRelative*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->renderFlags.updateDirectionAndPreserveFlags((MageEntityAnimationDirection)((entity->renderFlags + argStruct->relativeDirection + NUM_DIRECTIONS) % NUM_DIRECTIONS));
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_set_entity_direction_target_entity(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t targetEntityId;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityDirectionTargetEntity;
   auto argStruct = (ActionSetEntityDirectionTargetEntity*)args;

   int16_t targetEntityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->targetEntityId, scriptControl->currentEntityId);
   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (
      entityIndex != NO_PLAYER
      && targetEntityIndex != NO_PLAYER
      )
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      MageEntity* targetEntity = mapControl->getEntityByMapLocalId(targetEntityIndex);
      auto renderable = entity->getRenderableData();
      auto targetRenderable = targetEntity->getRenderableData();
      entity->renderFlags.updateDirectionAndPreserveFlags(renderable->center.getRelativeDirection(targetRenderable->center));
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_set_entity_direction_target_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t targetGeometryId;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityDirectionTargetGeometry;
   auto argStruct = (ActionSetEntityDirectionTargetGeometry*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto geometry = ROM->Get<MageGeometry>(argStruct->targetGeometryId);
      auto relativeDirection = entity->getRenderableData()->center.getRelativeDirection(geometry->GetPoint(0));
      entity->renderFlags.updateDirectionAndPreserveFlags(relativeDirection);
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_set_entity_glitched(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t entityId;
      uint8_t isGlitched;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityGlitched;
   auto argStruct = (ActionSetEntityGlitched*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->renderFlags = (MageEntityAnimationDirection)(
         (entity->renderFlags & RENDER_FLAGS_IS_GLITCHED_MASK)
         | (argStruct->isGlitched * RENDER_FLAGS_IS_GLITCHED));
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_set_entity_hackable_state_a(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t newValue;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityHackableStateA;
   auto argStruct = (ActionSetEntityHackableStateA*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->hackableStateA = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_hackable_state_b(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t newValue;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityHackableStateB;
   auto argStruct = (ActionSetEntityHackableStateB*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->hackableStateB = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_hackable_state_c(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t newValue;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityHackableStateC;
   auto argStruct = (ActionSetEntityHackableStateC*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->hackableStateC = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_hackable_state_d(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t newValue;
      uint8_t entityId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityHackableStateD;
   auto argStruct = (ActionSetEntityHackableStateD*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      entity->hackableStateD = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_hackable_state_a_u2(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t newValue;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityHackableStateAU2;
   auto argStruct = (ActionSetEntityHackableStateAU2*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      *(uint16_t*)((uint8_t*)&entity->hackableStateA) = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_hackable_state_c_u2(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t newValue;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityHackableStateCU2;
   auto argStruct = (ActionSetEntityHackableStateCU2*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      *(uint16_t*)((uint8_t*)&entity->hackableStateC) = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_hackable_state_a_u4(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t newValue;
      uint8_t entityId;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityHackableStateAU4;
   auto argStruct = (ActionSetEntityHackableStateAU4*)args;
   argStruct->newValue = argStruct->newValue;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      *(uint32_t*)((uint8_t*)&entity->hackableStateA) = argStruct->newValue;
   }
}

void MageScriptActions::action_set_entity_path(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t newValue;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityPath;
   auto argStruct = (ActionSetEntityPath*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      *(uint16_t*)((uint8_t*)&entity->hackableStateA) = argStruct->newValue;
   }
}

void MageScriptActions::action_set_save_flag(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t saveFlagOffset;
      uint8_t newBoolValue;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetSaveFlag;
   auto argStruct = (ActionSetSaveFlag*)args;
   auto currentSave = ROM->GetCurrentSave();
   uint16_t byteOffset = argStruct->saveFlagOffset / 8;
   uint8_t bitOffset = argStruct->saveFlagOffset % 8;
   uint8_t currentByteValue = currentSave->saveFlags[byteOffset];

   if (argStruct->newBoolValue)
   {
      currentByteValue |= 0x01u << bitOffset;
   }
   else
   {
      // tilde operator inverts all the bits on a byte; Bitwise NOT
      currentByteValue &= ~(0x01u << bitOffset);
   }
   currentSave->saveFlags[byteOffset] = currentByteValue;
}

void MageScriptActions::action_set_player_control(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t playerHasControl;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetPlayerControl;
   auto argStruct = (ActionSetPlayerControl*)args;
   //TODO FIXME: playerHasControl = argStruct->playerHasControl;
}

void MageScriptActions::action_set_map_tick_script(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t scriptId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetMapTickScript;
   auto argStruct = (ActionSetMapTickScript*)args;

   scriptControl->setEntityScript(argStruct->scriptId, MAGE_MAP_ENTITY, ON_TICK);
}

void MageScriptActions::action_set_hex_cursor_location(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t byteAddress;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetHexCursorLocation;
   auto argStruct = (ActionSetHexCursorLocation*)args;

   hexEditor->setHexCursorLocation(argStruct->byteAddress);
}

void MageScriptActions::action_set_warp_state(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t stringId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetWarpState;
   auto argStruct = (ActionSetWarpState*)args;
   auto currentSave = ROM->GetCurrentSave();

   currentSave->warpState = argStruct->stringId;
}

void MageScriptActions::action_set_hex_editor_state(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t state;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetHexEditorState;
   auto argStruct = (ActionSetHexEditorState*)args;

   if (hexEditor->isHexEditorOn() != (bool)argStruct->state)
   {
      hexEditor->toggleHexEditor();
   }
}

void MageScriptActions::action_set_hex_editor_dialog_mode(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t state;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetHexEditorDialogMode;
   auto argStruct = (ActionSetHexEditorDialogMode*)args;

   if (hexEditor->getHexDialogState() != (bool)argStruct->state)
   {
      hexEditor->toggleHexDialog();
   }
}

void MageScriptActions::action_set_hex_editor_control(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t playerHasHexEditorControl;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetHexEditorControl;
   auto argStruct = (ActionSetHexEditorControl*)args;
   hexEditor->SetPlayerHasClipboardControl(argStruct->playerHasHexEditorControl);
}

void MageScriptActions::action_set_hex_editor_control_clipboard(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t playerHasClipboardControl;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetHexEditorControlClipboard;
   auto argStruct = (ActionSetHexEditorControlClipboard*)args;
   hexEditor->SetPlayerHasClipboardControl(argStruct->playerHasClipboardControl);
}

void MageScriptActions::action_load_map(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t mapId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionLoadMap;
   auto argStruct = (ActionLoadMap*)args;
   scriptControl->mapLoadId = argStruct->mapId;
}

void MageScriptActions::action_show_dialog(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t dialogId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionShowDialog;
   auto argStruct = (ActionShowDialog*)args;

   if (resumeStateStruct->totalLoopsToNextAction == 0)
   {
      //debug_print("Opening dialog %d\n", argStruct->dialogId);
      dialogControl->load(argStruct->dialogId, scriptControl->currentEntityId);
      resumeStateStruct->totalLoopsToNextAction = 1;
   }
   else if (!dialogControl->isOpen())
   {
      // will be 0 any time there is no response; no jump
      resumeStateStruct->totalLoopsToNextAction = 0;
   }
}

void MageScriptActions::action_play_entity_animation(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t entityId;
      uint8_t animationId;
      uint8_t playCount;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionPlayEntityAnimation;
   auto argStruct = (ActionPlayEntityAnimation*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto renderable = entity->getRenderableData();
      if (resumeStateStruct->totalLoopsToNextAction == 0)
      {
         resumeStateStruct->totalLoopsToNextAction = argStruct->playCount;
         resumeStateStruct->loopsToNextAction = argStruct->playCount;
         entity->currentAnimation = argStruct->animationId;
         entity->currentFrameIndex = 0;
         renderable->currentFrameTicks = 0;
         entity->updateRenderableData();
      }
      else if (entity->currentFrameIndex == 0 && resumeStateStruct->currentSegmentIndex == renderable->frameCount - 1)
      {
         // we just reset to 0
         // the previously rendered frame was the last in the animation
         resumeStateStruct->loopsToNextAction--;
         if (resumeStateStruct->loopsToNextAction == 0)
         {
            resumeStateStruct->totalLoopsToNextAction = 0;
            entity->currentAnimation = MAGE_IDLE_ANIMATION_INDEX;
            entity->currentFrameIndex = 0;
            renderable->currentFrameTicks = 0;
            entity->updateRenderableData();
         }
      }
      // this is just a quick and dirty place to hold on to
      // the last frame that was rendered for this entity
      resumeStateStruct->currentSegmentIndex = entity->currentFrameIndex;
   }
}

void MageScriptActions::action_teleport_entity_to_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t geometryId;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionTeleportEntityToGeometry;
   auto argStruct = (ActionTeleportEntityToGeometry*)args;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto renderable = entity->getRenderableData();
      auto geometry = ROM->Get<MageGeometry>(argStruct->geometryId);

      auto offsetPoint = geometry->GetPoint(0) - entity->getRenderableData()->center - entity->location;
      entity->SetLocation(offsetPoint);
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_walk_entity_to_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint16_t geometryId;
      uint8_t entityId;
   } ActionWalkEntityToGeometry;
   auto argStruct = (ActionWalkEntityToGeometry*)args;
   argStruct->duration = argStruct->duration;
   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto renderable = entity->getRenderableData();
      auto geometry = ROM->Get<MageGeometry>(argStruct->geometryId);

      if (resumeStateStruct->totalLoopsToNextAction == 0)
      {
         //this is the points we're interpolating between
         resumeStateStruct->pointA = entity->location;
         resumeStateStruct->pointB = geometry->GetPoint(0) - entity->getRenderableData()->center - entity->location;
         entity->renderFlags.updateDirectionAndPreserveFlags(resumeStateStruct->pointA.getRelativeDirection(resumeStateStruct->pointB));
         entity->currentAnimation = MAGE_WALK_ANIMATION_INDEX;
         entity->currentFrameIndex = 0;
         renderable->currentFrameTicks = 0;
      }
      float progress = manageProgressOfAction(resumeStateStruct, argStruct->duration);
      Point betweenPoint = resumeStateStruct->pointA.lerp(resumeStateStruct->pointB, progress);
      entity->SetLocation(betweenPoint);
      if (progress >= 1.0f)
      {
         entity->currentAnimation = MAGE_IDLE_ANIMATION_INDEX;
         entity->currentFrameIndex = 0;
         renderable->currentFrameTicks = 0;
         resumeStateStruct->totalLoopsToNextAction = 0;
      }
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_walk_entity_along_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint16_t geometryId;
      uint8_t entityId;
   } ActionWalkEntityAlongGeometry;
   auto argStruct = (ActionWalkEntityAlongGeometry*)args;
   argStruct->duration = argStruct->duration;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto geometry = ROM->Get<MageGeometry>(argStruct->geometryId);

      // handle single point geometries
      if (geometry->GetPointCount() == 1)
      {
         resumeStateStruct->totalLoopsToNextAction = 1;
         auto offsetPoint = geometry->GetPoint(0) - entity->getRenderableData()->center - entity->location;
         entity->SetLocation(offsetPoint);
         entity->updateRenderableData();
         return;
      }
      // and for everything else...
      if (resumeStateStruct->totalLoopsToNextAction == 0)
      {
         uint16_t totalDelayLoops = argStruct->duration / MAGE_MIN_MILLIS_BETWEEN_FRAMES;
         //now set the resumeStateStruct variables:
         resumeStateStruct->totalLoopsToNextAction = totalDelayLoops;
         resumeStateStruct->loopsToNextAction = totalDelayLoops;
         resumeStateStruct->length = geometry->GetPathLength();
         initializeEntityGeometryPath(resumeStateStruct, entity->getRenderableData(), entity, geometry);
         entity->currentAnimation = MAGE_WALK_ANIMATION_INDEX;
         entity->currentFrameIndex = 0;
         entity->getRenderableData()->currentFrameTicks = 0;
      }
      resumeStateStruct->loopsToNextAction--;

      uint16_t sanitizedCurrentSegmentIndex = geometry->GetLoopableGeometrySegmentIndex(resumeStateStruct->currentSegmentIndex);
      float totalProgress = getProgressOfAction(resumeStateStruct);
      float currentProgressLength = resumeStateStruct->length * totalProgress;
      float currentSegmentLength = geometry->GetSegmentLength(sanitizedCurrentSegmentIndex);
      float lengthAtEndOfCurrentSegment = resumeStateStruct->lengthOfPreviousSegments + currentSegmentLength;
      float progressBetweenPoints = (currentProgressLength - resumeStateStruct->lengthOfPreviousSegments)
         / (lengthAtEndOfCurrentSegment - resumeStateStruct->lengthOfPreviousSegments);

      if (progressBetweenPoints > 1)
      {
         resumeStateStruct->lengthOfPreviousSegments += currentSegmentLength;
         resumeStateStruct->currentSegmentIndex++;
         sanitizedCurrentSegmentIndex = geometry->GetLoopableGeometrySegmentIndex(resumeStateStruct->currentSegmentIndex);
         currentSegmentLength = geometry->GetSegmentLength(sanitizedCurrentSegmentIndex);
         lengthAtEndOfCurrentSegment = resumeStateStruct->lengthOfPreviousSegments + currentSegmentLength;
         progressBetweenPoints = (currentProgressLength - resumeStateStruct->lengthOfPreviousSegments)
            / (lengthAtEndOfCurrentSegment - resumeStateStruct->lengthOfPreviousSegments);

         setResumeStatePointsAndEntityDirection(
            resumeStateStruct,
            entity,
            geometry);
      }

      Point betweenPoint = resumeStateStruct->pointA.lerp(resumeStateStruct->pointB, progressBetweenPoints);
      entity->SetLocation(betweenPoint);
      if (resumeStateStruct->loopsToNextAction == 0)
      {
         resumeStateStruct->totalLoopsToNextAction = 0;
         entity->currentAnimation = MAGE_IDLE_ANIMATION_INDEX;
         entity->currentFrameIndex = 0;
         entity->getRenderableData()->currentFrameTicks = 0;
      }
      entity->updateRenderableData();
   }
}
void MageScriptActions::action_loop_entity_along_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint16_t geometryId;
      uint8_t entityId;
   } ActionLoopEntityAlongGeometry;
   auto argStruct = (ActionLoopEntityAlongGeometry*)args;
   argStruct->duration = argStruct->duration;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto renderable = entity->getRenderableData();
      auto geometry = ROM->Get<MageGeometry>(argStruct->geometryId);

      // handle single point geometries
      if (geometry->GetPointCount() == 1)
      {
         resumeStateStruct->totalLoopsToNextAction = 1;
         entity->SetLocation(geometry->GetPoint(0) - entity->getRenderableData()->center - entity->location);
         entity->updateRenderableData();
         return;
      }

      // and for everything else...
      if (resumeStateStruct->totalLoopsToNextAction == 0)
      {
         uint16_t totalDelayLoops = argStruct->duration / MAGE_MIN_MILLIS_BETWEEN_FRAMES;
         //now set the resumeStateStruct variables:
         resumeStateStruct->totalLoopsToNextAction = totalDelayLoops;
         resumeStateStruct->loopsToNextAction = totalDelayLoops;
         resumeStateStruct->length = (geometry->GetTypeId() == MageGeometryType::Polyline)
            ? geometry->GetPathLength() * 2
            : geometry->GetPathLength();
         initializeEntityGeometryPath(resumeStateStruct, renderable, entity, geometry);
         entity->currentAnimation = MAGE_WALK_ANIMATION_INDEX;
         entity->currentFrameIndex = 0;
         renderable->currentFrameTicks = 0;
      }

      if (resumeStateStruct->loopsToNextAction == 0)
      {
         resumeStateStruct->loopsToNextAction = resumeStateStruct->totalLoopsToNextAction;
         initializeEntityGeometryPath(resumeStateStruct, renderable, entity, geometry);
      }
      resumeStateStruct->loopsToNextAction--;
      uint16_t sanitizedCurrentSegmentIndex = geometry->GetLoopableGeometrySegmentIndex(resumeStateStruct->currentSegmentIndex);
      float totalProgress = getProgressOfAction(resumeStateStruct);
      float currentProgressLength = resumeStateStruct->length * totalProgress;
      float currentSegmentLength = geometry->GetSegmentLength(sanitizedCurrentSegmentIndex);
      float lengthAtEndOfCurrentSegment = resumeStateStruct->lengthOfPreviousSegments + currentSegmentLength;
      float progressBetweenPoints = (currentProgressLength - resumeStateStruct->lengthOfPreviousSegments)
         / (lengthAtEndOfCurrentSegment - resumeStateStruct->lengthOfPreviousSegments);

      if (progressBetweenPoints > 1.0f)
      {
         resumeStateStruct->lengthOfPreviousSegments += currentSegmentLength;
         resumeStateStruct->currentSegmentIndex++;

         sanitizedCurrentSegmentIndex = geometry->GetLoopableGeometrySegmentIndex(resumeStateStruct->currentSegmentIndex);

         currentSegmentLength = geometry->GetSegmentLength(sanitizedCurrentSegmentIndex);
         lengthAtEndOfCurrentSegment = resumeStateStruct->lengthOfPreviousSegments + currentSegmentLength;
         progressBetweenPoints = (currentProgressLength - resumeStateStruct->lengthOfPreviousSegments)
            / (lengthAtEndOfCurrentSegment - resumeStateStruct->lengthOfPreviousSegments);

         setResumeStatePointsAndEntityDirection(resumeStateStruct, entity, geometry);
      }
      Point betweenPoint = resumeStateStruct->pointA.lerp(resumeStateStruct->pointB, progressBetweenPoints);
      entity->SetLocation(betweenPoint);
      entity->updateRenderableData();
   }
}

void MageScriptActions::action_set_camera_to_follow_entity(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t entityId;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetCameraToFollowEntity;
   auto argStruct = (ActionSetCameraToFollowEntity*)args;
   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   camera.followEntityId = entityIndex;
}

void MageScriptActions::action_teleport_camera_to_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t geometryId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionTeleportCameraToGeometry;
   auto argStruct = (ActionTeleportCameraToGeometry*)args;

   const auto entity = mapControl->getEntityByMapLocalId(scriptControl->currentEntityId);
   auto geometry = ROM->Get<MageGeometry>(argStruct->geometryId);

   camera.followEntityId = NO_PLAYER;
   const auto midScreen = Point{ HALF_WIDTH, HALF_HEIGHT };
   camera.position = geometry->GetPoint(0) - midScreen;
}

void MageScriptActions::action_pan_camera_to_entity(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint8_t entityId;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionPanCameraToEntity;
   auto argStruct = (ActionPanCameraToEntity*)args;
   argStruct->duration = argStruct->duration;

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto renderable = entity->getRenderableData();

      if (resumeStateStruct->totalLoopsToNextAction == 0)
      {
         camera.followEntityId = NO_PLAYER;
         //this is the points we're interpolating between
         resumeStateStruct->pointA = Point{ camera.position.x, camera.position.y };
      }
      float progress = manageProgressOfAction(resumeStateStruct, argStruct->duration);
      // yes, this is intentional;
      // if the entity is moving, pan will continue to the entity
      resumeStateStruct->pointB = { renderable->center.x - HALF_WIDTH, renderable->center.y - HALF_HEIGHT };
      Point betweenPoint = resumeStateStruct->pointA.lerp(resumeStateStruct->pointB, progress);
      camera.position.x = betweenPoint.x;
      camera.position.y = betweenPoint.y;
      if (progress >= 1.0f)
      {
         // Moved the camera there, may as well follow the entity now.
         camera.followEntityId = entityIndex;
      }
   }
}

void MageScriptActions::action_pan_camera_to_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint16_t geometryId;
      uint8_t paddingG;
   } ActionPanCameraToGeometry;
   auto argStruct = (ActionPanCameraToGeometry*)args;
   argStruct->duration = argStruct->duration;

   const auto entity = mapControl->getEntityByMapLocalId(scriptControl->currentEntityId);
   auto geometry = ROM->Get<MageGeometry>(argStruct->geometryId);


   if (resumeStateStruct->totalLoopsToNextAction == 0)
   {
      camera.followEntityId = NO_PLAYER;
      //this is the points we're interpolating between
      resumeStateStruct->pointA = {
         camera.position.x,
         camera.position.y,
      };
      resumeStateStruct->pointB = {
         geometry->GetPoint(0).x - HALF_WIDTH,
         geometry->GetPoint(0).y - HALF_HEIGHT,
      };
   }
   float progress = manageProgressOfAction(resumeStateStruct, argStruct->duration);

   Point betweenPoint = resumeStateStruct->pointA.lerp(resumeStateStruct->pointB, progress);
   camera.position.x = betweenPoint.x;
   camera.position.y = betweenPoint.y;
}

void MageScriptActions::action_pan_camera_along_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint16_t geometryId;
      uint8_t paddingG;
   } ActionPanCameraAlongGeometry;
   auto argStruct = (ActionPanCameraAlongGeometry*)args;
   argStruct->duration = argStruct->duration;
}

void MageScriptActions::action_loop_camera_along_geometry(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint16_t geometryId;
      uint8_t paddingG;
   } ActionLoopCameraAlongGeometry;
   auto argStruct = (ActionLoopCameraAlongGeometry*)args;
   argStruct->duration = argStruct->duration;
}

void MageScriptActions::action_set_screen_shake(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t duration; //in ms
      uint16_t frequency;
      uint8_t amplitude;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetScreenShake;
   auto argStruct = (ActionSetScreenShake*)args;

   float progress = manageProgressOfAction(
      resumeStateStruct,
      argStruct->duration);

   if (progress < 1.0)
   {
      camera.shaking = true;
      camera.shakeAmplitude = argStruct->amplitude;
      camera.shakePhase = (
         progress /
         (
            (float)argStruct->frequency
            / 1000.0f
            ));
   }
   else
   {
      camera.shaking = false;
      camera.shakeAmplitude = 0;
      camera.shakePhase = 0;
   }
}
void MageScriptActions::action_screen_fade_out(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint16_t color;
      uint8_t paddingG;
   } ActionScreenFadeOut;
   auto argStruct = (ActionScreenFadeOut*)args;
   argStruct->duration = argStruct->duration;
   argStruct->color = SCREEN_ENDIAN_U2_VALUE(argStruct->color);

   float progress = manageProgressOfAction(resumeStateStruct, argStruct->duration);

   frameBuffer->SetFade(argStruct->color, progress);
}

void MageScriptActions::action_screen_fade_in(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint32_t duration; //in ms
      uint16_t color;
      uint8_t paddingG;
   } ActionScreenFadeIn;
   auto argStruct = (ActionScreenFadeIn*)args;
   argStruct->duration = argStruct->duration;
   argStruct->color = SCREEN_ENDIAN_U2_VALUE(argStruct->color);
   float progress = manageProgressOfAction(
      resumeStateStruct,
      argStruct->duration);

   frameBuffer->SetFade(argStruct->color, 1.0f - progress);
}

void MageScriptActions::action_mutate_variable(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t value;
      uint8_t variableId;
      MageMutateOperation operation;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionMutateVariable;
   auto argStruct = (ActionMutateVariable*)args;
   auto currentSave = ROM->GetCurrentSave();
   uint16_t* currentValue = &currentSave->scriptVariables[argStruct->variableId];

   // I wanted to log some stats on how well our random function worked
   // on desktop and hardware after the new random seed changes.
   // Works really well on both. Can use this again if we need.
   //if(argStruct->operation == RNG) {
   //	uint16_t samples = 65000;
   //	uint16_t testVar = 0;
   //	uint16_t range = argStruct->value + 1; // to make verify it only goes 0~(n-1), not 0~n
   //	uint16_t values[range];
   //	for (int i = 0; i < range; ++i) {
   //		values[i] = 0;
   //	}
   //	for (int i = 0; i < samples; ++i) {
   //		mutate(
   //			argStruct->operation,
   //			&testVar,
   //			argStruct->value
   //);
   //		values[testVar] += 1;
   //	}
   //	for (int i = 0; i < range; ++i) {
   //		debug_print(
   //			"%05d: %05d",
   //			i,
   //			values[i]
   //);
   //	}
   //}
   mutate(
      argStruct->operation,
      currentValue,
      argStruct->value);
}

void MageScriptActions::action_mutate_variables(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t variableId;
      uint8_t sourceId;
      MageMutateOperation operation;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionMutateVariables;
   auto argStruct = (ActionMutateVariables*)args;
   auto currentSave = ROM->GetCurrentSave();
   uint16_t* currentValue = &currentSave->scriptVariables[argStruct->variableId];
   uint16_t sourceValue = currentSave->scriptVariables[argStruct->sourceId];

   mutate(argStruct->operation, currentValue, sourceValue);
}

void MageScriptActions::action_copy_variable(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t variableId;
      uint8_t entityId;
      MageEntityFieldOffset field;
      uint8_t inbound;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCopyVariable;
   auto argStruct = (ActionCopyVariable*)args;
   auto currentSave = ROM->GetCurrentSave();
   auto currentValue = &currentSave->scriptVariables[argStruct->variableId];

   int16_t entityIndex = scriptControl->GetUsefulEntityIndexFromActionEntityId(argStruct->entityId, scriptControl->currentEntityId);
   if (entityIndex != NO_PLAYER)
   {
      const auto entity = mapControl->getEntityByMapLocalId(entityIndex);
      auto variableValue = &currentSave->scriptVariables[argStruct->variableId];
      uint8_t* fieldValue = ((uint8_t*)entity) + (uint8_t)argStruct->field;


      switch (argStruct->field)
      {
      case MageEntityFieldOffset::x:
      case MageEntityFieldOffset::y:
      case MageEntityFieldOffset::onInteractScriptId:
      case MageEntityFieldOffset::onTickScriptId:
      case MageEntityFieldOffset::primaryId:
      case MageEntityFieldOffset::secondaryId:
         if (argStruct->inbound)
         {
            *variableValue = (uint16_t)*fieldValue;
         }
         else
         {
            uint16_t* destination = (uint16_t*)fieldValue;
            *destination = *variableValue;
         }
         break;
      case MageEntityFieldOffset::primaryIdType:
      case MageEntityFieldOffset::currentAnimation:
      case MageEntityFieldOffset::currentFrame:
      case MageEntityFieldOffset::direction:
      case MageEntityFieldOffset::hackableStateA:
      case MageEntityFieldOffset::hackableStateB:
      case MageEntityFieldOffset::hackableStateC:
      case MageEntityFieldOffset::hackableStateD:
         if (argStruct->inbound)
         {
            *variableValue = (uint8_t)*fieldValue;
         }
         else
         {
            *fieldValue = *variableValue % 256;
         }
         break;
      default: debug_print(
         "copyVariable received an invalid field: %d",
         argStruct->field);
      }
   }
}

void MageScriptActions::action_check_variable(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t value;
      uint8_t variableId;
      MageCheckComparison comparison;
      uint8_t expectedBool;
   } ActionCheckVariable;
   auto argStruct = (ActionCheckVariable*)args;
   auto currentSave = ROM->GetCurrentSave();
   uint16_t variableValue = currentSave->scriptVariables[argStruct->variableId];
   bool comparison = compare(argStruct->comparison, variableValue, argStruct->value);
   if (comparison == (bool)argStruct->expectedBool)
   {
      scriptControl->jumpScriptId = argStruct->successScriptId;
   }
}

void MageScriptActions::action_check_variables(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t variableId;
      uint8_t sourceId;
      MageCheckComparison comparison;
      uint8_t expectedBool;
      uint8_t paddingG;
   } ActionCheckVariables;
   auto argStruct = (ActionCheckVariables*)args;

   auto currentSave = ROM->GetCurrentSave();
   uint16_t variableValue = currentSave->scriptVariables[argStruct->variableId];
   uint16_t sourceValue = currentSave->scriptVariables[argStruct->sourceId];
   bool comparison = compare(
      argStruct->comparison,
      variableValue,
      sourceValue);
   if (comparison == (bool)argStruct->expectedBool)
   {
      scriptControl->jumpScriptId = argStruct->successScriptId;
   }
}

void MageScriptActions::action_slot_save(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t paddingA;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSlotSave;
   auto argStruct = (ActionSlotSave*)args;
   auto currentSave = ROM->GetCurrentSave();
   // In the case that someone hacks an on_tick script to save, we don't want it
   // just burning through 8 ROM writes per second, our chip would be fried in a
   // matter on minutes. So how do we counter? Throw up a "Save Completed" dialog
   // that FORCES user interaction to advance from. A player encountering like 10
   // of these dialogs right in a row should hopefully get the hint and reset
   // their board to get out of that dialog lock. Better to protect the player
   // with an annoying confirm dialog than allowing them to quietly burn through
   // the ROM chip's 10000 write cycles.
   if (resumeStateStruct->totalLoopsToNextAction == 0)
   {
      // do rom writes
      auto playerName = mapControl->getPlayerEntity()->name;
      auto currentSave = ROM->GetCurrentSave();
      memcpy((void*)currentSave->name, playerName.c_str(), MAGE_ENTITY_NAME_LENGTH < playerName.length() ? MAGE_ENTITY_NAME_LENGTH : playerName.length());
      //TODO FIXME: 
      // ROM->WriteSaveSlot(currentSaveIndex, sizeof(MageSaveGame), currentSave.get());
      // readSaveFromRomIntoRam(currentSaveIndex);

      //debug_print("Opening dialog %d\n", argStruct->dialogId);
      dialogControl->StartModalDialog("Save complete.");
      resumeStateStruct->totalLoopsToNextAction = 1;
   }
   else if (!dialogControl->isOpen())
   {
      resumeStateStruct->totalLoopsToNextAction = 0;
   }
}

void MageScriptActions::action_slot_load(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t slotIndex;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSlotLoad;
   auto argStruct = (ActionSlotLoad*)args;
   auto currentSave = ROM->GetCurrentSave();
   //delaying until next tick allows for displaying of an error message on read before resuming
   if (resumeStateStruct->totalLoopsToNextAction == 0)
   {
      //TODO FIX: SetSaveGameSlotIndex(argStruct->slotIndex);

      readSaveFromRomIntoRam(argStruct->slotIndex);
      mapControl->Load(currentSave->currentMapId);
      resumeStateStruct->totalLoopsToNextAction = 1;
   }
   else if (!dialogControl->isOpen())
   {
      resumeStateStruct->totalLoopsToNextAction = 0;
   }
}

void MageScriptActions::action_slot_erase(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t slotIndex;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSlotErase;
   auto argStruct = (ActionSlotErase*)args;
   // In the case that someone hacks an on_tick script to save, we don't want it
   // just burning through 8 ROM writes per second, our chip would be fried in a
   // matter on minutes. So how do we counter? Throw up a "Save Completed" dialog
   // that FORCES user interaction to advance from. A player encountering like 10
   // of these dialogs right in a row should hopefully get the hint and reset
   // their board to get out of that dialog lock. Better to protect the player
   // with an annoying confirm dialog than allowing them to quietly burn through
   // the ROM chip's 10000 write cycles.
   if (resumeStateStruct->totalLoopsToNextAction == 0)
   {
      // TODO FIXME:
      // setCurrentSaveToFreshState();

      // do rom writes
      //copyNameToAndFromPlayerAndSave(true);
      auto playerName = mapControl->getPlayerEntity()->name;
      //memcpy(currentSave->name, playerName.c_str(), MAGE_ENTITY_NAME_LENGTH < playerName.length() ? MAGE_ENTITY_NAME_LENGTH : playerName.length());
      //ROM->WriteSaveSlot(currentSaveIndex, sizeof(MageSaveGame), &currentSave);
      readSaveFromRomIntoRam(argStruct->slotIndex);

      //debug_print("Opening dialog %d\n", argStruct->dialogId);
      dialogControl->StartModalDialog("Save erased.");
      resumeStateStruct->totalLoopsToNextAction = 1;
   }
   else if (!dialogControl->isOpen())
   {
      resumeStateStruct->totalLoopsToNextAction = 0;
   }
}

void MageScriptActions::action_set_connect_serial_dialog(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t serialDialogId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetConnectSerialDialog;
   ActionSetConnectSerialDialog* argStruct = (ActionSetConnectSerialDialog*)args;
   ROM_ENDIAN_U2_BUFFER(&argStruct->serialDialogId, 1);
   commandControl->connectSerialDialogId = argStruct->serialDialogId;
}

void MageScriptActions::action_show_serial_dialog(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t serialDialogId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionShowSerialDialog;
   auto argStruct = (ActionShowSerialDialog*)args;
   ROM_ENDIAN_U2_BUFFER(&argStruct->serialDialogId, 1);
   if (resumeStateStruct->totalLoopsToNextAction == 0)
   {
      commandControl->showSerialDialog(argStruct->serialDialogId);
      if (commandControl->isInputTrapped)
      {
         resumeStateStruct->totalLoopsToNextAction = 1;
      }
   }
   else if (!commandControl->isInputTrapped)
   {
      resumeStateStruct->totalLoopsToNextAction = 0;
   }
}

void MageScriptActions::action_inventory_get(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t itemId;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionInventoryGet;
   auto argStruct = (ActionInventoryGet*)args;
   // TODO: implement this
}

void MageScriptActions::action_inventory_drop(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t itemId;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionInventoryDrop;
   auto argStruct = (ActionInventoryDrop*)args;
   // TODO: implement this
}

void MageScriptActions::action_check_inventory(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t itemId;
      uint8_t expectedBool;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCheckInventory;
   auto argStruct = (ActionCheckInventory*)args;
   ROM_ENDIAN_U2_BUFFER(&argStruct->successScriptId, 1);
   // TODO: implement this
}

void MageScriptActions::action_set_map_look_script(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t scriptId;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetMapLookScript;
   auto argStruct = (ActionSetMapLookScript*)args;
   ROM_ENDIAN_U2_BUFFER(&argStruct->scriptId, 1);
   // TODO: implement this
}

void MageScriptActions::action_set_entity_look_script(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t scriptId;
      uint8_t entityId;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetEntityLookScript;
   auto argStruct = (ActionSetEntityLookScript*)args;
   ROM_ENDIAN_U2_BUFFER(&argStruct->scriptId, 1);
   // TODO: implement this
}

void MageScriptActions::action_set_teleport_enabled(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t value;
      uint8_t paddingB;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetTeleportEnabled;
   auto argStruct = (ActionSetTeleportEnabled*)args;
   // TODO: implement this
}

void MageScriptActions::action_check_map(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint16_t mapId;
      uint8_t expectedBool;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCheckMap;
   auto argStruct = (ActionCheckMap*)args;
   ROM_ENDIAN_U2_BUFFER(&argStruct->successScriptId, 1);
   ROM_ENDIAN_U2_BUFFER(&argStruct->mapId, 1);
   // TODO: implement this
}

void MageScriptActions::action_set_ble_flag(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint8_t bleFlagOffset;
      uint8_t newBoolValue;
      uint8_t paddingC;
      uint8_t paddingD;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionSetBleFlag;
   auto argStruct = (ActionSetBleFlag*)args;
   // TODO: implement this
}

void MageScriptActions::action_check_ble_flag(uint8_t* args, MageScriptState* resumeStateStruct)
{
   typedef struct
   {
      uint16_t successScriptId;
      uint8_t bleFlagOffset;
      uint8_t expectedBoolValue;
      uint8_t paddingE;
      uint8_t paddingF;
      uint8_t paddingG;
   } ActionCheckBleFlag;
   auto argStruct = (ActionCheckBleFlag*)args;
   ROM_ENDIAN_U2_BUFFER(&argStruct->successScriptId, 1);
   // TODO: implement this
}

float MageScriptActions::getProgressOfAction(
   const MageScriptState* resumeStateStruct
)
{
   return 1.0f - (
      (float)resumeStateStruct->loopsToNextAction
      / (float)resumeStateStruct->totalLoopsToNextAction);
}

float MageScriptActions::manageProgressOfAction(
   MageScriptState* resumeStateStruct,
   uint32_t duration
)
{
   resumeStateStruct->loopsToNextAction--;
   if (resumeStateStruct->totalLoopsToNextAction == 0)
   {
      uint16_t totalDelayLoops = duration / MAGE_MIN_MILLIS_BETWEEN_FRAMES;
      resumeStateStruct->totalLoopsToNextAction = totalDelayLoops;
      resumeStateStruct->loopsToNextAction = totalDelayLoops;
   }
   float result = 1.0f - (
      (float)resumeStateStruct->loopsToNextAction
      / (float)resumeStateStruct->totalLoopsToNextAction);
   if (result >= 1.0f)
   {
      resumeStateStruct->totalLoopsToNextAction = 0;
      resumeStateStruct->loopsToNextAction = 0;
   }
   return result;
}

void MageScriptActions::initializeEntityGeometryPath(
   MageScriptState* resumeStateStruct,
   RenderableData* renderable,
   MageEntity* entity,
   const MageGeometry* geometry
)
{
   resumeStateStruct->lengthOfPreviousSegments = 0;
   resumeStateStruct->currentSegmentIndex = 0;
   setResumeStatePointsAndEntityDirection(
      resumeStateStruct,
      entity,
      geometry);
}

void MageScriptActions::mutate(MageMutateOperation operation, uint16_t* destination, uint16_t value)
{
   //protect against division by 0 errors
   uint16_t safeValue = value == 0 ? 1 : value;
   switch (operation)
   {
   case MageMutateOperation::SET: *destination = value; break;
   case MageMutateOperation::ADD: *destination += value; break;
   case MageMutateOperation::SUB: *destination -= value; break;
   case MageMutateOperation::DIV: *destination /= safeValue; break;
   case MageMutateOperation::MUL: *destination *= value; break;
   case MageMutateOperation::MOD: *destination %= safeValue; break;
   case MageMutateOperation::RNG: *destination = rand() % safeValue; break;
   default: debug_print(
      "mutateVariable received an invalid operation: %d",
      operation);
   }
}

bool MageScriptActions::compare(MageCheckComparison comparison, uint16_t a, uint16_t b)
{
   switch (comparison)
   {
   case LT: return a < b;
   case LTEQ: return a <= b;
   case EQ: return a == b;
   case GTEQ: return a >= b;
   case GT: return a > b;
   default:
      debug_print("checkComparison received an invalid comparison: %d", comparison);
      return false;
   }
}
