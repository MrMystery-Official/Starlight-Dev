#pragma once

#include <util/BinaryVectorWriter.h>
#include <util/Logger.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <glm/glm.hpp>
#include <functional>
#include <unordered_map>

namespace application::file::game::bfres
{
	class BfresBinaryVectorWriter : public application::util::BinaryVectorWriter
	{
	public:
		struct WriteableObject
		{
			virtual void Write(BfresBinaryVectorWriter& Reader) = 0;
		};

		BfresBinaryVectorWriter(std::unordered_map<uint64_t, std::string>& StringCache);

		void Write_10_10_10_2_SNorm(const glm::vec4& Vec);
		void Write_8_UNorm(float val);
		void Write_8_UInt(float val);
		void Write_8_SNorm(float val);
		void Write_8_SInt(float val);

		void Write_16_UNorm(float val);
		void Write_16_UInt(float val);
		void Write_16_SNorm(float val);
		void Write_16_SInt(float val);

		void WriteHalfFloat(float val);

		void WriteAttribute(uint32_t RawFormat, const glm::vec4& Vec);
		void WriteVector4F(const glm::vec4& Vec);
		uint64_t WriteString(const std::string& Str);

		void WriteStrings(const std::vector<std::string>& Strings);

		void WriteBooleanBits(const std::vector<bool>& Vec);

		void WriteRawBoolArray(const std::vector<bool>& Vec);

		template <typename T>
		void WriteRawArray(const std::vector<T>& Vec)
		{
			WriteRawUnsafeFixed(reinterpret_cast<const char*>(Vec.data()), Vec.size() * sizeof(T));
		}

		template <typename T>
		void WriteResObject(const T& Object)
		{
			WriteableObject* BaseObj = const_cast<WriteableObject*>(static_cast<const WriteableObject*>(&Object));
			if (BaseObj)
			{
				BaseObj->Write(*this);
			}
			else
			{
				application::util::Logger::Error("BfresBinaryVectorWriter", "Cannot cast ResObject to BfresBinaryVectorWriter::WriteableObject");
			}
		}

		template <typename T>
		void WriteArray(const std::vector<T>& Vec, uint64_t& Offset)
		{
			Offset = GetPosition();
			for (const T& Obj : Vec)
			{
				WriteResObject(Obj);
			}
		}

		template <typename T>
		void WriteCustom(std::function<void(T&)> Func, T& Object)
		{
			uint32_t Pos = GetPosition();
			Func(Object);
			Seek(Pos, Position::Begin);
		}

		std::unordered_map<uint64_t, std::string>& mStringCache;
	};
}