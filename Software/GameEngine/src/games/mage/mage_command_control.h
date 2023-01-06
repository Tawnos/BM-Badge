#ifndef GAMEENGINE_MAGE_COMMAND_CONTROL_H
#define GAMEENGINE_MAGE_COMMAND_CONTROL_H

#include "mage_rom.h"
#include "mage_defines.h"
#include "StringLoader.h"

class MageScriptControl;
class MapControl;
class TileManager;

#define COMMAND_NO_CONNECT_DIALOG_ID 0xFFFF

enum MageSerialCommands : uint8_t {
	COMMAND_NONE = 0,
	COMMAND_HELP = 1,
	COMMAND_LOOK = 2,
	COMMAND_GO = 3,
	COMMAND_GET = 4,
	COMMAND_DROP = 5,
	COMMAND_INVENTORY = 6,
	COMMAND_USE = 7,
	NUM_SERIAL_COMMANDS
};

enum MageSerialDialogResponseTypes : uint8_t {
	RESPONSE_NONE = 0,
	RESPONSE_ENTER_NUMBER = 1,
	RESPONSE_ENTER_STRING = 2,
	NUM_RESPONSE_TYPES
} ;

struct MageSerialDialog
{
	char name[32];
	uint16_t stringId;
	MageSerialDialogResponseTypes serialResponseType;
	uint8_t responseCount;
};

 struct MageSerialDialogResponse
 {
	uint16_t stringId;
	uint16_t scriptId;
};

class MageCommandControl {
	public:
		MageCommandControl(std::shared_ptr<MapControl> mapControl, std::shared_ptr<TileManager> tileManager, std::shared_ptr<MageScriptControl> scriptControl) noexcept
			: mapControl(mapControl), tileManager(tileManager), scriptControl(scriptControl)
		{}

		std::string commandResponseBuffer;
		std::string serialDialogBuffer;
		std::string postDialogBuffer;
		MageSerialDialog serialDialog = {};
		std::vector<MageSerialDialogResponse> serialDialogResponses{};
		uint16_t connectSerialDialogId = COMMAND_NO_CONNECT_DIALOG_ID;
		uint16_t serialDialogId = COMMAND_NO_CONNECT_DIALOG_ID;
		MageSerialCommands lastCommandUsed = COMMAND_NONE;
		bool isInputTrapped = false;
		void handleStart();
		void processCommand(char *commandString);
		void processCommandAsVerb(std::string input);
		void processCommandAsResponseInput(std::string input);
		void showSerialDialog(uint16_t serialDialogId);
		void reset();
		void sendBufferedOutput();
private:
	std::shared_ptr<MapControl> mapControl;
	std::shared_ptr<TileManager> tileManager;
	std::shared_ptr<MageScriptControl> scriptControl;
	std::shared_ptr<StringLoader> stringLoader;

	void badAsciiLowerCase(std::string* data)
	{
		size_t length = data->size();
		for (size_t i = 0; i < length; i++)
		{
			(*data)[i] = std::tolower((*data)[i]);
		}
	}
};

#endif //GAMEENGINE_MAGE_COMMAND_CONTROL_H
