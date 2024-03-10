#include "MSBT.h"

#include "Logger.h"
#include <fstream>
#include <iostream>
#include "Util.h"
#include <locale>
#include <codecvt>
#include <sstream>
#include "BinaryVectorWriter.h"
#include <windows.h>

//uint16_t Length, uint16_t Data[Length]
std::string MSBTFile::ReadString(std::vector<uint16_t> Data, uint16_t Start)
{
	std::string Result;
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> Converter;
	for (int i = 0; i < Data[Start]/2; i++)
	{
		Result += Converter.to_bytes(Data[i + Start + 1]);
	}
	return Result;
}

void MSBTFile::ParseFunction(std::string& Text, uint16_t Group, uint16_t Type, std::vector<uint16_t> Args)
{
	if (Group == 0)
	{
		if (Type == 1)
		{
			Text += "<ruby=[charSpan=" + std::to_string(Args[0]) + ",value=" + ReadString(Args, 1) + "]>";
		}
		if (Type == 2)
		{
			Text += "<font=[face=" + ReadString(Args) + "]>";
		}
		if (Type == 3)
		{
			Text += "<color=[id=" + std::to_string(Args[0]) + "]>";
		}
		if (Type == 4)
		{
			Text += "<pageBreak=[]>";
		}
	}

	if (Group == 1)
	{
		if (Type == 0)
		{
			Text += "<delay=[frames=" + std::to_string(Args[0]) + "]>";
		}
		if (Type == 3)
		{
			Text += "<playSound=[id=" + std::to_string(Args[0]) + "]>";
		}
		if (Type == 4)
		{
			Text += "<icon=[id=" + std::to_string(Args[0]) + "]>";
		}
	}

	if (Group == 2)
	{
		if (Type == 2)
		{
			Text += "<number2=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 3)
		{
			Text += "<currentHorseName=[]>";
		}
		if (Type == 4)
		{
			Text += "<selectedHorseName=[]>";
		}
		if (Type == 7)
		{
			Text += "<cookingAdjective=[]>";
		}
		if (Type == 8)
		{
			Text += "<cookingEffectCaption=[]>";
		}
		if (Type == 9)
		{
			Text += "<number9=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 11)
		{
			Text += "<string11=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 12)
		{
			Text += "<string12=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 14)
		{
			Text += "<number14=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 15)
		{
			Text += "<number15=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 16)
		{
			Text += "<number16=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 18)
		{
			Text += "<shopTradePriceItem=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 19)
		{
			Text += "<time=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 20)
		{
			Text += "<coords=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 21)
		{
			Text += "<number21=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 22)
		{
			Text += "<number22=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 24)
		{
			Text += "<attachmentAdjective=[]>";
		}
		if (Type == 25)
		{
			Text += "<equipmentBaseName=[]>";
		}
		if (Type == 26)
		{
			Text += "<essenceAdjective=[]>";
		}
		if (Type == 27)
		{
			Text += "<essenceBaseName=[]>";
		}
		if (Type == 28)
		{
			Text += "<weaponName=[]>";
		}
		if (Type == 29)
		{
			Text += "<playerName=[]>";
		}
		if (Type == 30)
		{
			Text += "<questItemName=[]>";
		}
		if (Type == 31)
		{
			Text += "<shopSelectItemName=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 32)
		{
			Text += "<sensorTargetNameOnActorMode=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 33)
		{
			Text += "<yonaDynamicName=[]>";
		}
		if (Type == 35)
		{
			Text += "<shopSelectItemName2=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 36)
		{
			Text += "<shopSelectItemName3=[ref=" + ReadString(Args) + "]>";
		}
		if (Type == 37)
		{
			Text += "<recipeName=[]>";
		}
	}

	if (Group == 3)
	{
		if (Type == 0)
		{
			std::string Info;
			for (uint16_t Data : Args)
			{
				Info += std::to_string(Data);
			}
			Text += "<resetAnim=[info=" + Info + "]>";
		}
		if (Type == 1)
		{
			Text += "<setItalicFont=[]>";
		}
	}

	if (Group == 4)
	{
		if (Type == 0)
		{
			Text += "<anim=[type=" + ReadString(Args) + "]>";
		}
	}

}

void MSBTFile::ReadLBL1(BinaryVectorReader& Reader)
{
	Reader.Seek(8, BinaryVectorReader::Position::Current); //Padding
	uint32_t Pos = Reader.GetPosition();
	uint32_t EntryCount = Reader.ReadUInt32();

	std::vector<std::pair<uint32_t, uint32_t>> Groups(EntryCount);

	for (int i = 0; i < EntryCount; i++)
	{
		Groups[i].first = Reader.ReadUInt32();
		Groups[i].second = Reader.ReadUInt32();
	}

	for (auto& [NumberOfLabels, Offset] : Groups)
	{
		Reader.Seek(Pos + Offset, BinaryVectorReader::Position::Begin);
		std::vector<std::string> Group;
		for (int i = 0; i < NumberOfLabels; i++)
		{
			MSBTFile::LabelEntry Entry;
			Entry.Length = Reader.ReadUInt8();
			Entry.Name = Reader.ReadString(Entry.Length);
			Entry.Index = Reader.ReadUInt32();
			Entry.Checksum = std::distance(Groups.begin(), std::find(Groups.begin(), Groups.end(), std::pair{ NumberOfLabels, Offset } ));
			this->m_LabelData.push_back(Entry);
			Group.push_back(Entry.Name);
		}
		this->Groups.push_back(Group);
	}

	while (Reader.GetPosition() % 8 != 0) //Aligning by 8 bytes
	{
		Reader.Seek(1, BinaryVectorReader::Position::Current);
	}
}

void MSBTFile::ReadTXT2(BinaryVectorReader& Reader)
{
	Reader.Seek(-4, BinaryVectorReader::Position::Current);
	uint32_t SectionSize = Reader.ReadUInt32();

	Reader.Seek(8, BinaryVectorReader::Position::Current); //Padding

	uint32_t Pos = Reader.GetPosition();
	uint32_t EntryCount = Reader.ReadUInt32();
	std::vector<uint32_t> Offsets(EntryCount);
	for (int i = 0; i < EntryCount; i++)
	{
		Offsets[i] = Reader.ReadUInt32();
	}

	for (int i = 0; i < EntryCount; i++)
	{
		uint32_t StartPos = Offsets[i] + Pos;
		uint32_t EndPos = (i + 1) < EntryCount ? (Pos + Offsets[i + 1]) : (Pos + SectionSize);

		Reader.Seek(StartPos, BinaryVectorReader::Position::Begin);

		MSBTFile::StringEntry Entry;
		
		//std::vector<uint16_t> Bytes(EndPos-StartPos);
		//Reader.ReadStruct(Bytes.data(), Bytes.size());
		
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> Converter;

		for (int j = 0; j < (EndPos - StartPos)/2; )
		{
			j++;
			uint16_t Data = Reader.ReadUInt16();
			if (Data == 0x000E)
			{
				uint16_t Group = Reader.ReadUInt16();
				uint16_t Type = Reader.ReadUInt16();
				uint16_t ArgSize = Reader.ReadUInt16();
				std::vector<uint16_t> Args(ArgSize / 2);
				Reader.Read((char*)Args.data(), ArgSize);

				ParseFunction(Entry.Text, Group, Type, Args);

				j += 2 + (ArgSize / 2);
			}
			else if (Data == 0x000F)
			{
				Reader.Seek(4, BinaryVectorReader::Position::Current);
				j += 2;
			}
			else if (Data == 0x0000)
			{
			}
			else
			{
				Entry.Text += Converter.to_bytes(Data);
			}
		}

		Entry.Encoding = this->StringEncoding;
		Entry.Index = i;
		this->m_StringData.push_back(Entry);
	}
}

MSBTFile::MSBTFile(std::vector<unsigned char> Bytes)
{
	BinaryVectorReader Reader(Bytes);
	Reader.ReadStruct(&Header, sizeof(MSBTHeader));
	Reader.Seek(8, BinaryVectorReader::Position::Current);

	if(Header.Magic[0] != 'M' || Header.Magic[1] != 's' || Header.Magic[2] != 'g' || Header.Magic[3] != 'S' || Header.Magic[4] != 't' || Header.Magic[5] != 'd' || Header.Magic[6] != 'B' || Header.Magic[7] != 'n')
	{
		Logger::Error("MSBTDecoder", "Wrong magic, expected MsgStdBn");
		return;
	}

	if (Header.Encoding == 0x00)
		this->StringEncoding = MSBTFile::StringEncodingStruct::UTF_8;
	else
		this->StringEncoding = MSBTFile::StringEncodingStruct::Unicode;

	for (int i = 0; i < Header.SectionCount; i++)
	{
		uint32_t Pos = Reader.GetPosition();

		std::string Signature = Reader.ReadString(4);
		uint32_t SectionSize = Reader.ReadUInt32();
		std::cout << Signature << std::endl;
		if (Signature == "LBL1")
		{
			this->ReadLBL1(Reader);
		}
		else if (Signature == "TXT2" || Signature == "TXTW")
		{
			this->ReadTXT2(Reader);
		}

		Reader.Seek(Pos + SectionSize + 0x10, BinaryVectorReader::Position::Begin);
		
		while (Reader.GetPosition() % 16 != 0) //Aligning by 16 bytes
		{
			Reader.Seek(1, BinaryVectorReader::Position::Current);
		}
	}

	for (MSBTFile::LabelEntry& Entry : this->m_LabelData)
	{
		Text.insert({ Entry.Name, this->m_StringData[Entry.Index] });
	}

	this->Loaded = true;
}

void MSBTFile::MessageToBinary(std::vector<uint16_t>& Bytes, std::string Message)
{
	bool InDataArea = false;

	uint32_t DataStage = 0;
	uint32_t ArgStage = 0;
	std::string Key = "";
	uint32_t ArgIndex = 0;
	std::vector<std::pair<std::string, std::string>> Args;

	for (char ch : Message)
	{
		if (ch == '<')
		{
			InDataArea = true;
			continue;
		}
		if (ch == '>')
		{
			std::cout << "Data for key " << Key << std::endl;
			for (auto& [Key, Val] : Args)
			{
				std::cout << "Argument: " << Key << ", " << Val << std::endl;
			}
			
			if (Key == "anim")
			{
				Bytes.push_back(0x000E);
				Bytes.push_back(4);
				Bytes.push_back(0);
				Bytes.push_back(Args[0].second.length() * 2 + 2);
				Bytes.push_back(Args[0].second.length() * 2);
				std::wstring AnimName;
				AnimName.resize(Args[0].second.length());
				MultiByteToWideChar(CP_ACP, 0, Args[0].second.data(), Args[0].second.length(), AnimName.data(), Args[0].second.length());
				std::wcout << AnimName << std::endl;
				for (wchar_t AnimChar : AnimName)
				{
					Bytes.push_back((uint16_t)AnimChar);
				}
			}

			if (Key == "resetAnim")
			{
				Bytes.push_back(0x000E);
				Bytes.push_back(3);
				Bytes.push_back(0);
				Bytes.push_back(2);
				Bytes.push_back((uint16_t)std::stoi(Args[0].second));
			}

			InDataArea = false;
			Key = "";
			Args.clear();
			ArgIndex = 0;
			ArgStage = 0;
			DataStage = 0;
			continue;
		}

		if (InDataArea)
		{
			if (ch == '=' && DataStage == 0)
			{
				DataStage++;
				continue;
			}

			if (ch == '['&& DataStage == 1)
			{
				Args.resize(1);
				continue;
			}

			if (ch == ']' && DataStage == 1)
			{
				continue;
			}

			if (ch == ',' && DataStage == 1)
			{
				ArgIndex++;
				Args.resize(ArgIndex + 1);
				ArgStage = 0;
				continue;
			}

			if (DataStage == 0)
			{
				Key += ch;
				continue;
			}

			if (DataStage == 1)
			{
				if (ch == '=')
				{
					ArgStage = 1;
					continue;
				}

				if (ArgStage == 0)
				{
					Args[ArgIndex].first += ch;
				}
				else
				{
					Args[ArgIndex].second += ch;
				}
			}
			continue;
		}

		wchar_t NewChar;
		MultiByteToWideChar(CP_ACP, 0, &ch, 1, &NewChar, 1);
		Bytes.push_back((uint16_t)NewChar);
	}
}

std::vector<unsigned char> MSBTFile::ToBinary()
{
	BinaryVectorWriter Writer;

	Writer.WriteBytes("MsgStdBn");
	Writer.WriteInteger(this->Header.ByteOrderMask, sizeof(uint16_t));
	Writer.WriteInteger(0, sizeof(uint16_t)); //Padding
	Writer.WriteInteger(this->Header.Encoding, sizeof(uint8_t));
	Writer.WriteInteger(this->Header.Version, sizeof(uint8_t));

	uint16_t SectionCount = 2; //Two sections are base, optional attribute section. TODO: Calculating
	Writer.WriteInteger(SectionCount, sizeof(uint16_t));
	
	Writer.WriteInteger(0, sizeof(uint16_t)); //Padding
	uint32_t FileSizeJumback = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t)); //File size, skipping until known

	Writer.Seek(10, BinaryVectorWriter::Position::Current); //Reserved

	//Label section
	Writer.WriteBytes("LBL1");
	uint32_t LabelSectionJumpback = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t)); //section size, skipping until known
	Writer.Seek(8, BinaryVectorWriter::Position::Current); //Padding
	uint32_t LabelOffsetJumpback = Writer.GetPosition();
	Writer.Seek(4, BinaryVectorWriter::Position::Current); //EntryCount

	uint32_t LabelGroupCount = this->Groups.size();

	for (std::vector<std::string>& Group : this->Groups)
	{
		Writer.WriteInteger((uint32_t)Group.size(), sizeof(uint32_t));
		Writer.Seek(4, BinaryVectorWriter::Position::Current); //Offset
	}
	
	uint32_t LabelDataJumback = Writer.GetPosition();

	std::vector<uint32_t> LabelOffsets;
	for (std::vector<std::string>& Group : this->Groups)
	{
		LabelOffsets.push_back(Writer.GetPosition() - LabelOffsetJumpback);
		for (std::string& Label : Group)
		{
			Writer.WriteInteger((uint8_t)Label.size(), sizeof(uint8_t));
			Writer.WriteRawUnsafeFixed(Label.data(), Label.size());
			Writer.WriteInteger(0, sizeof(uint32_t)); //Skipping index until known
		}
	}

	uint32_t LabelSectionSize = Writer.GetPosition() - LabelOffsetJumpback;

	while (Writer.GetPosition() % 16 != 0) //Aligning by 16 bytes
	{
		Writer.WriteByte(0xAB);
	}
	uint32_t TextSectionBegin = Writer.GetPosition();

	Writer.Seek(LabelSectionJumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(LabelSectionSize, sizeof(uint32_t));
	Writer.Seek(LabelOffsetJumpback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(LabelGroupCount, sizeof(uint32_t));
	for (uint32_t Offset : LabelOffsets)
	{
		Writer.Seek(4, BinaryVectorWriter::Position::Current); //Count
		Writer.WriteInteger(Offset, sizeof(uint32_t));
	}

	Writer.Seek(TextSectionBegin, BinaryVectorWriter::Position::Begin);
	Writer.WriteBytes("TXT2");
	uint32_t TextSectionSizeJumback = Writer.GetPosition();
	Writer.WriteInteger(0, sizeof(uint32_t));
	Writer.Seek(8, BinaryVectorWriter::Position::Current); //Padding
	uint32_t TextSectionEntryCountJumpback = Writer.GetPosition();
	Writer.WriteInteger((uint32_t)Text.size(), sizeof(uint32_t)); //EntryCount

	Writer.Seek(sizeof(uint32_t) * Text.size(), BinaryVectorWriter::Position::Current); //Skipping offsets

	std::vector<uint32_t> TextSectionOffsets;

	std::unordered_map<std::string, uint32_t> TextToIndex;
	uint32_t TextIndex = 0;
	for (auto& [Key, Val] : Text)
	{
		TextSectionOffsets.push_back((uint32_t)Writer.GetPosition() - TextSectionEntryCountJumpback);
		TextToIndex.insert({ Val.Text, TextIndex });
		TextIndex++;
		std::vector<uint16_t> Bytes;
		this->MessageToBinary(Bytes, Val.Text);
		for (uint16_t Data : Bytes)
		{
			Writer.WriteInteger(Data, sizeof(Data));
		}
	}

	uint32_t TextSectionSize = Writer.GetPosition() - TextSectionEntryCountJumpback;
	while (Writer.GetPosition() % 16 != 0) //Aligning by 16 bytes
	{
		Writer.WriteByte(0xAB);
	}

	uint32_t FileSize = Writer.GetPosition();

	Writer.Seek(TextSectionSizeJumback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(TextSectionSize, sizeof(uint32_t));
	Writer.Seek(TextSectionEntryCountJumpback, BinaryVectorWriter::Position::Begin);
	Writer.Seek(4, BinaryVectorWriter::Position::Current);
	for (uint32_t Offset : TextSectionOffsets)
	{
		Writer.WriteInteger(Offset, sizeof(uint32_t));
	}

	Writer.Seek(FileSizeJumback, BinaryVectorWriter::Position::Begin);
	Writer.WriteInteger(FileSize, sizeof(uint32_t));

	Writer.Seek(LabelDataJumback, BinaryVectorWriter::Position::Begin);

	for (std::vector<std::string>& Group : this->Groups)
	{
		for (std::string Label : Group)
		{
			Writer.Seek(1 + Label.size(), BinaryVectorWriter::Position::Current);

			uint32_t Index = 0xFFFFFFFF;

			for (auto [Key, Val] : Text)
			{
				if (Key == Label)
				{
					Index = TextToIndex[Val.Text];
				}
			}

			if (Index == 0xFFFFFFFF)
			{
				Logger::Error("MSBTEncoder", "Could not calculate index for label");
				continue;
			}

			Writer.WriteInteger(Index, sizeof(uint32_t));
		}
	}

	return Writer.GetData();
}

void MSBTFile::WriteToFile(std::string Path)
{
	std::ofstream File(Path, std::ios::binary);
	std::vector<unsigned char> Binary = this->ToBinary();
	std::copy(Binary.cbegin(), Binary.cend(),
		std::ostream_iterator<unsigned char>(File));
	File.close();
}

MSBTFile::MSBTFile(std::string Path)
{
	std::ifstream File(Path, std::ios::binary);

	if (!File.eof() && !File.fail())
	{
		File.seekg(0, std::ios_base::end);
		std::streampos FileSize = File.tellg();

		std::vector<unsigned char> Bytes(FileSize);

		File.seekg(0, std::ios_base::beg);
		File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

		this->MSBTFile::MSBTFile(Bytes);

		File.close();
	}
	else
	{
		Logger::Error("MSBTDecoder", "Could not open file \"" + Path + "\"");
	}
}