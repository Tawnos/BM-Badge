/*
This is intended to be a shared header file with common formats used in all the
MAGE() game functions. It contains common constants and structured used
throughout the MAGE() game code, and should be included in all future header
files as a way to consolidate redundant definitions and externs showing up all
over the code base.

Note: Do NOT try to include this in the "common.h" header file used outside of
the MAGE() game, as the use of c++ standard libraries will cause issues with
all of the old code used as the foundation of this badge.
*/

#ifndef _MAGE_DEFINES_H
#define _MAGE_DEFINES_H

#include <chrono>
#include "shim_timer.h"
using namespace std::chrono;

static inline const auto DrawWidth = 320;
static inline const auto DrawHeight = 240;
static inline const auto FramebufferSize = DrawWidth * DrawHeight;

static inline const auto MapGoDirectionNameLength = 12;

static inline const auto MAGE_COLLISION_SPOKE_COUNT = 6;

//this is the most unique entities that can be in any map.
static inline const auto MAX_ENTITIES_PER_MAP = 64;


//this is used to note that no player entity was found within the
//entities loaded into the map
static inline const auto NoPlayer = nullptr;
static inline const auto NO_PLAYER_INDEX = 255;

//this is a value used to indicate that an action's calling entity was
//the map, and not actually one of the entities on the map.
static inline const auto MAGE_MAP_ENTITY = 255;
static inline const auto MAGE_ENTITY_SELF = 254;
static inline const auto MAGE_ENTITY_PLAYER = 253;
static inline const auto MAGE_ENTITY_PATH = 65535;

//these are the failover values that the game will use when an invalid hacked entity state is found:
static inline const auto MAGE_TILESET_FAILOVER_ID = 0;
static inline const auto MAGE_TILE_FAILOVER_ID = 0;
static inline const auto MAGE_ANIMATION_DURATION_FAILOVER_VALUE = 0;
static inline const auto MAGE_FRAME_COUNT_FAILOVER_VALUE = 0;
static inline const auto MAGE_RENDER_FLAGS_FAILOVER_VALUE = 0;

//these are the agreed-upon indices for entity_type entity animations
//If you import entities that don't use this convention, their animations may
//not work as intended.
enum class PlayerAnimation
{
   Idle,
   Walk,
   Action,
   Invalid // enums always fill 
};

static inline const auto MAGE_IDLE_ANIMATION_INDEX = 0;
static inline const auto MAGE_WALK_ANIMATION_INDEX = 1;
static inline const auto MAGE_ACTION_ANIMATION_INDEX = 2;

//this is how many bytes of arguments each script action has.
//all actions will have this many bytes, even if some are not used by a particular action
static inline const auto MAGE_NUM_ACTION_ARGS = 7;

//these variables are reserved script and action IDs used to indicate when a script or action should not do anything.
#define MAGE_NO_SCRIPT std::nullopt
#define MAGE_NO_MAP (-1)
#define MAGE_NULL_SCRIPT 0
#define MAGE_NULL_ACTION 0

struct GameClock
{
   using rep = uint32_t;
   using period = std::milli;
   using duration = std::chrono::duration<rep, period>;
   using time_point = std::chrono::time_point<GameClock>;

   static time_point now() noexcept
   {
      auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
      auto epochTime = now_ms.time_since_epoch().count();
      return time_point{ std::chrono::milliseconds{now_ms.time_since_epoch().count()} };
   }
};

// 100px/sec, 1000ms/sec, 30frames/sec, 3px/frame or 6px/frame
// this is how many ms must have passed before the main game loop will run again:
// typical values:
// 60fps: ~16ms
// 30fps: ~33ms
// 24fps: ~41ms
static inline const auto TargetFPS = 30;
static inline const auto MinTimeBetweenRenders = GameClock::duration{ 1000 } / TargetFPS ;
static inline const auto MinTimeBetweenUIInput = GameClock::duration{ 1000 };
static inline const auto IntegrationStepSize = MinTimeBetweenRenders/3;


#ifdef DC801_EMBEDDED
// RTC for NRF52840:
// PRESCALER = round(32768 Hz / [target FPS (Hz)]) - 1
static inline const auto RtcPrescaler = std::round(32768 / TargetFPS) - 1;
#endif

//these are used for setting player speed
//speed is in x/y pixels per update tick
static inline const auto RunSpeed = uint16_t{ 3 };
static inline const auto WalkSpeed = uint16_t{ 2 };

#endif //_MAGE_DEFINES_H
