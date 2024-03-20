#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include "modules/keyboard.h"
#include "EngineSerial.h"
#include <signal.h>
#include <stdint.h>
#include <chrono>
#include <queue>
#include <utility>
#include "shim_timer.h"

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

template <typename IndexType, typename ValueType, int Length >
class EnumClassArray
{
public:
   template <typename T>
   constexpr ValueType& operator[](T i)
   {
      return vals[static_cast<int>(i)];
   }

   template <typename T>
   constexpr const ValueType& operator[](T i) const
   {
      return vals[static_cast<int>(i)];
   }

   constexpr int size() const { return Length; }

   constexpr auto begin() 
   {
      return &vals[0];
   }
   constexpr auto end()
   {
      return &vals[Length];
   }

private:
   std::array<ValueType, Length> vals{};
};

struct InputState
{
   std::optional<GameClock::time_point> lastPressed;
   std::optional<GameClock::time_point> lastReleased;
   std::optional<GameClock::time_point> lastChecked;

   constexpr bool Pressed() const
   {
      return lastPressed.has_value()
         && (!lastReleased.has_value() || lastReleased.value() <= lastPressed.value());
   }
};

//struct InputState
//{
//   const GameClock::duration deltaTime;
//
   //inline bool PlayerIsActioning()
   //{
   //   return IsPressed(KeyPress::Rjoy_left);
   //}

   //inline bool Hack()
   //{
   //   return IsPressed(KeyPress::Rjoy_up);
   //}

   //inline bool Use()
   //{
   //   return IsPressed(KeyPress::Rjoy_right);
   //}

   //inline bool Up()
   //{
   //   return IsPressed(KeyPress::Ljoy_up) && !IsPressed(KeyPress::Ljoy_down);
   //}
   //inline bool Down()
   //{
   //   return IsPressed(KeyPress::Ljoy_down) && !IsPressed(KeyPress::Ljoy_up);
   //}
   //inline bool Left()
   //{
   //   return IsPressed(KeyPress::Ljoy_left) && !IsPressed(KeyPress::Ljoy_right);
   //}
   //inline bool Right()
   //{
   //   return IsPressed(KeyPress::Ljoy_right) && !IsPressed(KeyPress::Ljoy_left);
   //}

 
   //inline bool AdvanceDialog()
   //{
   //   return IsPressed(KeyPress::Rjoy_down)
   //       || IsPressed(KeyPress::Rjoy_left)
   //       || IsPressed(KeyPress::Rjoy_right);
   //}

   //inline bool Increment()
   //{
   //   return IsPressed(KeyPress::Rjoy_up) && !IsPressed(KeyPress::Rjoy_down);
   //}
   //inline bool Decrement()
   //{
   //   return IsPressed(KeyPress::Rjoy_down) && !IsPressed(KeyPress::Rjoy_up);
   //}

   //inline bool Running()
   //{
   //   return IsPressed(KeyPress::Rjoy_right);
   //}
//
//   //Xor
//   //Add
//   //Sub
//   //Bit128
//   //Bit64
//   //Bit32
//   //Bit16
//   //Bit8
//   //Bit4
//   //Bit2
//   //Bit1
//
//};

class EngineInput
{
public:
   void UpdateState();

   [[nodiscard("Value of KeepRunning should be used to handle the main input loop")]]
   const bool KeepRunning() { return running; }

   [[nodiscard("Value of Reset should be used to trigger map/engine reload when true")]]
   bool Reset()
   {
      auto curReset = reset;
      reset = false;
      return curReset;
   }

   inline bool PlayerIsActioning()
   {
      return IsPressed(KeyPress::Rjoy_left);
   }

   inline bool Hack()
   {
      return IsPressed(KeyPress::Rjoy_up);
   }

   inline bool Use()
   {
      return IsPressed(KeyPress::Rjoy_right);
   }

   inline bool Up()
   {
      return IsPressed(KeyPress::Ljoy_up) && !IsPressed(KeyPress::Ljoy_down);
   }
   inline bool Down()
   {
      return IsPressed(KeyPress::Ljoy_down) && !IsPressed(KeyPress::Ljoy_up);
   }
   inline bool Left()
   {
      return IsPressed(KeyPress::Ljoy_left) && !IsPressed(KeyPress::Ljoy_right);
   }
   inline bool Right()
   {
      return IsPressed(KeyPress::Ljoy_right) && !IsPressed(KeyPress::Ljoy_left);
   }


   inline bool AdvanceDialog()
   {
      return IsPressed(KeyPress::Rjoy_down)
          || IsPressed(KeyPress::Rjoy_left)
          || IsPressed(KeyPress::Rjoy_right);
   }

   inline bool Increment()
   {
      return IsPressed(KeyPress::Rjoy_up) && !IsPressed(KeyPress::Rjoy_down);
   }
   inline bool Decrement()
   {
      return IsPressed(KeyPress::Rjoy_down) && !IsPressed(KeyPress::Rjoy_up);
   }

   inline bool Running()
   {
      return IsPressed(KeyPress::Rjoy_right);
   }

   //this is a global that holds the amount of millis that a blocking delay will
   //prevent the main loop from continuing for. It is set by the blockingDelay() action.
   uint32_t blockingDelayTime{ 0 };
   
   constexpr bool IsPressed(KeyPress key)
   {
      inputStates[key].lastChecked.emplace(GameClock::now());
      return inputStates[key].Pressed();
   }

   constexpr const EnumClassArray<KeyPress, InputState, KEYBOARD_NUM_KEYS>& GetInputStates()
   {
      return inputStates;
   }

   GameClock::time_point lastUpdate{ GameClock::now() };
   GameClock::duration lastDelta{ 0 };
private:
   bool running{ true };
   bool reset{ false };
   bool isEntityDebugOn{ false };
   EnumClassArray<KeyPress, InputState, KEYBOARD_NUM_KEYS> inputStates{};

   std::optional<KeyPress> mapScanCode(int scanCode) const;
#ifndef DC801_EMBEDDED
   std::string GetCommandStringFromStandardIn();
   void UpdateDesktopInputState(const GameClock::time_point& curTime);
#else
   std::unique_ptr<EngineSerial> serial{};
#endif
};



#endif