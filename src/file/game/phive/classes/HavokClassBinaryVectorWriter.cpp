#include "HavokClassBinaryVectorWriter.h"

#include <file/game/phive/classes/HavokClasses.h>
#include <util/Logger.h>

namespace application::file::game::phive::classes
{
	void HavokClassBinaryVectorWriter::WriteRawArray(void* Ptr, unsigned int ArraySizeInBytes, unsigned int ArrayCount, bool IsPrimitive)
	{
		if (IsPrimitive)
		{
			WriteRawUnsafeFixed(static_cast<const char*>(Ptr), ArraySizeInBytes);
			return;
		}

		unsigned int ElementSize = ArraySizeInBytes / ArrayCount;

		for (unsigned int i = 0; i < ArrayCount; i++)
		{
			void* ElementPointer = static_cast<char*>(Ptr) + i * ElementSize;
			application::file::game::phive::classes::HavokClasses::hkReadableWriteableObject* Object = static_cast<application::file::game::phive::classes::HavokClasses::hkReadableWriteableObject*>(ElementPointer);

			Object->Skip(*this);
			Object->Write(*this);
		}
	}

	void HavokClassBinaryVectorWriter::InstallWriteItemCallback(const std::function<uint32_t(const std::string& Type, const std::string& ChildName, uint32_t DataOffset, uint32_t Count)>& Callback)
	{
		mWriteItemCallback = Callback;
	}

	void HavokClassBinaryVectorWriter::InstallWritePatchCallback(const std::function<void(const std::string& Name, unsigned int Offset)>& Callback)
	{
		mWritePatchCallback = Callback;
	}

	uint32_t HavokClassBinaryVectorWriter::WriteItemCallback(void* Ptr, unsigned int Count, void* ChildPtr, const std::string& ChildNameOverwrite)
	{
		application::file::game::phive::classes::HavokClasses::hkReadableWriteableObject* Object = static_cast<application::file::game::phive::classes::HavokClasses::hkReadableWriteableObject*>(Ptr);

		if (!mWriteItemCallback.has_value())
		{
			application::util::Logger::Error("HavokClassBinaryVectorWriter", "No item write callback installed");
			return 0;
		}

		std::string ChildName = ChildNameOverwrite;
		if (ChildPtr != nullptr && ChildNameOverwrite.empty())
		{
			application::file::game::phive::classes::HavokClasses::hkReadableWriteableObject* ChildObject = static_cast<application::file::game::phive::classes::HavokClasses::hkReadableWriteableObject*>(ChildPtr);
			ChildName = ChildObject->GetHavokClassName();
		}

		return mWriteItemCallback.value()(Object->GetHavokClassName(), ChildName, GetPosition(), Count);
	}

	void HavokClassBinaryVectorWriter::WritePatchCallback(const std::string& Name, unsigned int Offset)
	{
		if (!mWritePatchCallback.has_value())
		{
			application::util::Logger::Error("HavokClassBinaryVectorWriter", "No patch write callback installed");
			return;
		}

		mWritePatchCallback.value()(Name, Offset);
	}
}