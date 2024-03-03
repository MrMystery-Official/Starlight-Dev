#include "EXB.h"

#include "Logger.h"
#include "BinaryVectorWriter.h"
#include <iostream>

void EXB::AddToStringTable(std::string Value, std::vector<std::string>& StringTable)
{
	int Index = -1;
	auto it = std::find(StringTable.begin(), StringTable.end(), Value);

	if (it == StringTable.end())
	{
		StringTable.push_back(Value);
	}
}

int EXB::GetOffsetInStringTable(std::string Value, std::vector<std::string>& StringTable)
{
	int Index = -1;
	auto it = std::find(StringTable.begin(), StringTable.end(), Value);

	if (it != StringTable.end())
	{
		Index = it - StringTable.begin();
	}

	if (Index == -1)
	{
		Logger::Warning("EXBStringMgr", "Can not find string \"" + Value + "\" in StringTable of EXB");
		return -1;
	}

	int Offset = 0;

	for (int i = 0; i < Index; i++)
	{
		Offset += StringTable.at(i).length() + 1;
	}

	return Offset;
}

std::string EXB::ReadStringFromStringPool(BinaryVectorReader* Reader, uint32_t Offset) {
	int BaseOffset = Reader->GetPosition();
	Reader->Seek(Header.StringOffset + Offset, BinaryVectorReader::Position::Begin);
	std::string Result;
	char CurrentCharacter = Reader->ReadInt8();
	while (CurrentCharacter != 0x00) {
		Result += CurrentCharacter;
		CurrentCharacter = Reader->ReadInt8();
	}
	Reader->Seek(BaseOffset, BinaryVectorReader::Position::Begin);
	return Result;
}

EXB::EXB(std::vector<unsigned char> Bytes)
{
	BinaryVectorReader Reader(Bytes);

	Reader.ReadStruct(&Header, sizeof(HeaderStruct));

	if (Header.Magic[0] != 'E' || Header.Magic[1] != 'X' || Header.Magic[2] != 'B')
	{
		Logger::Info("sss", Header.Magic);
		Logger::Error("EXBDecoder", "Wrong magic, expected magic EXB");
		return;
	}
	if (Header.Version != 0x02)
	{
		Logger::Error("EXBDecoder", "Wrong version, expected 0x02");
		return;
	}

	//Signature Offsets
	Reader.Seek(Header.SignatureTableOffset, BinaryVectorReader::Position::Begin);
	uint32_t SignatureCount = Reader.ReadUInt32();
	std::vector<uint32_t> SignatureOffsets(SignatureCount);
	for (int i = 0; i < SignatureCount; i++)
	{
		SignatureOffsets[i] = Reader.ReadUInt32();
	}

	//Command Info
	Reader.Seek(Header.CommandInfoOffset, BinaryVectorReader::Position::Begin);
	uint32_t InfoCount = Reader.ReadUInt32();
	Commands.resize(InfoCount);
	for (int i = 0; i < InfoCount; i++)
	{
		Reader.ReadStruct(&Commands[i], 28); //28 = sizeof(CommandInfoStruct) - Instruction vector
	}

	//Command Instructions
	Reader.Seek(Header.CommandTableOffset, BinaryVectorReader::Position::Begin);
	uint32_t InstructionCount = Reader.ReadUInt32();
	Instructions.resize(InstructionCount);
	for (int i = 0; i < InstructionCount; i++)
	{
		InstructionStruct Instruction;
		Instruction.Type = static_cast<EXB::Command>(Reader.ReadUInt8());
		std::cout << "InstType: " << (int)Instruction.Type << "\n";
		if (Instruction.Type == EXB::Command::Terminator)
		{
			Reader.Seek(7, BinaryVectorReader::Position::Current);
			Instructions[i] = Instruction;
			continue;
		}
		Instruction.DataType = static_cast<EXB::Type>(Reader.ReadUInt8());
		if (Instruction.Type != EXB::Command::UserFunction)
		{
			Instruction.LHSSource = static_cast<EXB::Source>(Reader.ReadUInt8());
			Instruction.RHSSource = static_cast<EXB::Source>(Reader.ReadUInt8());
			Instruction.LHSIndexValue = Reader.ReadUInt16();
			Instruction.RHSIndexValue = Reader.ReadUInt16();
			auto Func = [&Instruction, &Reader](std::string i, EXB* EXBFile)
				{
					if ((i == "LHS" ? Instruction.LHSSource : Instruction.RHSSource) == EXB::Source::ParamTbl)
					{
						uint32_t Jumpback = Reader.GetPosition();
						Reader.Seek(EXBFile->Header.ParameterRegionOffset + (i == "LHS" ? Instruction.LHSIndexValue : Instruction.RHSIndexValue), BinaryVectorReader::Position::Begin);
						if (Instruction.DataType == EXB::Type::Bool)
						{
							(i == "LHS" ? Instruction.LHSValue : Instruction.RHSValue) = (bool)Reader.ReadUInt32();
						}
						else if (Instruction.DataType == EXB::Type::S32)
						{
							(i == "LHS" ? Instruction.LHSValue : Instruction.RHSValue) = Reader.ReadUInt32();
						}
						else if (Instruction.DataType == EXB::Type::F32)
						{
							(i == "LHS" ? Instruction.LHSValue : Instruction.RHSValue) = Reader.ReadFloat();
						}
						else if (Instruction.DataType == EXB::Type::Vec3f)
						{
							(i == "LHS" ? Instruction.LHSValue : Instruction.RHSValue) = Vector3F(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
						}
						Reader.Seek(Jumpback, BinaryVectorReader::Position::Begin);
					}
					if ((i == "LHS" ? Instruction.LHSSource : Instruction.RHSSource) == EXB::Source::ParamTblStr)
					{
						uint32_t Jumpback = Reader.GetPosition();
						Reader.Seek(EXBFile->Header.ParameterRegionOffset + (i == "LHS" ? Instruction.LHSIndexValue : Instruction.RHSIndexValue), BinaryVectorReader::Position::Begin);
						(i == "LHS" ? Instruction.LHSValue : Instruction.RHSValue) = EXBFile->ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
						Reader.Seek(Jumpback, BinaryVectorReader::Position::Begin);
					}
					if ((i == "LHS" ? Instruction.LHSSource : Instruction.RHSSource) == EXB::Source::Imm)
					{
						(i == "LHS" ? Instruction.LHSValue : Instruction.RHSValue) = (i == "LHS" ? Instruction.LHSIndexValue : Instruction.RHSIndexValue);
					}
					if ((i == "LHS" ? Instruction.LHSSource : Instruction.RHSSource) == EXB::Source::ImmStr)
					{
						(i == "LHS" ? Instruction.LHSValue : Instruction.RHSValue) = EXBFile->ReadStringFromStringPool(&Reader, (i == "LHS" ? Instruction.LHSIndexValue : Instruction.RHSIndexValue));
					}
				};

			Func("LHS", this);
			Func("RHS", this);
		}
		else
		{
			Instruction.StaticMemoryIndex = Reader.ReadUInt16();
			Instruction.Signature = ReadStringFromStringPool(&Reader, SignatureOffsets[Reader.ReadUInt32()]);
			std::cout << "Read sig: " << Instruction.Signature << std::endl;
		}
		Instructions[i] = Instruction;
	}

	//Match instructions to commands
	uint32_t InstructionIndex = 0;
	for (EXB::CommandInfoStruct& Command : Commands)
	{
		Command.Instructions.resize(Command.InstructionCount);
		for (int i = 0; i < Command.InstructionCount; i++)
		{
			Command.Instructions[i] = this->Instructions[InstructionIndex + i];
		}
		InstructionIndex += Command.Instructions.size();
	}

	this->Loaded = true;
}

std::vector<unsigned char> EXB::ToBinary(uint32_t EXBInstanceCount)
{
	BinaryVectorWriter Writer;

	std::vector<std::string> StringTable;

	Writer.WriteBytes("EXB ");
	Writer.WriteByte(0x02);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);

	Writer.Seek(36, BinaryVectorWriter::Position::Current);
	Writer.WriteInteger(this->Commands.size(), sizeof(uint32_t));

	Logger::Info("EXBDebugger", "Command size: " + std::to_string(this->Commands.size()));
	Logger::Info("EXBDebugger", "Command size: " + std::to_string(this->Instructions.size()));

	uint32_t InstructionIndex = 0;
	uint32_t MaxStatic = 0;
	uint32_t Max32 = 0;
	uint32_t Max64 = 0;
	for (EXB::CommandInfoStruct Cmd : this->Commands)
	{
		Writer.WriteInteger((int32_t)Cmd.BaseIndexPreCommandEntry, sizeof(int32_t));
		Writer.WriteInteger(Cmd.PreEntryStaticMemoryUsage, sizeof(uint32_t));
		Writer.WriteInteger(InstructionIndex, sizeof(uint32_t));
		Writer.WriteInteger(Cmd.Instructions.size(), sizeof(uint32_t));
		InstructionIndex += Cmd.Instructions.size();
		uint32_t StaticSize = 0;
		for (EXB::InstructionStruct Instruction : Cmd.Instructions)
		{
			if (Instruction.LHSSource != EXB::Source::Unknown || Instruction.RHSSource != EXB::Source::Unknown)
			{
				uint32_t Size = Instruction.DataType == EXB::Type::Vec3f ? 12 : 4;
				if (Instruction.LHSSource == EXB::Source::StaticMem)
					StaticSize = std::max(StaticSize, Instruction.LHSIndexValue + Size);
				if (Instruction.RHSSource == EXB::Source::StaticMem)
					StaticSize = std::max(StaticSize, Instruction.RHSIndexValue + Size);
			}
			else if (Instruction.StaticMemoryIndex != 0xFFFF)
			{
				uint32_t Size = Instruction.DataType == EXB::Type::Vec3f ? 12 : 4;
				StaticSize = std::max(StaticSize, Instruction.StaticMemoryIndex + Size);
			}
		}
		MaxStatic = std::max(MaxStatic, StaticSize);
		Writer.WriteInteger(StaticSize, sizeof(uint32_t));
		
		uint32_t Scratch32Size = 0;
		for (EXB::InstructionStruct Instruction : Cmd.Instructions)
		{
			if (Instruction.LHSSource != EXB::Source::Unknown || Instruction.RHSSource != EXB::Source::Unknown)
			{
				uint32_t Size = Instruction.DataType == EXB::Type::Vec3f ? 12 : 4;
				if (Instruction.LHSSource == EXB::Source::Scratch32)
					Scratch32Size = std::max(Scratch32Size, Instruction.LHSIndexValue + Size);
				if (Instruction.RHSSource == EXB::Source::Scratch32)
					Scratch32Size = std::max(Scratch32Size, Instruction.RHSIndexValue + Size);
			}
		}
		Max32 = std::max(Max32, Scratch32Size);
		Writer.WriteInteger(Scratch32Size, sizeof(uint16_t));
		
		uint32_t Scratch64Size = 0;
		for (EXB::InstructionStruct Instruction : Cmd.Instructions)
		{
			if (Instruction.LHSSource != EXB::Source::Unknown || Instruction.RHSSource != EXB::Source::Unknown)
			{
				uint32_t Size = Instruction.DataType == EXB::Type::Vec3f ? 12 : 4;
				if (Instruction.LHSSource == EXB::Source::Scratch32)
					Scratch64Size = std::max(Scratch64Size, Instruction.LHSIndexValue + Size);
				if (Instruction.RHSSource == EXB::Source::Scratch32)
					Scratch64Size = std::max(Scratch64Size, Instruction.RHSIndexValue + Size);
			}
		}
		Max64 = std::max(Max64, Scratch64Size);
		Writer.WriteInteger(Scratch64Size, sizeof(uint16_t));
		
		if (Cmd.Instructions[Cmd.Instructions.size() - 2].LHSSource != EXB::Source::Unknown && (Cmd.Instructions[Cmd.Instructions.size() - 2].LHSSource == EXB::Source::ParamTbl || Cmd.Instructions[Cmd.Instructions.size() - 2].LHSSource == EXB::Source::Output))
		{
			Writer.WriteInteger((uint16_t)Cmd.Instructions[Cmd.Instructions.size() - 2].DataType, sizeof(uint16_t));
		}
		else
		{
			Writer.WriteInteger(1, sizeof(uint16_t));
		}
		if (Cmd.Instructions[0].RHSSource != EXB::Source::Unknown && (Cmd.Instructions[0].RHSSource == EXB::Source::ParamTbl || Cmd.Instructions[0].RHSSource == EXB::Source::Input))
		{
			Writer.WriteInteger((uint16_t)Cmd.Instructions[Cmd.Instructions.size() - 2].DataType, sizeof(uint16_t));
		}
		else
		{
			Writer.WriteInteger(1, sizeof(uint16_t));
		}
	}

	std::vector<EXB::InstructionStruct> NewInstructions;

	for (EXB::CommandInfoStruct Cmd : this->Commands)
	{
		for (EXB::InstructionStruct Inst : Cmd.Instructions)
		{
			NewInstructions.push_back(Inst);
		}
	}
	this->Instructions = NewInstructions;

	uint32_t CommandStart = Writer.GetPosition();
	Writer.WriteInteger(NewInstructions.size(), sizeof(uint32_t));
	std::vector<int> SignatureOffsets;

	for (EXB::InstructionStruct Instruction : NewInstructions)
	{
		if (Instruction.Type != EXB::Command::Terminator)
		{
			std::cout << "Not terminator\n";
			std::cout << "InstType: " << (int)Instruction.Type << "\n";
			Writer.WriteInteger((uint8_t)Instruction.Type, sizeof(uint8_t));
			Writer.WriteInteger((uint8_t)Instruction.DataType, sizeof(uint8_t));
			
			if (Instruction.LHSSource != EXB::Source::Unknown)
				Writer.WriteInteger((uint8_t)Instruction.LHSSource, sizeof(uint8_t));
			if (Instruction.RHSSource != EXB::Source::Unknown)
				Writer.WriteInteger((uint8_t)Instruction.RHSSource, sizeof(uint8_t));
			
			if (Instruction.LHSIndexValue != 0xFFFF)
				Writer.WriteInteger((uint16_t)Instruction.LHSIndexValue, sizeof(uint16_t));
			if (Instruction.RHSIndexValue != 0xFFFF)
				Writer.WriteInteger((uint16_t)Instruction.RHSIndexValue, sizeof(uint16_t));
		
			if (Instruction.StaticMemoryIndex != 0xFFFF)
				Writer.WriteInteger(Instruction.StaticMemoryIndex, sizeof(uint16_t));

			if (Instruction.Signature != "MapEditor_EXB_NoVal")
			{
				AddToStringTable(Instruction.Signature, StringTable);
				if (std::find(SignatureOffsets.begin(), SignatureOffsets.end(), GetOffsetInStringTable(Instruction.Signature, StringTable)) == SignatureOffsets.end())
				{
					SignatureOffsets.push_back(GetOffsetInStringTable(Instruction.Signature, StringTable));
				}
				Writer.WriteInteger((uint32_t)std::distance(SignatureOffsets.begin(), std::find(SignatureOffsets.begin(), SignatureOffsets.end(), GetOffsetInStringTable(Instruction.Signature, StringTable))), sizeof(uint32_t));
				std::cout << "Sig: " << Instruction.Signature << std::endl;
			}
		}
		else
		{
			Writer.WriteInteger(1, sizeof(uint8_t));
			Writer.Seek(7, BinaryVectorWriter::Position::Current);
		}
	}

	uint32_t SigStart = Writer.GetPosition();
	Writer.WriteInteger(SignatureOffsets.size(), sizeof(uint32_t));
	for (int Offset : SignatureOffsets)
	{
		Writer.WriteInteger((uint32_t)Offset, sizeof(uint32_t));
	}

	uint32_t ParamStart = Writer.GetPosition();
	uint32_t StringStart = ParamStart;
	for (EXB::InstructionStruct Instruction : NewInstructions)
	{
		if (*reinterpret_cast<uint32_t*>(&Instruction.LHSValue) != 0xFFFF)
		{
			if (Instruction.LHSSource == EXB::Source::ParamTbl)
			{
				Writer.Seek(ParamStart + Instruction.LHSIndexValue, BinaryVectorWriter::Position::Begin);
				if (Instruction.DataType == EXB::Type::Vec3f)
				{
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&reinterpret_cast<Vector3F*>(&Instruction.LHSValue)->GetRawData()[0]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&reinterpret_cast<Vector3F*>(&Instruction.LHSValue)->GetRawData()[1]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&reinterpret_cast<Vector3F*>(&Instruction.LHSValue)->GetRawData()[2]), sizeof(float));
					StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
				}
				else if (Instruction.DataType == EXB::Type::S32)
				{
					Writer.WriteInteger(*reinterpret_cast<uint32_t*>(&Instruction.LHSValue), sizeof(uint32_t));
					StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
				}
				else if (Instruction.DataType == EXB::Type::F32)
				{
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Instruction.LHSValue), sizeof(float));
					StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
				}
				else if (Instruction.DataType == EXB::Type::S32)
				{
					Writer.WriteInteger((uint32_t)*reinterpret_cast<bool*>(&Instruction.LHSValue), sizeof(uint32_t));
					StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
				}
			}
			if (Instruction.LHSSource == EXB::Source::ParamTblStr)
			{
				Writer.Seek(ParamStart + Instruction.LHSIndexValue, BinaryVectorWriter::Position::Begin);
				AddToStringTable(*reinterpret_cast<std::string*>(&Instruction.LHSValue), StringTable);
				Writer.WriteInteger(GetOffsetInStringTable(*reinterpret_cast<std::string*>(&Instruction.LHSValue), StringTable), sizeof(uint32_t));
				StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
			}

			if (Instruction.RHSSource == EXB::Source::ParamTbl)
			{
				Writer.Seek(ParamStart + Instruction.RHSIndexValue, BinaryVectorWriter::Position::Begin);
				if (Instruction.DataType == EXB::Type::Vec3f)
				{
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&reinterpret_cast<Vector3F*>(&Instruction.RHSValue)->GetRawData()[0]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&reinterpret_cast<Vector3F*>(&Instruction.RHSValue)->GetRawData()[1]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&reinterpret_cast<Vector3F*>(&Instruction.RHSValue)->GetRawData()[2]), sizeof(float));
					StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
				}
				else if (Instruction.DataType == EXB::Type::S32)
				{
					Writer.WriteInteger(*reinterpret_cast<uint32_t*>(&Instruction.RHSValue), sizeof(uint32_t));
					StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
				}
				else if (Instruction.DataType == EXB::Type::F32)
				{
					Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&Instruction.RHSValue), sizeof(float));
					StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
				}
				else if (Instruction.DataType == EXB::Type::S32)
				{
					Writer.WriteInteger((uint32_t) * reinterpret_cast<bool*>(&Instruction.RHSValue), sizeof(uint32_t));
					StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
				}
			}
			if (Instruction.RHSSource == EXB::Source::ParamTblStr)
			{
				Writer.Seek(ParamStart + Instruction.RHSIndexValue, BinaryVectorWriter::Position::Begin);
				AddToStringTable(*reinterpret_cast<std::string*>(&Instruction.RHSValue), StringTable);
				Writer.WriteInteger(GetOffsetInStringTable(*reinterpret_cast<std::string*>(&Instruction.RHSValue), StringTable), sizeof(uint32_t));
				StringStart = std::max(StringStart, (uint32_t)Writer.GetPosition());
			}
		}
	}

	Writer.Seek(StringStart, BinaryVectorWriter::Position::Begin);
	for (std::string String : StringTable)
	{
		Writer.WriteBytes(String.c_str());
		Writer.WriteByte(0x00);
	}

	uint32_t End = Writer.GetPosition();

	Writer.Seek(8, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(MaxStatic, sizeof(uint32_t));
	Writer.WriteInteger(EXBInstanceCount, sizeof(uint32_t));
	Writer.WriteInteger(Max32, sizeof(uint32_t));
	Writer.WriteInteger(Max64, sizeof(uint32_t));
	Writer.WriteInteger(44, sizeof(uint32_t));
	Writer.WriteInteger(CommandStart, sizeof(uint32_t));
	Writer.WriteInteger(SigStart, sizeof(uint32_t));
	Writer.WriteInteger(ParamStart, sizeof(uint32_t));
	Writer.WriteInteger(StringStart, sizeof(uint32_t));

	return Writer.GetData();
}