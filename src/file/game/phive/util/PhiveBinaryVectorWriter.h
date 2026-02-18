#pragma once

#include <util/BinaryVectorWriter.h>
#include <util/Logger.h>
#include <file/game/phive/util/PhiveWrapper.h>
#include <map>
#include <string>
#include <iostream>

namespace application::file::game::phive::util
{
	class PhiveBinaryVectorWriter : public application::util::BinaryVectorWriter
	{
	public:

		struct hkWriteable
		{
			virtual void Write(PhiveBinaryVectorWriter& Writer) {};
			virtual void Write(PhiveBinaryVectorWriter& Writer, unsigned int Offset) {};
		};

		std::map<std::string, uint32_t> mArrayOffsets;
		std::map<std::string, uint32_t> mPointerOffsets;
		uint32_t mItemPointerIndex = 2;

		void QueryHkArrayWrite(std::string Name);
		void QueryHkArrayToItemPointerWrite(std::string Name);
		void QueryHkPointerWrite(std::string Name);

		template <typename T>
		void WriteHkArray(std::vector<T>* VectorPointer, uint32_t ElementSize, std::string ClassName, std::string ArrayName, bool WriteLocal = false)
		{
			if (!mArrayOffsets.contains(ArrayName))
				return;

			if (VectorPointer->empty())
				return;

			if (ClassName == "hkaiNavMesh" || ClassName == "hkInt32" || ClassName == "hkVector4" || ClassName == "hkaiNavMeshStaticTreeFaceIterator")
				Align(16);

			if (ClassName == "hkaiClusterGraph")
				Align(8);

			uint32_t Jumpback = GetPosition();

			Seek(mArrayOffsets[ArrayName], BinaryVectorWriter::Position::Begin);

			WriteInteger(0, sizeof(uint64_t));
			WriteInteger(mItemPointerIndex++, sizeof(uint32_t));
			WriteInteger(0, sizeof(uint32_t));

			if (!WriteLocal)
			{
				mPhiveWrapper.mItems.push_back(PhiveWrapper::PhiveWrapperItem{
					.mTypeIndex = mPhiveWrapper.GetTypeNameIndex(ClassName),
					.mFlags = (uint16_t)mPhiveWrapper.GetTypeNameFlag(ClassName),
					.mDataOffset = Jumpback,
					.mCount = (uint32_t)VectorPointer->size()
					});
			}

			Seek(Jumpback, BinaryVectorWriter::Position::Begin);

			for (T& Element : *VectorPointer)
			{
				if (ClassName.starts_with("hk"))
				{
					hkWriteable* BaseObj = (hkWriteable*)&Element;
					if (BaseObj)
					{
						BaseObj->Write(*this);
					}
				}
				else
				{
					WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Element), ElementSize);
				}
			}

			mArrayOffsets.erase(ArrayName);
		}

		template <typename T>
		void WriteHkArrayToItemPointer(std::vector<T>* VectorPointer, uint32_t ElementSize, std::string ClassName, std::string ArrayName)
		{
			if (!mArrayOffsets.contains(ArrayName))
				return;

			if (VectorPointer->empty())
				return;

			if (ClassName == "hkaiNavMesh" || ClassName == "hkInt32" || ClassName == "hkVector4" || ClassName == "hkaiNavMeshStaticTreeFaceIterator")
				Align(16);

			if (ClassName == "hkaiClusterGraph")
				Align(8);

			uint32_t Jumpback = GetPosition();

			Seek(mArrayOffsets[ArrayName], BinaryVectorWriter::Position::Begin);

			WriteInteger(mItemPointerIndex++, sizeof(uint64_t));
			WriteInteger(0, sizeof(uint32_t));
			WriteInteger(0, sizeof(uint32_t));


			mPhiveWrapper.mItems.push_back(PhiveWrapper::PhiveWrapperItem{
				.mTypeIndex = mPhiveWrapper.GetTypeNameIndex(ClassName),
				.mFlags = (uint16_t)mPhiveWrapper.GetTypeNameFlag(ClassName),
				.mDataOffset = Jumpback,
				.mCount = (uint32_t)VectorPointer->size()
				});

			Seek(Jumpback, BinaryVectorWriter::Position::Begin);

			for (T& Element : *VectorPointer)
			{
				if (ClassName.starts_with("hk"))
				{
					hkWriteable* BaseObj = (hkWriteable*)&Element;
					if (BaseObj)
					{
						BaseObj->Write(*this);
					}
				}
				else
				{
					WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Element), ElementSize);
				}
			}

			mArrayOffsets.erase(ArrayName);
		}

		template <typename T>
		void WriteHkPointer(T* ObjectPointer, uint32_t ObjectSize, std::string ClassName, std::string PointerName)
		{
			if (!mPointerOffsets.contains(PointerName))
				return;

			if (ClassName == "hkaiNavMesh" || ClassName == "hkInt32" || ClassName == "hkVector4" || ClassName == "hkcdStaticAabbTree__Impl")
				Align(16);

			if (ClassName == "hkaiClusterGraph" || ClassName == "hkaiNavMeshStaticTreeFaceIterator")
				Align(8);

			uint32_t Jumpback = GetPosition();

			Seek(mPointerOffsets[PointerName], BinaryVectorWriter::Position::Begin);

			WriteInteger(mItemPointerIndex++, sizeof(uint64_t));

			mPhiveWrapper.mItems.push_back(PhiveWrapper::PhiveWrapperItem{
				.mTypeIndex = mPhiveWrapper.GetTypeNameIndex(ClassName),
				.mFlags = (uint16_t)mPhiveWrapper.GetTypeNameFlag(ClassName),
				.mDataOffset = Jumpback,
				.mCount = 1
				});

			Seek(Jumpback, BinaryVectorWriter::Position::Begin);

			if constexpr (std::is_base_of_v<hkWriteable, T>) {
				hkWriteable* BaseObj = (hkWriteable*)ObjectPointer;
				if (BaseObj)
				{
					BaseObj->Write(*this);
				}
				else
				{
					application::util::Logger::Error("PhiveBinaryVectorWriter", "Could not write pointer");
				}
			}
			else
			{
				WriteRawUnsafeFixed(reinterpret_cast<const char*>(ObjectPointer), ObjectSize);
			}

			mPointerOffsets.erase(PointerName);

		}

		application::file::game::phive::util::PhiveWrapper& mPhiveWrapper;

		PhiveBinaryVectorWriter(application::file::game::phive::util::PhiveWrapper& mWrapper);
	};
}