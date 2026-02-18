#pragma once

#include <util/BinaryVectorWriter.h>
#include <string>
#include <functional>
#include <optional>
#include <file/game/phive/classes/HavokDataHolder.h>

namespace application::file::game::phive::classes
{
	class HavokClassBinaryVectorWriter : public application::util::BinaryVectorWriter
	{
	public:
		void InstallWriteItemCallback(const std::function<uint32_t(const std::string& Type, const std::string& ChildName, uint32_t DataOffset, uint32_t Count)>& Callback);
		void InstallWritePatchCallback(const std::function<void(const std::string& Name, unsigned int Offset)>& Callback);

		uint32_t WriteItemCallback(void* Ptr, unsigned int Count = 1, void* ChildPtr = nullptr, const std::string& ChildNameOverwrite = "");
		void WritePatchCallback(const std::string& Name, unsigned int Offset);
		void WriteRawArray(void* Ptr, unsigned int ArraySizeInBytes, unsigned int ArrayCount, bool IsPrimitive);

	private:
		std::optional<std::function<uint32_t(const std::string& Type, const std::string& ChildName, uint32_t DataOffset, uint32_t Count)>> mWriteItemCallback = std::nullopt;
		std::optional<std::function<void(const std::string& Name, unsigned int Offset)>> mWritePatchCallback = std::nullopt;
	};
}