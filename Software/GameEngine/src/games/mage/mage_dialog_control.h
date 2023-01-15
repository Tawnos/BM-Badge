#ifndef MAGE_DIALOG_CONTROL_H
#define MAGE_DIALOG_CONTROL_H

#include "mage_defines.h"
#include "mage_entity_type.h"
#include "fonts/Monaco9.h"
#include "engine/EngineInput.h"
#include "engine/EnginePanic.h"
#include <optional>

class MapControl;
class MageScriptControl;
class StringLoader;
class TileManager;

#define DIALOG_SCREEN_NO_PORTRAIT 255

#define DIALOG_TILES_TOP_LEFT 0
#define DIALOG_TILES_TOP_REPEAT 1
#define DIALOG_TILES_TOP_RIGHT 2
#define DIALOG_TILES_LEFT_REPEAT 4
#define DIALOG_TILES_CENTER_REPEAT 5
#define DIALOG_TILES_RIGHT_REPEAT 6
#define DIALOG_TILES_BOTTOM_LEFT 8
#define DIALOG_TILES_BOTTOM_REPEAT 9
#define DIALOG_TILES_BOTTOM_RIGHT 10

#define DIALOG_TILES_SCROLL_END 3
#define DIALOG_TILES_SCROLL_REPEAT 7
#define DIALOG_TILES_SCROLL_POSITION 11

#define DIALOG_TILES_CHECKBOX 12
#define DIALOG_TILES_CHECK 13
#define DIALOG_TILES_HIGHLIGHT 14
#define DIALOG_TILES_ARROW 15

enum class MageDialogScreenAlignment : uint8_t
{
   BOTTOM_LEFT = 0,
   BOTTOM_RIGHT = 1,
   TOP_LEFT = 2,
   TOP_RIGHT = 3
};
const inline uint8_t ALIGNMENT_COUNT = 4;

enum class MageDialogResponseType : uint8_t
{
   NO_RESPONSE = 0,
   SELECT_FROM_SHORT_LIST = 1,
   SELECT_FROM_LONG_LIST = 2,
   ENTER_NUMBER = 3,
   ENTER_ALPHANUMERIC = 4,
};


struct MageDialogScreen
{
   // TODO: portraits, after we have some graphics for them
   uint16_t nameStringIndex;
   uint16_t borderTilesetIndex;
   MageDialogScreenAlignment alignment;
   uint8_t fontIndex;
   uint8_t messageCount;
   MageDialogResponseType responseType;
   uint8_t responseCount;
   uint8_t entityIndex;
   uint8_t portraitIndex;
   uint8_t emoteIndex;
};

/*

struct MageDialogScreen {
      uint16_t  name_index;
      uint16_t  border_tileset_index;
      dialog_screen_alignment_type  alignment;
      uint8_t  font_index;
      uint8_t  message_count;
      dialog_response_type  response_type;
      uint8_t  response_count;
      uint8_t  entity_id;
      uint8_t  portrait_id;
      uint8_t  emote;


      getMessage(uint8_t messageNumber)
      {
         return messages[messageNumber % message_count];
      }
      private:
      //uint16  messages[message_count];
      // dialog_response responses [response_count];

      std::vector<uint16_t> messages;
      std::vector<dialog_response> responses;
};
*/

struct MageDialogResponse
{
   uint16_t stringIndex;
   uint16_t mapLocalScriptIndex;
};

struct MageDialogAlignmentCoords
{
   Rect text;
   Rect label;
   Rect portrait;
};

struct MageDialog
{
   char name[32];
   uint32_t screenCount;
};

class MageDialogControl
{
public:
   MageDialogControl(
      std::shared_ptr<FrameBuffer> frameBuffer, 
      std::shared_ptr<EngineInput> inputHandler,
      std::shared_ptr<TileManager> tileManager, 
      std::shared_ptr<StringLoader> stringLoader, 
      std::shared_ptr<MageScriptControl> scriptControl, 
      std::shared_ptr<MapControl> mapControl) noexcept
   :  frameBuffer(frameBuffer), 
      inputHandler(inputHandler),
      tileManager(tileManager), 
      stringLoader(stringLoader), 
      scriptControl(scriptControl),  
      mapControl(mapControl)
   {}

   void load(uint16_t dialogId, int16_t currentEntityId);
   void loadNextScreen();
   void StartModalDialog(std::string messageString);

   constexpr void MageDialogControl::close() { open = false; }
   constexpr bool isOpen() const { return open; }

   void update();
   void draw();

   void loadCurrentScreenPortrait();
   uint32_t getDialogAddress(uint16_t dialogId) const;

private:
   std::shared_ptr<FrameBuffer> frameBuffer;
   std::shared_ptr<TileManager> tileManager;
   std::shared_ptr<StringLoader> stringLoader;
   std::shared_ptr<MageScriptControl> scriptControl;
   std::shared_ptr<MapControl> mapControl;
   std::shared_ptr<EngineInput> inputHandler;
   bool open{ false };

   uint8_t getTileIdFromXY(uint8_t x, uint8_t y, Rect box) const;
   void drawDialogBox(const std::string& string, Rect box, bool drawArrow = false, bool drawPortrait = false) const;

   bool shouldShowResponses() const
   {
      // last page of messages on this screen
      // and we have responses
      return currentMessageIndex == (currentScreen->messageCount - 1)
         && (currentScreen->responseType == MageDialogResponseType::SELECT_FROM_SHORT_LIST
            || currentScreen->responseType == MageDialogResponseType::SELECT_FROM_LONG_LIST);
   }

   char dialogName[32]{};
   const MageTileset* currentFrameTileset{nullptr};
   int16_t triggeringEntityId{0};
   int32_t currentDialogIndex{0};
   uint32_t currentDialogAddress{0};
   uint32_t currentDialogScreenCount{0};
   int32_t currentScreenIndex{0};
   int32_t currentMessageIndex{0};
   uint16_t currentImageIndex{0};
   uint32_t currentImageAddress{0};
   uint32_t cursorPhase{0};
   uint8_t currentResponseIndex{0};
   uint8_t currentPortraitId{ DIALOG_SCREEN_NO_PORTRAIT };
   RenderableData currentPortraitRenderableData{};
   MageDialogScreen* currentScreen{};
   std::string currentEntityName{};
   std::string currentMessage{};
   std::vector<uint16_t> messageIds{};
   std::vector<MageDialogResponse> responses{};

};

#endif //MAGE_DIALOG_CONTROL_H
