#include "AINB.h"

#include "Logger.h"
#include "StarlightData.h"
#include <iostream>

std::ostream& operator<<(std::ostream& os, Vector3F& vec) {
	os << vec.GetX() << ", " << vec.GetY() << ", " << vec.GetZ();
	return os;
}

std::string AINBFile::ValueToString(AINBFile::AINBValue Value)
{
	std::stringstream ss;
	std::visit([&](auto& elem) { ss << elem; }, Value);
	return ss.str();
}

std::string AINBFile::NodeTypeToString(AINBFile::NodeTypes Type)
{
	switch (Type)
	{
	case AINBFile::NodeTypes::UserDefined:
		return "UserDefined";
	case AINBFile::NodeTypes::Element_S32Selector:
		return "Element_S32Selector";
	case AINBFile::NodeTypes::Element_Sequential:
		return "Element_Sequential";
	case AINBFile::NodeTypes::Element_Simultaneous:
		return "Element_Simultaneous";
	case AINBFile::NodeTypes::Element_F32Selector:
		return "Element_F32Selector";
	case AINBFile::NodeTypes::Element_StringSelector:
		return "Element_StringSelector";
	case AINBFile::NodeTypes::Element_RandomSelector:
		return "Element_RandomSelector";
	case AINBFile::NodeTypes::Element_BoolSelector:
		return "Element_BoolSelector";
	case AINBFile::NodeTypes::Element_Fork:
		return "Element_Fork";
	case AINBFile::NodeTypes::Element_Join:
		return "Element_Join";
	case AINBFile::NodeTypes::Element_Alert:
		return "Element_Alert";
	case AINBFile::NodeTypes::Element_Expression:
		return "Element_Expression";
	case AINBFile::NodeTypes::Element_ModuleIF_Input_S32:
		return "Element_ModuleIF_Input_S32";
	case AINBFile::NodeTypes::Element_ModuleIF_Input_F32:
		return "Element_ModuleIF_Input_F32";
	case AINBFile::NodeTypes::Element_ModuleIF_Input_Vec3f:
		return "Element_ModuleIF_Input_Vec3f";
	case AINBFile::NodeTypes::Element_ModuleIF_Input_String:
		return "Element_ModuleIF_Input_String";
	case AINBFile::NodeTypes::Element_ModuleIF_Input_Bool:
		return "Element_ModuleIF_Input_Bool";
	case AINBFile::NodeTypes::Element_ModuleIF_Input_Ptr:
		return "Element_ModuleIF_Input_Ptr";
	case AINBFile::NodeTypes::Element_ModuleIF_Output_S32:
		return "Element_ModuleIF_Output_S32";
	case AINBFile::NodeTypes::Element_ModuleIF_Output_F32:
		return "Element_ModuleIF_Output_F32";
	case AINBFile::NodeTypes::Element_ModuleIF_Output_Vec3f:
		return "Element_ModuleIF_Output_Vec3f";
	case AINBFile::NodeTypes::Element_ModuleIF_Output_String:
		return "Element_ModuleIF_Output_String";
	case AINBFile::NodeTypes::Element_ModuleIF_Output_Bool:
		return "Element_ModuleIF_Output_Bool";
	case AINBFile::NodeTypes::Element_ModuleIF_Output_Ptr:
		return "Element_ModuleIF_Output_Ptr";
	case AINBFile::NodeTypes::Element_ModuleIF_Child:
		return "Element_ModuleIF_Child";
	case AINBFile::NodeTypes::Element_StateEnd:
		return "Element_StateEnd";
	case AINBFile::NodeTypes::Element_SplitTiming:
		return "Element_SplitTiming";
	}
}

std::string AINBFile::ReadStringFromStringPool(BinaryVectorReader* Reader, uint32_t Offset) {
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

AINBFile::GUIDData AINBFile::ReadGUID(BinaryVectorReader* Reader) {
	AINBFile::GUIDData Data;
	Data.Part1 = Reader->ReadUInt32();
	Data.Part2 = Reader->ReadUInt16();
	Data.Part3 = Reader->ReadUInt16();
	Data.Part4 = Reader->ReadUInt16();

	Reader->Read(reinterpret_cast<char*>(Data.Part5), 6);

	return Data;
}

AINBFile::AttachmentEntry AINBFile::ReadAttachmentEntry(BinaryVectorReader* Reader) {
	AINBFile::AttachmentEntry Entry;
	Entry.Name = ReadStringFromStringPool(Reader, Reader->ReadUInt32());
	Entry.Offset = Reader->ReadUInt32();
	Entry.EXBFunctionCount = Reader->ReadUInt16();
	Entry.EXBIOSize = Reader->ReadUInt16();
	Entry.Name = Reader->ReadUInt32();
	return Entry;
}

AINBFile::InputEntry AINBFile::ReadInputEntry(BinaryVectorReader* Reader, int Type)
{
	AINBFile::InputEntry Entry;
	Entry.Name = ReadStringFromStringPool(Reader, Reader->ReadUInt32());
	if (Type == 5)
	{ //userdefined
		Entry.Class = ReadStringFromStringPool(Reader, Reader->ReadUInt32());
	}
	Entry.NodeIndex = Reader->ReadInt16();
	Entry.ParameterIndex = Reader->ReadInt16();
	if (Entry.NodeIndex <= -100 && Entry.NodeIndex >= -8192)
	{
		Entry.MultiIndex = -100 - Entry.NodeIndex;
		Entry.MultiCount = Entry.ParameterIndex;
	}
	uint16_t Index = Reader->ReadUInt16();
	uint16_t Flags = Reader->ReadUInt16();
	if (Flags > 0) {
		if (Flags & 0x80) {
			Entry.Flags.push_back(FlagsStruct::PulseThreadLocalStorage);
		}
		if (Flags & 0x100) {
			Entry.Flags.push_back(FlagsStruct::SetPointerFlagBitZero);
		}
		if ((Flags & 0xc200) == 0xc200) {
			Entry.EXBIndex = Index;
			Entry.Function = this->EXBFile.Commands[Entry.EXBIndex];
			this->Functions[Entry.EXBIndex] = Entry.Function;
		}
		else if (Flags & 0x8000)
		{
			Entry.GlobalParametersIndex = Index;
		}
	}

	if (Type == 0) { //int
		Entry.Value = Reader->ReadUInt32();
	}
	if (Type == 1) { //bool
		Entry.Value = Reader->ReadUInt32();
	}
	if (Type == 2) { //float
		Entry.Value = Reader->ReadFloat();
	}
	if (Type == 3) { //string
		Entry.Value = ReadStringFromStringPool(Reader, Reader->ReadUInt32());
	}
	if (Type == 4) { //vec3f
		//Don't try to change this... I don't know why, but you have to store the data first in variables and then in a vec3f
		float X = Reader->ReadFloat();
		float Y = Reader->ReadFloat();
		float Z = Reader->ReadFloat();
		Vector3F Vec3f(X, Y, Z);
		Entry.Value = Vector3F(X, Y, Z);
	}
	if (Type == 5) { //userdefined
		Entry.Value = (uint32_t)Reader->ReadUInt32();
	}
	Entry.ValueType = Type;
	return Entry;
}

AINBFile::OutputEntry AINBFile::ReadOutputEntry(BinaryVectorReader* Reader, int Type) {
	AINBFile::OutputEntry Entry;
	uint32_t Flags = Reader->ReadUInt32();
	Entry.Name = ReadStringFromStringPool(Reader, Flags & 0x3FFFFFFF);
	uint32_t Flag = Flags & 0x80000000;
	if (Flag > 0) {
		Entry.SetPointerFlagsBitZero = true;
	}
	if (Type == 5) {
		Entry.Class = ReadStringFromStringPool(Reader, Reader->ReadUInt32());
	}
	return Entry;
}

AINBFile::AINBFile(std::vector<unsigned char> Bytes, bool SkipErrors) {
	BinaryVectorReader Reader(Bytes);

	this->EXBInstances = 0;

	//Checking magic, should be AIB
	char magic[4];
	Reader.Read(magic, 3);
	Reader.Seek(1, BinaryVectorReader::Position::Current);
	magic[3] = '\0';
	if (strcmp(magic, "AIB") != 0) {
		Logger::Error("AINBDecoder", "Wrong magic, expected AIB");
		return;
	}

	strcpy_s(Header.Magic, 4, magic);

	Header.Version = Reader.ReadUInt32();

	if (Header.Version != 0x404 && Header.Version != 0x407) {
		Logger::Error("AINBDecoder", "Wrong version, expected 0x404(4.4) or 0x407(4.7, got " + std::to_string(Header.Version));
		return;
	}

	Header.FileNameOffset = Reader.ReadUInt32();
	Header.CommandCount = Reader.ReadUInt32();
	Header.NodeCount = Reader.ReadUInt32();
	Header.PreconditionCount = Reader.ReadUInt32();
	Header.AttachmentCount = Reader.ReadUInt32();
	Header.OutputCount = Reader.ReadUInt32();
	Header.GlobalParameterOffset = Reader.ReadUInt32();
	Header.StringOffset = Reader.ReadUInt32();
	Header.FileName = ReadStringFromStringPool(&Reader, Header.FileNameOffset);

	Header.ResolveOffset = Reader.ReadUInt32();
	Header.ImmediateOffset = Reader.ReadUInt32();
	Header.ResidentUpdateOffset = Reader.ReadUInt32();
	Header.IOOffset = Reader.ReadUInt32();
	Header.MultiOffset = Reader.ReadUInt32();
	Header.AttachmentOffset = Reader.ReadUInt32();
	Header.AttachmentIndexOffset = Reader.ReadUInt32();
	Header.EXBOffset = Reader.ReadUInt32();
	Header.ChildReplacementOffset = Reader.ReadUInt32();
	Header.PreconditionOffset = Reader.ReadUInt32();
	Header.x50Section = Reader.ReadUInt32();
	Header.x54Value = Reader.ReadUInt32();
	Header.x58Section = Reader.ReadUInt32();
	Header.EmbedAinbOffset = Reader.ReadUInt32();
	Header.FileCategory = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
	Header.x64Value = Reader.ReadUInt32();
	Header.EntryStringOffset = Reader.ReadUInt32();
	Header.x6cSection = Reader.ReadUInt32();
	Header.FileHashOffset = Reader.ReadUInt32();
	
	/*
	if (Header.NodeCount == 0)
	{
		Logger::Error("AINBDecoder", "AINB is empty (0 nodes)");
		if(SkipErrors) return;
	}
	*/

	/* Commands */
	this->Commands.resize(Header.CommandCount);
	for (int i = 0; i < Header.CommandCount; i++) {
		Command Cmd;
		Cmd.Name = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
		Cmd.GUID = ReadGUID(&Reader);
		Cmd.LeftNodeIndex = (int16_t)Reader.ReadUInt16();
		Cmd.RightNodeIndex = (int16_t)Reader.ReadUInt16() - 1;
		this->Commands[i] = Cmd;
	}
	int CommandEnd = Reader.GetPosition();

	/* Global Parameters */
	Reader.Seek(Header.GlobalParameterOffset, BinaryVectorReader::Position::Begin);
	this->GlobalHeader.resize(6);
	for (int i = 0; i < 6; i++) {
		GlobalHeaderEntry GHeaderEntry;
		GHeaderEntry.Count = Reader.ReadUInt16();
		GHeaderEntry.Index = Reader.ReadUInt16();
		GHeaderEntry.Offset = Reader.ReadUInt16();
		Reader.Seek(2, BinaryVectorReader::Position::Current);
		this->GlobalHeader[i] = GHeaderEntry;
	}
	this->GlobalParameters.resize(this->GlobalHeader.size());
	for (int j = 0; j < this->GlobalHeader.size(); j++) {
		GlobalHeaderEntry Type = this->GlobalHeader[j];
		std::vector<GlobalEntry> Parameters(Type.Count);
		for (int i = 0; i < Type.Count; i++) {
			GlobalEntry GEntry;
			uint32_t BitField = Reader.ReadUInt32();
			bool ValidIndex = (bool)(BitField >> 31);
			if (ValidIndex) {
				GEntry.Index = (BitField >> 24) & 0b1111111;
				if (GEntry.Index >= MaxGlobalIndex) {
					this->MaxGlobalIndex = GEntry.Index + 1;
				}
			}
			GEntry.Name = ReadStringFromStringPool(&Reader, BitField & 0x3FFFFF);
			GEntry.Notes = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
			Parameters[i] = GEntry;
		}
		this->GlobalParameters[j] = Parameters;
	}
	int Pos = Reader.GetPosition();
	for (int i = 0; i < this->GlobalParameters.size(); i++) {
		GlobalHeaderEntry Type = this->GlobalHeader[i];
		Reader.Seek(Pos + Type.Offset, BinaryVectorReader::Position::Begin);
		for (GlobalEntry& Entry : this->GlobalParameters[i])
		{
			if (i == (int)AINBFile::GlobalType::Int) { //int
				Entry.GlobalValue = (uint32_t)Reader.ReadUInt32();
			}
			if (i == (int)AINBFile::GlobalType::Bool) { //bool
				Entry.GlobalValue = (bool)Reader.ReadUInt32();
			}
			if (i == (int)AINBFile::GlobalType::Float) { //float
				Entry.GlobalValue = Reader.ReadFloat();
			}
			if (i == (int)AINBFile::GlobalType::String) { //string
				Entry.GlobalValue = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
			}
			if (i == (int)AINBFile::GlobalType::Vec3f) { //vec3f
				Entry.GlobalValue = Vector3F(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
			}
			if (i == (int)AINBFile::GlobalType::UserDefined) { //userdefined
				Entry.GlobalValue = "None";
			}
			Entry.GlobalValueType = i;
		}
	}
	this->GlobalReferences.resize(this->MaxGlobalIndex);
	for (int i = 0; i < this->MaxGlobalIndex; i++) {
		GlobalFileRef FileRef;
		FileRef.FileName = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
		FileRef.NameHash = Reader.ReadUInt32();
		FileRef.UnknownHash1 = Reader.ReadUInt32();
		FileRef.UnknownHash2 = Reader.ReadUInt32();
		this->GlobalReferences[i] = FileRef;
	}
	for (int i = 0; i < this->GlobalParameters.size(); i++) {
		for (GlobalEntry& Entry : this->GlobalParameters[i]) {
			if (Entry.Index != 0xFFFFFFFF) {
				Entry.FileReference = this->GlobalReferences[Entry.Index];
			}
		}
	}
	/*
	std::vector<std::vector<GlobalEntry>> GlobalParametersNew;
	for (const std::vector<GlobalEntry>& InnerVector : this->GlobalParameters) {
		if (!InnerVector.empty()) {
			GlobalParametersNew.push_back(InnerVector);
		}
	}
	this->GlobalParameters = GlobalParametersNew;
	*/

	/* EXB Section */
	if (Header.EXBOffset != 0) {		
		Reader.Seek(Header.EXBOffset, BinaryVectorReader::Position::Begin);
		std::vector<unsigned char> Bytes(Reader.GetSize() - Reader.GetPosition());
		Reader.ReadStruct(Bytes.data(), Reader.GetSize() - Reader.GetPosition());
		this->EXBFile = EXB(Bytes);
		this->Functions.resize(EXBFile.Commands.size());
	}

	/* Immediate Parameters */
	Reader.Seek(Header.ImmediateOffset, BinaryVectorReader::Position::Begin);
	this->ImmediateOffsets.resize(6);
	for (int i = 0; i < 6; i++) {
		this->ImmediateOffsets[i] = Reader.ReadUInt32();
	}
	this->ImmediateParameters.resize(6);
	for (int i = 0; i < this->ImmediateOffsets.size(); i++) {
		Reader.Seek(this->ImmediateOffsets[i], BinaryVectorReader::Position::Begin);
		if (i < 5) {
			while (Reader.GetPosition() < this->ImmediateOffsets[i + 1]) {
				ImmediateParameter Parameter;
				Parameter.Name = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
				if (i == 5) { //userdefined
					Parameter.Class = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
				}
				uint16_t Index = Reader.ReadUInt16();
				uint16_t Flags = Reader.ReadUInt16();

				if (Flags > 0) {
					if (Flags & 0x80) {
						Parameter.Flags.push_back(FlagsStruct::PulseThreadLocalStorage);
					}
					if (Flags & 0x100) {
						Parameter.Flags.push_back(FlagsStruct::SetPointerFlagBitZero);
					}
					if ((Flags & 0xc200) == 0xc200) {
						Parameter.EXBIndex = Index;
						Parameter.Function = this->EXBFile.Commands[Parameter.EXBIndex];
						this->Functions[Parameter.EXBIndex] = Parameter.Function;
					}
					else if (Flags & 0x8000)
					{
						Parameter.GlobalParametersIndex = Index;
					}
				}

				if (i == 0) { //int
					Parameter.Value = Reader.ReadUInt32();
				}
				if (i == 1) { //bool
					Parameter.Value = (bool)Reader.ReadUInt32();
				}
				if (i == 2) { //float
					Parameter.Value = Reader.ReadFloat();
				}
				if (i == 3) { //string
					Parameter.Value = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
				}
				if (i == 4) { //vec3f
					//Don't try to change this... I don't know why, but you have to store the data first in variables and then in a vec3f
					float X = Reader.ReadFloat();
					float Y = Reader.ReadFloat();
					float Z = Reader.ReadFloat();
					Vector3F Vec3f(X, Y, Z);
					Parameter.Value = Vector3F(X, Y, Z);
				}
				Parameter.ValueType = i;

				this->ImmediateParameters[i].push_back(Parameter);
			}
		}
		else {
			while (Reader.GetPosition() < Header.IOOffset) {
				ImmediateParameter Parameter;

				Parameter.Name = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
				if (i == 5) { //userdefined
					Parameter.Class = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
				}
				uint16_t Index = Reader.ReadUInt16();
				uint16_t Flags = Reader.ReadUInt16();

				if (Flags > 0) {
					if (Flags & 0x80) {
						Parameter.Flags.push_back(FlagsStruct::PulseThreadLocalStorage);
					}
					if (Flags & 0x100) {
						Parameter.Flags.push_back(FlagsStruct::SetPointerFlagBitZero);
					}
					if ((Flags & 0xc200) == 0xc200) {
						Logger::Warning("AINBDecoder", "Found ImmediateParameter using EXB function, which is currently unsupported. Saving will break this file, please send the following message to mrmystery0778 on Discord");
						Logger::Warning("AINBDecoder", "Line 439, Flag 0xc200 found. GlobalParamIndex skipped. Name: " + this->Header.FileName + ", Category: " + this->Header.FileCategory);
					}
					else if (Flags & 0x8000)
					{
						Parameter.GlobalParametersIndex = Index;
					}
				}

				if (i == 0) { //int
					Parameter.Value = Reader.ReadUInt32();
				}
				if (i == 1) { //bool
					Parameter.Value = (bool)Reader.ReadUInt32();
				}
				if (i == 2) { //float
					Parameter.Value = Reader.ReadFloat();
				}
				if (i == 3) { //string
					Parameter.Value = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
				}
				if (i == 4) { //vec3f
					Parameter.Value = Vector3F(Reader.ReadFloat(), Reader.ReadFloat(), Reader.ReadFloat());
				}
				Parameter.ValueType = i;
				this->ImmediateParameters[i].push_back(Parameter);
			}
		}
	}

	/* Attachment Parameters */
	if (Header.AttachmentCount >= 1) {
		Reader.Seek(Header.AttachmentOffset, BinaryVectorReader::Position::Begin);
		this->AttachmentParameters.resize(Header.AttachmentCount);
		this->AttachmentParameters[0] = ReadAttachmentEntry(&Reader);
		if (Header.AttachmentCount >= 2)
		{
			int i = 1;
			while (Reader.GetPosition() < this->AttachmentParameters[0].Offset) {
				this->AttachmentParameters[i] = ReadAttachmentEntry(&Reader);
				i++;
			}
		}
		for (AttachmentEntry& Param : this->AttachmentParameters) {
			Reader.Seek(4, BinaryVectorReader::Position::Current);
			std::vector<std::vector<ImmediateParameter>> Parameters(6);
			for (int i = 0; i < 6; i++) {
				uint32_t Index = Reader.ReadUInt32();
				if (Index == 65535)
				{
					Logger::Error("AINBDecoder", "Attachment Index was -1");
					break;
				}
				uint32_t Count = Reader.ReadUInt32();
				if (Count > 1000)
				{
					Logger::Error("AINBDecoder", "Attachment Count was too big (>1000)");
					break;
				}
				Parameters[i].resize(Count);
				for (int j = 0; j < Count; j++) {
					if (j + Index >= this->ImmediateParameters[i].size())
					{
						Parameters[i].resize(j);
						break;
					}
					Parameters[i][j] = this->ImmediateParameters[i][Index + j];
				}
				Reader.Seek(48, BinaryVectorReader::Position::Current);
			}
			Param.Parameters = Parameters;
		}
		Reader.Seek(Header.AttachmentIndexOffset, BinaryVectorReader::Position::Begin);
		while (Reader.GetPosition() < Header.AttachmentOffset) {
			this->AttachmentArray.push_back(Reader.ReadUInt32());
		}
	}

	/* IO Parameters */
	Reader.Seek(Header.IOOffset, BinaryVectorReader::Position::Begin);
	for (int i = 0; i < 12; i++) {
		if (i % 2 == 0) {
			this->IOOffsets[0].push_back(Reader.ReadUInt32());
		}
		else {
			this->IOOffsets[1].push_back(Reader.ReadUInt32());
		}
	}
	this->InputParameters.resize(6);
	this->OutputParameters.resize(6);
	//IOOffsets[x],  x = 0 -> Input,  x = 1 -> Output
	for (int i = 0; i < 6; i++) {
		while (Reader.GetPosition() < this->IOOffsets[1][i]) {
			this->InputParameters[i].push_back(ReadInputEntry(&Reader, i));
		}
		if (i < 5) {
			while (Reader.GetPosition() < this->IOOffsets[0][i + 1]) {
				this->OutputParameters[i].push_back(ReadOutputEntry(&Reader, i));
			}
		}
		else {
			while (Reader.GetPosition() < Header.MultiOffset) {
				this->OutputParameters[i].push_back(ReadOutputEntry(&Reader, i));
			}
		}
	}

	/* Multi Parameters */
	for (int Type = 0; Type < this->InputParameters.size(); Type++) {
		for (InputEntry& Parameter : this->InputParameters[Type]) {
			if (Parameter.MultiIndex != -1) {
				Reader.Seek(Header.MultiOffset + Parameter.MultiIndex * 8, BinaryVectorReader::Position::Begin);
				Parameter.Sources.resize(Parameter.MultiCount);
				for (int i = 0; i < Parameter.MultiCount; i++) {
					MultiEntry Entry;

					Entry.NodeIndex = Reader.ReadUInt16();
					Entry.ParameterIndex = Reader.ReadUInt16();
					uint16_t Index = Reader.ReadUInt16();
					uint16_t Flags = Reader.ReadUInt16();
					if (Flags > 0) {
						if (Flags & 0x80) {
							Entry.Flags.push_back(FlagsStruct::PulseThreadLocalStorage);
						}
						if (Flags & 0x100) {
							Entry.Flags.push_back(FlagsStruct::SetPointerFlagBitZero);
						}
						if ((Flags & 0xc200) == 0xc200) {
							Entry.EXBIndex = Index;
							Entry.Function = this->EXBFile.Commands[Entry.EXBIndex];
							this->Functions[Entry.EXBIndex] = Entry.Function;
						}
						else if (Flags & 0x8000)
						{
							Entry.GlobalParametersIndex = Index;
						}
					}

					Parameter.Sources[i] = Entry;
				}
			}
		}
	}

	/* Resident Update Array */
	Reader.Seek(Header.ResidentUpdateOffset, BinaryVectorReader::Position::Begin);
	if (Header.ResidentUpdateOffset != Header.PreconditionOffset) {
		std::vector<uint32_t> Offsets;
		Offsets.push_back(Reader.ReadUInt32());
		while (Reader.GetPosition() < Offsets[0]) {
			Offsets.push_back(Reader.ReadUInt32());
		}
		for (uint32_t Offset : Offsets) {
			Reader.Seek(Offset, BinaryVectorReader::Position::Begin);

			ResidentEntry Entry;
			uint32_t Flags = Reader.ReadUInt32();
			if (Flags >> 31) {
				Entry.Flags.push_back(FlagsStruct::UpdatePostCurrentCommandCalc);
			}
			if (Flags & 1) {
				Entry.Flags.push_back(FlagsStruct::IsValidUpdate);
			}
			if (std::find(Entry.Flags.begin(), Entry.Flags.end(), FlagsStruct::IsValidUpdate) == Entry.Flags.end()) {
				Entry.String = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
			}

			this->ResidentUpdateArray.push_back(Entry);
		}
	}

	/* Precondition Nodes */
	Reader.Seek(Header.PreconditionOffset, BinaryVectorReader::Position::Begin);
	uint32_t End = Header.EXBOffset != 0 ? Header.EXBOffset : Header.EmbedAinbOffset;
	while (Reader.GetPosition() < End) {
		this->PreconditionNodes.push_back(Reader.ReadUInt16());
		Reader.Seek(2, BinaryVectorReader::Position::Current);
	}

	/* Entry String */
	Reader.Seek(Header.EntryStringOffset, BinaryVectorReader::Position::Begin);
	uint32_t Count = Reader.ReadUInt32();
	this->EntryStrings.resize(Count);
	for (int i = 0; i < Count; i++) {
		EntryStringEntry Entry;
		Entry.NodeIndex = Reader.ReadUInt32();
		Entry.MainState = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
		Entry.State = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
		this->EntryStrings[i] = Entry;
	}

	if (this->EXBFile.Loaded)
	{
		this->Functions.shrink_to_fit();
		for (EXB::CommandInfoStruct Command : this->EXBFile.Commands)
		{
			bool FoundCmd = false;
			for (EXB::CommandInfoStruct Child : this->Functions)
			{
				if (Child.BaseIndexPreCommandEntry == Command.BaseIndexPreCommandEntry &&
					Child.InstructionCount == Command.InstructionCount &&
					Child.InputDataType == Command.InputDataType &&
					Child.StaticMemorySize == Command.StaticMemorySize)
				{
					FoundCmd = true;
					break;
				}
			}

			if (!FoundCmd)
			{
				this->Functions.push_back(Command);
			}
		}
	}

	/* Nodes - initialize all nodes and assign corresponding parameters */
	Reader.Seek(CommandEnd, BinaryVectorReader::Position::Begin);
	this->Nodes.resize(Header.NodeCount);
	for (int i = 0; i < Header.NodeCount; i++) {
		Node Node;
		uint32_t EXBCount = 0;

		Node.Type = Reader.ReadUInt16();
		Node.NodeIndex = Reader.ReadUInt16();
		Node.AttachmentCount = Reader.ReadUInt16();

		uint8_t Flags = Reader.ReadUInt8();

		if (Flags > 0) {
			if (Flags & 0b1) {
				Node.Flags.push_back(FlagsStruct::IsPreconditionNode);
			}
			if (Flags & 0b10) {
				Node.Flags.push_back(FlagsStruct::IsExternalAINB);
			}
			if (Flags & 0b100) {
				Node.Flags.push_back(FlagsStruct::IsResidentNode);
			}
		}

		Reader.Seek(1, BinaryVectorReader::Position::Current);

		Node.Name = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());

		Node.NameHash = Reader.ReadUInt32();

		Reader.Seek(4, BinaryVectorReader::Position::Current);

		Node.ParametersOffset = Reader.ReadUInt32();
		Node.EXBFunctionCount = Reader.ReadUInt16();
		Node.EXBIOSize = Reader.ReadUInt16();
		Node.MultiParamCount = Reader.ReadUInt16();

		Reader.Seek(2, BinaryVectorReader::Position::Current);

		Node.BaseAttachmentIndex = Reader.ReadUInt32();
		Node.BasePreconditionNode = Reader.ReadUInt16();
		Node.PreconditionCount = Reader.ReadUInt16();

		Reader.Seek(4, BinaryVectorReader::Position::Current);

		Node.GUID = ReadGUID(&Reader);

		if (Node.PreconditionCount > 0) {
			for (int i = 0; i < Node.PreconditionCount; i++) {
				Node.PreconditionNodes.push_back(this->PreconditionNodes[Node.BasePreconditionNode] + i);
			}
		}

		if (Node.AttachmentCount > 0) {
			for (int i = 0; i < Node.AttachmentCount; i++) {
				Node.Attachments.push_back(this->AttachmentParameters[this->AttachmentArray[Node.BaseAttachmentIndex + i]]);
			}
			for (AINBFile::AttachmentEntry Attachment : Node.Attachments)
			{
				bool ParametersEmpty = true;
				for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
				{
					if (!Attachment.Parameters[Type].empty())
					{
						ParametersEmpty = false;
						break;
					}
				}
				if (!ParametersEmpty)
				{
					for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
					{
						for (AINBFile::ImmediateParameter Entry1 : Attachment.Parameters[Type])
						{
							if (Entry1.EXBIndex != 0xFFFF)
							{
								EXBCount++;
							}
						}
					}
				}
			}
		}

		int NextNodeOffset = Reader.GetPosition();

		std::vector<std::vector<ImmediateParameter>> LocalImmediateParameters(6);
		Reader.Seek(Node.ParametersOffset, BinaryVectorReader::Position::Begin);
		for (int i = 0; i < 6; i++) {
			uint32_t Index = Reader.ReadUInt32();
			uint32_t Count = Reader.ReadUInt32();

			for (int j = 0; j < Count; j++) {
				LocalImmediateParameters[i].push_back(this->ImmediateParameters[i][Index + j]);
				if (LocalImmediateParameters[i].size() <= (Index + j)) continue;
				if (LocalImmediateParameters[i][Index + j].EXBIndex != 0xFFFF)
					EXBCount++;
			}
		}
		if (LocalImmediateParameters.size() > 0) {
			Node.ImmediateParameters = LocalImmediateParameters;
		}
		std::vector<std::vector<InputEntry>> LocalInputParameters(6);
		std::vector<std::vector<OutputEntry>> LocalOutputParameters(6);
		for (int i = 0; i < 6; i++) {
			uint32_t Index = Reader.ReadUInt32();
			uint32_t Count = Reader.ReadUInt32();
			for (int j = 0; j < Count; j++) {
				LocalInputParameters[i].push_back(this->InputParameters[i][Index + j]);
				
				if (LocalInputParameters[i].size() <= (Index + j)) continue;

				if (LocalInputParameters[i][Index + j].EXBIndex != 0xFFFF)
					EXBCount++;
				for (AINBFile::MultiEntry MEntry : LocalInputParameters[i][Index + j].Sources)
				{
					if (MEntry.EXBIndex != 0xFFFF)
						EXBCount++;
				}
			}

			Index = Reader.ReadUInt32();
			Count = Reader.ReadUInt32();
			for (int j = 0; j < Count; j++) {
				LocalOutputParameters[i].push_back(this->OutputParameters[i][Index + j]);
			}
		}
		if (LocalInputParameters.size() > 0) {
			Node.InputParameters = LocalInputParameters;
		}
		if (LocalOutputParameters.size() > 0) {
			Node.OutputParameters = LocalOutputParameters;
		}

		EXBInstances += EXBCount;

		std::vector<uint8_t> Counts;
		std::vector<uint8_t> Indices;
		for (int i = 0; i < 10; i++) {
			Counts.push_back(Reader.ReadUInt8());
			Indices.push_back(Reader.ReadUInt8());
		}

		int Start = Reader.GetPosition();
		if (Counts.size() > 0) {
			for (int i = 0; i < 10; i++) {
				Reader.Seek(Start + Indices[i] * 4, BinaryVectorReader::Position::Begin);
				std::vector<uint32_t> Offsets;
				for (int j = 0; j < Counts[i]; j++) {
					Offsets.push_back(Reader.ReadUInt32());
				}
				for (uint32_t Offset : Offsets) {
					Reader.Seek(Offset, BinaryVectorReader::Position::Begin);
					LinkedNodeInfo Info;
					Info.Type = static_cast<AINBFile::LinkedNodeMapping>(i);
					Info.NodeIndex = Reader.ReadUInt32();
					if (i == 0 || i == 4 || i == 5) {
						Info.Parameter = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
					}
					if (i == 2) {
						std::string Ref = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
						if (Ref != "") {
							if (Node.Type == (uint16_t)NodeTypes::Element_BoolSelector) {
								Info.Condition = Ref;
							}
							else {
								Info.ConnectionName = Ref;
							}
						}
						else
						{
							int OffsetIndex = 0;
							for (uint32_t Offset2 : Offsets) {
								if (Offset2 == Offset) {
									break;
								}
								OffsetIndex++;
							}
							bool IsEnd = OffsetIndex == (Offsets.size() - 1);
							if (Node.Type == (uint16_t)NodeTypes::Element_S32Selector) {
								uint16_t Index = Reader.ReadUInt16();
								bool Flag = Reader.ReadUInt16() >> 15; //Is Valid Index
								if (Flag && Index < this->GlobalParameters[0].size()) {
									Node.Input = this->GlobalParameters[0][Index];
								}
								if (IsEnd) {
									Info.Condition = "Default";
								}
								else {
									Info.Condition = std::to_string(Reader.ReadInt32());
								}
							}
							else if (Node.Type == (uint16_t)NodeTypes::Element_F32Selector) {
								uint16_t Index = Reader.ReadUInt16();
								bool Flag = Reader.ReadUInt16() >> 15; //Is Valid Index
								if (Flag && Index < this->GlobalParameters[2].size()) {
									Node.Input = this->GlobalParameters[2][Index];
								}
								if (!IsEnd) {
									Info.ConditionMin = Reader.ReadFloat();
									Reader.Seek(4, BinaryVectorReader::Position::Current);
									Info.ConditionMax = Reader.ReadFloat();
								}
								else {
									Info.DynamicStateName = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
									Info.DynamicStateValue = "Default";
								}
							}
							else if (Node.Type == (uint16_t)NodeTypes::Element_StringSelector) {
								uint16_t Index = Reader.ReadUInt16();
								bool Flag = Reader.ReadUInt16() >> 15; //Is Valid Index
								if (Flag && Index < this->GlobalParameters[3].size()) {
									Node.Input = this->GlobalParameters[3][Index];
								}
								if (IsEnd) {
									Info.DynamicStateName = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
									Info.DynamicStateValue = "Default";
								}
								else {
									Info.Condition = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
								}
							}
							else if (Node.Type == (uint16_t)NodeTypes::Element_RandomSelector) {
								Info.Probability = Reader.ReadFloat();
							}
							else {
								Info.ConnectionName = Ref;
							}
						}
					}
					if (i == 3) {
						Info.UpdateInfo = this->ResidentUpdateArray[Reader.ReadUInt32()];
					}
					Node.LinkedNodes[i].push_back(Info);
				}
			}
		}

		Reader.Seek(NextNodeOffset, BinaryVectorReader::Position::Begin);
		this->Nodes[i] = Node;
	}

	/* Child replacement */
	Reader.Seek(Header.ChildReplacementOffset, BinaryVectorReader::Position::Begin);
	this->IsReplaced = Reader.ReadUInt8();
	Reader.Seek(1, BinaryVectorReader::Position::Current);
	Count = Reader.ReadUInt16();
	int16_t NodeCount = Reader.ReadInt16();
	int16_t AttachmentCount = Reader.ReadInt16();
	this->Replacements.resize(Count);
	for (int i = 0; i < Count; i++) {
		ChildReplace Replace;
		Replace.Type = Reader.ReadUInt8();
		Reader.Seek(1, BinaryVectorReader::Position::Current);
		Replace.NodeIndex = Reader.ReadUInt16();
		if (Replace.Type == 0 || Replace.Type == 1) {
			Replace.ChildIndex = Reader.ReadUInt16();
			if (Replace.Type == 1) {
				Replace.ReplacementIndex = Reader.ReadUInt16();
			}
			else {
				Reader.Seek(2, BinaryVectorReader::Position::Current);
			}
		}
		if (Replace.Type == 2) {
			Replace.AttachmentIndex = Reader.ReadUInt16();
			Reader.Seek(2, BinaryVectorReader::Position::Current);
		}
		this->Replacements[i] = Replace;
	}
	if (this->Replacements.size() > 0) {
		for (ChildReplace& Replacement : this->Replacements) {
			if (Replacement.Type == 0) {
				for (int Type = 0; Type < 10; Type++) {
					for (LinkedNodeInfo& LinkedNode : this->Nodes[Replacement.NodeIndex].LinkedNodes[Type]) {
						if (Replacement.ChildIndex == 0) {
							LinkedNode.IsRemovedAtRuntime = true;
						}
					}
				}
			}
			if (Replacement.Type == 1) {
				int i = 0;
				for (int Type = 0; Type < 10; Type++) {
					for (LinkedNodeInfo& LinkedNode : this->Nodes[Replacement.NodeIndex].LinkedNodes[Type]) {
						if (Replacement.ChildIndex == i) {
							LinkedNode.ReplacementNodeIndex = Replacement.ReplacementIndex;
						}
						i++;
					}
				}
			}
			if (Replacement.Type == 2) {
				this->Nodes[Replacement.NodeIndex].Attachments[Replacement.AttachmentIndex].IsRemovedAtRuntie = true;
			}
		}
	}

	/* Embedded AINB */
	Reader.Seek(Header.EmbedAinbOffset, BinaryVectorReader::Position::Begin);
	Count = Reader.ReadUInt32();
	this->EmbeddedAinbArray.resize(Count);
	for (int i = 0; i < Count; i++) {
		EmbeddedAinb Entry;
		Entry.FilePath = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
		Entry.FileCategory = ReadStringFromStringPool(&Reader, Reader.ReadUInt32());
		Entry.Count = Reader.ReadUInt32();
		this->EmbeddedAinbArray[i] = Entry;
	}

	this->Loaded = true;
}

AINBFile::Node& AINBFile::GetBaseNode() {
	for (AINBFile::Node& Node : this->Nodes) {
		bool Base = true;
		for (std::vector<AINBFile::LinkedNodeInfo> Type : Node.LinkedNodes) {
			if (!Type.empty()) {
				Base = false;
				break;
			}
		}
		if (Base)
			return Node;
	}
	return this->Nodes[0];
}

AINBFile::AINBFile(std::string FilePath) {
	std::ifstream File(FilePath, std::ios::binary);

	if (!File.eof() && !File.fail())
	{
		File.seekg(0, std::ios_base::end);
		std::streampos FileSize = File.tellg();

		std::vector<unsigned char> Bytes(FileSize);

		File.seekg(0, std::ios_base::beg);
		File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

		this->AINBFile::AINBFile(Bytes);

		File.close();
	}
	else
	{
		Logger::Error("AINBDecoder", "Could not open \"" + FilePath + "\"");
	}
}

std::string AINBFile::StandardTypeToString(int Type) {
	switch (Type) {
	case 0:
		return "int";
	case 1:
		return "bool";
	case 2:
		return "float";
	case 3:
		return "string";
	case 4:
		return "vec3f";
	case 5:
		return "userdefined";
	default:
		return "Invalid Type";
	}
}

std::string AINBFile::FlagToString(AINBFile::FlagsStruct Flag) {
	switch ((int)Flag) {
	case 0:
		return "Pulse Thread Local Storage";
	case 1:
		return "Set Pointer Flag Bit Zero";
	case 2:
		return "Is Valid Update";
	case 3:
		return "Update Post Current Command Calc";
	case 4:
		return "Is Precondition Node";
	case 5:
		return "Is External AINB";
	case 6:
		return "Is Resident Node";
	default:
		return "Invalid Flag";
	}
}

std::string AINBFile::NodeLinkTypeToString(int Mapping) {
	switch (Mapping) {
	case 0:
		return "Output/bool Input/float Input Link";
	case 2:
		return "Standard Link";
	case 3:
		return "Resident Update Link";
	case 4:
		return "String Input Link";
	case 5:
		return "Int Input Link";
	default:
		return "Invalid Link Type";
	}
}

int GetOffsetInStringTable(std::string Value, std::vector<std::string>& StringTable)
{
	int Index = -1;
	auto it = std::find(StringTable.begin(), StringTable.end(), Value);

	if (it != StringTable.end())
	{
		Index = it - StringTable.begin();
	}

	if (Index == -1)
	{
		Logger::Warning("AINBDecoder", "Could not find string \"" + Value + "\" in StringTable");
		return -1;
	}

	int Offset = 0;

	for (int i = 0; i < Index; i++)
	{
		Offset += StringTable.at(i).length() + 1;
	}

	return Offset;
}

void AddToStringTable(std::string Value, std::vector<std::string>& StringTable)
{
	int Index = -1;
	auto it = std::find(StringTable.begin(), StringTable.end(), Value);

	if (it == StringTable.end())
	{
		StringTable.push_back(Value);
	}
}

/*

	Class variables used:
	Header.FileName
	Header.FileCategory
	Commands
	Nodes
	GlobalParameters
	EmbeddedAinbArray
*/

std::vector<unsigned char> AINBFile::ToBinary()
{
	BinaryVectorWriter Writer;

	std::vector<std::string> StringTable;

	Writer.WriteBytes("AIB ");
	Writer.WriteByte(0x07);
	Writer.WriteByte(0x04);
	Writer.WriteByte(0x00);
	Writer.WriteByte(0x00);

	AddToStringTable(this->Header.FileName, StringTable);
	Writer.WriteInteger(GetOffsetInStringTable(this->Header.FileName, StringTable), sizeof(uint32_t));

	Writer.WriteInteger(this->Commands.size(), sizeof(uint32_t));
	Writer.WriteInteger(this->Nodes.size(), sizeof(uint32_t));
	Writer.WriteInteger(0, sizeof(uint32_t)); //Precondition Count

	Writer.WriteInteger(0, sizeof(uint32_t)); //Placeholder

	uint32_t OutputNodeCount = 0;
	for (AINBFile::Node& Node : this->Nodes)
	{
		if ((uint32_t)Node.Type >= 200 && (uint32_t)Node.Type < 300)
			OutputNodeCount++;
	}
	Writer.WriteInteger(OutputNodeCount, sizeof(uint32_t));

	Writer.WriteInteger(116 + 24 * this->Commands.size() + 60 * this->Nodes.size(), sizeof(uint32_t));

	for (int i = 0; i < 11; i++)
	{
		Writer.WriteInteger(4, sizeof(uint32_t)); //Skip offsets until known
	}

	Writer.WriteInteger(0, sizeof(uint64_t)); //Skip
	Writer.WriteInteger(0, sizeof(uint32_t));
	Writer.WriteInteger(0, sizeof(uint32_t));

	AddToStringTable(this->Header.FileCategory, StringTable);
	Writer.WriteInteger(GetOffsetInStringTable(this->Header.FileCategory, StringTable), sizeof(uint32_t));
	uint32_t FileCategoryNum = 0;
	if (this->Header.FileCategory == "Logic") FileCategoryNum = 1;
	if (this->Header.FileCategory == "Sequence") FileCategoryNum = 2;
	Writer.WriteInteger(FileCategoryNum, sizeof(uint32_t));

	Writer.WriteInteger(0, sizeof(uint64_t)); //Skip offset
	Writer.WriteInteger(0, sizeof(uint32_t));

	AINBFile::GUIDData BaseGUIDCommand;
	BaseGUIDCommand.Part1 = 0xc39b7607;
	BaseGUIDCommand.Part2 = 0xf66a;
	BaseGUIDCommand.Part3 = 0x2997;
	BaseGUIDCommand.Part4 = 0xf891;
	BaseGUIDCommand.Part5[0] = 0xc4;
	BaseGUIDCommand.Part5[1] = 0x11;
	BaseGUIDCommand.Part5[2] = 0x32;
	BaseGUIDCommand.Part5[3] = 0xda;
	BaseGUIDCommand.Part5[4] = 0x8d;
	BaseGUIDCommand.Part5[5] = 0x66;

	for (AINBFile::Command Command : this->Commands)
	{
		Command.GUID = BaseGUIDCommand;
		BaseGUIDCommand.Part1++;
		AddToStringTable(Command.Name, StringTable);
		Writer.WriteInteger(GetOffsetInStringTable(Command.Name, StringTable), sizeof(uint32_t));

		Writer.WriteInteger(Command.GUID.Part1, sizeof(uint32_t));
		Writer.WriteInteger(Command.GUID.Part2, sizeof(uint16_t));
		Writer.WriteInteger(Command.GUID.Part3, sizeof(uint16_t));
		Writer.WriteInteger(Command.GUID.Part4, sizeof(uint16_t));
		Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(Command.GUID.Part5), 6);

		Writer.WriteInteger((uint16_t)Command.LeftNodeIndex, sizeof(uint16_t));
		Writer.WriteInteger((uint16_t)Command.RightNodeIndex + 1, sizeof(uint16_t));
	}

	//TODO: Reconstruct parameters, etc.

	std::map<uint16_t, uint32_t> AttachCounts;
	std::vector<AINBFile::AttachmentEntry> Attachments;
	std::vector<uint32_t> AttachmentIndices;
	std::vector<AINBFile::MultiEntry> Multis;
	std::vector<uint16_t> PreconditionNodes;

	AINBFile::GUIDData BaseGUID;
	BaseGUID.Part1 = 0xc29b7607;
	BaseGUID.Part2 = 0xf76a;
	BaseGUID.Part3 = 0x4997;
	BaseGUID.Part4 = 0xf893;
	BaseGUID.Part5[0] = 0xb8;
	BaseGUID.Part5[1] = 0x87;
	BaseGUID.Part5[2] = 0x81;
	BaseGUID.Part5[3] = 0xca;
	BaseGUID.Part5[4] = 0x8c;
	BaseGUID.Part5[5] = 0x65;

	for (int LoopNodeIndex = 0; LoopNodeIndex < this->Nodes.size(); LoopNodeIndex++)
	{
		this->Nodes[LoopNodeIndex].NodeIndex = LoopNodeIndex;
	}

	for (AINBFile::Node& Node : this->Nodes)
	{
		Node.GUID = BaseGUID;
		BaseGUID.Part1++;
		if (Node.Name.find(".module") != std::string::npos)
		{
			bool FoundExternalAINBFlag = false;
			for (AINBFile::FlagsStruct Flag : Node.Flags)
			{
				if (Flag == AINBFile::FlagsStruct::IsExternalAINB)
				{
					FoundExternalAINBFlag = true;
					break;
				}
			}
			if (!FoundExternalAINBFlag)
			{
				Node.Flags.push_back(AINBFile::FlagsStruct::IsExternalAINB);
			}
		}
		AddToStringTable(Node.Name, StringTable);
		AttachCounts.insert({ Node.NodeIndex, Node.Attachments.size()});
		for (AINBFile::AttachmentEntry Entry : Node.Attachments)
		{
			bool Found = false;

			for (AINBFile::AttachmentEntry Attachment : Attachments)
			{
				if (Attachment.Name == Entry.Name && Attachment.Offset == Entry.Offset)
				{
					Found = true;
					break;
				}
			}

			if (!Found)
			{
				Attachments.push_back(Entry);
			}
		}
		for (AINBFile::AttachmentEntry Entry : Node.Attachments)
		{
			uint32_t Index = 0;
			for (AINBFile::AttachmentEntry Attachment : Attachments)
			{
				if (Attachment.Name == Entry.Name && Attachment.Offset == Entry.Offset)
				{
					break;
				}
				Index++;
			}
			AttachmentIndices.push_back(Index);
		}

		/*
		for (int i = 0; i < AINBFile::ValueTypeCount; i++)
		{
			for (AINBFile::InputEntry Param : Node.InputParameters[i])
			{
				if (!Param.Sources.empty())
				{
					for (AINBFile::MultiEntry Entry : Param.Sources)
					{
						Multis.push_back(Entry);

						bool FoundPreconditionFlagDest = false;
						for (AINBFile::FlagsStruct Flag : this->Nodes[Entry.NodeIndex].Flags)
						{
							if (Flag == AINBFile::FlagsStruct::IsPreconditionNode)
							{
								FoundPreconditionFlagDest = true;
								break;
							}
						}
						if (!FoundPreconditionFlagDest)
						{
							this->Nodes[Entry.NodeIndex].Flags.push_back(AINBFile::FlagsStruct::IsPreconditionNode);
						}
					}
				}
				if (Param.NodeIndex >= 0) //Input param is linked to other node, TODO: Multiple precondition nodes
				{
					//PreconditionNodes.insert({ BasePreconditionOffset, Param.NodeIndex });
					//BasePreconditionOffset++;

					bool FoundPreconditionFlagDest = false;
					for (AINBFile::FlagsStruct Flag : this->Nodes[Param.NodeIndex].Flags)
					{
						if (Flag == AINBFile::FlagsStruct::IsPreconditionNode)
						{
							FoundPreconditionFlagDest = true;
							break;
						}
					}
					if (!FoundPreconditionFlagDest)
					{
						this->Nodes[Param.NodeIndex].Flags.push_back(AINBFile::FlagsStruct::IsPreconditionNode);
					}
				}
			}
		}
		*/

		for (int i = 0; i < AINBFile::ValueTypeCount; i++)
		{
			for (AINBFile::InputEntry Param : Node.InputParameters[i])
			{
				if (!Param.Sources.empty())
				{
					for (AINBFile::MultiEntry Entry : Param.Sources)
					{
						Multis.push_back(Entry);
					}
				}
			}
		}

		if (this->Header.FileCategory == "Logic")
		{
			if (std::find(Node.Flags.begin(), Node.Flags.end(), AINBFile::FlagsStruct::IsPreconditionNode) == Node.Flags.end())
				Node.Flags.push_back(AINBFile::FlagsStruct::IsPreconditionNode);
		}
		else
		{
			for (int i = 0; i < AINBFile::ValueTypeCount; i++)
			{
				for (AINBFile::InputEntry& Param : Node.InputParameters[i])
				{
					if (!Param.Sources.empty())
					{
						for (AINBFile::MultiEntry& Entry : Param.Sources)
						{
							if (std::find(this->Nodes[Entry.NodeIndex].Flags.begin(), this->Nodes[Entry.NodeIndex].Flags.end(), AINBFile::FlagsStruct::IsPreconditionNode) == this->Nodes[Entry.NodeIndex].Flags.end())
							{
								//Checking if node is a linked node through standard links
								bool NeedsPreconditionFlag = true;
								for (AINBFile::LinkedNodeInfo& Info : this->Nodes[Entry.NodeIndex].LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink])
								{
									if (Info.NodeIndex == Node.NodeIndex)
									{
										NeedsPreconditionFlag = false;
										break;
									}
								}

								if(NeedsPreconditionFlag)
									this->Nodes[Entry.NodeIndex].Flags.push_back(AINBFile::FlagsStruct::IsPreconditionNode);
							}
						}
					}
					if (Param.NodeIndex >= 0) //Input param is linked to other node
					{
						if (std::find(this->Nodes[Param.NodeIndex].Flags.begin(), this->Nodes[Param.NodeIndex].Flags.end(), AINBFile::FlagsStruct::IsPreconditionNode) == this->Nodes[Param.NodeIndex].Flags.end())
						{
							//Checking if node is a linked node through standard links
							bool NeedsPreconditionFlag = true;
							for (AINBFile::LinkedNodeInfo& Info : this->Nodes[Param.NodeIndex].LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink])
							{
								if (Info.NodeIndex == Node.NodeIndex)
								{
									NeedsPreconditionFlag = false;
									break;
								}
							}
							if(NeedsPreconditionFlag)
								this->Nodes[Param.NodeIndex].Flags.push_back(AINBFile::FlagsStruct::IsPreconditionNode);
						}
					}
				}
			}
		}
	}

	for (AINBFile::Node& Node : this->Nodes)
	{
		Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::OutputBoolInputFloatInputLink].clear();

		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBFile::InputEntry& Entry : Node.InputParameters[Type])
			{
				if (Entry.NodeIndex == -1)
				{
					Entry.ParameterIndex = 0;
				}
			}
		}
	}

	for (AINBFile::Node& Node : this->Nodes)
	{
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBFile::InputEntry& Entry : Node.InputParameters[Type])
			{
				if (Entry.NodeIndex >= 0)
				{
					if (Type == (int)AINBFile::ValueType::Bool || Type == (int)AINBFile::ValueType::Float)
					{
						AINBFile::LinkedNodeInfo Info;
						Info.NodeIndex = Entry.NodeIndex;
						Info.Parameter = Entry.Name;
						Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::OutputBoolInputFloatInputLink].push_back(Info);
					}
					else
					{
						AINBFile::LinkedNodeInfo Info;
						Info.NodeIndex = Node.NodeIndex;
						Info.Parameter = this->Nodes[Entry.NodeIndex].OutputParameters[Type][Entry.ParameterIndex].Name;
						this->Nodes[Entry.NodeIndex].LinkedNodes[(int)AINBFile::LinkedNodeMapping::OutputBoolInputFloatInputLink].push_back(Info);
					}
					continue;
				}
				for (AINBFile::MultiEntry& Multi : Entry.Sources)
				{
					if (Type == (int)AINBFile::ValueType::Bool || Type == (int)AINBFile::ValueType::Float)
					{
						AINBFile::LinkedNodeInfo Info;
						Info.NodeIndex = Multi.NodeIndex;
						Info.Parameter = this->Nodes[Multi.NodeIndex].OutputParameters[Type][Multi.ParameterIndex].Name;
						/*
						bool Found = false;
						for (AINBFile::LinkedNodeInfo& ChildInfo : this->Nodes[Multi.NodeIndex].LinkedNodes[(int)AINBFile::LinkedNodeMapping::OutputBoolInputFloatInputLink])
						{
							if (ChildInfo.Parameter == Info.Parameter)
							{
								Found = true;
								break;
							}
						}
						if(!Found)
							this->Nodes[Multi.NodeIndex].LinkedNodes[(int)AINBFile::LinkedNodeMapping::OutputBoolInputFloatInputLink].push_back(Info);
							*/
						Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::OutputBoolInputFloatInputLink].push_back(Info);
					}
					else
					{
						AINBFile::LinkedNodeInfo Info;
						Info.NodeIndex = Node.NodeIndex;
						Info.Parameter = this->Nodes[Multi.NodeIndex].OutputParameters[Entry.ValueType][Multi.ParameterIndex].Name;
						this->Nodes[Multi.NodeIndex].LinkedNodes[(int)AINBFile::LinkedNodeMapping::OutputBoolInputFloatInputLink].push_back(Info);
					}
				}
			}
		}
	}

	std::map<uint16_t, uint16_t> NodeIndexToPreconditionNodeIndex;

	uint16_t NodeIndexToPreconditionNodeIndexCount = 0;
	for (AINBFile::Node& Node : this->Nodes)
	{
		if (std::find(Node.Flags.begin(), Node.Flags.end(), AINBFile::FlagsStruct::IsPreconditionNode) != Node.Flags.end())
		{
			NodeIndexToPreconditionNodeIndex[Node.NodeIndex] = NodeIndexToPreconditionNodeIndexCount;
			NodeIndexToPreconditionNodeIndexCount++;
		}
	}
	uint16_t BasePreconditionNode = 0;
	for (AINBFile::Node& Node : this->Nodes)
	{
		Node.PreconditionNodes.clear();
		Node.PreconditionCount = 0;
		Node.BasePreconditionNode = BasePreconditionNode;
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBFile::InputEntry& Entry : Node.InputParameters[Type])
			{
				if (Entry.NodeIndex >= 0)
				{
					if (std::find(Node.PreconditionNodes.begin(), Node.PreconditionNodes.end(), NodeIndexToPreconditionNodeIndex[Entry.NodeIndex]) != Node.PreconditionNodes.end())
						continue;

					bool NeedsPreconditionNode = true;
					for (AINBFile::LinkedNodeInfo& Info : this->Nodes[Entry.NodeIndex].LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink])
					{
						if (Info.NodeIndex == Node.NodeIndex)
						{
							NeedsPreconditionNode = false;
							break;
						}
					}

					if (!NeedsPreconditionNode)
						continue;

					Node.PreconditionNodes.push_back(NodeIndexToPreconditionNodeIndex[Entry.NodeIndex]);
					Node.PreconditionCount++;
					PreconditionNodes.push_back(NodeIndexToPreconditionNodeIndex[Entry.NodeIndex]);
				}
				for (AINBFile::MultiEntry& Multi : Entry.Sources)
				{
					if (std::find(Node.PreconditionNodes.begin(), Node.PreconditionNodes.end(), NodeIndexToPreconditionNodeIndex[Multi.NodeIndex]) != Node.PreconditionNodes.end())
						continue;

					bool NeedsPreconditionNode = true;
					for (AINBFile::LinkedNodeInfo& Info : this->Nodes[Multi.NodeIndex].LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink])
					{
						if (Info.NodeIndex == Node.NodeIndex)
						{
							NeedsPreconditionNode = false;
							break;
						}
					}

					if (!NeedsPreconditionNode)
						continue;

					Node.PreconditionNodes.push_back(NodeIndexToPreconditionNodeIndex[Multi.NodeIndex]);
					Node.PreconditionCount++;
					PreconditionNodes.push_back(NodeIndexToPreconditionNodeIndex[Multi.NodeIndex]);
				}
			}
		}
		if (Node.PreconditionCount == 0)
		{
			Node.BasePreconditionNode = 0;
			continue;
		}
		BasePreconditionNode += Node.PreconditionCount;
	}

	std::vector<std::vector<AINBFile::GlobalEntry>> NewGlobalsParams(6);
	if (GlobalParameters.size() == AINBFile::GlobalTypeCount)
	{
		for (int i = 0; i < AINBFile::GlobalTypeCount; i++)
		{
			for (AINBFile::GlobalEntry Entry : this->GlobalParameters[i])
			{
				NewGlobalsParams[(int)Entry.GlobalValueType].push_back(Entry);
			}
		}
	}
	this->GlobalParameters = NewGlobalsParams;

	bool GlobalParametersEmpty = true;
	for (int Type = 0; Type < AINBFile::GlobalTypeCount; Type++)
	{
		if (!this->GlobalParameters[Type].empty())
		{
			GlobalParametersEmpty = false;
			break;
		}
	}

	Writer.Seek(this->Nodes.size() * 60, BinaryVectorWriter::Position::Current);

	if(!GlobalParametersEmpty) {
		uint16_t Index = 0;
		uint16_t Pos = 0;

		for (int Type = 0; Type < this->GlobalParameters.size(); Type++)
		{
			Writer.WriteInteger(this->GlobalParameters[Type].size(), sizeof(uint16_t));
			Writer.WriteInteger(Index, sizeof(uint16_t));
			Index += this->GlobalParameters[Type].size();
			Writer.WriteInteger(Pos, sizeof(uint16_t));
			if (Type == (int)AINBFile::GlobalType::Vec3f)
			{
				Pos += this->GlobalParameters[Type].size() * 12;
			}
			else
			{
				Pos += this->GlobalParameters[Type].size() * 4;
			}
			Writer.WriteInteger(0, sizeof(uint16_t));
		}

		std::vector<AINBFile::GlobalFileRef> Files;
		for (int Type = 0; Type < this->GlobalParameters.size(); Type++)
		{
			for (AINBFile::GlobalEntry Entry : this->GlobalParameters[Type])
			{
				AddToStringTable(Entry.Name, StringTable);
				uint32_t NameOffset = GetOffsetInStringTable(Entry.Name, StringTable);
				if (Entry.FileReference.FileName != "")
				{
					int FileIndex = -1;

					for (int i = 0; i < Files.size(); i++)
					{
						if (Files[i].FileName == Entry.FileReference.FileName && Files[i].NameHash == Entry.FileReference.NameHash)
						{
							FileIndex = i;
							break;
						}
					}

					if (FileIndex == -1)
					{
						Files.push_back(Entry.FileReference);
						FileIndex = Files.size() - 1;
					}
					NameOffset = NameOffset | (1 << 31);
					NameOffset = NameOffset | (FileIndex << 24);
				}
				else
				{
					NameOffset = NameOffset | (1 << 23);
				}
				Writer.WriteInteger(NameOffset, sizeof(uint32_t));
				AddToStringTable(Entry.Notes, StringTable);
				Writer.WriteInteger(GetOffsetInStringTable(Entry.Notes, StringTable), sizeof(uint32_t));
			}
		}

		int Start = Writer.GetPosition();
		int Size = 0;
		for (int Type = 0; Type < AINBFile::GlobalTypeCount; Type++)
		{
			for (AINBFile::GlobalEntry Entry : this->GlobalParameters[Type])
			{
				if (Type == (int)AINBFile::GlobalType::Int)
				{
					Writer.WriteInteger(*reinterpret_cast<uint32_t*>(&Entry.GlobalValue), sizeof(uint32_t));
					Size += 4;
				}
				if (Type == (int)AINBFile::GlobalType::Float)
				{
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Entry.GlobalValue), sizeof(float));
					Size += 4;
				}
				if (Type == (int)AINBFile::GlobalType::Bool)
				{
					Writer.WriteInteger((uint32_t)*reinterpret_cast<bool*>(&Entry.GlobalValue), sizeof(uint32_t));
					Size += 4;
				}
				if (Type == (int)AINBFile::GlobalType::Vec3f)
				{
					Vector3F* Vec3f = reinterpret_cast<Vector3F*>(&Entry.GlobalValue);
					Writer.WriteRawUnsafeFixed((const char*)&Vec3f->GetRawData()[0], sizeof(float));
					Writer.WriteRawUnsafeFixed((const char*)&Vec3f->GetRawData()[1], sizeof(float));
					Writer.WriteRawUnsafeFixed((const char*)&Vec3f->GetRawData()[2], sizeof(float));
					Size += 12;
				}
				if (Type == (int)AINBFile::GlobalType::String)
				{
					AddToStringTable(*reinterpret_cast<std::string*>(&Entry.GlobalValue), StringTable);
					Writer.WriteInteger(GetOffsetInStringTable(*reinterpret_cast<std::string*>(&Entry.GlobalValue), StringTable), sizeof(uint32_t));
					Size += 4;
				}
			}
		}
		Writer.Seek(Start + Size, BinaryVectorWriter::Position::Begin);
		for (AINBFile::GlobalFileRef File : Files)
		{
			AddToStringTable(File.FileName, StringTable);
			Writer.WriteInteger(GetOffsetInStringTable(File.FileName, StringTable), sizeof(uint32_t));
			Writer.WriteInteger(File.NameHash, sizeof(uint32_t));
			Writer.WriteInteger(File.UnknownHash1, sizeof(uint32_t));
			Writer.WriteInteger(File.UnknownHash2, sizeof(uint32_t));
		}
	}
	else
	{
		Writer.Seek(48, BinaryVectorWriter::Position::Current);
	}

	std::vector<AINBFile::ImmediateParameter> ImmediateParameters[ValueTypeCount];
	std::vector<AINBFile::InputEntry> InputParameters[ValueTypeCount];
	std::vector<AINBFile::OutputEntry> OutputParameters[ValueTypeCount];

	std::map<AINBFile::ValueType, int> ImmediateCurrent;
	ImmediateCurrent.insert({ AINBFile::ValueType::Bool, 0 });
	ImmediateCurrent.insert({ AINBFile::ValueType::Int, 0 });
	ImmediateCurrent.insert({ AINBFile::ValueType::Float, 0 });
	ImmediateCurrent.insert({ AINBFile::ValueType::String, 0 });
	ImmediateCurrent.insert({ AINBFile::ValueType::Vec3f, 0 });
	ImmediateCurrent.insert({ AINBFile::ValueType::UserDefined, 0 });
	std::map<AINBFile::ValueType, int> InputCurrent;
	InputCurrent.insert({ AINBFile::ValueType::Bool, 0 });
	InputCurrent.insert({ AINBFile::ValueType::Int, 0 });
	InputCurrent.insert({ AINBFile::ValueType::Float, 0 });
	InputCurrent.insert({ AINBFile::ValueType::String, 0 });
	InputCurrent.insert({ AINBFile::ValueType::Vec3f, 0 });
	InputCurrent.insert({ AINBFile::ValueType::UserDefined, 0 });
	std::map<AINBFile::ValueType, int> OutputCurrent;
	OutputCurrent.insert({ AINBFile::ValueType::Bool, 0 });
	OutputCurrent.insert({ AINBFile::ValueType::Int, 0 });
	OutputCurrent.insert({ AINBFile::ValueType::Float, 0 });
	OutputCurrent.insert({ AINBFile::ValueType::String, 0 });
	OutputCurrent.insert({ AINBFile::ValueType::Vec3f, 0 });
	OutputCurrent.insert({ AINBFile::ValueType::UserDefined, 0 });
	std::vector<int> Bodies;
	std::vector<AINBFile::ResidentEntry> Residents;

	std::map<uint16_t, std::pair<uint32_t, uint32_t>> EXBInfo; //NodeIndex -> {EXBCount, EXBSize}
	std::vector<EXB::CommandInfoStruct> EXBCommands;

	//Reconstructing EXBInfo
	for (AINBFile::Node& Node : this->Nodes)
	{
		uint32_t EXBCount = 0;
		uint32_t EXBSize = 0;
		for (AINBFile::AttachmentEntry& Attachment : Node.Attachments)
		{
			uint32_t AttachEXBCount = 0;
			uint32_t AttachEXBSize = 0;

			for (int Type = 0; Type < Attachment.Parameters.size(); Type++)
			{
				for (AINBFile::ImmediateParameter Entry : Attachment.Parameters[Type])
				{
					if (Entry.Function.InstructionCount > 0 && Entry.Function.Instructions.size() > 0)
					{
						EXBCount++;
						AttachEXBCount++;
						uint32_t Size = 0;
						for (EXB::InstructionStruct Instruction : Entry.Function.Instructions)
						{
							uint32_t TypeSize = Instruction.DataType == EXB::Type::Vec3f ? 12 : 4;
							if (Instruction.LHSSource != EXB::Source::Unknown || Instruction.RHSSource != EXB::Source::Unknown)
							{
								if (Instruction.LHSSource == EXB::Source::Output)
									Size = std::max(Size, Instruction.LHSIndexValue + TypeSize);
								if (Instruction.RHSSource == EXB::Source::Input)
									Size = std::max(Size, Instruction.RHSIndexValue + TypeSize);
							}
						}
						EXBSize += Size;
						AttachEXBSize += Size;
					}
				}
			}
		}
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBFile::InputEntry Entry : Node.InputParameters[Type])
			{
				if (Entry.Function.InstructionCount > 0 && Entry.Function.Instructions.size() > 0)
				{
					EXBCount++;
					uint32_t Size = 0;
					for (EXB::InstructionStruct Instruction : Entry.Function.Instructions)
					{
						uint32_t TypeSize = Instruction.DataType == EXB::Type::Vec3f ? 12 : 4;
						if (Instruction.LHSSource != EXB::Source::Unknown || Instruction.RHSSource != EXB::Source::Unknown)
						{
							if (Instruction.LHSSource == EXB::Source::Output)
								Size = std::max(Size, Instruction.LHSIndexValue + TypeSize);
							if (Instruction.RHSSource == EXB::Source::Input)
								Size = std::max(Size, Instruction.RHSIndexValue + TypeSize);
						}
					}
					EXBSize += Size;
				}
				for (AINBFile::MultiEntry Parameter : Entry.Sources)
				{
					if (Parameter.Function.InstructionCount > 0 && Parameter.Function.Instructions.size() > 0)
					{
						EXBCount++;
						uint32_t Size = 0;
						for (EXB::InstructionStruct Instruction : Parameter.Function.Instructions)
						{
							uint32_t TypeSize = Instruction.DataType == EXB::Type::Vec3f ? 12 : 4;
							if (Instruction.LHSSource != EXB::Source::Unknown || Instruction.RHSSource != EXB::Source::Unknown)
							{
								if (Instruction.LHSSource == EXB::Source::Output)
									Size = std::max(Size, Instruction.LHSIndexValue + TypeSize);
								if (Instruction.RHSSource == EXB::Source::Input)
									Size = std::max(Size, Instruction.RHSIndexValue + TypeSize);
							}
						}
						EXBSize += Size;
					}
				}
			}
		}
		EXBInfo.insert({ Node.NodeIndex, {EXBCount, EXBSize} });

		//Constructing new, merged EXB
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBFile::InputEntry& Entry : Node.InputParameters[Type])
			{
				if (Entry.Function.InstructionCount > 0 && Entry.Function.Instructions.size() > 0)
				{
					int NewEXBIndex = -1;
					for (int i = 0; i < EXBCommands.size(); i++)
					{
						if (EXBCommands[i].Instructions.size() != Entry.Function.Instructions.size() ||
							EXBCommands[i].InputDataType != Entry.Function.InputDataType ||
							EXBCommands[i].OutputDataType != Entry.Function.OutputDataType ||
							EXBCommands[i].Scratch32MemorySize != Entry.Function.Scratch32MemorySize ||
							EXBCommands[i].Scratch64MemorySize != Entry.Function.Scratch64MemorySize ||
							EXBCommands[i].StaticMemorySize != Entry.Function.StaticMemorySize ||
							EXBCommands[i].BaseIndexPreCommandEntry != Entry.Function.BaseIndexPreCommandEntry
							) continue;

						for (int j = 0; j < EXBCommands[i].Instructions.size(); j++)
						{
							if (EXBCommands[i].Instructions[j].DataType != Entry.Function.Instructions[j].DataType ||
								EXBCommands[i].Instructions[j].LHSIndexValue != Entry.Function.Instructions[j].LHSIndexValue ||
								EXBCommands[i].Instructions[j].LHSSource != Entry.Function.Instructions[j].LHSSource ||
								EXBCommands[i].Instructions[j].RHSIndexValue != Entry.Function.Instructions[j].RHSIndexValue ||
								EXBCommands[i].Instructions[j].RHSSource != Entry.Function.Instructions[j].RHSSource ||
								EXBCommands[i].Instructions[j].Signature != Entry.Function.Instructions[j].Signature ||
								EXBCommands[i].Instructions[j].StaticMemoryIndex != Entry.Function.Instructions[j].StaticMemoryIndex ||
								EXBCommands[i].Instructions[j].Type != Entry.Function.Instructions[j].Type) continue;

							NewEXBIndex = i;
							break;
						}
						if (NewEXBIndex != -1)
							break;
					}
					if (NewEXBIndex == -1)
					{
						EXBCommands.push_back(Entry.Function);
						NewEXBIndex = EXBCommands.size() - 1;
					}
					Entry.EXBIndex = NewEXBIndex;
				}
			}
		}
	}

	EXBFile.Loaded = !EXBCommands.empty();

	int16_t CurrentInputNodeIndexMulti = -100;

	struct WriterReplacement
	{
		uint8_t Type = -1;
		uint16_t NodeIndex = -1;
		uint16_t Iteration = -1;
		uint16_t ReplacementNodeIndex = -1;
	};

	std::vector<WriterReplacement> Replacements;

	for (AINBFile::Node& Node : this->Nodes)
	{
		if (Node.Type == (int)AINBFile::NodeTypes::Element_S32Selector)
		{
			std::vector<AINBFile::LinkedNodeInfo> NewLinkedNodeInfo;
			for (AINBFile::LinkedNodeInfo Info : Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink])
			{
				if (Info.Condition != "Default")
					NewLinkedNodeInfo.push_back(Info);
			}
			for (AINBFile::LinkedNodeInfo Info : Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink])
			{
				if (Info.Condition == "Default")
					NewLinkedNodeInfo.push_back(Info);
			}
			Node.LinkedNodes[(int)AINBFile::LinkedNodeMapping::StandardLink] = NewLinkedNodeInfo;
		}

		Bodies.push_back(Writer.GetPosition());

		bool ImmediateParametersEmpty = true;
		bool InputParametersEmpty = true;
		bool OutputParametersEmpty = true;
		bool LinkedNodesEmpty = true;
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			if (!Node.ImmediateParameters[Type].empty())
			{
				ImmediateParametersEmpty = false;
			}
			if (!Node.InputParameters[Type].empty())
			{
				InputParametersEmpty = false;
			}
			if (!Node.OutputParameters[Type].empty())
			{
				OutputParametersEmpty = false;
			}
		}

		for (int Type = 0; Type < AINBFile::LinkedNodeTypeCount; Type++)
		{
			if (!Node.LinkedNodes[Type].empty())
			{
				LinkedNodesEmpty = false;
				break;
			}
		}

		if (!ImmediateParametersEmpty)
		{
			for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
			{
				if (!Node.ImmediateParameters[Type].empty())
				{
					for (AINBFile::ImmediateParameter Param : Node.ImmediateParameters[Type])
					{
						ImmediateParameters[Type].push_back(Param);
					}
					Writer.WriteInteger(ImmediateParameters[Type].size() - Node.ImmediateParameters[Type].size(), sizeof(uint32_t));
					Writer.WriteInteger(Node.ImmediateParameters[Type].size(), sizeof(uint32_t));
					ImmediateCurrent.find((AINBFile::ValueType)Type)->second = ImmediateParameters[Type].size();
				}
				else
				{
					Writer.WriteInteger(ImmediateCurrent[(AINBFile::ValueType)Type], sizeof(uint32_t));
					Writer.WriteInteger(0, sizeof(uint32_t));
				}
			}
		}
		else
		{
			for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
			{
				Writer.WriteInteger(ImmediateCurrent[(AINBFile::ValueType)Type], sizeof(uint32_t));
				Writer.WriteInteger(0, sizeof(uint32_t));
			}
		}

		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			if (!InputParametersEmpty)
			{
				if (!Node.InputParameters[Type].empty())
				{
					for (AINBFile::InputEntry& Param : Node.InputParameters[Type])
					{
						if (!Param.Sources.empty())
						{
							Param.ParameterIndex = Param.Sources.size();
							Param.NodeIndex = CurrentInputNodeIndexMulti;
							CurrentInputNodeIndexMulti -= Param.Sources.size();
						}
						InputParameters[Type].push_back(Param);
					}
					Writer.WriteInteger(InputParameters[Type].size() - Node.InputParameters[Type].size(), sizeof(uint32_t));
					Writer.WriteInteger(Node.InputParameters[Type].size(), sizeof(uint32_t));
					InputCurrent.find((AINBFile::ValueType)Type)->second = InputParameters[Type].size();
				}
				else
				{
					Writer.WriteInteger(InputCurrent[(AINBFile::ValueType)Type], sizeof(uint32_t));
					Writer.WriteInteger(0, sizeof(uint32_t));
				}
			}
			else
			{
				Writer.WriteInteger(InputCurrent[(AINBFile::ValueType)Type], sizeof(uint32_t));
				Writer.WriteInteger(0, sizeof(uint32_t));
			}

			if (!OutputParametersEmpty)
			{
				if (!Node.OutputParameters[Type].empty())
				{
					for (AINBFile::OutputEntry Param : Node.OutputParameters[Type])
					{
						OutputParameters[Type].push_back(Param);
					}
					Writer.WriteInteger(OutputParameters[Type].size() - Node.OutputParameters[Type].size(), sizeof(uint32_t));
					Writer.WriteInteger(Node.OutputParameters[Type].size(), sizeof(uint32_t));
					OutputCurrent.find((AINBFile::ValueType)Type)->second = OutputParameters[Type].size();
				}
				else
				{
					Writer.WriteInteger(OutputCurrent[(AINBFile::ValueType)Type], sizeof(uint32_t));
					Writer.WriteInteger(0, sizeof(uint32_t));
				}
			}
			else
			{
				Writer.WriteInteger(OutputCurrent[(AINBFile::ValueType)Type], sizeof(uint32_t));
				Writer.WriteInteger(0, sizeof(uint32_t));
			}
		}
		if (!LinkedNodesEmpty)
		{
			uint8_t Total = 0;
			for (int Type = 0; Type < AINBFile::LinkedNodeTypeCount; Type++)
			{
				if (!Node.LinkedNodes[Type].empty())
				{
					Writer.WriteInteger(Node.LinkedNodes[Type].size(), sizeof(uint8_t));
					Writer.WriteInteger(Total, sizeof(uint8_t));
					Total += Node.LinkedNodes[Type].size();
				}
				else
				{
					Writer.WriteInteger(0, sizeof(uint8_t));
					Writer.WriteInteger(Total, sizeof(uint8_t));
				}
			}

			uint32_t Start = Writer.GetPosition();
			uint32_t Current = Start + Total * 4;
			int i = 0;
			for (int Type = 0; Type < AINBFile::LinkedNodeTypeCount; Type++)
			{
				if (Type == (int)AINBFile::LinkedNodeMapping::OutputBoolInputFloatInputLink)
				{
					for (AINBFile::LinkedNodeInfo Entry : Node.LinkedNodes[Type])
					{
						Writer.WriteInteger(Current, sizeof(uint32_t));
						uint32_t Pos = Writer.GetPosition();
						Writer.Seek(Current, BinaryVectorWriter::Position::Begin);
						Writer.WriteInteger(Entry.NodeIndex, sizeof(uint32_t));
						AddToStringTable(Entry.Parameter, StringTable);
						Writer.WriteInteger(GetOffsetInStringTable(Entry.Parameter, StringTable), sizeof(uint32_t));
						Writer.Seek(Pos, BinaryVectorWriter::Position::Begin);
						bool IsInput = false;
						if (Node.Type == (uint16_t)AINBFile::NodeTypes::Element_Expression ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_S32Selector ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_F32Selector ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_StringSelector ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_RandomSelector ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_BoolSelector)
						{
							for (int i = 0; i < AINBFile::ValueTypeCount; i++)
							{
								for (AINBFile::InputEntry Param : Node.InputParameters[i])
								{
									if (Param.NodeIndex == Entry.NodeIndex)
									{
										IsInput = true;
										break;
									}
								}
							}
						}

						if (IsInput)
						{
							Current += 16;
						}
						else if (Node.Type == (uint16_t)AINBFile::NodeTypes::Element_Expression && !Node.OutputParameters[(int)AINBFile::ValueType::Vec3f].empty())
						{
							for (AINBFile::OutputEntry Parameter : Node.OutputParameters[(int)AINBFile::ValueType::Vec3f])
							{
								if (Entry.Parameter == Parameter.Name)
								{
									Current += 24;
								}
							}
						}
						else
						{
							Current += 8;
						}

						if (Entry.IsRemovedAtRuntime)
						{
							Replacements.push_back({ (uint8_t)0, (uint16_t)Node.NodeIndex, (uint16_t)i, (uint16_t)0 });
						}
						else if (Entry.ReplacementNodeIndex != 0xFFFF)
						{
							Replacements.push_back({ (uint8_t)1, (uint16_t)Node.NodeIndex, (uint16_t)i, (uint16_t)Entry.ReplacementNodeIndex });
						}
						i++;
					}
				}
				else if (Type == (int)AINBFile::LinkedNodeMapping::StandardLink)
				{
					for (AINBFile::LinkedNodeInfo Entry : Node.LinkedNodes[Type])
					{
						if (Node.Type == (int)AINBFile::NodeTypes::Element_F32Selector)
						{
							Writer.WriteInteger(Current, sizeof(uint32_t));
							int Pos = Writer.GetPosition();
							Writer.Seek(Current, BinaryVectorWriter::Position::Begin);
							Writer.WriteInteger(Entry.NodeIndex, sizeof(uint32_t));
							AddToStringTable("", StringTable);
							Writer.WriteInteger(GetOffsetInStringTable("", StringTable), sizeof(uint32_t));
							if (Node.Input.Index != 0xFFFFFFFF)
							{
								uint32_t Index = 0;
								for (AINBFile::GlobalEntry GEntry : this->GlobalParameters[(int)AINBFile::GlobalType::Float])
								{
									if (GEntry.Index == Node.Input.Index && GEntry.Name == Node.Input.Name)
									{
										break;
									}
									Index++;
								}
								Writer.WriteInteger(Index | (1 << 31), sizeof(uint32_t));
							}
							else
							{
								Writer.WriteInteger(0, sizeof(uint32_t));
							}
							Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Entry.ConditionMin), sizeof(float));
							Writer.WriteInteger(0, sizeof(uint32_t));
							if (Entry.ConditionMax != -1.0f)
							{
								Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Entry.ConditionMax), sizeof(float));
							}
							else
							{
								Writer.WriteInteger(0, sizeof(uint32_t));
							}
							Writer.Seek(Pos, BinaryVectorWriter::Position::Begin);
							Current += 40;
						}
						else if (Node.Type == (int)AINBFile::NodeTypes::Element_StringSelector ||
							Node.Type == (int)AINBFile::NodeTypes::Element_S32Selector ||
							Node.Type == (int)AINBFile::NodeTypes::Element_RandomSelector)
						{
							Writer.WriteInteger(Current, sizeof(uint32_t));
							int Pos = Writer.GetPosition();
							Writer.Seek(Current, BinaryVectorWriter::Position::Begin);
							Writer.WriteInteger(Entry.NodeIndex, sizeof(uint32_t));
							AddToStringTable("", StringTable);
							Writer.WriteInteger(GetOffsetInStringTable("", StringTable), sizeof(uint32_t));
							if (Node.Input.Index != -1)
							{
								uint32_t Index = 0;
								for (AINBFile::GlobalEntry GEntry : this->GlobalParameters[Node.Type])
								{
									if (GEntry.Index == Node.Input.Index && GEntry.Name == Node.Input.Name)
									{
										break;
									}
									Index++;
								}

								Writer.WriteInteger(Index | (1 << 31), sizeof(uint32_t));
							}
							else
							{
								Writer.WriteInteger(0, sizeof(uint32_t));
							}
							if (Entry.Probability != 0.0f)
							{
								Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Entry.Probability), sizeof(float));
							}
							else if (Entry.Condition != "MapEditor_AINB_NoVal")
							{
								if (Entry.Condition != "Default")
								{
									if (Node.Type == (int)AINBFile::NodeTypes::Element_S32Selector)
									{
										Writer.WriteInteger(static_cast<signed int>(std::stoi(Entry.Condition)), sizeof(int32_t));
									}
									else
									{
										AddToStringTable(Entry.Condition, StringTable);
										Writer.WriteInteger(GetOffsetInStringTable(Entry.Condition, StringTable), sizeof(uint32_t));
									}
								}
								else
								{
									Writer.WriteInteger(0, sizeof(uint32_t));
								}
							}
							Writer.Seek(Pos, BinaryVectorWriter::Position::Begin);
							Current += 16;
						}
						else
						{
							Writer.WriteInteger(Current, sizeof(uint32_t));
							int Pos = Writer.GetPosition();
							Writer.Seek(Current, BinaryVectorWriter::Position::Begin);
							Writer.WriteInteger(Entry.NodeIndex, sizeof(uint32_t));
							if (Entry.ConnectionName != "MapEditor_AINB_NoVal")
							{
								AddToStringTable(Entry.ConnectionName, StringTable);
								Writer.WriteInteger(GetOffsetInStringTable(Entry.ConnectionName, StringTable), sizeof(uint32_t));
							}
							else if (Entry.Condition != "MapEditor_AINB_NoVal")
							{
								AddToStringTable(Entry.Condition, StringTable);
								Writer.WriteInteger(GetOffsetInStringTable(Entry.Condition, StringTable), sizeof(uint32_t));
							}
							Writer.Seek(Pos, BinaryVectorWriter::Position::Begin);
							Current += 8;
						}

						if (Entry.IsRemovedAtRuntime)
						{
							Replacements.push_back({ (uint8_t)0, (uint16_t)Node.NodeIndex, (uint16_t)i, (uint16_t)0 });
						}
						else if (Entry.ReplacementNodeIndex != 0xFFFF)
						{
							Replacements.push_back({ (uint8_t)1, (uint16_t)Node.NodeIndex, (uint16_t)i, (uint16_t)Entry.ReplacementNodeIndex });
						}
						i++;
					}
				}
				else if (Type != (int)AINBFile::LinkedNodeMapping::ResidentUpdateLink)
				{
					for (AINBFile::LinkedNodeInfo Entry : Node.LinkedNodes[Type])
					{
						Writer.WriteInteger(Current, sizeof(uint32_t));
						int Pos = Writer.GetPosition();
						Writer.Seek(Pos, BinaryVectorWriter::Position::Begin);
						Writer.WriteInteger(Entry.NodeIndex, sizeof(uint32_t));
						if (Entry.Condition != "MapEditor_AINB_NoVal")
						{
							AddToStringTable(Entry.Condition, StringTable);
							Writer.WriteInteger(GetOffsetInStringTable(Entry.Condition, StringTable), sizeof(uint32_t));
						}
						else if (Entry.Parameter != "")
						{
							AddToStringTable(Entry.Parameter, StringTable);
							Writer.WriteInteger(GetOffsetInStringTable(Entry.Parameter, StringTable), sizeof(uint32_t));
						}
						else
						{
							AddToStringTable(Entry.ConnectionName, StringTable);
							Writer.WriteInteger(GetOffsetInStringTable(Entry.ConnectionName, StringTable), sizeof(uint32_t));
						}

						if (Node.Input.Index != -1 && (Node.Type == (int)AINBFile::NodeTypes::Element_StringSelector || Node.Type == (int)AINBFile::NodeTypes::Element_S32Selector))
						{
							uint32_t Index = 0;
							for (AINBFile::GlobalEntry GEntry : this->GlobalParameters[Node.Type])
							{
								if (GEntry.Index == Node.Input.Index && GEntry.Name == Node.Input.Name)
								{
									break;
								}
								Index++;
							}

							Writer.WriteInteger(Index | (1 << 31), sizeof(uint32_t));
						}
						Writer.Seek(Pos, BinaryVectorWriter::Position::Begin);

						if (Node.Type == (uint16_t)AINBFile::NodeTypes::Element_Expression ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_S32Selector ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_F32Selector ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_StringSelector ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_RandomSelector ||
							Node.Type == (uint16_t)AINBFile::NodeTypes::Element_BoolSelector)
						{
							Current += 16;
						}
						else
						{
							Current += 8;
						}

						if (Entry.IsRemovedAtRuntime)
						{
							Replacements.push_back({ (uint8_t)0, (uint16_t)Node.NodeIndex, (uint16_t)i, (uint16_t)0 });
						}
						else if (Entry.ReplacementNodeIndex != 0xFFFF)
						{
							Replacements.push_back({ (uint8_t)1, (uint16_t)Node.NodeIndex, (uint16_t)i, (uint16_t)Entry.ReplacementNodeIndex });
						}
						i++;
					}
				}
				else
				{
					for (AINBFile::LinkedNodeInfo Entry : Node.LinkedNodes[Type])
					{
						Writer.WriteInteger(Current, sizeof(uint32_t));
						int Pos = Writer.GetPosition();
						Writer.Seek(Current, BinaryVectorWriter::Position::Begin);
						Writer.WriteInteger(Entry.NodeIndex, sizeof(uint32_t));
						Residents.push_back(Entry.UpdateInfo);
						Writer.WriteInteger(Residents.size() - 1, sizeof(uint32_t));
						Current += 8;
						if (Entry.IsRemovedAtRuntime)
						{
							Replacements.push_back({ (uint8_t)0, (uint16_t)Node.NodeIndex, (uint16_t)i, (uint16_t)0 });
						}
						else if (Entry.ReplacementNodeIndex != 0xFFFF)
						{
							Replacements.push_back({ (uint8_t)1, (uint16_t)Node.NodeIndex, (uint16_t)i, (uint16_t)Entry.ReplacementNodeIndex });
						}
						i++;
					}
				}
			}
			Writer.Seek(Current, BinaryVectorWriter::Position::Begin);
		}
		else
		{
			for (int i = 0; i < 5; i++)
			{
				Writer.WriteInteger(0, sizeof(uint32_t));
			}
		}
	}

	uint32_t AttachmentIndexStart = Writer.GetPosition();
	if (!this->Nodes.empty())
	{
		uint32_t BaseAttach = 0;
		Writer.Seek(116 + 24 * this->Commands.size(), BinaryVectorWriter::Position::Begin);
		for (AINBFile::Node Node : this->Nodes)
		{
			Writer.WriteInteger(Node.Type, sizeof(uint16_t));
			Writer.WriteInteger(Node.NodeIndex, sizeof(uint16_t));
			Writer.WriteInteger(AttachCounts[Node.NodeIndex], sizeof(uint16_t));
			uint8_t Flags = 0;
			for (AINBFile::FlagsStruct Flag : Node.Flags)
			{
				if (Flag == FlagsStruct::IsPreconditionNode)
				{
					Flags = Flags | 1;
				}
				if (Flag == FlagsStruct::IsExternalAINB)
				{
					Flags = Flags | 2;
				}
				if (Flag == FlagsStruct::IsResidentNode)
				{
					Flags = Flags | 4;
				}
			}
			Writer.WriteInteger(Flags, sizeof(uint8_t));
			Writer.WriteInteger(0, 1);
			Writer.WriteInteger(GetOffsetInStringTable(Node.Name, StringTable), sizeof(uint32_t));
			Writer.WriteInteger(Node.NameHash, sizeof(uint32_t)); //TODO: MurMurHash3
			Writer.WriteInteger(0, sizeof(uint32_t));
			Writer.WriteInteger(Bodies[Node.NodeIndex], sizeof(uint32_t));
			Writer.WriteInteger(EXBInfo[Node.NodeIndex].first, sizeof(uint16_t));
			Writer.WriteInteger(EXBInfo[Node.NodeIndex].second, sizeof(uint16_t));
			uint16_t MultiParamCount = 0;
			for (int i = 0; i < AINBFile::ValueTypeCount; i++)
			{
				for (InputEntry& Param : Node.InputParameters[i])
				{
					MultiParamCount += Param.Sources.size();
				}
			}
			Node.MultiParamCount = MultiParamCount;
			Writer.WriteInteger(Node.MultiParamCount, sizeof(uint16_t));
			Writer.WriteInteger(0, sizeof(uint16_t));
			Writer.WriteInteger(BaseAttach, sizeof(uint32_t));
			Writer.WriteInteger(Node.BasePreconditionNode, sizeof(uint16_t));
			Writer.WriteInteger(Node.PreconditionCount, sizeof(uint16_t));
			Writer.WriteInteger(0, sizeof(uint32_t)); //Padding
			Writer.WriteInteger(Node.GUID.Part1, sizeof(uint32_t));
			Writer.WriteInteger(Node.GUID.Part2, sizeof(uint16_t));
			Writer.WriteInteger(Node.GUID.Part3, sizeof(uint16_t));
			Writer.WriteInteger(Node.GUID.Part4, sizeof(uint16_t));
			Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(Node.GUID.Part5), 6);
			BaseAttach += AttachCounts[Node.NodeIndex];
		}
	}

	Writer.Seek(AttachmentIndexStart, BinaryVectorWriter::Position::Begin);
	
	for (uint32_t Entry : AttachmentIndices)
	{
		Writer.WriteInteger(Entry, sizeof(uint32_t));
	}
	uint32_t AttachmentStart = Writer.GetPosition();
	int AttachmentIndex = 0;
	if (!Attachments.empty())
	{
		for (AINBFile::AttachmentEntry Attachment : Attachments)
		{
			AddToStringTable(Attachment.Name, StringTable);
			Writer.WriteInteger(GetOffsetInStringTable(Attachment.Name, StringTable), sizeof(uint32_t));
			Writer.WriteInteger(AttachmentStart + 16 * Attachments.size() + 100 * AttachmentIndex, sizeof(uint32_t));
			Writer.WriteInteger(0, sizeof(uint32_t)); //TODO: Again, EXB...
			Writer.WriteInteger(Attachment.NameHash, sizeof(uint32_t)); //Hash..
			AttachmentIndex++;
		}
		for (AINBFile::AttachmentEntry Attachment : Attachments)
		{
			if (Attachment.Name.find("Debug") != std::string::npos)
			{
				Writer.WriteInteger(1, sizeof(uint32_t));
			}
			else
			{
				Writer.WriteInteger(0, sizeof(uint32_t));
			}
			for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
			{
				if (!Attachment.Parameters[Type].empty())
				{
					for (AINBFile::ImmediateParameter Param : Attachment.Parameters[Type])
					{
						ImmediateParameters[Type].push_back(Param);
					}
					Writer.WriteInteger(ImmediateParameters[Type].size() - Attachment.Parameters[Type].size(), sizeof(uint32_t));
					Writer.WriteInteger(Attachment.Parameters[Type].size(), sizeof(uint32_t));
					ImmediateCurrent.find((AINBFile::ValueType)Type)->second = ImmediateParameters[Type].size();
				}
				else
				{
					Writer.WriteInteger(ImmediateCurrent[(AINBFile::ValueType)Type], sizeof(uint32_t));
					Writer.WriteInteger(0, sizeof(uint32_t));
				}
			}
			int Pos = Writer.GetPosition();
			for (int i = 0; i < 6; i++)
			{
				Writer.WriteInteger(0, sizeof(uint32_t));
				Writer.WriteInteger(Pos + 24, sizeof(uint32_t));
			}
		}
	}
	else
	{
		AttachmentStart = Writer.GetPosition();
	}

	uint32_t ImmediateStart = Writer.GetPosition();
	int Current = ImmediateStart + 24;
	bool ImmediateParametersEmpty = true;
	for (int i = 0; i < AINBFile::ValueTypeCount; i++)
	{
		if (!ImmediateParameters[i].empty())
		{
			ImmediateParametersEmpty = false;
			break;
		}
	}
	if (!ImmediateParametersEmpty)
	{
		for (int i = 0; i < AINBFile::ValueTypeCount; i++)
		{
			Writer.WriteInteger(Current, sizeof(uint32_t));
			if (i != (int)AINBFile::ValueType::Vec3f)
			{
				Current += ImmediateParameters[i].size() * 12;
			}
			else
			{
				Current += ImmediateParameters[i].size() * 20;
			}
		}
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBFile::ImmediateParameter Entry : ImmediateParameters[Type])
			{
				AddToStringTable(Entry.Name, StringTable);
				Writer.WriteInteger(GetOffsetInStringTable(Entry.Name, StringTable), sizeof(uint32_t));
				uint16_t Flags = 0x0;
				if (Type == (int)AINBFile::ValueType::UserDefined)
				{
					AddToStringTable(Entry.Class, StringTable);
					Writer.WriteInteger(GetOffsetInStringTable(Entry.Class, StringTable), sizeof(uint32_t));
				}
				if (Entry.GlobalParametersIndex != 0xFFFF)
				{
					Writer.WriteInteger(Entry.GlobalParametersIndex, sizeof(uint16_t));
					Flags += 0x8000;
				}
				else if(Entry.EXBIndex != 0xFFFF)
				{
					Writer.WriteInteger(Entry.EXBIndex, sizeof(uint16_t));
					Flags += 0xc200;
				}
				else
				{
					Writer.WriteInteger(0, sizeof(uint16_t));
				}
				if (!Entry.Flags.empty())
				{
					for (AINBFile::FlagsStruct Flag : Entry.Flags)
					{
						if (Flag == AINBFile::FlagsStruct::PulseThreadLocalStorage)
						{
							Flags += 0x80;
						}
						if (Flag == AINBFile::FlagsStruct::SetPointerFlagBitZero)
						{
							Flags += 0x100;
						}
					}
				}
				Writer.WriteInteger(Flags, sizeof(uint16_t));
				if (Type == (int)AINBFile::ValueType::Int)
				{
					Writer.WriteInteger(*reinterpret_cast<int*>(&Entry.Value), sizeof(int32_t));
				}
				else if (Type == (int)AINBFile::ValueType::Bool)
				{
					Writer.WriteInteger(*reinterpret_cast<bool*>(&Entry.Value), sizeof(uint32_t));
				}
				else if (Type == (int)AINBFile::ValueType::Float)
				{
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Entry.Value), sizeof(float));
				}
				else if (Type == (int)AINBFile::ValueType::String)
				{
					AddToStringTable(*reinterpret_cast<std::string*>(&Entry.Value), StringTable);
					Writer.WriteInteger(GetOffsetInStringTable(*reinterpret_cast<std::string*>(&Entry.Value), StringTable), sizeof(uint32_t));
				}
				else if (Type == (int)AINBFile::ValueType::Vec3f)
				{
					Vector3F Vec3f = *reinterpret_cast<Vector3F*>(&Entry.Value);
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[0]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[1]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[2]), sizeof(float));
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < 6; i++)
		{
			Writer.WriteInteger(0, sizeof(uint32_t));
		}
	}

	uint32_t IOStart = Writer.GetPosition();
	Current = IOStart + 48;

	bool InputParametersEmpty = true;
	bool OutputParametersEmpty = true;

	for (int i = 0; i < AINBFile::ValueTypeCount; i++)
	{
		if (!InputParameters[i].empty()) InputParametersEmpty = false;
		if (!OutputParameters[i].empty()) OutputParametersEmpty = false;
	}

	if (!InputParametersEmpty || !OutputParametersEmpty)
	{
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			Writer.WriteInteger(Current, sizeof(uint32_t));
			if (Type == (int)AINBFile::ValueType::Vec3f)
			{
				Current += InputParameters[Type].size() * 24;
			}
			else if (Type == (int)AINBFile::ValueType::UserDefined)
			{
				Current += InputParameters[Type].size() * 20;
			}
			else
			{
				Current += InputParameters[Type].size() * 16;
			}
			Writer.WriteInteger(Current, sizeof(uint32_t));
			if (Type != (int)AINBFile::ValueType::UserDefined)
			{
				Current += OutputParameters[Type].size() * 4;
			}
			else
			{
				Current += OutputParameters[Type].size() * 8;
			}
		}
		for (int Type = 0; Type < AINBFile::ValueTypeCount; Type++)
		{
			for (AINBFile::InputEntry Entry : InputParameters[Type])
			{
				AddToStringTable(Entry.Name, StringTable);
				Writer.WriteInteger(GetOffsetInStringTable(Entry.Name, StringTable), sizeof(uint32_t));
				uint16_t Flags = 0x0;
				if (Type == (int)AINBFile::ValueType::UserDefined)
				{
					AddToStringTable(Entry.Class, StringTable);
					Writer.WriteInteger(GetOffsetInStringTable(Entry.Class, StringTable), sizeof(uint32_t));
				}
			
				Writer.WriteInteger(Entry.NodeIndex, sizeof(int16_t));
				Writer.WriteInteger(Entry.ParameterIndex, sizeof(int16_t));

				if (Entry.GlobalParametersIndex != 0xFFFF)
				{
					Writer.WriteInteger(Entry.GlobalParametersIndex, sizeof(uint16_t));
					Flags += 0x8000;
				}
				else if (Entry.EXBIndex != 0xFFFF)
				{
					Writer.WriteInteger(Entry.EXBIndex, sizeof(uint16_t));
					Flags += 0xc200;
				}
				else
				{
					Writer.WriteInteger(0, sizeof(uint16_t));
				}

				if (!Entry.Flags.empty())
				{
					for (AINBFile::FlagsStruct Flag : Entry.Flags)
					{
						if (Flag == AINBFile::FlagsStruct::PulseThreadLocalStorage)
						{
							Flags += 0x80;
						}
						if (Flag == AINBFile::FlagsStruct::SetPointerFlagBitZero)
						{
							Flags += 0x100;
						}
					}
				}
				Writer.WriteInteger(Flags, sizeof(uint16_t));

				if (Type == (int)AINBFile::ValueType::Int)
				{
					Writer.WriteInteger(*reinterpret_cast<int*>(&Entry.Value), sizeof(int32_t));
				}
				else if (Type == (int)AINBFile::ValueType::Bool)
				{
					Writer.WriteInteger(*reinterpret_cast<bool*>(&Entry.Value), sizeof(uint32_t));
				}
				else if (Type == (int)AINBFile::ValueType::Float)
				{
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Entry.Value), sizeof(float));
				}
				else if (Type == (int)AINBFile::ValueType::String)
				{
					AddToStringTable(*reinterpret_cast<std::string*>(&Entry.Value), StringTable);
					Writer.WriteInteger(GetOffsetInStringTable(*reinterpret_cast<std::string*>(&Entry.Value), StringTable), sizeof(uint32_t));
				}
				else if (Type == (int)AINBFile::ValueType::Vec3f)
				{
					Vector3F Vec3f = *reinterpret_cast<Vector3F*>(&Entry.Value);
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[0]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[1]), sizeof(float));
					Writer.WriteRawUnsafeFixed(reinterpret_cast<char*>(&Vec3f.GetRawData()[2]), sizeof(float));
				}
				else if (Type == (int)AINBFile::ValueType::UserDefined)
				{
					Writer.WriteInteger(*reinterpret_cast<uint32_t*>(&Entry.Value), sizeof(uint32_t));
				}
			}
			for (AINBFile::OutputEntry Entry : OutputParameters[Type])
			{
				AddToStringTable(Entry.Name, StringTable);
				uint32_t Offset = GetOffsetInStringTable(Entry.Name, StringTable);
				if (Entry.SetPointerFlagsBitZero)
				{
					Writer.WriteInteger(Offset | (1 << 31), sizeof(uint32_t));
				}
				else
				{
					Writer.WriteInteger(Offset, sizeof(uint32_t));
				}
				if (Type == (int)AINBFile::ValueType::UserDefined)
				{
					AddToStringTable(Entry.Class, StringTable);
					Writer.WriteInteger(GetOffsetInStringTable(Entry.Class, StringTable), sizeof(uint32_t));
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < 12; i++)
		{
			Writer.WriteInteger(Current, sizeof(uint32_t));
		}
	}

	uint32_t MultiStart = Writer.GetPosition();
	if (!Multis.empty())
	{
		for (AINBFile::MultiEntry Entry : Multis)
		{
			Writer.WriteInteger(Entry.NodeIndex, sizeof(int16_t));
			Writer.WriteInteger(Entry.ParameterIndex, sizeof(int16_t));
			uint16_t Flags = 0x0;
			if (Entry.GlobalParametersIndex != 0xFFFF)
			{
				Writer.WriteInteger(Entry.GlobalParametersIndex, sizeof(uint16_t));
				Flags += 0x8000;
			}
			else if (Entry.EXBIndex != 0xFFFF)
			{
				Writer.WriteInteger(Entry.EXBIndex, sizeof(uint16_t));
				Flags += 0xc200;
			}
			else
			{
				Writer.WriteInteger(0, sizeof(uint16_t));
			}
			if (!Entry.Flags.empty())
			{
				for (AINBFile::FlagsStruct Flag : Entry.Flags)
				{
					if (Flag == AINBFile::FlagsStruct::PulseThreadLocalStorage)
					{
						Flags += 0x80;
					}
					if (Flag == AINBFile::FlagsStruct::SetPointerFlagBitZero)
					{
						Flags += 0x100;
					}
				}
			}
			Writer.WriteInteger(Flags, sizeof(uint16_t));
		}
	}

	uint32_t ResidentStart = Writer.GetPosition();
	if (!Residents.empty())
	{
		Current = ResidentStart + Residents.size() * 4;
		int n = 0;
		for (int i = 0; i < Residents.size(); i++)
		{
			if (Residents[i].String != "")
			{
				n = 8;
			}
			else
			{
				n = 4;
			}
			Writer.WriteInteger(Current, sizeof(uint32_t));
			Current += n;
		}
		for (AINBFile::ResidentEntry Resident : Residents)
		{
			uint32_t Flags = 0;
			for (AINBFile::FlagsStruct Flag : Resident.Flags)
			{
				if (Flag == AINBFile::FlagsStruct::IsValidUpdate)
				{
					Flags  = Flags | 1;
				}
				if (Flag == AINBFile::FlagsStruct::UpdatePostCurrentCommandCalc)
				{
					Flags = Flags | (1 << 31);
				}
			}
			Writer.WriteInteger(Flags, sizeof(uint32_t));
			if (Resident.String != "")
			{
				AddToStringTable(Resident.String, StringTable);
				Writer.WriteInteger(GetOffsetInStringTable(Resident.String, StringTable), sizeof(uint32_t));
			}
		}
	}

	uint32_t PreconditionStart = Writer.GetPosition();
	if (!PreconditionNodes.empty())
	{
		for (int i = 0; i < PreconditionNodes.size(); i++)
		{
			Writer.WriteInteger(PreconditionNodes[i], sizeof(uint16_t));
			Writer.WriteInteger(0, sizeof(uint16_t)); //Purpose unknown
		}
	}

	uint32_t EXBStart = Writer.GetPosition();
	if (EXBFile.Loaded)
	{
		EXBFile.Commands = EXBCommands;
		std::vector<unsigned char> Bytes = EXBFile.ToBinary(EXBInstances);
		Logger::Info("AINBEncoder", "EXB size: " + std::to_string(Bytes.size()));
		Writer.WriteRawUnsafeFixed((const char*)Bytes.data(), Bytes.size());
	}

	uint32_t EmbedAINBStart = Writer.GetPosition();

	this->EmbeddedAinbArray.clear();
	std::map<std::string, uint32_t> EmbeddedAINBs;
	for (AINBFile::Node& Node : this->Nodes)
	{
		if (std::find(Node.Flags.begin(), Node.Flags.end(), AINBFile::FlagsStruct::IsExternalAINB) != Node.Flags.end())
		{
			bool Initial = true;
			for (auto& [Key, Val] : EmbeddedAINBs)
			{
				if (Key == Node.GetName())
				{
					Initial = false;
					break;
				}
			}
			if (!Initial) EmbeddedAINBs[Node.GetName()] = EmbeddedAINBs[Node.GetName()] + 1;
			else EmbeddedAINBs.insert({ Node.GetName(), 1 });
		}
	}
	for (auto& [Key, Val] : EmbeddedAINBs)
	{
		AINBFile::EmbeddedAinb EAINB;
		EAINB.FilePath = Key + ".ainb";
		EAINB.FileCategory = this->Header.FileCategory;
		EAINB.Count = Val;
		this->EmbeddedAinbArray.push_back(EAINB);
	}

	Writer.WriteInteger(this->EmbeddedAinbArray.size(), sizeof(uint32_t));
	for (AINBFile::EmbeddedAinb AINB : this->EmbeddedAinbArray)
	{
		AddToStringTable(AINB.FilePath, StringTable);
		AddToStringTable(AINB.FileCategory, StringTable);
		Writer.WriteInteger(GetOffsetInStringTable(AINB.FilePath, StringTable), sizeof(uint32_t));
		Writer.WriteInteger(GetOffsetInStringTable(AINB.FileCategory, StringTable), sizeof(uint32_t));
		Writer.WriteInteger(AINB.Count, sizeof(uint32_t));
	}

	//TODO: Entry Strings
	uint32_t EntryStringsStart = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t)); //Entry Strings size

	uint32_t HashStart = Writer.GetPosition();
	Writer.WriteInteger(StarlightData::GetU64Signature(), sizeof(uint64_t)); //Setting two placeholder hashes, they are unused in game

	uint32_t ChildReplaceStart = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint16_t)); //Set at rutime
	if (!Replacements.empty())
	{
		Writer.WriteInteger(Replacements.size(), sizeof(uint16_t));
		uint32_t AttachCount = 0;
		uint32_t NodeCount = 0;
		int16_t OverrideNode = this->Nodes.size();
		int16_t OverrideAttach = this->AttachmentParameters.size();

		for (WriterReplacement Replacement : Replacements)
		{
			if (Replacement.Type == 2)
			{
				AttachCount += 1;
				OverrideAttach -= 1;
			}
			if (Replacement.Type == 0 || Replacement.Type == 1)
			{
				NodeCount += 1;
				if (Replacement.Type == 0)
				{
					OverrideNode -= 1;
				}
				if (Replacement.Type == 1)
				{
					OverrideNode -= 2;
				}
			}
		}
		if (NodeCount > 0)
		{
			Writer.WriteInteger(OverrideNode, sizeof(int16_t));
		}
		else
		{
			Writer.WriteInteger(-1, sizeof(int16_t));
		}
		if (AttachCount > 0)
		{
			Writer.WriteInteger(OverrideAttach, sizeof(int16_t));
		}
		else
		{
			Writer.WriteInteger(-1, sizeof(int16_t));
		}
		for (WriterReplacement Replacement : Replacements)
		{
			Writer.WriteInteger(Replacement.Type, sizeof(uint8_t));
			Writer.WriteInteger(0, sizeof(uint8_t));
			Writer.WriteInteger(Replacement.NodeIndex, sizeof(uint16_t));
			Writer.WriteInteger(Replacement.Iteration, sizeof(uint16_t));
			if (Replacement.Type == 1)
			{
				Writer.WriteInteger(Replacement.ReplacementNodeIndex, sizeof(uint16_t));
			}
		}
	}
	else
	{
		Writer.WriteInteger(0, sizeof(uint16_t));
		Writer.WriteInteger(-1, sizeof(int16_t));
		Writer.WriteInteger(-1, sizeof(int16_t));
	}

	uint32_t x6cStart = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t));
	uint32_t ResolveStart = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t));
	uint32_t StringsStart = Writer.GetPosition();

	StringTable.push_back(StarlightData::GetStringTableSignature());

	for (std::string String : StringTable)
	{
		Writer.WriteBytes(String.c_str());
		Writer.WriteByte(0x00);
	}

	Writer.Seek(24, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(Attachments.size(), sizeof(uint32_t));
	Writer.Seek(36, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(StringsStart, sizeof(uint32_t));
	Writer.WriteInteger(ResolveStart, sizeof(uint32_t));
	Writer.WriteInteger(ImmediateStart, sizeof(uint32_t));
	Writer.WriteInteger(ResidentStart, sizeof(uint32_t));
	Writer.WriteInteger(IOStart, sizeof(uint32_t));
	Writer.WriteInteger(MultiStart, sizeof(uint32_t));
	Writer.WriteInteger(AttachmentStart, sizeof(uint32_t));
	Writer.WriteInteger(AttachmentIndexStart, sizeof(uint32_t));
	Writer.WriteInteger(EXBFile.Loaded ? EXBStart : 0, sizeof(uint32_t));
	Writer.WriteInteger(ChildReplaceStart, sizeof(uint32_t));
	Writer.WriteInteger(PreconditionStart, sizeof(uint32_t));
	Writer.WriteInteger(ResidentStart, sizeof(uint32_t)); //Always the same
	Writer.Seek(8, BinaryVectorWriter::Position::Current);
	Writer.WriteInteger(EmbedAINBStart, sizeof(uint32_t));
	Writer.Seek(8, BinaryVectorWriter::Position::Current);
	Writer.WriteInteger(EntryStringsStart, sizeof(uint32_t));
	Writer.WriteInteger(x6cStart, sizeof(uint32_t));
	Writer.WriteInteger(HashStart, sizeof(uint32_t));

	Writer.Seek(20, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(PreconditionNodes.size(), sizeof(uint32_t));

	return Writer.GetData();
}

std::string AINBFile::Node::GetName()
{
	if (this->Type == (uint16_t)AINBFile::NodeTypes::UserDefined)
		return this->Name;

	return AINBFile::NodeTypeToString((AINBFile::NodeTypes)this->Type);
}

//I hate this file format just like BFRES, thousands of lines of code, just for reading and writing a 800 byte file...

void AINBFile::Write(std::string Path)
{
    std::ofstream File(Path, std::ios::binary);
    std::vector<unsigned char> Binary = this->ToBinary();
    std::copy(Binary.cbegin(), Binary.cend(),
        std::ostream_iterator<unsigned char>(File));
    File.close();
}