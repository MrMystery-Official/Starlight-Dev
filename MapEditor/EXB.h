#pragma once

#include <vector>
#include <string>
#include "BinaryVectorReader.h"
#include <variant>
#include "Vector3F.h"

class EXB
{
public:

	using EXBValue = std::variant<bool, uint32_t, float, Vector3F, std::string>; // Same order as ValueType enum

	enum class Command : uint8_t
	{
		Terminator = 1,
		Store = 2,
		Negate = 3,
		NegateBool = 4,
		Add = 5,
		Substract = 6,
		Multiply = 7,
		Divide = 8,
		Modulus = 9,
		Increment = 10,
		Decrement = 11,
		ScalarMultiplyVec3f = 12,
		ScalarDivideVec3f = 13,
		LeftShift = 14,
		RightShift = 15,
		LessThan = 16,
		LessThanEqual = 17,
		GreaterThan = 18,
		GreaterThanEqual = 19,
		Equal = 20,
		NotEqual = 21,
		And = 22,
		Xor = 23,
		Or = 24,
		LogicalAnd = 25,
		LocigalOr = 26,
		UserFunction = 27,
		JumpIfLhsZero = 28,
		Jump = 29,
		Unknown = 30
	};

	enum class Type : uint16_t
	{
		None = 0,
		ImmediateOrUser = 1,
		Bool = 2,
		S32 = 3,
		F32 = 4,
		String = 5,
		Vec3f = 6
	};

	enum class Source : uint8_t
	{
		Imm = 0,
		ImmStr = 1,
		StaticMem = 2,
		ParamTbl = 3,
		ParamTblStr = 4,
		Output = 5,
		Input = 6,
		Scratch32 = 7,
		Scratch64 = 8,
		UserOut = 9,
		UserIn = 10,
		Unknown = 11
	};

	struct HeaderStruct
	{
		char Magic[4];
		uint32_t Version;
		uint32_t StaticSize = 0;
		uint32_t FieldEntryCount = 0;
		uint32_t Scratch32Size = 0;
		uint32_t Scratch64Size = 0;
		uint32_t CommandInfoOffset = 0;
		uint32_t CommandTableOffset = 0;
		uint32_t SignatureTableOffset = 0;
		uint32_t ParameterRegionOffset = 0;
		uint32_t StringOffset = 0;
	};

	struct InstructionStruct
	{
		EXB::Command Type = EXB::Command::Unknown;
		EXB::Type DataType = EXB::Type::None;

		EXB::Source LHSSource = EXB::Source::Unknown;
		EXB::Source RHSSource = EXB::Source::Unknown;
		uint16_t LHSIndexValue = 0xFFFF;
		uint16_t RHSIndexValue = 0xFFFF;
		EXBValue LHSValue = (uint32_t)0xFFFFFFFF;
		EXBValue RHSValue = (uint32_t)0xFFFFFFFF;

		uint16_t StaticMemoryIndex = 0xFFFF;
		std::string Signature = "MapEditor_EXB_NoVal";
	};

	struct CommandInfoStruct
	{
		int32_t BaseIndexPreCommandEntry = 0;
		uint32_t PreEntryStaticMemoryUsage = 0;
		uint32_t InstructionBaseIndex = 0;
		uint32_t InstructionCount = 0;
		uint32_t StaticMemorySize = 0;
		uint16_t Scratch32MemorySize = 0;
		uint16_t Scratch64MemorySize = 0;
		EXB::Type OutputDataType = EXB::Type::None;
		EXB::Type InputDataType = EXB::Type::None;

		std::vector<InstructionStruct> Instructions;
	};

	HeaderStruct Header;

	std::vector<CommandInfoStruct> Commands;
	std::vector<InstructionStruct> Instructions;

	bool Loaded = false;

	void AddToStringTable(std::string Value, std::vector<std::string>& StringTable);
	int GetOffsetInStringTable(std::string Value, std::vector<std::string>& StringTable);

	std::string ReadStringFromStringPool(BinaryVectorReader* Reader, uint32_t Offset);
	EXB() {}
	EXB(std::vector<unsigned char> Bytes);

	std::vector<unsigned char> ToBinary(uint32_t EXBInstanceCount);
};