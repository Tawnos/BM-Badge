﻿#ifndef ENGINE_ROM_H_
#define ENGINE_ROM_H_

#include "EnginePanic.h"
#include "utility.h"
#include <stdint.h>
#include <stddef.h>
#include <array>
#include <memory>
#include <typeinfo>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <tuple>
#include <vector>

#define DESKTOP_SAVE_FILE_PATH "MAGE/save_games/"
static const inline uint8_t ENGINE_ROM_SAVE_GAME_SLOTS = 3;
static const char* saveFileSlotNames[ENGINE_ROM_SAVE_GAME_SLOTS] = {
   DESKTOP_SAVE_FILE_PATH "save_0.dat",
   DESKTOP_SAVE_FILE_PATH "save_1.dat",
   DESKTOP_SAVE_FILE_PATH "save_2.dat"
};

static const inline auto ENGINE_VERSION = 3;

template <typename TDataTag>
class Header
{
public:
   Header() noexcept = default;
   Header(uint32_t count, const uint32_t* offset) noexcept
      : count(count),
      offsets(offset, count),
      lengths(offset + sizeof(uint32_t) * count, count)
   {}

   constexpr const uint16_t Count() const { return count; }
   constexpr const uint32_t GetOffset(uint16_t i) const
   {
      return offsets[i % count];
   }
   constexpr const uint32_t GetLength(uint16_t i) const
   {
      return lengths[i % count];
   }

private:
   uint16_t count;
   std::span<const uint32_t> offsets;
   std::span<const uint32_t> lengths;
}; //class Header

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
// https://ngathanasiou.wordpress.com/2020/07/09/avoiding-compile-time-recursion/
/////////////////////////////////////////////////////////////////////////////////
template <class T, uint32_t I, class Tuple>
constexpr bool match_v = std::is_same_v<T, std::tuple_element_t<I, Tuple>>;

template <class T, class Tuple, class Idxs = std::make_integer_sequence<uint32_t, std::tuple_size_v<Tuple>>>
struct type_index;

template <class T, template <class...> class Tuple, class... Args, uint32_t... Is>
struct type_index<T, Tuple<Args...>, std::integer_sequence<uint32_t, Is...>>
   : std::integral_constant<uint32_t, ((Is* match_v<T, Is, Tuple<Args...>>) + ... + 0)>
{
   static_assert(1 == (match_v<T, Is, Tuple<Args...>> +... + 0), "T doesn't appear only once in type Tuple");
};

template <class T, class Tuple>
constexpr uint32_t type_index_v = type_index<T, Tuple>::value;
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

//This is the smallest page that can be erased on the FL256SSVF01 chip which uses uniform 256kB page sizes
//262144 bits = 256kB = 32kiB
static const inline uint32_t ENGINE_ROM_ERASE_PAGE_SIZE = 262144;

//this is all the bytes on our ROM chip. We aren't able to write more than this
//to the ROM chip, as there are no more bytes on it. Per the datasheet, there are 32MB = 2^25 bits = 4194304 bytes = 4MiB
//We are also subtracting ENGINE_ROM_SAVE_RESERVED_MEMORY_SIZE for save data at the end of rom
static const inline uint32_t ENGINE_ROM_QSPI_CHIP_SIZE = 33554432;
static const inline uint32_t ENGINE_ROM_SAVE_RESERVED_MEMORY_SIZE = (ENGINE_ROM_ERASE_PAGE_SIZE * ENGINE_ROM_SAVE_GAME_SLOTS);
static const inline uint32_t ENGINE_ROM_MAX_DAT_FILE_SIZE = (ENGINE_ROM_QSPI_CHIP_SIZE - ENGINE_ROM_SAVE_RESERVED_MEMORY_SIZE);
static const inline uint32_t ENGINE_ROM_SAVE_OFFSET = (ENGINE_ROM_MAX_DAT_FILE_SIZE);

//this is the length of the 'identifier' at the start of the game.dat file:
static const inline uint32_t ENGINE_ROM_IDENTIFIER_STRING_LENGTH = 8;

//this is the length of the 'engine rom version number' at the start of the game.dat file:
//it is to determine if the game rom is compatible with the engine version
static const inline uint32_t ENGINE_ROM_VERSION_NUMBER_LENGTH = 4;

//this is the length of the crc32 that follows the magic string in game.dat
//it is used to let us check if we need to re-flash the ROM chip with the file on
//the SD card.
static const inline uint32_t ENGINE_ROM_CRC32_LENGTH = 4;

//this is the length of the scenario data from the 0 offset to the end
static const inline uint32_t ENGINE_ROM_GAME_LENGTH = 4;
static const inline uint32_t ENGINE_ROM_START_OF_CRC_OFFSET = ENGINE_ROM_IDENTIFIER_STRING_LENGTH + ENGINE_ROM_VERSION_NUMBER_LENGTH;
static const inline uint32_t ENGINE_ROM_MAGIC_HASH_LENGTH = ENGINE_ROM_START_OF_CRC_OFFSET + ENGINE_ROM_CRC32_LENGTH + ENGINE_ROM_GAME_LENGTH;

template<typename TSave, typename... THeaders>
struct EngineROM
{
   // noexcept because we want to fail fast/call std::terminate if this fails
   EngineROM(const char* romData, uint32_t length) noexcept
      : romData(romData, length),
      headers(headersFor<THeaders...>(ENGINE_ROM_MAGIC_HASH_LENGTH))
   {
      // Verify magic string is on ROM:
      if (!Magic())
      {
         //let out the magic s̶m̶o̶k̶e̶ goat
         ENGINE_PANIC(
            "ROM header invalid. Game cannot start.\n"
            "Goat is sad.   ##### ####     \n"
            "             ##   #  ##       \n"
            "            #   (-)    #      \n"
            "            #+       ######   \n"
            "            #^             ## \n"
            "             ###           #  \n"
            "               #  #      # #  \n"
            "               ##  ##  ##  #  \n"
            "               ######  #####  \n"
         );
      }
   }

   ~EngineROM() noexcept = default;

   bool Magic() const
   {
      const auto magicString = std::string{ "MAGEGAME" };

      for (std::size_t i = 0; i < magicString.size(); i++)
      {
         if (magicString[i] != romData[i])
         {
            return false;
         }
      }
      return true;
   }

   template <typename TData>
   constexpr uint16_t GetCount() const { return getHeader<TData>().Count(); }

   template <typename T>
   void Read(T& t, uint32_t& offset, size_t count = 1) const
   {
      static_assert(std::is_scalar_v<T> || std::is_standard_layout_v<T>, "T must be a scalar or standard-layout type");
      const auto elementSize = sizeof(std::remove_all_extents_t<T>);
      const auto dataLength = count * elementSize;

      const auto dataPointer = &romData[offset];
      memcpy(&t, dataPointer, dataLength);
      offset += dataLength;
   }

   template <typename T>
   constexpr uint32_t GetOffsetByIndex(uint16_t index) const
   {
      return getHeader<T>().GetOffset(index);
   }

   template <typename TLookup, typename TCast = TLookup>
   constexpr const TCast* GetReadPointerByIndex(uint16_t index) const
   {
      return reinterpret_cast<const TCast*>(&romData[getHeader<TLookup>().GetOffset(index)]);
   }

   template <typename T>
   constexpr const T* GetReadPointerToOffset(uint32_t offset) const
   {
      return reinterpret_cast<const T*>(&romData[offset]);
   }

   template <typename T>
   inline std::unique_ptr<T> InitializeRAMCopy(uint16_t index) const
   {
      static_assert(std::is_constructible_v<T, uint32_t&>, "Must be constructible from an offset");

      auto offset = getHeader<T>().GetOffset(index);
      return std::make_unique<T>(offset);
   }

   template <typename T, std::size_t Extent = std::dynamic_extent>
   constexpr auto GetViewOf(uint32_t& offset, uint16_t count) const -> std::span<const T, Extent>
   {
      const auto data = (const T*)&romData[offset];
      offset += count * sizeof(T);
      return std::span<const T, Extent>(data, count);
   }

   bool VerifyEqualsAtOffset(uint32_t offset, std::string value) const
   {
      if (value.empty())
      {
         ENGINE_PANIC("EngineROM<THeaders...>::VerifyEqualsAtOffset: Empty string");
      }

      for (std::size_t i = 0; i < value.size(); i++)
      {
         if (i >= romData.size() || value[i] != romData[i])
         {
            return false;
         }
      }
      return true;
   }

   constexpr TSave GetCurrentSaveCopy() const
   {
      return currentSave;
   }

   constexpr const TSave& GetCurrentSave() const
   {
      return currentSave;
   }

   constexpr void SetCurrentSave(TSave save)
   {
      currentSave = save;
   }

   constexpr const TSave& ResetCurrentSave(uint32_t scenarioDataCRC32)
   {
      auto newSave = TSave{};
      newSave.scenarioDataCRC32 = scenarioDataCRC32;
      currentSave = newSave;
      return currentSave;
   }

   void LoadSaveSlot(uint8_t slotIndex)
   {
#ifdef DC801_EMBEDDED
      auto saveAddress = uint32_t{ ENGINE_ROM_SAVE_OFFSET + (slotIndex * ENGINE_ROM_ERASE_PAGE_SIZE) };
      Read(currentSave, saveAddress);
#else
      auto saveFilePath = std::filesystem::directory_entry{ std::filesystem::absolute(DESKTOP_SAVE_FILE_PATH) };
      if (!saveFilePath.exists())
      {
         if (!std::filesystem::create_directories(saveFilePath))
         {
            throw "Couldn't create save directory";
         }
      }
      const char* saveFileName = saveFileSlotNames[slotIndex];

      auto fileDirEntry = std::filesystem::directory_entry{ std::filesystem::absolute(saveFileName) };
      if (fileDirEntry.exists())
      {
         auto fileSize = fileDirEntry.file_size();
         debug_print("Save file size: %zu\n", (std::size_t)fileDirEntry.file_size());

         auto saveFile = std::fstream{ saveFileName, std::ios::in | std::ios::binary };
         if (!saveFile.good())
         {
            int error = errno;
            fprintf(stderr, "Error: %s\n", strerror(error));
            ENGINE_PANIC("Desktop build: SAVE file missing");
         }
         else
         {
            saveFile.read((char*)&currentSave, sizeof(TSave));
            if (saveFile.gcount() != sizeof(TSave))
            {
               // The file on disk can't be read?
               // Empty out the destination.
               currentSave = TSave{};
            }
            saveFile.close();
         }
      }
#endif
   }

private:
   std::span<const char> romData;
   std::tuple<Header<THeaders>...> headers;
   TSave currentSave{};

   template <typename TData>
   constexpr const Header<TData>& getHeader() const
   {
      constexpr auto index = type_index_v<Header<TData>, std::tuple<Header<THeaders>...>>;
      return std::get<index>(headers);
   }

   template <typename T>
   constexpr auto headerFor(uint32_t& offset) const
   {
      const auto count = *(const uint32_t*)(&romData[offset]);
      offset += sizeof(uint32_t);
      const auto header = Header<T>(count, (const uint32_t*)(&romData[offset]));
      offset += sizeof(uint32_t) * count * 2;
      return header;
   }

   template <typename... TRest>
   constexpr auto headersFor(uint32_t offset) const
   {
      return std::tuple<Header<TRest>...>{headerFor<TRest>(offset)...};
   }
};

#endif
