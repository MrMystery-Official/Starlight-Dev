#pragma once

#include <file/game/phive/classes/HavokClassBinaryVectorReader.h>
#include <file/game/phive/classes/HavokClassBinaryVectorWriter.h>
#include <file/game/phive/starlight_physics/common/hkAabb.h>
#include <util/Logger.h>
#include <vector>
#include <functional>
#include <cassert>
#include <bit>
#include <glm/vec3.hpp>

/*
    Havok reflection dump processed by application::file::game::phive::util::PhiveClassGeneratorMultiImpl
*/

namespace application::file::game::phive::classes
{
    namespace HavokClasses
    {
        struct hkReadableWriteableObject
        {
            bool mIsItem = false;
            void MarkAsItem() { mIsItem = true; }

            virtual std::string GetHavokClassName() = 0;

            virtual unsigned int GetHavokClassAlignment() { return 1; }

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) = 0;

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) {}
            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) = 0;
        };



        struct hkInt64 : public hkReadableWriteableObject
        {
            long long mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkInt64";
            }
        };



        template<typename tT>
        struct hkRelPtr : public hkReadableWriteableObject
        {
            hkInt64 mOffset;
            tT mObj;

            bool mIsNullPtr = true;
            unsigned long long mParentOffset = std::numeric_limits<unsigned long long>::max();
            unsigned long long mLastElementOffset = std::numeric_limits<unsigned long long>::max();

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                mOffset.Read(Reader);

                uint32_t Jumback = Reader.GetPosition();
                if (mOffset.mParent > 0)
                {
                    Reader.Seek(mOffset.mParent - 8, application::util::BinaryVectorReader::Position::Current);
                    hkReadableWriteableObject* BaseObj = static_cast<hkReadableWriteableObject*>(&mObj);
                    if (BaseObj)
                    {
                        BaseObj->Read(Reader);
                        mIsNullPtr = false;
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkRelPtr", "Cannot cast to hkReadableWriteableObject");
                    }
                }
                Reader.Seek(Jumback, application::util::BinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                mParentOffset = Writer.GetPosition();
                Writer.Seek(8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkRelPtr", "mParentOffset was not set");
                    return;
                }

                if (!mIsNullPtr) Writer.Align(8);
                unsigned long long Offset = Writer.GetPosition();

                Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                if (mIsNullPtr)
                {
                    mOffset.mParent = 0;
                    mLastElementOffset = Offset;
                    mOffset.Write(Writer);
                    Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);
                    mLastElementOffset = Writer.GetPosition();
                    return;
                }

                mOffset.mParent = Offset - Writer.GetPosition();
                mOffset.Skip(Writer);

                Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);
                if (mIsItem && !mIsNullPtr) Writer.WriteItemCallback(this, 1, &mObj);

                hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&mObj);
                if (BaseObj)
                {
                    BaseObj->Write(Writer);
                }
                else
                {
                    application::util::Logger::Warning("HavokClasses::hkRelPtr", "Cannot cast to hkReadableWriteableObject");
                }
                mLastElementOffset = Writer.GetPosition();
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkRelPtr";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        template<typename tTYPE>
        struct hkRefPtr : public hkReadableWriteableObject
        {
            hkInt64 mOffset;
            tTYPE mObj;

            bool mIsNullPtr = true;
            unsigned long long mParentOffset = std::numeric_limits<unsigned long long>::max();
            unsigned long long mLastElementOffset = std::numeric_limits<unsigned long long>::max();
            bool mIsPatched = false;
            bool mManualPatching = false;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                mOffset.Read(Reader);

                if (mOffset.mParent == 0)
                    return;

                uint64_t PatchOffsetTarget = Reader.GetPosition() - 8;
                for (const auto& Patch : Reader.mDataHolder->GetPatches())
                {
                    for (uint32_t Offset : Patch.mOffsets)
                    {
                        if (Offset == PatchOffsetTarget)
                        {
                            mIsPatched = true;
                            break;
                        }
                    }

                    if (mIsPatched) break;
                }

                if (mIsPatched)
                {
                    mOffset.mParent = Reader.mDataHolder->GetItems()[mOffset.mParent].mDataOffset;
                }
                else
                {
                    mOffset.mParent += Reader.GetPosition() - 8;
                }

                uint32_t Jumback = Reader.GetPosition();
                if (mOffset.mParent > 0)
                {
                    Reader.Seek(mOffset.mParent, application::util::BinaryVectorReader::Position::Begin);
                    hkReadableWriteableObject* BaseObj = static_cast<hkReadableWriteableObject*>(&mObj);
                    if (BaseObj)
                    {
                        BaseObj->Read(Reader);
                        mIsNullPtr = false;
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkRefPtr", "Cannot cast to hkReadableWriteableObject");
                    }
                }
                Reader.Seek(Jumback, application::util::BinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                mParentOffset = Writer.GetPosition();
                if (!mManualPatching && !mIsNullPtr) Writer.WritePatchCallback("hkRefPtr<" + mObj.GetHavokClassName() + ">", mParentOffset);
                Writer.Seek(8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkRefPtr", "mParentOffset was not set");
                    return;
                }

                if (!mIsNullPtr) Writer.Align(mObj.GetHavokClassAlignment());
                unsigned long long Offset = Writer.GetPosition();

                if (mIsNullPtr)
                {
                    Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
                    mOffset.mParent = 0;
                    mLastElementOffset = Offset;
                    mOffset.Write(Writer);
                    Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);
                    mLastElementOffset = Writer.GetPosition();
                    return;
                }

                mOffset.mParent = Offset - mParentOffset;

                if (mIsPatched)
                {
                    mOffset.mParent = Writer.WriteItemCallback(&mObj);
                }

                if (mIsItem && !mIsNullPtr && !mIsPatched) Writer.WriteItemCallback(this, 1, &mObj);

                hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&mObj);
                if (BaseObj)
                {
                    BaseObj->Skip(Writer);
                    BaseObj->Write(Writer);
                }
                else
                {
                    application::util::Logger::Warning("HavokClasses::hkRefPtr", "Cannot cast to hkReadableWriteableObject");
                }
                mLastElementOffset = Writer.GetPosition();

                Writer.Seek(mParentOffset, application::util::BinaryVectorWriter::Position::Begin);
                mOffset.Skip(Writer);
                Writer.Seek(mLastElementOffset, application::util::BinaryVectorWriter::Position::Begin);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkRefPtr";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        template<typename tT>
        struct hkRelArray : public hkReadableWriteableObject
        {
            hkInt64 mOffset;
            int mSize = 0;
            int mCapacityAndFlags = 0;

            std::vector<tT> mElements;
            unsigned long long mParentOffset = std::numeric_limits<unsigned long long>::max();
            unsigned long long mLastElementOffset = std::numeric_limits<unsigned long long>::max();

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                mOffset.Read(Reader);
                Reader.ReadStruct(&mSize, sizeof(mSize));
                Reader.ReadStruct(&mCapacityAndFlags, sizeof(mCapacityAndFlags));


                uint32_t Jumback = Reader.GetPosition();
                if (mOffset.mParent > 0)
                {
                    Reader.Seek(mOffset.mParent - 16, application::util::BinaryVectorReader::Position::Current);
                    mElements.resize(mSize);
                    for (int i = 0; i < mSize; i++)
                    {
                        hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&mElements[i]);
                        if (BaseObj)
                        {
                            BaseObj->Read(Reader);
                        }
                        else
                        {
                            application::util::Logger::Warning("HavokClasses::hkRelArray", "Cannot cast to hkReadableWriteableObject");
                        }
                    }
                }
                Reader.Seek(Jumback, application::util::BinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                mParentOffset = Writer.GetPosition();
                Writer.Seek(16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkRelArray", "mParentOffset was not set");
                    return;
                }

                if (!mElements.empty()) Writer.Align(16);
                unsigned long long Offset = Writer.GetPosition();

                Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                mOffset.mParent = !mElements.empty() ? (Offset - Writer.GetPosition()) : 0;
                mSize = mElements.size();
                mCapacityAndFlags = 0;
                mOffset.Skip(Writer);
                Writer.WriteInteger((int32_t)mSize, sizeof(int32_t));
                Writer.WriteInteger((int32_t)mCapacityAndFlags, sizeof(int32_t));

                Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);
                if (mIsItem && !mElements.empty()) Writer.WriteItemCallback(&mElements[0], mElements.size()); //CHECK THIS

                for (tT& Element : mElements)
                {
                    if constexpr (std::is_integral_v<tT>)
                    {
                        Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Element), sizeof(tT));
                        continue;
                    }

                    hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
                    if (BaseObj)
                    {
                        BaseObj->Skip(Writer);
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkRelArray", "Cannot cast to hkReadableWriteableObject");
                    }
                }

                for (tT& Element : mElements)
                {
                    if constexpr (std::is_integral_v<tT>)
                    {
                        Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Element), sizeof(tT));
                        continue;
                    }

                    hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
                    if (BaseObj)
                    {
                        BaseObj->Write(Writer);
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkRelArray", "Cannot cast to hkReadableWriteableObject");
                    }
                }

                mLastElementOffset = Writer.GetPosition();
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkRelArray";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        template<typename tT, typename tOFFSET_TYPE>
        struct hkRelArrayView : public hkReadableWriteableObject
        {
            tOFFSET_TYPE mOffset;
            int mSize = 0;

            std::vector<tT> mElements;
            unsigned long long mParentOffset = std::numeric_limits<unsigned long long>::max();
            unsigned long long mLastElementOffset = std::numeric_limits<unsigned long long>::max();

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(sizeof(mOffset));
                Reader.ReadStruct(&mOffset, sizeof(mOffset));
                Reader.ReadStruct(&mSize, sizeof(mSize));


                uint32_t Jumback = Reader.GetPosition();
                if (mOffset > 0)
                {
                    Reader.Seek(mOffset - 8, application::util::BinaryVectorReader::Position::Current);
                    mElements.resize(mSize);
                    for (int i = 0; i < mSize; i++)
                    {
                        if constexpr (std::is_integral_v<tT>)
                        {
                            Reader.ReadStruct(&mElements[i], sizeof(tT));
                            continue;
                        }

                        hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&mElements[i]);
                        if (BaseObj)
                        {
                            BaseObj->Read(Reader);
                        }
                        else
                        {
                            application::util::Logger::Warning("HavokClasses::hkRelArrayView", "Cannot cast to hkReadableWriteableObject");
                        }
                    }
                }
                Reader.Seek(Jumback, application::util::BinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(sizeof(mOffset));
                mParentOffset = Writer.GetPosition();
                Writer.Seek(sizeof(mOffset) + 4, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkRelArrayView", "mParentOffset was not set");
                    return;
                }


                if (!mElements.empty()) Writer.Align(16);
                unsigned long long Offset = Writer.GetPosition();

                Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                mOffset = !mElements.empty() ? (Offset - Writer.GetPosition()) : 0;
                mSize = mElements.size();
                Writer.WriteInteger(mOffset, sizeof(mOffset));
                Writer.WriteInteger((int32_t)mSize, sizeof(int32_t));

                Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);
                if (mIsItem && !mElements.empty()) Writer.WriteItemCallback(this, mElements.size(), &mElements[0]);

                for (tT& Element : mElements)
                {
                    if constexpr (std::is_integral_v<tT>)
                    {
                        Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Element), sizeof(tT));
                        continue;
                    }

                    hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
                    if (BaseObj)
                    {
                        BaseObj->Skip(Writer);
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkRelArrayView", "Cannot cast to hkReadableWriteableObject");
                    }
                }

                for (tT& Element : mElements)
                {
                    if constexpr (std::is_integral_v<tT>)
                    {
                        continue;
                    }

                    hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
                    if (BaseObj)
                    {
                        BaseObj->Write(Writer);
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkRelArrayView", "Cannot cast to hkReadableWriteableObject");
                    }
                }

                mLastElementOffset = Writer.GetPosition();
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkRelArrayView";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }

            void ForEveryElement(const std::function<void(tT& Element)>& Func)
            {
                for (tT& Element : mElements)
                {
                    Func(Element);
                }
            }
        };


        template<typename tENUM, typename tSTORAGE>
        struct hkEnum : public hkReadableWriteableObject
        {
            tSTORAGE mStorage;
            tENUM mEnumValue;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                if constexpr (std::is_integral_v<tSTORAGE>)
                {
                    Reader.Align(sizeof(mStorage));
                    Reader.ReadStruct(&mStorage, sizeof(mStorage));
                    mEnumValue = static_cast<tENUM>(mStorage);
                }
                else
                {
                    Reader.Align(sizeof(mStorage.mParent));
                    mStorage.Read(Reader);
                    mEnumValue = static_cast<tENUM>(mStorage.mParent);
                }
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if constexpr (std::is_integral_v<tSTORAGE>)
                {
                    Writer.Align(sizeof(mStorage));
                    mStorage = static_cast<tSTORAGE>(mEnumValue);
                    if (mIsItem) Writer.WriteItemCallback(this);
                    Writer.WriteRawUnsafeFixed(&mStorage, sizeof(mStorage));
                }
                else
                {
                    Writer.Align(sizeof(mStorage.mParent));
                    mStorage.mParent = static_cast<int>(mEnumValue);
                    if (mIsItem) Writer.WriteItemCallback(this);
                    mStorage.Skip(Writer);
                }
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkEnum";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                if constexpr (std::is_integral_v<tSTORAGE>)
                {
                    return sizeof(mStorage);
                }
                else
                {
                    return sizeof(mStorage.mParent);
                }
            }
        };



        template<typename tBITS, typename tSTORAGE>
        struct hkFlags : public hkReadableWriteableObject
        {
            tSTORAGE mStorage;

            bool IsFlagSet(tSTORAGE BitValue)
            {
                if constexpr (std::is_integral_v<tSTORAGE>)
                {
                    return (mStorage & BitValue) != 0;
                }
                else
                {
                    return (mStorage.mParent & BitValue.mParent) != 0;
                }
            }

            void SetFlag(tSTORAGE BitValue, bool bSet)
            {
                if constexpr (std::is_integral_v<tSTORAGE>)
                {
                    if (bSet)
                        mStorage |= BitValue;
                    else
                        mStorage &= ~BitValue;
                }
                else
                {
                    if (bSet)
                        mStorage.mParent |= BitValue.mParent;
                    else
                        mStorage.mParent &= ~BitValue.mParent;
                }
            }

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                if constexpr (std::is_integral_v<tSTORAGE>)
                {
                    Reader.Align(sizeof(mStorage));
                    Reader.ReadStruct(&mStorage, sizeof(mStorage));
                }
                else
                {
                    Reader.Align(sizeof(mStorage.mParent));
                    mStorage.Read(Reader);
                }
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if constexpr (std::is_integral_v<tSTORAGE>)
                {
                    Writer.Align(sizeof(mStorage));
                    if (mIsItem) Writer.WriteItemCallback(this);
                    Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mStorage), sizeof(mStorage));
                }
                else
                {
                    Writer.Align(sizeof(mStorage.mParent));
                    if (mIsItem) Writer.WriteItemCallback(this);
                    mStorage.Skip(Writer);
                }
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkFlags";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                if constexpr (std::is_integral_v<tSTORAGE>)
                {
                    return sizeof(mStorage);
                }
                else
                {
                    return sizeof(mStorage.mParent);
                }
            }
        };



        struct hkBaseObject : public hkReadableWriteableObject
        {
            unsigned long long mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkBaseObject";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkVector4f : public hkReadableWriteableObject
        {
            float mData[4] = { 0.0f };

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                Reader.ReadStruct(&mData[0], sizeof(float) * 4);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mData), sizeof(mData));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkVector4f";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        enum class hknpShapeType__Enum : int
        {
            CONVEX = 0,
            SPHERE = 1,
            CAPSULE = 2,
            CYLINDER = 3,
            TRIANGLE = 4,
            BOX = 5,
            DEBRIS = 6,
            COMPOSITE = 7,
            MESH = 8,
            COMPRESSED_MESH_DEPRECATED = 9,
            EXTERN_MESH = 10,
            COMPOUND = 11,
            DISTANCE_FIELD_BASE = 12,
            HEIGHT_FIELD = 13,
            DISTANCE_FIELD = 14,
            PARTICLE_SYSTEM = 15,
            SCALED_CONVEX = 16,
            TRANSFORMED = 17,
            MASKED = 18,
            MASKED_COMPOUND = 19,
            BREAKABLE_COMPOUND = 20,
            LOD = 21,
            DUMMY = 22,
            USER_0 = 23,
            USER_1 = 24,
            USER_2 = 25,
            USER_3 = 26,
            USER_4 = 27,
            USER_5 = 28,
            USER_6 = 29,
            USER_7 = 30,
            NUM_SHAPE_TYPES = 31,
            INVALID = 32
        };



        struct hkUlong : public hkReadableWriteableObject
        {
            unsigned long long mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkUlong";
            }
        };



        struct hkReferencedObject : public hkReadableWriteableObject
        {
            hkBaseObject mParent;
            hkUlong mSizeAndFlags;
            hkUlong mRefCount;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mSizeAndFlags.Read(Reader);
                mRefCount.Read(Reader);
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mSizeAndFlags.Skip(Writer);
                mRefCount.Skip(Writer);
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mSizeAndFlags.Write(Writer);
                mRefCount.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkReferencedObject";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkUint8 : public hkReadableWriteableObject
        {
            unsigned char mParent = 0;

            hkUint8() = default;
			hkUint8(uint8_t n) : mParent(n) {}

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkUint8";
            }
        };



        enum class hknpCollisionDispatchType__Enum : int
        {
            NONE = 0,
            DEBRIS = 1,
            CONVEX = 2,
            COMPOSITE = 3,
            DISTANCE_FIELD = 4,
            USER_0 = 5,
            USER_1 = 6,
            USER_2 = 7,
            USER_3 = 8,
            USER_4 = 9,
            USER_5 = 10,
            USER_6 = 11,
            USER_7 = 12,
            NUM_TYPES = 13
        };



        struct hkUint16 : public hkReadableWriteableObject
        {
            unsigned short mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkUint16";
            }
        };



        enum class hknpShape__FlagsEnum : int
        {
            IS_CONVEX_SHAPE = 1,
            IS_CONVEX_POLYTOPE_SHAPE = 2,
            IS_COMPOSITE_SHAPE = 4,
            IS_HEIGHT_FIELD_SHAPE = 8,
            USE_SINGLE_POINT_MANIFOLD = 16,
            IS_INTERIOR_TRIANGLE = 32,
            SUPPORTS_COLLISIONS_WITH_INTERIOR_TRIANGLES = 64,
            USE_NORMAL_TO_FIND_SUPPORT_PLANE = 128,
            IS_TRIANGLE_OR_QUAD_SHAPE = 512,
            IS_QUAD_SHAPE = 1024,
            IS_SDF_EDGE_COLLISION_ENABLED = 2048,
            HAS_SURFACE_VELOCITY = 32768,
            USER_FLAG_0 = 4096,
            USER_FLAG_1 = 8192,
            USER_FLAG_2 = 16384,
            INHERITED_FLAGS = 32912
        };


        enum hknpMeshShapeFlags : uint8_t
        {
            IS_TRIANGLE = 1 << 0,
            IS_QUAD = 1 << 1,
            IS_FLAT_CONVEX_QUAD = 1 << 2,
            DISABLE_ALL_EDGES_TRIANGLE_1 = 1 << 4,
            DISABLE_ALL_EDGES_TRIANGLE_2 = 1 << 5
        };


        struct hkReal : public hkReadableWriteableObject
        {
            float mParent = 0.0f;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkReal";
            }
        };



        struct hkUint64 : public hkReadableWriteableObject
        {
            unsigned long long mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkUint64";
            }
        };



        struct hknpShapePropertyBase : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkUint16 mPropertyKey;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mPropertyKey.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mPropertyKey.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mPropertyKey.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpShapePropertyBase";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hknpShapeProperties__Entry : public hkReadableWriteableObject
        {
            hkRelPtr<hknpShapePropertyBase> mObject;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mObject.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mObject.Skip(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mObject.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpShapeProperties::Entry";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hknpShapeProperties : public hkReadableWriteableObject
        {
            hkRelArray<hknpShapeProperties__Entry> mProperties;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mProperties.Read(Reader);
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mProperties.Skip(Writer);
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mProperties.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpShapeProperties";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hknpShape : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkEnum<hknpShapeType__Enum, hkUint8> mType;
            hkEnum<hknpCollisionDispatchType__Enum, hkUint8> mDispatchType;
            hkFlags<hknpShape__FlagsEnum, hkUint16> mFlags;
            hkUint8 mNumShapeKeyBits;
            hkReal mConvexRadius;
            hkUint64 mUserData;
            hknpShapeProperties mProperties;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mType.Read(Reader);
                mDispatchType.Read(Reader);
                mFlags.Read(Reader);
                mNumShapeKeyBits.Read(Reader);
                mConvexRadius.Read(Reader);
                mUserData.Read(Reader);
                mProperties.Read(Reader);
                Reader.Seek(Jumpback + 64, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mType.Skip(Writer);
                mDispatchType.Skip(Writer);
                mFlags.Skip(Writer);
                mNumShapeKeyBits.Skip(Writer);
                mConvexRadius.Skip(Writer);
                mUserData.Skip(Writer);
                mProperties.Skip(Writer);
                Writer.Seek(Jumpback + 64, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mType.Write(Writer);
                mDispatchType.Write(Writer);
                mFlags.Write(Writer);
                mNumShapeKeyBits.Write(Writer);
                mConvexRadius.Write(Writer);
                mUserData.Write(Writer);
                mProperties.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpShape";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkUint32 : public hkReadableWriteableObject
        {
            unsigned int mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkUint32";
            }
        };



        struct hknpCompositeShape : public hkReadableWriteableObject
        {
            hknpShape mParent;
            hkUint32 mShapeTagCodecInfo;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mShapeTagCodecInfo.Read(Reader);
                Reader.Seek(Jumpback + 72, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mShapeTagCodecInfo.Skip(Writer);
                Writer.Seek(Jumpback + 72, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mShapeTagCodecInfo.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpCompositeShape";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkVector4 : public hkReadableWriteableObject
        {
            hkVector4f mParent;

            hkVector4() = default;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                mParent.Read(Reader);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            glm::vec4 ToVec4() const
            {
                return glm::vec4(mParent.mData[0], mParent.mData[1], mParent.mData[2], mParent.mData[3]);
			}

            void FromVec4(const glm::vec4& Vec)
            {
                mParent.mData[0] = Vec.x;
                mParent.mData[1] = Vec.y;
                mParent.mData[2] = Vec.z;
                mParent.mData[3] = Vec.w;
			}

            virtual std::string GetHavokClassName() override
            {
                return "hkVector4";
            }
        };



        struct hknpMeshShapeVertexConversionUtil : public hkReadableWriteableObject
        {
            hkVector4 mBitScale16;
            hkVector4 mBitScale16Inv;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mBitScale16.Read(Reader);
                mBitScale16Inv.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mBitScale16.Skip(Writer);
                mBitScale16Inv.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mBitScale16.Write(Writer);
                mBitScale16Inv.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpMeshShapeVertexConversionUtil";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };

        struct hknpAabb8 : public hkReadableWriteableObject
        {
            hkUint32 mMin;
            hkUint32 mMax;

            hknpAabb8() = default;

            hknpAabb8(uint32_t Min, uint32_t Max)
            {
                mMin.mParent = Min;
                mMax.mParent = Max;
            }

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mMin.Read(Reader);
                mMax.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
			}

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mMin.Skip(Writer);
                mMax.Skip(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mMin.Write(Writer);
                mMax.Write(Writer);
			}

            void SetEmpty()
            {
                mMin.mParent = std::numeric_limits<uint32_t>::max();
                mMax.mParent = std::numeric_limits<uint32_t>::lowest();
            }

            uint8_t& GetMin(int i)
            {
                return reinterpret_cast<uint8_t*>(&mMin)[i];
            }

            uint8_t& GetMax(int i)
            {
                return reinterpret_cast<uint8_t*>(&mMax)[i];
            }

            void IncludeAabb(hknpAabb8& other)
            {
                for (int i = 0; i < 3; i++)
                {
                    GetMin(i) = std::min(GetMin(i), other.GetMin(i));
                    GetMax(i) = std::max(GetMax(i), other.GetMax(i));
                }
            }

            bool ContainsAabb(hknpAabb8& other)
            {
                for (int i = 0; i < 3; i++)
                {
                    if (other.GetMin(i) < GetMin(i) || other.GetMax(i) > GetMax(i))
                    {
                        return false;
                    }
                }

                return true;
            }

            bool operator==(const hknpAabb8& other) const
            {
                return (mMax.mParent & 0x00ffffff) == (other.mMax.mParent & 0x00ffffff) && (mMin.mParent & 0x00ffffff) == (other.mMin.mParent & 0x00ffffff);
            }

            bool operator!=(const hknpAabb8& other) const
            {
                return !(*this == other);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpAabb8";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };

        struct hknpTransposedFourAabbs8 : public hkReadableWriteableObject
        {
            hkUint32 mLx;
            hkUint32 mHx;
            hkUint32 mLy;
            hkUint32 mHy;
            hkUint32 mLz;
            hkUint32 mHz;

            inline uint8_t* GetLowX() { return reinterpret_cast<uint8_t*>(&mLx.mParent); }
            inline uint8_t* GetHighX() { return reinterpret_cast<uint8_t*>(&mHx.mParent); }

            inline uint8_t* GetLowY() { return reinterpret_cast<uint8_t*>(&mLy.mParent); }
            inline uint8_t* GetHighY() { return reinterpret_cast<uint8_t*>(&mHy.mParent); }

            inline uint8_t* GetLowZ() { return reinterpret_cast<uint8_t*>(&mLz.mParent); }
            inline uint8_t* GetHighZ() { return reinterpret_cast<uint8_t*>(&mHz.mParent); }

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mLx.Read(Reader);
                mHx.Read(Reader);
                mLy.Read(Reader);
                mHy.Read(Reader);
                mLz.Read(Reader);
                mHz.Read(Reader);
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mLx.Skip(Writer);
                mHx.Skip(Writer);
                mLy.Skip(Writer);
                mHy.Skip(Writer);
                mLz.Skip(Writer);
                mHz.Skip(Writer);
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mLx.Write(Writer);
                mHx.Write(Writer);
                mLy.Write(Writer);
                mHy.Write(Writer);
                mLz.Write(Writer);
                mHz.Write(Writer);
            }

            void SetEmpty()
            {
                mLx.mParent = 0xffffffffU;
                mLy.mParent = 0xffffffffU;
                mLz.mParent = 0xffffffffU;

                mHx.mParent = 0;
                mHy.mParent = 0;
                mHz.mParent = 0;
            }

            void SetAabb(int index, const hknpAabb8& aabb)
            {
                assert(index >= 0 && index < 4 && "Index out-of-bounds");

                const uint8_t* aabbMin = reinterpret_cast<const uint8_t*>(&aabb.mMin.mParent);
                const uint8_t* aabbMax = reinterpret_cast<const uint8_t*>(&aabb.mMax.mParent);

                GetLowX()[index] = aabbMin[0];
                GetLowY()[index] = aabbMin[1];
                GetLowZ()[index] = aabbMin[2];

                GetHighX()[index] = aabbMax[0];
                GetHighY()[index] = aabbMax[1];
                GetHighZ()[index] = aabbMax[2];
            }

            void ClearAabb(int index)
            {
                assert(index >= 0 && index < 4);

                GetLowX()[index] = 255;
                GetLowY()[index] = 255;
                GetLowZ()[index] = 255;

                GetHighX()[index] = 0;
                GetHighY()[index] = 0;
                GetHighZ()[index] = 0;
            }

            inline glm::bvec4 ActiveMask() const
            {
                glm::vec4 Hx = glm::vec4(const_cast<hknpTransposedFourAabbs8*>(this)->GetHighX()[0], const_cast<hknpTransposedFourAabbs8*>(this)->GetHighX()[1], const_cast<hknpTransposedFourAabbs8*>(this)->GetHighX()[2], const_cast<hknpTransposedFourAabbs8*>(this)->GetHighX()[3]);
                return glm::greaterThan(Hx, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
            }

            inline bool IsAabbSet(int index) const
            {
                return ActiveMask()[index];
            }

            hknpAabb8 GetAabb(int index)
            {
                assert(index >= 0 && index < 4 && "Index out-of-bounds");

                uint32_t min = 0;
                uint32_t max = 0;

                uint8_t* aabbMin = reinterpret_cast<uint8_t*>(&min);
                uint8_t* aabbMax = reinterpret_cast<uint8_t*>(&max);

                aabbMin[0] = GetLowX()[index];
                aabbMin[1] = GetLowY()[index];
                aabbMin[2] = GetLowZ()[index];

                aabbMax[0] = GetHighX()[index];
                aabbMax[1] = GetHighY()[index];
                aabbMax[2] = GetHighZ()[index];

                return hknpAabb8(min, max);
            }

            hknpAabb8 GetCompoundAabb()
            {
                std::pair<uint32_t, uint32_t> result;
                uint8_t* resultMin = reinterpret_cast<uint8_t*>(&result.first);
                uint8_t* resultMax = reinterpret_cast<uint8_t*>(&result.second);

                resultMin[0] = GetLowX()[0];
                resultMax[0] = GetHighX()[0];
                resultMin[1] = GetLowY()[0];
                resultMax[1] = GetHighY()[0];
                resultMin[2] = GetLowZ()[0];
                resultMax[2] = GetHighZ()[0];

                for (int i = 1; i < 4; i++)
                {
                    resultMin[0] = std::min(GetLowX()[i], resultMin[0]);
                    resultMax[0] = std::max(GetHighX()[i], resultMax[0]);
                    resultMin[1] = std::min(GetLowY()[i], resultMin[1]);
                    resultMax[1] = std::max(GetHighY()[i], resultMax[1]);
                    resultMin[2] = std::min(GetLowZ()[i], resultMin[2]);
                    resultMax[2] = std::max(GetHighZ()[i], resultMax[2]);
                }

                return hknpAabb8(result.first, result.second);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpTransposedFourAabbs8";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hknpAabb8TreeNode : public hkReadableWriteableObject
        {
            hknpTransposedFourAabbs8 mParent;
            hkUint8 mData[4];

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.ReadRawArray(&mData, sizeof(mData), 4, false);
                Reader.Seek(Jumpback + 28, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.WriteRawArray(&mData, sizeof(mData), 4, false);
                Writer.Seek(Jumpback + 28, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            void Clear()
            {
                mParent.SetEmpty();

                for (int i = 0; i < 4; i++)
                {
                    mData[i].mParent = 0;
                }
            }

            bool IsLeaf() const
            {
                return mData[2].mParent > mData[3].mParent;
            }

            inline bool IsChildValid(int index) const
            {
				return mParent.IsAabbSet(index);
            }

            inline uint8_t ActiveMask() const
            {
                const uint8_t* hx = reinterpret_cast<const uint8_t*>(&mParent.mHx);

                uint8_t mask = 0;
                if (hx[0] > 0) mask |= (1 << 0);
                if (hx[1] > 0) mask |= (1 << 1);
                if (hx[2] > 0) mask |= (1 << 2);
                if (hx[3] > 0) mask |= (1 << 3);

                return mask;
            }

            inline int CountActiveAabbs() const
            {
                uint8_t mask = ActiveMask();
                return std::popcount(mask);
            }

            bool IsEmpty() const
            {
                return !IsValid();
            }

            bool IsValid() const
            {
                return ActiveMask() > 0;
            }

            void SwapElements(int index1, int index2)
            {
                hknpAabb8 aabb1 = mParent.GetAabb(index1);
                hknpAabb8 aabb2 = mParent.GetAabb(index2);
                mParent.SetAabb(index1, aabb2);
                mParent.SetAabb(index2, aabb1);

                std::swap(mData[index1], mData[index2]);
            }

            void ConfigureAsLeafOrInternal(bool targetStateAsLeaf)
            {
                if (IsLeaf() != targetStateAsLeaf)
                {
                    if (IsChildValid(2) && IsChildValid(3))
                    {
                        SwapElements(2, 3);
                    }
                    else if (IsChildValid(2))
                    {
                        if (targetStateAsLeaf)
                        {
                            if (mData[2].mParent == 0)
                            {
                                SwapElements(1, 2);
                            }

                            mData[3].mParent = mData[2].mParent - 1;
                        }
                        else
                        {
                            mData[3].mParent= mData[2].mParent;
                        }
                    }
                    else
                    {
                        assert(targetStateAsLeaf);
                        mData[2].mParent = 1;
                    }
                }

                if (IsLeaf() != targetStateAsLeaf)
                {
                    const int8_t* hx = reinterpret_cast<const int8_t*>(&mParent.mHx);
					int8_t hxValues[4] = { hx[0], hx[1], hx[2], hx[3] };

                    bool a = IsChildValid(0);
                    bool b = IsChildValid(1);
                    bool c = IsChildValid(2);
                    bool d = IsChildValid(3);

                    application::util::Logger::Error("HavokClasses", "Could not set leaf state correctly, %u %u %u %u", hxValues[0], hxValues[1], hxValues[2], hxValues[3]);
                }

                assert(IsLeaf() == targetStateAsLeaf);
            }

            void CompactChildren(bool targetStateAsLeaf)
            {
                assert(!IsEmpty() && "hknpAabb8TreeNode was empty");

                int j = 0;
                for (int i = 0; i < 4; i++)
                {
                    if (!IsChildValid(i))
                    {
                        continue;
                    }

                    hknpAabb8 aabb = mParent.GetAabb(i);
                    mParent.SetAabb(j, aabb);
                    mData[j] = mData[i];
                    j++;
                }

                for (int i = j; i < 4; i++)
                {
                    mParent.SetAabb(i, hknpAabb8(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::lowest())); //invalid
                    mData[i].mParent = 0;
                }

                ConfigureAsLeafOrInternal(targetStateAsLeaf);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpAabb8TreeNode";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hknpMeshShape__GeometrySection__Primitive : public hkReadableWriteableObject
        {
            hkUint8 mAId;
            hkUint8 mBId;
            hkUint8 mCId;
            hkUint8 mDId;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                mAId.Read(Reader);
                mBId.Read(Reader);
                mCId.Read(Reader);
                mDId.Read(Reader);
                Reader.Seek(Jumpback + 4, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mAId.Skip(Writer);
                mBId.Skip(Writer);
                mCId.Skip(Writer);
                mDId.Skip(Writer);
                Writer.Seek(Jumpback + 4, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mAId.Write(Writer);
                mBId.Write(Writer);
                mCId.Write(Writer);
                mDId.Write(Writer);
            }

            bool IsTriangle() const
            {
                return mCId.mParent == mDId.mParent;
            }

            bool IsFlatConvexQuad() const
            {
                return !IsTriangle() && mBId.mParent > mDId.mParent;
            }

            void SetIsFlatConvexQuad(bool isFlatConvexQuad, bool& trianglesSwappedOut, int orderingOffsetsOut[2])
            {
                auto AreTrianglesEquivalent = [](hkUint8 a1Id, hkUint8 b1Id, hkUint8 c1Id, hkUint8 a2Id, hkUint8 b2Id, hkUint8 c2Id, int& orderingOffsetOut)
                    {
                        hkUint8 tri1[] = { a1Id, b1Id, c1Id };
                        hkUint8 tri2[] = { a2Id, b2Id, c2Id };

                        for (int offset = 0; offset <= 2; offset++)
                        {
                            bool match = true;
                            for (int i = 0; i < 3; i++)
                            {
                                if (tri1[i].mParent != tri2[(i + offset) % 3].mParent)
                                {
                                    match = false;
                                    break;
                                }
                            }

                            if (match)
                            {
                                orderingOffsetOut = offset;
                                return true;
                            }
                        }

                        orderingOffsetOut = -1;
                        return false;
                    };

                trianglesSwappedOut = false;

                if (this->IsFlatConvexQuad() == isFlatConvexQuad)
                {
                    return;
                }

                trianglesSwappedOut = true;

                hkUint8 firstTriangle[3] = { mAId, mCId, mDId };
                hkUint8 result[4];
                for (int offset = 0; offset <= 2; offset++)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        result[i] = firstTriangle[(i + offset) % 3];
                    }

                    result[3] = mBId;

                    if ((result[1].mParent > result[3].mParent) != isFlatConvexQuad)
                    {
                        continue;
                    }

                    int otherTriangleOffset;
                    if (!AreTrianglesEquivalent(result[0], result[2], result[3], mAId, mBId, mCId, otherTriangleOffset))
                    {
                        continue;
                    }

                    // Found ordering which satisfies the criteria
                    // Since the triangles are flipped, the first offset is for the second triangle

                    orderingOffsetsOut[1] = offset;
                    orderingOffsetsOut[0] = otherTriangleOffset;

                    mAId = result[0];
                    mBId = result[1];
                    mCId = result[2];
                    mDId = result[3];

                    break;
                }

                assert(this->IsFlatConvexQuad() == isFlatConvexQuad);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpMeshShape::GeometrySection::Primitive";
            }
        };



        struct hkFloat3 : public hkReadableWriteableObject
        {
            float mX = 0.0f;
            float mY = 0.0f;
            float mZ = 0.0f;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.ReadStruct(&mX, sizeof(mX));
                Reader.ReadStruct(&mY, sizeof(mY));
                Reader.ReadStruct(&mZ, sizeof(mZ));
                Reader.Seek(Jumpback + 12, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mX), sizeof(mX));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mY), sizeof(mY));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mZ), sizeof(mZ));
                Writer.Seek(Jumpback + 12, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkFloat3";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hkInt16 : public hkReadableWriteableObject
        {
            short mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkInt16";
            }
        };



        struct hknpMeshShape__GeometrySection : public hkReadableWriteableObject
        {
            hkRelArrayView<hknpAabb8TreeNode, int> mSectionBvh;
            hkRelArrayView<hknpMeshShape__GeometrySection__Primitive, int> mPrimitives;
            hkRelArrayView<hkUint8, int> mVertexBuffer;
            hkRelArrayView<hkUint8, int> mInteriorPrimitiveBitField;
            hkUint32 mSectionOffset[3];
            hkFloat3 mBitScale8Inv;
            hkInt16 mBitOffset[3];

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mSectionBvh.Read(Reader);
                mPrimitives.Read(Reader);
                mVertexBuffer.Read(Reader);
                mInteriorPrimitiveBitField.Read(Reader);
                Reader.ReadRawArray(&mSectionOffset, sizeof(mSectionOffset), 3, false);
                mBitScale8Inv.Read(Reader);
                Reader.ReadRawArray(&mBitOffset, sizeof(mBitOffset), 3, false);
                Reader.Seek(Jumpback + 64, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mSectionBvh.Skip(Writer);
                mPrimitives.Skip(Writer);
                mVertexBuffer.Skip(Writer);
                mInteriorPrimitiveBitField.Skip(Writer);
                Writer.WriteRawArray(&mSectionOffset, sizeof(mSectionOffset), 3, false);
                mBitScale8Inv.Skip(Writer);
                Writer.WriteRawArray(&mBitOffset, sizeof(mBitOffset), 3, false);
                Writer.Seek(Jumpback + 64, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            //DO NOT USE
            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mSectionBvh.Write(Writer);
                mPrimitives.Write(Writer);
                mVertexBuffer.Write(Writer);
                mInteriorPrimitiveBitField.Write(Writer);
                mBitScale8Inv.Write(Writer);
            }

            void MarkTriangleAsInterior(int primitiveIndex, int triangleIndex)
            {
                hkUint8* byte = &mInteriorPrimitiveBitField.mElements[2 * primitiveIndex / 8];
                uint8_t mask = 1 << ((2 * primitiveIndex % 8) + triangleIndex);
                byte->mParent |= mask;
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpMeshShape::GeometrySection";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }

            unsigned int GetVertexCount()
            {
                return mVertexBuffer.mElements.size() / 6;
            }
        };


        struct hkRelArrayViewGeometrySection : public hkReadableWriteableObject
        {
            int mOffset = 0;
            int mSize = 0;

            std::vector<hknpMeshShape__GeometrySection> mElements;
            unsigned long long mParentOffset = std::numeric_limits<unsigned long long>::max();
            unsigned long long mLastElementOffset = std::numeric_limits<unsigned long long>::max();

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(sizeof(mOffset));
                Reader.ReadStruct(&mOffset, sizeof(mOffset));
                Reader.ReadStruct(&mSize, sizeof(mSize));


                uint32_t Jumback = Reader.GetPosition();
                if (mOffset > 0)
                {
                    Reader.Seek(mOffset - 8, application::util::BinaryVectorReader::Position::Current);
                    mElements.resize(mSize);
                    for (int i = 0; i < mSize; i++)
                    {
                        hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&mElements[i]);
                        if (BaseObj)
                        {
                            BaseObj->Read(Reader);
                        }
                        else
                        {
                            application::util::Logger::Warning("HavokClasses::hkRelArrayViewGeometrySection", "Cannot cast to hkReadableWriteableObject");
                        }
                    }
                }
                Reader.Seek(Jumback, application::util::BinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(sizeof(mOffset));
                mParentOffset = Writer.GetPosition();
                Writer.Seek(sizeof(mOffset) + 4, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkRelArrayView", "mParentOffset was not set");
                    return;
                }


                if (!mElements.empty()) Writer.Align(16);
                unsigned long long Offset = Writer.GetPosition();

                Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                mOffset = !mElements.empty() ? (Offset - Writer.GetPosition()) : 0;
                mSize = mElements.size();
                Writer.WriteInteger(mOffset, sizeof(mOffset));
                Writer.WriteInteger((int32_t)mSize, sizeof(int32_t));

                Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);
                if (mIsItem && !mElements.empty()) Writer.WriteItemCallback(this, mElements.size(), &mElements[0]);

                for (hknpMeshShape__GeometrySection& Element : mElements)
                {
                    hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
                    if (BaseObj)
                    {
                        BaseObj->Skip(Writer);
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkRelArrayView", "Cannot cast to hkReadableWriteableObject");
                    }
                }

                for (hknpMeshShape__GeometrySection& Element : mElements)
                {
                    Element.mSectionBvh.Write(Writer);
                }
                for (hknpMeshShape__GeometrySection& Element : mElements)
                {
                    Element.mPrimitives.Write(Writer);
                }
                for (hknpMeshShape__GeometrySection& Element : mElements)
                {
                    Element.mVertexBuffer.Write(Writer);
                }
                for (hknpMeshShape__GeometrySection& Element : mElements)
                {
                    Element.mInteriorPrimitiveBitField.Write(Writer);
                }

                mLastElementOffset = Writer.GetPosition();
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkRelArrayView";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }

            void ForEveryElement(const std::function<void(hknpMeshShape__GeometrySection& Element)>& Func)
            {
                for (hknpMeshShape__GeometrySection& Element : mElements)
                {
                    Func(Element);
                }
            }
        };


        struct hknpMeshShape__ShapeTagTableEntry : public hkReadableWriteableObject
        {
            hkUint32 mMeshPrimitiveKey;
            hkUint16 mShapeTag;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mMeshPrimitiveKey.Read(Reader);
                mShapeTag.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mMeshPrimitiveKey.Skip(Writer);
                mShapeTag.Skip(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mMeshPrimitiveKey.Write(Writer);
                mShapeTag.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpMeshShape::ShapeTagTableEntry";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hkcdFourAabb : public hkReadableWriteableObject
        {
            hkVector4 mLx;
            hkVector4 mHx;
            hkVector4 mLy;
            hkVector4 mHy;
            hkVector4 mLz;
            hkVector4 mHz;

            hkcdFourAabb() = default;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mLx.Read(Reader);
                mHx.Read(Reader);
                mLy.Read(Reader);
                mHy.Read(Reader);
                mLz.Read(Reader);
                mHz.Read(Reader);
                Reader.Seek(Jumpback + 96, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mLx.Skip(Writer);
                mHx.Skip(Writer);
                mLy.Skip(Writer);
                mHy.Skip(Writer);
                mLz.Skip(Writer);
                mHz.Skip(Writer);
                Writer.Seek(Jumpback + 96, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mLx.Write(Writer);
                mHx.Write(Writer);
                mLy.Write(Writer);
                mHy.Write(Writer);
                mLz.Write(Writer);
                mHz.Write(Writer);
            }

            inline void SetEmpty()
            {
                float maxVal = std::numeric_limits<float>::max();
                float minVal = std::numeric_limits<float>::lowest();

                std::fill_n(mLx.mParent.mData, 4, maxVal);
                std::fill_n(mLy.mParent.mData, 4, maxVal);
                std::fill_n(mLz.mParent.mData, 4, maxVal);

                std::fill_n(mHx.mParent.mData, 4, minVal);
                std::fill_n(mHy.mParent.mData, 4, minVal);
                std::fill_n(mHz.mParent.mData, 4, minVal);
            }

            inline void GetAabb(int index, starlight_physics::common::hkAabb* aabb) const
            {
                aabb->mMin = glm::vec4(mLx.mParent.mData[index], mLy.mParent.mData[index], mLz.mParent.mData[index], 0.0f);
                aabb->mMax = glm::vec4(mHx.mParent.mData[index], mHy.mParent.mData[index], mHz.mParent.mData[index], 0.0f);
            }

            inline void GetCompoundAabb(starlight_physics::common::hkAabb* aabb) const
            {
                float maxVal = std::numeric_limits<float>::max();
                float minVal = std::numeric_limits<float>::lowest();

                glm::vec3 compoundMin(maxVal);
                glm::vec3 compoundMax(minVal);

                bool anyActive = false;

                for (int i = 0; i < 4; ++i)
                {
                    // An AABB is active/valid if Min <= Max
                    if (mLx.mParent.mData[i] <= mHx.mParent.mData[i])
                    {
                        anyActive = true;

                        // Update Compound Min
                        compoundMin.x = std::min(compoundMin.x, mLx.mParent.mData[i]);
                        compoundMin.y = std::min(compoundMin.y, mLy.mParent.mData[i]);
                        compoundMin.z = std::min(compoundMin.z, mLz.mParent.mData[i]);

                        // Update Compound Max
                        compoundMax.x = std::max(compoundMax.x, mHx.mParent.mData[i]);
                        compoundMax.y = std::max(compoundMax.y, mHy.mParent.mData[i]);
                        compoundMax.z = std::max(compoundMax.z, mHz.mParent.mData[i]);
                    }
                }

                if (!anyActive)
                {
                    aabb->mMin = glm::vec4(maxVal, maxVal, maxVal, 1.0f);
                    aabb->mMax = glm::vec4(minVal, minVal, minVal, 1.0f);
                }
                else
                {
                    aabb->mMin = glm::vec4(compoundMin, 1.0f);
                    aabb->mMax = glm::vec4(compoundMax, 1.0f);
                }
            }

            inline void SetAabb(int index, const starlight_physics::common::hkAabb& aabb)
            {
				mLx.mParent.mData[index] = aabb.mMin.x;
				mLy.mParent.mData[index] = aabb.mMin.y;
				mLz.mParent.mData[index] = aabb.mMin.z;

                mHx.mParent.mData[index] = aabb.mMax.x;
                mHy.mParent.mData[index] = aabb.mMax.y;
                mHz.mParent.mData[index] = aabb.mMax.z;
            }

            inline glm::bvec4 ActiveMask()
            {
				glm::vec4 Lx = glm::vec4(mLx.mParent.mData[0], mLx.mParent.mData[1], mLx.mParent.mData[2], mLx.mParent.mData[3]);
				glm::vec4 Hx = glm::vec4(mHx.mParent.mData[0], mHx.mParent.mData[1], mHx.mParent.mData[2], mHx.mParent.mData[3]);
                return glm::lessThanEqual(Lx, Hx);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdFourAabb";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        struct hkcdSimdTree__Node : public hkReadableWriteableObject
        {
            hkcdFourAabb mParent;
            hkUint32 mData[4];
            bool mIsLeaf = false;
            bool mIsActive = false;

            hkcdSimdTree__Node() = default;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.ReadRawArray(&mData, sizeof(mData), 4, false);
                Reader.ReadStruct(&mIsLeaf, sizeof(mIsLeaf));
                Reader.ReadStruct(&mIsActive, sizeof(mIsActive));
                Reader.Seek(Jumpback + 128, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.WriteRawArray(&mData, sizeof(mData), 4, false);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mIsLeaf), sizeof(mIsLeaf));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mIsActive), sizeof(mIsActive));
                Writer.Seek(Jumpback + 128, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            glm::u32vec4 LoadData()
            {
                glm::u32vec4 result;

                result.x = mData[0].mParent;
                result.y = mData[1].mParent;
                result.z = mData[2].mParent;
                result.w = mData[3].mParent;

                return result;
            }

            int CountLeaves()
            {
                assert(mIsLeaf && "Count leaves called on internal node!");

                glm::bvec4 mask = mParent.ActiveMask();

                // Count true values in the boolean vector
                int count = 0;
                if (mask.x) count++;
                if (mask.y) count++;
                if (mask.z) count++;
                if (mask.w) count++;

                return count;
            }

            void ClearLeafData(int index)
            {
                mData[index].mParent = (uint32_t)-1;
            }

            void ClearInternalData(int index)
            {
                mData[index].mParent = 0;
            }

            void ClearAabb(int index)
            {
                const float emptyMin = std::numeric_limits<float>::max();
                const float emptyMax = -std::numeric_limits<float>::max();
                mParent.mLx.mParent.mData[index] = emptyMin;
                mParent.mHx.mParent.mData[index] = emptyMax;
                mParent.mLy.mParent.mData[index] = emptyMin;
                mParent.mHy.mParent.mData[index] = emptyMax;
                mParent.mLz.mParent.mData[index] = emptyMin;
                mParent.mHz.mParent.mData[index] = emptyMax;
            }

            inline bool IsLeafValid(int index) const
            {
                assert(mIsLeaf && "isLeafValid called on internal node!");
                return mData[index].mParent != (uint32_t)-1;
            }

            inline void SetChildData(int index, uint32_t data) { mData[index].mParent = data; }

            inline void ClearLeavesData()
            {
                ClearLeafData(0);
                ClearLeafData(1);
                ClearLeafData(2);
                ClearLeafData(3);
            }

            inline void ClearInternalsData()
            {
                ClearInternalData(0);
                ClearInternalData(1);
                ClearInternalData(2);
                ClearInternalData(3);
            }

            inline void ClearInternal()
            {
                mParent.SetEmpty();
                ClearInternalsData();
                mIsLeaf = false;
                mIsActive = false;
            }

            inline void ClearLeaf()
            {
                mParent.SetEmpty();
                ClearLeavesData();
                mIsLeaf = true;
            }

            inline bool IsInternalValid(int index) const
            {
                assert(!mIsLeaf && "isNodeValid called on leaf node!");
                return mData[index].mParent != 0;
            }

            inline bool IsDataValid(int index) const
            {
                if (mIsLeaf)
                {
                    return IsLeafValid(index);
                }
                else
                {
                    return IsInternalValid(index);
                }
            }

            inline void SetChild(int index, const starlight_physics::common::hkAabb& aabb, uint32_t data)
            {
                mParent.SetAabb(index, aabb);
                mData[index].mParent = data;
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdSimdTree::Node";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        struct hkcdSimdTree : public hkReadableWriteableObject
        {
            hkRelArray<hkcdSimdTree__Node> mNodes;
            bool mIsCompact = false;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mNodes.Read(Reader);
                Reader.ReadStruct(&mIsCompact, sizeof(mIsCompact));
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mNodes.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mIsCompact), sizeof(mIsCompact));
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mNodes.Write(Writer);
            }

            inline void Clear()
            {
                mNodes.mElements.resize(2);
                mNodes.mElements[0].ClearInternal();
                mNodes.mElements[1].ClearInternal();

                mIsCompact = true;
            }

            inline bool IsEmpty()
            {
                return !glm::any(mNodes.mElements[1].mParent.ActiveMask());
            }

            void SortByAabbsSize()
            {
                assert(mIsCompact);

                struct SortData
                {
                    float mAabbVolume;
                    int mOriginalIndex;

                    bool operator<(const SortData& rhs) const { return this->mAabbVolume > rhs.mAabbVolume; }
                };

                // tmp data.
                glm::u32vec4 data;
                starlight_physics::common::hkAabb aabbs[4];

                SortData sortData[4];

                // Rotate aabbs
                for (hkcdSimdTree__Node& node : mNodes.mElements)
                {
                    data = node.LoadData();
                    glm::vec4 volumes = (node.mParent.mHx.ToVec4() - node.mParent.mLx.ToVec4()) * (node.mParent.mHy.ToVec4() - node.mParent.mLy.ToVec4()) * (node.mParent.mHz.ToVec4() - node.mParent.mLz.ToVec4());

                    int activeCount = 0;
                    for (int i = 0; i < 4; i++)
                    {
                        if (node.IsDataValid(i))
                        {
                            node.mParent.GetAabb(i, &aabbs[i]);
                            sortData[i].mAabbVolume = volumes[i];
                            sortData[i].mOriginalIndex = i;
                            activeCount++;
                        }
                    }

                    auto insertionSort = [](SortData* storage, int count)
                        {
                            for (int i = 1; i < count; ++i) {
                                SortData key = storage[i];
                                int j = i - 1;

                                while (j >= 0 && key < storage[j])
                                {
                                    storage[j + 1] = storage[j];
                                    j = j - 1;
                                }
                                storage[j + 1] = key;
                            }
                        };

                    insertionSort(sortData, activeCount);

                    for (int i = 0; i < activeCount; i++)
                    {
                        node.mData[i].mParent = data[sortData[i].mOriginalIndex];
                        node.mParent.SetAabb(i, aabbs[sortData[i].mOriginalIndex]);
                    }
                }
            }

            bool RecurseRefitLeaf(std::vector<uint32_t>& path, hkcdSimdTree__Node* node, const glm::vec4& currentDataPoint, const starlight_physics::common::hkAabb& newAabb)
            {
                glm::vec4 x = glm::vec4(currentDataPoint.x);
                glm::bvec4 cx = glm::greaterThanEqual(x, glm::vec4(node->mParent.mLx.mParent.mData[0], node->mParent.mLx.mParent.mData[1], node->mParent.mLx.mParent.mData[2], node->mParent.mLx.mParent.mData[3])) && glm::lessThanEqual(x, glm::vec4(node->mParent.mHx.mParent.mData[0], node->mParent.mHx.mParent.mData[1], node->mParent.mHx.mParent.mData[2], node->mParent.mHx.mParent.mData[3]));

                glm::vec4 y = glm::vec4(currentDataPoint.y);
                glm::bvec4 cy = glm::greaterThanEqual(y, glm::vec4(node->mParent.mLy.mParent.mData[0], node->mParent.mLy.mParent.mData[1], node->mParent.mLy.mParent.mData[2], node->mParent.mLy.mParent.mData[3])) && glm::lessThanEqual(y, glm::vec4(node->mParent.mHy.mParent.mData[0], node->mParent.mHy.mParent.mData[1], node->mParent.mHy.mParent.mData[2], node->mParent.mHy.mParent.mData[3]));

                glm::vec4 z = glm::vec4(currentDataPoint.z);
                glm::bvec4 cz = glm::greaterThanEqual(z, glm::vec4(node->mParent.mLz.mParent.mData[0], node->mParent.mLz.mParent.mData[1], node->mParent.mLz.mParent.mData[2], node->mParent.mLz.mParent.mData[3])) && glm::lessThanEqual(z, glm::vec4(node->mParent.mHz.mParent.mData[0], node->mParent.mHz.mParent.mData[1], node->mParent.mHz.mParent.mData[2], node->mParent.mHz.mParent.mData[3]));

                const glm::bvec4 c = cx & cy & cz;

                if (glm::any(c))
                {
                    const uint32_t key = reinterpret_cast<const uint32_t*>(&currentDataPoint[0])[3] & 0x00ffffff;

                    for (int i = 0; i < 4; ++i)
                    {
                        if (c[i])
                        {
                            if (node->mIsLeaf && node->mData[i].mParent == key)
                            {
                                node->mParent.SetAabb(i, newAabb);

                                for (int pathIndex = (int)path.size() - 1; pathIndex >= 0; --pathIndex)
                                {
                                    const uint32_t parentId = path[pathIndex];

                                    const int childIndex = (int)(parentId & 3);

                                    auto* parentNode = &mNodes.mElements[parentId >> 2];

                                    starlight_physics::common::hkAabb compoundAabb;
                                    node->mParent.GetCompoundAabb(&compoundAabb);

                                    parentNode->mParent.SetAabb(childIndex, compoundAabb);

                                    node = parentNode;
                                }
                                return true;
                            }
                            else if (!node->mIsLeaf)
                            {
                                uint32_t nodeIndex = (uint32_t)(node - &mNodes.mElements[0]);
                                path.push_back((nodeIndex << 2) | (uint32_t)i);

                                auto* nextNode = &mNodes.mElements[node->mData[i].mParent];
                                if (RecurseRefitLeaf(path, nextNode, currentDataPoint, newAabb))
                                {
                                    return true;
                                }

                                path.pop_back();
                            }
                        }
                    }
                }

                return false;
            }

            bool RefitLeaf(const glm::vec4& dataPoint, const starlight_physics::common::hkAabb& newAabb)
            {
                std::vector<uint32_t> path;
                path.reserve(32);
                return RecurseRefitLeaf(path, &mNodes.mElements[1], dataPoint, newAabb);
            }

            inline void GetDomain(starlight_physics::common::hkAabb* aabb) const
            {
                const auto& root = mNodes.mElements[1];
                aabb->mMin = glm::vec4(
                    glm::min(glm::min(root.mParent.mLx.mParent.mData[0], root.mParent.mLx.mParent.mData[1]), glm::min(root.mParent.mLx.mParent.mData[2], root.mParent.mLx.mParent.mData[3])),
                    glm::min(glm::min(root.mParent.mLy.mParent.mData[0], root.mParent.mLy.mParent.mData[1]), glm::min(root.mParent.mLy.mParent.mData[2], root.mParent.mLy.mParent.mData[3])),
                    glm::min(glm::min(root.mParent.mLz.mParent.mData[0], root.mParent.mLz.mParent.mData[1]), glm::min(root.mParent.mLz.mParent.mData[2], root.mParent.mLz.mParent.mData[3])),
                    0.0f);
                aabb->mMax = glm::vec4(
                    glm::max(glm::max(root.mParent.mHx.mParent.mData[0], root.mParent.mHx.mParent.mData[1]), glm::max(root.mParent.mHx.mParent.mData[2], root.mParent.mHx.mParent.mData[3])),
                    glm::max(glm::max(root.mParent.mHy.mParent.mData[0], root.mParent.mHy.mParent.mData[1]), glm::max(root.mParent.mHy.mParent.mData[2], root.mParent.mHy.mParent.mData[3])),
                    glm::max(glm::max(root.mParent.mHz.mParent.mData[0], root.mParent.mHz.mParent.mData[1]), glm::max(root.mParent.mHz.mParent.mData[2], root.mParent.mHz.mParent.mData[3])),
                    0.0f);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdSimdTree";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hknpMeshShapePrimitiveMapping : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkRelArray<hkUint32> mSectionStart;
            hkRelArray<unsigned int> mBitString;
            hkUint32 mBitsPerEntry;
            hkUint32 mTriangleIndexBitMask;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mSectionStart.Read(Reader);
                mBitString.Read(Reader);
                mBitsPerEntry.Read(Reader);
                mTriangleIndexBitMask.Read(Reader);
                Reader.Seek(Jumpback + 64, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mSectionStart.Skip(Writer);
                mBitString.Skip(Writer);
                mBitsPerEntry.Skip(Writer);
                mTriangleIndexBitMask.Skip(Writer);
                Writer.Seek(Jumpback + 64, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mSectionStart.Write(Writer);
                mBitString.Write(Writer);
                mBitsPerEntry.Write(Writer);
                mTriangleIndexBitMask.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpMeshShapePrimitiveMapping";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hknpMeshShape : public hkReadableWriteableObject
        {
            hknpCompositeShape mParent;
            hknpMeshShapeVertexConversionUtil mVertexConversionUtil;
            hkRelArrayView<hknpMeshShape__ShapeTagTableEntry, int> mShapeTagTable;
            hkcdSimdTree mTopLevelTree;
            hkRelArrayViewGeometrySection mGeometrySections;
            hkRelPtr<hknpMeshShapePrimitiveMapping> mPrimitiveMapping;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mVertexConversionUtil.Read(Reader);
                mShapeTagTable.Read(Reader);
                mTopLevelTree.Read(Reader);
                mGeometrySections.Read(Reader);
                mPrimitiveMapping.Read(Reader);
                Reader.Seek(Jumpback + 160, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mVertexConversionUtil.Skip(Writer);
                mShapeTagTable.Skip(Writer);
                mTopLevelTree.Skip(Writer);
                mGeometrySections.Skip(Writer);
                mPrimitiveMapping.Skip(Writer);
                Writer.Seek(Jumpback + 160, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mVertexConversionUtil.Write(Writer);
                mShapeTagTable.Write(Writer);
                mTopLevelTree.Write(Writer);
                mGeometrySections.Write(Writer);
                mPrimitiveMapping.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpMeshShape";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };






        //hknpCompoundShape stuff
        struct hkQuaternionf : public hkReadableWriteableObject
        {
            hkVector4f mVec;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mVec.Read(Reader);
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mVec.Skip(Writer);
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mVec.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkQuaternionf";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        struct hkQuaternion : public hkReadableWriteableObject
        {
            hkQuaternionf mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                mParent.Read(Reader);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkQuaternion";
            }
        };



        struct hknpShapeInstance : public hkReadableWriteableObject
        {
            hkQuaternion mRotation;
            hkFloat3 mTranslation;
            hkUint8 mFlags;
            hkUint8 mPadding[3];
            hkFloat3 mScale;
            hkUint16 mShapeTag;
            hkUint16 mLeafIndex;
            hkRelPtr<hknpShape> mShape;
            hkUint64 mUserData;

            uint64_t mBinaryPosition = 0;

            void ApplyPatch(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer)
            {
                //+32 is the header of the tag file
                Writer.Seek(mBinaryPosition + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                mRotation.Skip(Writer);
                mTranslation.Skip(Writer);
                mFlags.Skip(Writer);
                Writer.WriteRawArray(&mPadding, sizeof(mPadding), 3, false);
                mScale.Skip(Writer);
                mShapeTag.Skip(Writer);
                mLeafIndex.Skip(Writer);
                mShape.mOffset.Skip(Writer); //Directly write raw offset
                mUserData.Skip(Writer);
            }

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mBinaryPosition = Jumpback;

                mRotation.Read(Reader);
                mTranslation.Read(Reader);
                mFlags.Read(Reader);
                Reader.ReadRawArray(&mPadding, sizeof(mPadding), 3, false);
                mScale.Read(Reader);
                mShapeTag.Read(Reader);
                mLeafIndex.Read(Reader);
                mShape.Read(Reader);
                mUserData.Read(Reader);
                Reader.Seek(Jumpback + 64, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mRotation.Skip(Writer);
                mTranslation.Skip(Writer);
                mFlags.Skip(Writer);
                Writer.WriteRawArray(&mPadding, sizeof(mPadding), 3, false);
                mScale.Skip(Writer);
                mShapeTag.Skip(Writer);
                mLeafIndex.Skip(Writer);
                mShape.Skip(Writer);
                mUserData.Skip(Writer);
                Writer.Seek(Jumpback + 64, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mRotation.Write(Writer);
                mTranslation.Write(Writer);
                mFlags.Write(Writer);
                //mPadding.Write(Writer);
                mScale.Write(Writer);
                mShapeTag.Write(Writer);
                mLeafIndex.Write(Writer);
                mShape.Write(Writer);
                mUserData.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpShapeInstance";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        template<typename tVALUE_TYPE>
        struct hkFreeListArrayElement : public hkReadableWriteableObject
        {
            tVALUE_TYPE mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                mParent.Read(Reader);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkFreeListArrayElement";
            }
        };



        template<typename tVALUE_TYPE>
        struct hkFreeListRelArrayStorage : public hkReadableWriteableObject
        {
            hkRelArray<tVALUE_TYPE> mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkFreeListRelArrayStorage";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkInt32 : public hkReadableWriteableObject
        {
            int mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkInt32";
            }
        };



        template<int vGROWTH, typename tSTORAGE>
        struct hkFreeListArray : public hkReadableWriteableObject
        {
            tSTORAGE mElements;
            hkInt32 mFirstFree;
            hkUint32 mNumAllocated;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mElements.Read(Reader);
                mFirstFree.Read(Reader);
                mNumAllocated.Read(Reader);
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mElements.Skip(Writer);
                mFirstFree.Skip(Writer);
                mNumAllocated.Skip(Writer);
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mElements.Write(Writer);
                mFirstFree.Write(Writer);
                mNumAllocated.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkFreeListArray";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        template<typename tVALUE_TYPE, int vGROWTH>
        struct hkFreeListRelArray : public hkReadableWriteableObject
        {
            hkFreeListArray<vGROWTH, hkFreeListRelArrayStorage<hkFreeListArrayElement<tVALUE_TYPE>>> mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkFreeListRelArray";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkReflect__Detail__Opaque : public hkReadableWriteableObject
        {
            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkReflect::Detail::Opaque";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 1;
            }
        };



        struct hknpShapeSignals : public hkReadableWriteableObject
        {
            hkRefPtr<hkReflect__Detail__Opaque> mShapeMutated;
            hkRefPtr<hkReflect__Detail__Opaque> mShapeDestroyed;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mShapeMutated.Read(Reader);
                mShapeDestroyed.Read(Reader);
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mShapeMutated.Skip(Writer);
                mShapeDestroyed.Skip(Writer);
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mShapeMutated.Write(Writer);
                mShapeDestroyed.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpShapeSignals";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkAabb : public hkReadableWriteableObject
        {
            hkVector4 mMin;
            hkVector4 mMax;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mMin.Read(Reader);
                mMax.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mMin.Skip(Writer);
                mMax.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mMin.Write(Writer);
                mMax.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkAabb";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        struct hknpCompoundShape__VelocityInfo : public hkReadableWriteableObject
        {
            hkVector4 mLinearVelocity;
            hkVector4 mAngularVelocity;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mLinearVelocity.Read(Reader);
                mAngularVelocity.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mLinearVelocity.Skip(Writer);
                mAngularVelocity.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mLinearVelocity.Write(Writer);
                mAngularVelocity.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpCompoundShape::VelocityInfo";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        struct hkBool : public hkReadableWriteableObject
        {
            bool mBool = false;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.ReadStruct(&mBool, sizeof(mBool));
                Reader.Seek(Jumpback + 1, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mBool), sizeof(mBool));
                Writer.Seek(Jumpback + 1, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkBool";
            }
        };



        struct hkcdDynamicTree__Codec32 : public hkReadableWriteableObject
        {
            hkAabb mAabb;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mAabb.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mAabb.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mAabb.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdDynamicTree::Codec32";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        struct hkcdDynamicTree__AnisotropicMetric : public hkReadableWriteableObject
        {

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.Seek(Jumpback + 1, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.Seek(Jumpback + 1, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdDynamicTree::AnisotropicMetric";
            }
        };



        template<int vGROWTH, typename tMETRIC, typename tCODEC>
        struct hkcdDynamicTree__DynamicStorage : public hkReadableWriteableObject
        {
            tMETRIC mParent;
            hkRelArray<tCODEC> mNodes;
            unsigned short mFirstFree = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mNodes.Read(Reader);
                Reader.ReadStruct(&mFirstFree, sizeof(mFirstFree));
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mNodes.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mFirstFree), sizeof(mFirstFree));
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mNodes.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdDynamicTree::DynamicStorage";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        template<typename tCODEC>
        struct hkcdDynamicTree__DefaultDynamicStorage : public hkReadableWriteableObject
        {
            hkcdDynamicTree__DynamicStorage<0, hkcdDynamicTree__AnisotropicMetric, hkcdDynamicTree__Codec32> mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdDynamicTree::DefaultDynamicStorage";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkcdDynamicTree__DynamicStorage16 : public hkReadableWriteableObject
        {
            hkcdDynamicTree__DefaultDynamicStorage<hkcdDynamicTree__Codec32> mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                mParent.Read(Reader);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdDynamicTree::DynamicStorage16";
            }
        };



        template<typename tSTORAGE>
        struct hkcdDynamicTree__Tree : public hkReadableWriteableObject
        {
            hkcdDynamicTree__DynamicStorage16 mParent;
            hkUint32 mNumLeaves;
            hkUint32 mPath;
            unsigned short mRoot = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mNumLeaves.Read(Reader);
                mPath.Read(Reader);
                Reader.ReadStruct(&mRoot, sizeof(mRoot));
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mNumLeaves.Skip(Writer);
                mPath.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mRoot), sizeof(mRoot));
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mNumLeaves.Write(Writer);
                mPath.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdDynamicTree::Tree";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkcdDynamicTree__DefaultTree32 : public hkReadableWriteableObject
        {
            hkcdDynamicTree__Tree<hkcdDynamicTree__DynamicStorage16> mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                mParent.Read(Reader);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdDynamicTree::DefaultTree32";
            }
        };



        struct hknpCompoundShapeCdDynamicTree : public hkReadableWriteableObject
        {
            hkcdDynamicTree__DefaultTree32 mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpCompoundShapeCdDynamicTree";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        enum class hknpCompoundShapeBoundingVolumeType__Enum : int
        {
            NONE = 0,
            AABB_TREE = 1,
            SIMD_TREE = 2
        };



        struct hknpCompoundShapeData : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hknpCompoundShapeCdDynamicTree mAabbTree;
            hkcdSimdTree mSimdTree;
            hkEnum<hknpCompoundShapeBoundingVolumeType__Enum, hkUint8> mType;
            bool mEnableSAH3 = false;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mAabbTree.Read(Reader);
                mSimdTree.Read(Reader);
                mType.Read(Reader);
                Reader.ReadStruct(&mEnableSAH3, sizeof(mEnableSAH3));
                Reader.Seek(Jumpback + 88, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mAabbTree.Skip(Writer);
                mSimdTree.Skip(Writer);
                mType.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mEnableSAH3), sizeof(mEnableSAH3));
                Writer.Seek(Jumpback + 88, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mAabbTree.Write(Writer);
                mSimdTree.Write(Writer);
                mType.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpCompoundShapeData";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hknpCompoundShape : public hkReadableWriteableObject
        {
            hknpCompositeShape mParent;
            hkFreeListRelArray<hknpShapeInstance, 8> mInstances;
            hkRelArray<hknpCompoundShape__VelocityInfo> mInstanceVelocities;
            hkAabb mAabb;
            hkReal mBoundingRadius;
            hkBool mIsMutable;
            int mEstimatedNumShapeKeys = 0;
            hknpShapeSignals mMutationSignals;
            hknpCompoundShapeData mBoundingVolumeData;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mInstances.Read(Reader);
                mInstanceVelocities.Read(Reader);
                mAabb.Read(Reader);
                mBoundingRadius.Read(Reader);
                mIsMutable.Read(Reader);
                Reader.ReadStruct(&mEstimatedNumShapeKeys, sizeof(mEstimatedNumShapeKeys));
                mMutationSignals.Read(Reader);
                mBoundingVolumeData.Read(Reader);
                Reader.Seek(Jumpback + 272, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mInstances.Skip(Writer);
                mInstanceVelocities.Skip(Writer);
                mAabb.Skip(Writer);
                mBoundingRadius.Skip(Writer);
                mIsMutable.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mEstimatedNumShapeKeys), sizeof(mEstimatedNumShapeKeys));
                mMutationSignals.Skip(Writer);
                mBoundingVolumeData.Skip(Writer);
                Writer.Seek(Jumpback + 272, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mInstances.Write(Writer);
                mInstanceVelocities.Write(Writer);
                mAabb.Write(Writer);
                mBoundingRadius.Write(Writer);
                mIsMutable.Write(Writer);
                mMutationSignals.Write(Writer);
                mBoundingVolumeData.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hknpCompoundShape";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        template<typename tTYPE>
        struct hkaiIndex : public hkReadableWriteableObject
        {
            tTYPE mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                mParent.Read(Reader);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiIndex";
            }
        };



        struct hkaiNavMesh__Face : public hkReadableWriteableObject
        {
            hkaiIndex<hkInt32> mStartEdgeIndex;
            hkaiIndex<hkInt32> mStartUserEdgeIndex;
            hkInt16 mNumEdges;
            hkInt16 mNumUserEdges;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mStartEdgeIndex.Read(Reader);
                mStartUserEdgeIndex.Read(Reader);
                mNumEdges.Read(Reader);
                mNumUserEdges.Read(Reader);
                Reader.Seek(Jumpback + 12, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mStartEdgeIndex.Skip(Writer);
                mStartUserEdgeIndex.Skip(Writer);
                mNumEdges.Skip(Writer);
                mNumUserEdges.Skip(Writer);
                Writer.Seek(Jumpback + 12, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mStartEdgeIndex.Write(Writer);
                mStartUserEdgeIndex.Write(Writer);
                mNumEdges.Write(Writer);
                mNumUserEdges.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMesh::Face";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };

        template<typename tT>
        struct hkArray : public hkReadableWriteableObject
        {
            hkInt64 mOffset;
            int mSize = 0;
            int mCapacityAndFlags = 0;

            std::vector<tT> mElements;
            unsigned long long mParentOffset = std::numeric_limits<unsigned long long>::max();
            unsigned long long mLastElementOffset = std::numeric_limits<unsigned long long>::max();
            bool mIsPatched = false;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);

                mOffset.Read(Reader);
                Reader.ReadStruct(&mSize, sizeof(mSize));
                Reader.ReadStruct(&mCapacityAndFlags, sizeof(mCapacityAndFlags));

                uint64_t PatchOffsetTarget = Reader.GetPosition() - 16;
                for (const auto& Patch : Reader.mDataHolder->GetPatches())
                {
                    for (uint32_t Offset : Patch.mOffsets)
                    {
                        if (Offset == PatchOffsetTarget)
                        {
                            mIsPatched = true;
                            break;
                        }
                    }

                    if (mIsPatched) break;
                }

                if (mIsPatched)
                {
                    mSize = Reader.mDataHolder->GetItems()[mOffset.mParent].mCount;
                    mOffset.mParent = Reader.mDataHolder->GetItems()[mOffset.mParent].mDataOffset;
                }
                else
                {
                    mOffset.mParent += Reader.GetPosition() - 16;
                }

                uint32_t Jumback = Reader.GetPosition();
                if (mOffset.mParent > 0)
                {
                    Reader.Seek(mOffset.mParent, application::util::BinaryVectorReader::Position::Begin);
                    mElements.resize(mSize);
                    for (int i = 0; i < mSize; i++)
                    {
                        hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&mElements[i]);
                        if (BaseObj)
                        {
                            BaseObj->Read(Reader);
                        }
                        else
                        {
                            application::util::Logger::Warning("HavokClasses::hkArray::Read", "Cannot cast to hkReadableWriteableObject");
                        }
                    }
                }
                Reader.Seek(Jumback, application::util::BinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                mParentOffset = Writer.GetPosition();
                if (!mElements.empty() && mIsPatched) Writer.WritePatchCallback("hkArray<" + mElements[0].GetHavokClassName() + ">", mParentOffset);
                Writer.Seek(16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
            }

            void WriteSkip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer)
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkArray::WriteSkip", "mParentOffset was not set");
                    return;
                }

                if (!mElements.empty()) Writer.Align(16);
                unsigned long long Offset = Writer.GetPosition();

                if (mIsPatched)
                {
                    if (mElements.empty())
                    {
                        mOffset.mParent = 0;
                    }
                    else
                    {
                        mOffset.mParent = Writer.WriteItemCallback(&mElements[0], mElements.size());
                    }
                    mSize = 0;
                }
                else
                {
                    mOffset.mParent = !mElements.empty() ? (Offset - mParentOffset) : 0;
                    mSize = mElements.size();
                }
                mCapacityAndFlags = 0;

                if (mIsItem && !mElements.empty() && !mIsPatched) Writer.WriteItemCallback(this, mElements.size(), &mElements[0]);

                for (tT& Element : mElements)
                {
                    if constexpr (std::is_integral_v<tT>)
                    {
                        Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Element), sizeof(tT));
                        continue;
                    }

                    hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
                    if (BaseObj)
                    {
                        BaseObj->Skip(Writer);
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkArray", "Cannot cast to hkReadableWriteableObject");
                    }
                }
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkArray::Write", "mParentOffset was not set");
                    return;
                }

                for (tT& Element : mElements)
                {
                    if constexpr (std::is_integral_v<tT>)
                    {
                        //Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Element), sizeof(tT));
                        continue;
                    }

                    hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Element);
                    if (BaseObj)
                    {
                        BaseObj->Write(Writer);
                    }
                    else
                    {
                        application::util::Logger::Warning("HavokClasses::hkArray", "Cannot cast to hkReadableWriteableObject");
                    }
                }

                mLastElementOffset = Writer.GetPosition();

                Writer.Seek(mParentOffset, application::util::BinaryVectorWriter::Position::Begin);

                mOffset.Skip(Writer);
                Writer.WriteInteger((int32_t)mSize, sizeof(int32_t));
                Writer.WriteInteger((int32_t)mCapacityAndFlags, sizeof(int32_t));

                Writer.Seek(mLastElementOffset, application::util::BinaryVectorWriter::Position::Begin);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkArray";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }

            void ForEveryElement(const std::function<void(tT& Element)>& Func)
            {
                for (tT& Element : mElements)
                {
                    Func(Element);
                }
            }
        };


        struct hkaiPackedKey_ : public hkReadableWriteableObject
        {
            unsigned int mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiPackedKey_";
            }
        };



        struct hkHalf16 : public hkReadableWriteableObject
        {
            hkInt16 mValue;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(2);
                unsigned int Jumpback = Reader.GetPosition();
                mValue.Read(Reader);
                Reader.Seek(Jumpback + 2, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(2);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mValue.Skip(Writer);
                Writer.Seek(Jumpback + 2, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mValue.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkHalf16";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 2;
            }
        };



        enum class hkaiNavMesh__EdgeFlagBits : uint8_t
        {
            EDGE_SILHOUETTE = 1,
            EDGE_RETRIANGULATED = 2,
            EDGE_ORIGINAL = 4,
            EDGE_ORPHANED_USER_EDGE = 8,
            EDGE_USER = 16,
            EDGE_BLOCKED = 32,
            EDGE_EXTERNAL_OPPOSITE = 64,
            EDGE_GARBAGE = 128
        };



        struct hkaiNavMesh__Edge : public hkReadableWriteableObject
        {
            hkaiIndex<hkInt32> mA;
            hkaiIndex<hkInt32> mB;
            hkaiPackedKey_ mOppositeEdge;
            hkaiPackedKey_ mOppositeFace;
            hkFlags<hkaiNavMesh__EdgeFlagBits, hkUint8> mFlags;
            hkUint8 mPaddingByte;
            hkHalf16 mUserEdgeCost;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mA.Read(Reader);
                mB.Read(Reader);
                mOppositeEdge.Read(Reader);
                mOppositeFace.Read(Reader);
                mFlags.Read(Reader);
                mPaddingByte.Read(Reader);
                mUserEdgeCost.Read(Reader);
                Reader.Seek(Jumpback + 20, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mA.Skip(Writer);
                mB.Skip(Writer);
                mOppositeEdge.Skip(Writer);
                mOppositeFace.Skip(Writer);
                mFlags.Skip(Writer);
                mPaddingByte.Skip(Writer);
                mUserEdgeCost.Skip(Writer);
                Writer.Seek(Jumpback + 20, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mA.Write(Writer);
                mB.Write(Writer);
                mOppositeEdge.Write(Writer);
                mOppositeFace.Write(Writer);
                mFlags.Write(Writer);
                mPaddingByte.Write(Writer);
                mUserEdgeCost.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMesh::Edge";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hkaiUpVector : public hkReadableWriteableObject
        {
            hkVector4 mUp;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mUp.Read(Reader);
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mUp.Skip(Writer);
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mUp.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiUpVector";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        enum class hkaiAnnotatedStreamingSet__Side : int
        {
            SIDE_A = 0,
            SIDE_B = 1
        };



        struct hkaiStreamingSet__VolumeConnection : public hkReadableWriteableObject
        {
            hkaiIndex<hkInt32> mACellIndex;
            hkaiIndex<hkInt32> mBCellIndex;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mACellIndex.Read(Reader);
                mBCellIndex.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mACellIndex.Skip(Writer);
                mBCellIndex.Skip(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mACellIndex.Write(Writer);
                mBCellIndex.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiStreamingSet::VolumeConnection";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hkaiStreamingSet__ClusterGraphConnection : public hkReadableWriteableObject
        {
            int mANodeIndex = 0;
            int mBNodeIndex = 0;
            hkUint32 mAEdgeData;
            hkUint32 mBEdgeData;
            hkHalf16 mAEdgeCost;
            hkHalf16 mBEdgeCost;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.ReadStruct(&mANodeIndex, sizeof(mANodeIndex));
                Reader.ReadStruct(&mBNodeIndex, sizeof(mBNodeIndex));
                mAEdgeData.Read(Reader);
                mBEdgeData.Read(Reader);
                mAEdgeCost.Read(Reader);
                mBEdgeCost.Read(Reader);
                Reader.Seek(Jumpback + 20, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mANodeIndex), sizeof(mANodeIndex));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mBNodeIndex), sizeof(mBNodeIndex));
                mAEdgeData.Skip(Writer);
                mBEdgeData.Skip(Writer);
                mAEdgeCost.Skip(Writer);
                mBEdgeCost.Skip(Writer);
                Writer.Seek(Jumpback + 20, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mAEdgeData.Write(Writer);
                mBEdgeData.Write(Writer);
                mAEdgeCost.Write(Writer);
                mBEdgeCost.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiStreamingSet::ClusterGraphConnection";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        template<typename tKEY, typename tVAL>
        struct hkPair : public hkReadableWriteableObject
        {
            tKEY m0;
            tVAL m0x31;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                m0.Read(Reader);
                m0x31.Read(Reader);
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                m0.Skip(Writer);
                m0x31.Skip(Writer);
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                m0.Write(Writer);

                m0x31.WriteSkip(Writer);
                m0x31.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkPair";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkHashMapDetail__Index : public hkReadableWriteableObject
        {
            hkUint64 mEntries; //T*
            int mHashMod = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mEntries.Read(Reader);
                Reader.ReadStruct(&mHashMod, sizeof(mHashMod));
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mEntries.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mHashMod), sizeof(mHashMod));
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mEntries.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkHashMapDetail::Index";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        template<typename tITEM>
        struct hkHashBase : public hkReadableWriteableObject
        {
            hkArray<tITEM> mItems;
            hkHashMapDetail__Index mIndex;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mItems.Read(Reader);
                mIndex.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mItems.mIsPatched = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mItems.Skip(Writer);
                mIndex.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mItems.WriteSkip(Writer);
                mIndex.Write(Writer);

                mItems.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkHashBase";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        template<typename tKEY, typename tVALUE>
        struct hkHashMap : public hkReadableWriteableObject
        {
            hkHashBase<hkPair<tKEY, tVALUE>> mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkHashMap";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkaiFaceEdgeIndexPair : public hkReadableWriteableObject
        {
            hkaiIndex<hkInt32> mFaceIndex;
            hkaiIndex<hkInt32> mEdgeIndex;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mFaceIndex.Read(Reader);
                mEdgeIndex.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mFaceIndex.Skip(Writer);
                mEdgeIndex.Skip(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mFaceIndex.Write(Writer);
                mEdgeIndex.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiFaceEdgeIndexPair";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hkaiStreamingSet__NavMeshConnection : public hkReadableWriteableObject
        {
            hkaiFaceEdgeIndexPair mAFaceEdgeIndex;
            hkaiFaceEdgeIndexPair mBFaceEdgeIndex;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mAFaceEdgeIndex.Read(Reader);
                mBFaceEdgeIndex.Read(Reader);
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mAFaceEdgeIndex.Skip(Writer);
                mBFaceEdgeIndex.Skip(Writer);
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mAFaceEdgeIndex.Write(Writer);
                mBFaceEdgeIndex.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiStreamingSet::NavMeshConnection";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hkaiStreamingSet : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkUint32 mASectionUid;
            hkUint32 mBSectionUid;
            hkArray<hkaiStreamingSet__NavMeshConnection> mMeshConnections;
            hkHashMap<hkUint32, hkArray<hkaiStreamingSet__ClusterGraphConnection>> mGraphConnections;
            hkArray<hkaiStreamingSet__VolumeConnection> mVolumeConnections;
            hkArray<hkAabb> mAConnectionAabbs;
            hkArray<hkAabb> mBConnectionAabbs;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mASectionUid.Read(Reader);
                mBSectionUid.Read(Reader);
                mMeshConnections.Read(Reader);
                mGraphConnections.Read(Reader);
                mVolumeConnections.Read(Reader);
                mAConnectionAabbs.Read(Reader);
                mBConnectionAabbs.Read(Reader);
                Reader.Seek(Jumpback + 128, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mMeshConnections.mIsPatched = true;
                mVolumeConnections.mIsPatched = true;
                mAConnectionAabbs.mIsPatched = true;
                mBConnectionAabbs.mIsPatched = true;

                for (auto& Item : mGraphConnections.mParent.mItems.mElements)
                {
                    Item.m0x31.mIsPatched = true;
                }

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mASectionUid.Skip(Writer);
                mBSectionUid.Skip(Writer);
                mMeshConnections.Skip(Writer);
                mGraphConnections.Skip(Writer);
                mVolumeConnections.Skip(Writer);
                mAConnectionAabbs.Skip(Writer);
                mBConnectionAabbs.Skip(Writer);
                Writer.Seek(Jumpback + 128, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mASectionUid.Write(Writer);
                mBSectionUid.Write(Writer);
                mMeshConnections.WriteSkip(Writer);
                mGraphConnections.Write(Writer);
                mVolumeConnections.WriteSkip(Writer);
                mAConnectionAabbs.WriteSkip(Writer);
                mBConnectionAabbs.WriteSkip(Writer);

                mMeshConnections.Write(Writer);
                mVolumeConnections.Write(Writer);
                mAConnectionAabbs.Write(Writer);
                mBConnectionAabbs.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiStreamingSet";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkaiAnnotatedStreamingSet : public hkReadableWriteableObject
        {
            hkEnum<hkaiAnnotatedStreamingSet__Side, hkUint8> mSide;
            hkRefPtr<hkaiStreamingSet> mStreamingSet;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mSide.Read(Reader);
                mStreamingSet.Read(Reader);
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mStreamingSet.mIsPatched = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mSide.Skip(Writer);
                mStreamingSet.Skip(Writer);
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mSide.Write(Writer);
                mStreamingSet.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiAnnotatedStreamingSet";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkaiNavMeshFaceIterator : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMeshFaceIterator";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 255;
            }
        };



        struct hkcdCompressedAabbCodecs__AabbCodecBase : public hkReadableWriteableObject
        {

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.Seek(Jumpback, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.Seek(Jumpback, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdCompressedAabbCodecs::AabbCodecBase";
            }
        };



        struct hkcdCompressedAabbCodecs__CompressedAabbCodec : public hkReadableWriteableObject
        {
            hkcdCompressedAabbCodecs__AabbCodecBase mParent;
            hkUint8 mXyz[3];

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                Reader.ReadRawArray(&mXyz, sizeof(mXyz), 3, false);
                Reader.Seek(Jumpback + 3, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                Writer.WriteRawArray(&mXyz, sizeof(mXyz), 3, false);
                Writer.Seek(Jumpback + 3, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                //mXyz.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdCompressedAabbCodecs::CompressedAabbCodec";
            }
        };



        struct hkcdCompressedAabbCodecs__Aabb6BytesCodec : public hkReadableWriteableObject
        {
            hkcdCompressedAabbCodecs__CompressedAabbCodec mParent;
            hkUint8 mHiData;
            hkUint16 mLoData;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(2);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mHiData.Read(Reader);
                mLoData.Read(Reader);
                Reader.Seek(Jumpback + 6, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(2);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mHiData.Skip(Writer);
                mLoData.Skip(Writer);
                Writer.Seek(Jumpback + 6, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mHiData.Write(Writer);
                mLoData.Write(Writer);
            }

            glm::vec3 DecompressMin(glm::vec3 ParentMin, glm::vec3 ParentMax)
            {
                float x =
                    (mParent.mXyz[0].mParent >> 4) * (float)(mParent.mXyz[0].mParent >> 4) * (1.0f / 226.0f) * (ParentMax.x - ParentMin.x) +
                    ParentMin.x;
                float y =
                    (mParent.mXyz[1].mParent >> 4) * (float)(mParent.mXyz[1].mParent >> 4) * (1.0f / 226.0f) * (ParentMax.y - ParentMin.y) +
                    ParentMin.y;
                float z =
                    (mParent.mXyz[2].mParent >> 4) * (float)(mParent.mXyz[2].mParent >> 4) * (1.0f / 226.0f) * (ParentMax.z - ParentMin.z) +
                    ParentMin.z;
                return glm::vec3(x, y, z);
            }

            glm::vec3 DecompressMax(glm::vec3 parentMin, glm::vec3 parentMax)
            {
                float x = -((mParent.mXyz[0].mParent & 0x0F) * (float)(mParent.mXyz[0].mParent & 0x0F)) * (1.0f / 226.0f) *
                    (parentMax.x - parentMin.x) + parentMax.x;
                float y = -((mParent.mXyz[1].mParent & 0x0F) * (float)(mParent.mXyz[1].mParent & 0x0F)) * (1.0f / 226.0f) *
                    (parentMax.y - parentMin.y) + parentMax.y;
                float z = -((mParent.mXyz[2].mParent & 0x0F) * (float)(mParent.mXyz[2].mParent & 0x0F)) * (1.0f / 226.0f) *
                    (parentMax.z - parentMin.z) + parentMax.z;
                return glm::vec3(x, y, z);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdCompressedAabbCodecs::Aabb6BytesCodec";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 2;
            }
        };



        struct hkcdStaticTree__AabbTreeBase : public hkReadableWriteableObject
        {

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(1);
                //unsigned int Jumpback = Reader.GetPosition();
                //Reader.Seek(Jumpback + 1, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(1);
                //unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                //Writer.Seek(Jumpback + 1, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdStaticTree::AabbTreeBase";
            }
        };



        template<typename tNODE>
        struct hkcdStaticTree__AabbTree : public hkReadableWriteableObject
        {
            hkcdStaticTree__AabbTreeBase mParent;
            hkRelArray<tNODE> mNodes;
            hkAabb mDomain;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mNodes.Read(Reader);
                mDomain.Read(Reader);
                Reader.Seek(Jumpback + 48, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mNodes.Skip(Writer);
                mDomain.Skip(Writer);
                Writer.Seek(Jumpback + 48, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mNodes.Write(Writer);
                mDomain.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdStaticTree::AabbTree";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        struct hkcdStaticTree__Aabb6BytesTree : public hkReadableWriteableObject
        {
            hkcdStaticTree__AabbTree<hkcdCompressedAabbCodecs__Aabb6BytesCodec> mParent;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                mParent.Read(Reader);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdStaticTree::Aabb6BytesTree";
            }
        };



        struct hkcdStaticAabbTree__Impl : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkcdStaticTree__Aabb6BytesTree mTree;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mTree.Read(Reader);
                Reader.Seek(Jumpback + 80, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mTree.Skip(Writer);
                Writer.Seek(Jumpback + 80, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mTree.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdStaticAabbTree::Impl";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };



        struct hkcdStaticAabbTree : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkRefPtr<hkcdStaticAabbTree__Impl> mTreePtr;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mTreePtr.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mTreePtr.mIsPatched = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mTreePtr.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mTreePtr.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkcdStaticAabbTree";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkaiNavMeshStaticTreeFaceIterator : public hkReadableWriteableObject
        {
            hkaiNavMeshFaceIterator mParent;
            hkRefPtr<hkcdStaticAabbTree> mTree;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mTree.Read(Reader);
                Reader.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mTree.mIsPatched = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mTree.Skip(Writer);
                Writer.Seek(Jumpback + 32, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mTree.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMeshStaticTreeFaceIterator";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkaiNavMeshClearanceCache__McpDataInteger : public hkReadableWriteableObject
        {
            hkUint8 mInterpolant;
            hkUint8 mClearance;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                mInterpolant.Read(Reader);
                mClearance.Read(Reader);
                Reader.Seek(Jumpback + 2, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mInterpolant.Skip(Writer);
                mClearance.Skip(Writer);
                Writer.Seek(Jumpback + 2, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mInterpolant.Write(Writer);
                mClearance.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMeshClearanceCache::McpDataInteger";
            }
        };



        struct hkaiNavMeshClearanceCache__EdgeDataInteger : public hkReadableWriteableObject
        {
            hkaiNavMeshClearanceCache__McpDataInteger mMcp;
            hkUint8 mStartVertexClearance;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(1);
                unsigned int Jumpback = Reader.GetPosition();
                mMcp.Read(Reader);
                mStartVertexClearance.Read(Reader);
                Reader.Seek(Jumpback + 3, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(1);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mMcp.Skip(Writer);
                mStartVertexClearance.Skip(Writer);
                Writer.Seek(Jumpback + 3, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mMcp.Write(Writer);
                mStartVertexClearance.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMeshClearanceCache::EdgeDataInteger";
            }
        };



        struct hkaiNavMeshClearanceCache : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkReal mClearanceCeiling;
            hkReal mClearanceBias;
            hkReal mClearanceIntToRealMultiplier;
            hkReal mClearanceRealToIntMultiplier;
            hkArray<hkUint32> mFaceOffsets;
            hkArray<hkUint8> mEdgePairClearances;
            int mUnusedEdgePairElements = 0;
            hkArray<hkaiNavMeshClearanceCache__EdgeDataInteger> mEdgeData;
            int mUncalculatedFacesLowerBound = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mClearanceCeiling.Read(Reader);
                mClearanceBias.Read(Reader);
                mClearanceIntToRealMultiplier.Read(Reader);
                mClearanceRealToIntMultiplier.Read(Reader);
                mFaceOffsets.Read(Reader);
                mEdgePairClearances.Read(Reader);
                Reader.ReadStruct(&mUnusedEdgePairElements, sizeof(mUnusedEdgePairElements));
                mEdgeData.Read(Reader);
                Reader.ReadStruct(&mUncalculatedFacesLowerBound, sizeof(mUncalculatedFacesLowerBound));
                Reader.Seek(Jumpback + 104, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mFaceOffsets.mIsPatched = true;
                mEdgePairClearances.mIsPatched = true;
                mEdgeData.mIsPatched = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mClearanceCeiling.Skip(Writer);
                mClearanceBias.Skip(Writer);
                mClearanceIntToRealMultiplier.Skip(Writer);
                mClearanceRealToIntMultiplier.Skip(Writer);
                mFaceOffsets.Skip(Writer);
                mEdgePairClearances.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mUnusedEdgePairElements), sizeof(mUnusedEdgePairElements));
                mEdgeData.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mUncalculatedFacesLowerBound), sizeof(mUncalculatedFacesLowerBound));
                Writer.Seek(Jumpback + 104, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mClearanceCeiling.Write(Writer);
                mClearanceBias.Write(Writer);
                mClearanceIntToRealMultiplier.Write(Writer);
                mClearanceRealToIntMultiplier.Write(Writer);
                mFaceOffsets.WriteSkip(Writer);
                mEdgePairClearances.WriteSkip(Writer);
                mEdgeData.WriteSkip(Writer);

                mFaceOffsets.Write(Writer);
                mEdgePairClearances.Write(Writer);
                mEdgeData.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMeshClearanceCache";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkaiNavMeshClearanceCacheSeeding__CacheData : public hkReadableWriteableObject
        {
            hkUint32 mSeedingDataIdentifier;
            hkRefPtr<hkaiNavMeshClearanceCache> mInitialCache;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mSeedingDataIdentifier.Read(Reader);
                mInitialCache.Read(Reader);
                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mInitialCache.mIsPatched = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mSeedingDataIdentifier.Skip(Writer);
                mInitialCache.Skip(Writer);
                Writer.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mSeedingDataIdentifier.Write(Writer);
                mInitialCache.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMeshClearanceCacheSeeding::CacheData";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkaiNavMeshClearanceCacheSeeding__CacheDataSet : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkArray<hkaiNavMeshClearanceCacheSeeding__CacheData> mCacheDatas;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mCacheDatas.Read(Reader);
                Reader.Seek(Jumpback + 40, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mCacheDatas.mIsPatched = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mCacheDatas.Skip(Writer);
                Writer.Seek(Jumpback + 40, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);

                mCacheDatas.WriteSkip(Writer);
                mCacheDatas.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMeshClearanceCacheSeeding::CacheDataSet";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkaiNavMesh : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkArray<hkaiNavMesh__Face> mFaces;
            hkArray<hkaiNavMesh__Edge> mEdges;
            hkArray<hkVector4> mVertices;
            hkArray<hkaiAnnotatedStreamingSet> mStreamingSets;
            hkArray<hkInt32> mFaceData;
            hkArray<hkInt32> mEdgeData;
            int mFaceDataStriding = 0;
            int mEdgeDataStriding = 0;
            unsigned char mFlags = 0;
            hkAabb mAabb;
            hkReal mErosionRadius;
            hkaiUpVector mUp;
            hkUint64 mUserData;
            hkRefPtr<hkaiNavMeshStaticTreeFaceIterator> mCachedFaceIterator;
            hkRefPtr<hkaiNavMeshClearanceCacheSeeding__CacheDataSet> mClearanceCacheSeedingDataSet;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(16);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mFaces.Read(Reader);
                mEdges.Read(Reader);
                mVertices.Read(Reader);
                mStreamingSets.Read(Reader);
                mFaceData.Read(Reader);
                mEdgeData.Read(Reader);
                Reader.ReadStruct(&mFaceDataStriding, sizeof(mFaceDataStriding));
                Reader.ReadStruct(&mEdgeDataStriding, sizeof(mEdgeDataStriding));
                Reader.ReadStruct(&mFlags, sizeof(mFlags));
                mAabb.Read(Reader);
                mErosionRadius.Read(Reader);
                mUp.Read(Reader);
                mUserData.Read(Reader);
                mCachedFaceIterator.Read(Reader);
                mClearanceCacheSeedingDataSet.Read(Reader);
                Reader.Seek(Jumpback + 240, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mFaces.mIsPatched = true;
                mEdges.mIsPatched = true;
                mVertices.mIsPatched = true;
                mStreamingSets.mIsPatched = true;
                mFaceData.mIsPatched = true;
                mEdgeData.mIsPatched = true;
                mCachedFaceIterator.mIsPatched = true;
                mClearanceCacheSeedingDataSet.mIsPatched = true;

                Writer.Align(16);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mFaces.Skip(Writer);
                mEdges.Skip(Writer);
                mVertices.Skip(Writer);
                mStreamingSets.Skip(Writer);
                mFaceData.Skip(Writer);
                mEdgeData.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mFaceDataStriding), sizeof(mFaceDataStriding));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mEdgeDataStriding), sizeof(mEdgeDataStriding));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mFlags), sizeof(mFlags));
                mAabb.Skip(Writer);
                mErosionRadius.Skip(Writer);
                mUp.Skip(Writer);
                mUserData.Skip(Writer);
                mCachedFaceIterator.Skip(Writer);
                mClearanceCacheSeedingDataSet.Skip(Writer);
                Writer.Seek(Jumpback + 240, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mFaces.WriteSkip(Writer);
                mEdges.WriteSkip(Writer);
                mVertices.WriteSkip(Writer);
                mStreamingSets.WriteSkip(Writer);
                mFaceData.WriteSkip(Writer);
                mEdgeData.WriteSkip(Writer);
                mAabb.Write(Writer);
                mErosionRadius.Write(Writer);
                mUp.Write(Writer);
                mUserData.Write(Writer);
                mCachedFaceIterator.Write(Writer);
                mClearanceCacheSeedingDataSet.Write(Writer);

                mFaces.Write(Writer);
                mEdges.Write(Writer);
                mVertices.Write(Writer);
                mStreamingSets.Write(Writer);
                mFaceData.Write(Writer);
                mEdgeData.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiNavMesh";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 16;
            }
        };


        enum class hkaiClusterGraph__EdgeBits : uint16_t
        {
            EDGE_IS_USER = 2,
            EDGE_EXTERNAL_OPPOSITE = 64
        };



        struct hkaiPackedClusterKey_ : public hkReadableWriteableObject
        {
            unsigned int mParent = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.ReadStruct(&mParent, sizeof(mParent));
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mParent), sizeof(mParent));
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiPackedClusterKey_";
            }
        };



        struct hkaiClusterGraph__Edge : public hkReadableWriteableObject
        {
            hkHalf16 mCost;
            hkFlags<hkaiClusterGraph__EdgeBits, hkUint16> mFlags;
            hkaiPackedClusterKey_ mSource;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                mCost.Read(Reader);
                mFlags.Read(Reader);
                mSource.Read(Reader);
                Reader.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mCost.Skip(Writer);
                mFlags.Skip(Writer);
                mSource.Skip(Writer);
                Writer.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mCost.Write(Writer);
                mFlags.Write(Writer);
                mSource.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiClusterGraph::Edge";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hkaiClusterGraph__Node : public hkReadableWriteableObject
        {
            int mStartEdgeIndex = 0;
            int mNumEdges = 0;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(4);
                unsigned int Jumpback = Reader.GetPosition();
                Reader.ReadStruct(&mStartEdgeIndex, sizeof(mStartEdgeIndex));
                Reader.ReadStruct(&mNumEdges, sizeof(mNumEdges));
                Reader.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(4);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mStartEdgeIndex), sizeof(mStartEdgeIndex));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mNumEdges), sizeof(mNumEdges));
                Writer.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiClusterGraph::Node";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 4;
            }
        };



        struct hkaiClusterGraph : public hkReadableWriteableObject
        {
            hkReferencedObject mParent;
            hkArray<hkVector4> mPositions;
            hkArray<hkaiClusterGraph__Node> mNodes;
            hkArray<hkaiClusterGraph__Edge> mEdges;
            hkArray<hkUint32> mNodeData;
            hkArray<hkUint32> mEdgeData;
            hkArray<hkaiIndex<hkInt32>> mFeatureToNodeIndex;
            int mNodeDataStriding = 0;
            int mEdgeDataStriding = 0;
            hkArray<hkaiAnnotatedStreamingSet> mStreamingSets;
            hkUint64 mUserData;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mParent.Read(Reader);
                mPositions.Read(Reader);
                mNodes.Read(Reader);
                mEdges.Read(Reader);
                mNodeData.Read(Reader);
                mEdgeData.Read(Reader);
                mFeatureToNodeIndex.Read(Reader);
                Reader.ReadStruct(&mNodeDataStriding, sizeof(mNodeDataStriding));
                Reader.ReadStruct(&mEdgeDataStriding, sizeof(mEdgeDataStriding));
                mStreamingSets.Read(Reader);
                mUserData.Read(Reader);
                Reader.Seek(Jumpback + 152, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mPositions.mIsPatched = true;
                mNodes.mIsPatched = true;
                mEdges.mIsPatched = true;
                mNodeData.mIsPatched = true;
                mEdgeData.mIsPatched = true;
                mFeatureToNodeIndex.mIsPatched = true;
                mStreamingSets.mIsPatched = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mParent.Skip(Writer);
                mPositions.Skip(Writer);
                mNodes.Skip(Writer);
                mEdges.Skip(Writer);
                mNodeData.Skip(Writer);
                mEdgeData.Skip(Writer);
                mFeatureToNodeIndex.Skip(Writer);
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mNodeDataStriding), sizeof(mNodeDataStriding));
                Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&mEdgeDataStriding), sizeof(mEdgeDataStriding));
                mStreamingSets.Skip(Writer);
                mUserData.Skip(Writer);
                Writer.Seek(Jumpback + 152, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mParent.Write(Writer);
                mPositions.WriteSkip(Writer);
                mNodes.WriteSkip(Writer);
                mEdges.WriteSkip(Writer);
                mNodeData.WriteSkip(Writer);
                mEdgeData.WriteSkip(Writer);
                mFeatureToNodeIndex.WriteSkip(Writer);
                mStreamingSets.WriteSkip(Writer);
                mUserData.Write(Writer);

                mPositions.Write(Writer);
                mNodes.Write(Writer);
                mEdges.Write(Writer);
                mNodeData.Write(Writer);
                mEdgeData.Write(Writer);
                mFeatureToNodeIndex.Write(Writer);
                mStreamingSets.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkaiClusterGraph";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };


        struct hkStringPtr : public hkReadableWriteableObject
        {
            hkInt64 mOffset;
            std::string mString;

            unsigned long long mParentOffset = std::numeric_limits<unsigned long long>::max();
            unsigned long long mLastElementOffset = std::numeric_limits<unsigned long long>::max();
            bool mIsPatched = true;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();

                mOffset.Read(Reader);

                uint64_t PatchOffsetTarget = Reader.GetPosition() - 8;
                for (const auto& Patch : Reader.mDataHolder->GetPatches())
                {
                    for (uint32_t Offset : Patch.mOffsets)
                    {
                        if (Offset == PatchOffsetTarget)
                        {
                            mIsPatched = true;
                            break;
                        }
                    }

                    if (mIsPatched) break;
                }

                if (mIsPatched)
                {
                    mOffset.mParent = Reader.mDataHolder->GetItems()[mOffset.mParent].mDataOffset;
                }
                else
                {
                    mOffset.mParent += Jumpback;
                }

                Reader.Seek(mOffset.mParent, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);

                char c;

                while ((c = static_cast<char>(Reader.ReadUInt8())) != '\0')
                {
                    mString += c;
                }

                Reader.Seek(Jumpback + 8, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                mParentOffset = Writer.GetPosition();
                Writer.Seek(8, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkStringPtr", "mParentOffset was not set");
                    return;
                }

                bool IsNullPtr = mString.empty();

                if (!IsNullPtr) Writer.Align(2);
                unsigned long long Offset = Writer.GetPosition();

                if (IsNullPtr)
                {
                    Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                    mOffset.mParent = 0;
                    mLastElementOffset = Offset;
                    mOffset.Write(Writer);
                    Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);
                    mLastElementOffset = Writer.GetPosition();
                    return;
                }

                if (mIsPatched)
                {
                    mOffset.mParent = Writer.WriteItemCallback(this, mString.size() + 1, &mString[0], "char");
                    Writer.WritePatchCallback("hkStringPtr", mParentOffset);
                }
                else
                {
                    mOffset.mParent = Offset - mParentOffset;
                }

                Writer.WriteRawUnsafeFixed(mString.c_str(), mString.size());
                Writer.WriteByte(0x00); //Terminator
                mLastElementOffset = Writer.GetPosition();

                Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                mOffset.Skip(Writer);

                Writer.Seek(mLastElementOffset, application::util::BinaryVectorWriter::Position::Begin);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkStringPtr";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };


        template <typename T>
        struct hkRootLevelContainer__NamedVariant : public hkReadableWriteableObject
        {
            hkStringPtr mName;
            hkStringPtr mClassName;
            hkRefPtr<T> mVariant;

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();
                mName.Read(Reader);
                mClassName.Read(Reader);
                mVariant.Read(Reader);
                Reader.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mName.mIsPatched = true;
                mClassName.mIsPatched = true;

                mVariant.mIsPatched = true;
                mVariant.mManualPatching = true;

                Writer.Align(8);
                unsigned int Jumpback = Writer.GetPosition();
                if (mIsItem) Writer.WriteItemCallback(this);
                mName.Skip(Writer);
                mClassName.Skip(Writer);

                Writer.WritePatchCallback("hkRefVariant", Writer.GetPosition());
                mVariant.Skip(Writer);
                Writer.Seek(Jumpback + 24, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
            }

            void WriteNames(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer)
            {
                mName.Write(Writer);
                mClassName.Write(Writer);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                mVariant.Write(Writer);
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkRootLevelContainer::NamedVariant";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };



        struct hkNavMeshRootLevelContainer : public hkReadableWriteableObject
        {
            hkRootLevelContainer__NamedVariant<hkaiNavMesh> mNavMeshVariant;
            hkRootLevelContainer__NamedVariant<hkaiClusterGraph> mClusterGraphVariant;

            unsigned long long mParentOffset = std::numeric_limits<unsigned long long>::max();
            unsigned long long mLastElementOffset = std::numeric_limits<unsigned long long>::max();

            virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override
            {
                Reader.Align(8);
                unsigned int Jumpback = Reader.GetPosition();

                uint64_t NamedVariantOffset = Reader.ReadUInt64();

                bool IsPatched = false;

                uint64_t PatchOffsetTarget = Reader.GetPosition() - 8;
                for (const auto& Patch : Reader.mDataHolder->GetPatches())
                {
                    for (uint32_t Offset : Patch.mOffsets)
                    {
                        if (Offset == PatchOffsetTarget)
                        {
                            IsPatched = true;
                            break;
                        }
                    }

                    if (IsPatched) break;
                }

                if (IsPatched)
                {
                    NamedVariantOffset = Reader.mDataHolder->GetItems()[NamedVariantOffset].mDataOffset;
                }
                else
                {
                    NamedVariantOffset += Jumpback;
                }

                Reader.Seek(NamedVariantOffset, application::util::BinaryVectorReader::Position::Begin);

                mNavMeshVariant.Read(Reader);
                mClusterGraphVariant.Read(Reader);

                Reader.Seek(Jumpback + 16, application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);
            }

            virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                Writer.Align(8);
                mParentOffset = Writer.GetPosition();
                Writer.Seek(16, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Current);
            }

            virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override
            {
                if (mParentOffset == std::numeric_limits<unsigned long long>::max())
                {
                    application::util::Logger::Warning("HavokClasses::hkNavMeshRootLevelContainer", "mParentOffset was not set");
                    return;
                }

                unsigned long long Offset = Writer.GetPosition();

                Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);
                Writer.WriteItemCallback(this);
                Writer.Seek(Offset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                uint32_t ItemIndex = Writer.WriteItemCallback(&mNavMeshVariant, 2); //Simulate hkArray writing

                Writer.Seek(mParentOffset, application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);

                Writer.WritePatchCallback("hkArray<hkRootLevelContainer::NamedVariant>", Writer.GetPosition());

                Writer.WriteInteger((uint64_t)ItemIndex, sizeof(uint64_t));
                Writer.WriteInteger((int32_t)0, sizeof(int32_t));
                Writer.WriteInteger((int32_t)0, sizeof(int32_t));

                Writer.Seek(Offset, application::util::BinaryVectorWriter::Position::Begin);

                mNavMeshVariant.Skip(Writer);
                mClusterGraphVariant.Skip(Writer);

                mNavMeshVariant.WriteNames(Writer);
                mClusterGraphVariant.WriteNames(Writer);

                mNavMeshVariant.Write(Writer);
                mClusterGraphVariant.Write(Writer);

                mLastElementOffset = Writer.GetPosition();
            }

            virtual std::string GetHavokClassName() override
            {
                return "hkRootLevelContainer";
            }

            virtual unsigned int GetHavokClassAlignment() override
            {
                return 8;
            }
        };


        template <typename T>
        T DecodeHavokTagFileWithRoot(const std::vector<unsigned char>& Bytes, HavokDataHolder& DataHolder)
        {
            T Root;

            hkReadableWriteableObject* BaseObj = reinterpret_cast<hkReadableWriteableObject*>(&Root);
            if (BaseObj)
            {
                application::file::game::phive::classes::HavokClassBinaryVectorReader Reader(Bytes);

                Reader.mDataHolder = &DataHolder;

                BaseObj->Read(Reader);
                application::util::Logger::Info("HavokClasses", "Successfully decoded");
            }
            else
            {
                application::util::Logger::Warning("HavokClasses", "Cannot cast to hkReadableWriteableObject, thrown from DecodeHavokTagFileWithRoot<T>");
            }

            return Root;
        }


        template <typename T>
        void EncodeHavokTagFileWithRoot(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer, T& Root)
        {
            Root.Skip(Writer);
            Root.Write(Writer);

            application::util::Logger::Info("HavokClasses", "Successfully encoded");
        }
    }
}