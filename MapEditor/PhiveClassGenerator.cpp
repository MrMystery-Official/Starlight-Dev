#include "PhiveClassGenerator.h"

#include "Util.h"
#include <iostream>
#include <fstream>

std::map<std::string, std::vector<std::string>> PhiveClassGenerator::Classes;
std::map<std::string, std::vector<std::string>> PhiveClassGenerator::ClassDependencies;
uint32_t PhiveClassGenerator::PointerArrayCount = 0;

std::string PhiveClassGenerator::ConvertClassName(std::string Name)
{
	Util::ReplaceString(Name, "::", "__");
	return Name;
}

std::string PhiveClassGenerator::GetPointerArrayName()
{
	return std::to_string(PointerArrayCount++);
}

void PhiveClassGenerator::GenerateClass(json& Data, std::string Name)
{
	if (Classes.count(Name))
		return;

	std::vector<std::string> ClassQuery;
	for (auto Class : Data)
	{
		if (Class["Name"] != Name)
			continue;

		std::vector<std::string> FullCode;

		std::vector<std::string> ReadCode;
		std::vector<std::string> WriteCode;
		std::vector<std::string> WriteArrayCode;

		if (!Class["Alignment"].is_null())
		{
			ReadCode.push_back("Reader.Align(" + std::to_string(Class["Alignment"].get<int>()) + ");");
			WriteCode.push_back("Writer.Align(" + std::to_string(Class["Alignment"].get<int>()) + ");");
		}

		FullCode.push_back("struct " + ConvertClassName(Class["Name"].get<std::string>()));
		if (!Class["Parent"].is_null())
		{
			if (Class["Parent"].get<std::string>().starts_with("hk"))
			{
				FullCode.push_back(" : public " + ConvertClassName(Class["Parent"].get<std::string>()));
				ReadCode.push_back(ConvertClassName(Class["Parent"].get<std::string>()) + "::Read(Reader);");
				WriteCode.push_back(ConvertClassName(Class["Parent"].get<std::string>()) + "::Write(Writer);");

				ClassQuery.push_back(Class["Parent"].get<std::string>());
				FullCode.push_back(" {\n");
			}
			else
			{
				FullCode.push_back(" : public PhiveBinaryVectorReader::hkReadable, public PhiveBinaryVectorWriter::hkWriteable {\n");
				FullCode.push_back("	" + Class["Parent"].get<std::string>() + " m_primitiveBase;\n");
				ReadCode.push_back("Reader.ReadStruct(&m_primitiveBase, sizeof(m_primitiveBase));");
				WriteCode.push_back("Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_primitiveBase), sizeof(m_primitiveBase));");
			}
		}
		else
		{
			FullCode.push_back(" : public PhiveBinaryVectorReader::hkReadable, public PhiveBinaryVectorWriter::hkWriteable {\n");
		}

		if (Class["Kind"] == "Array" && !Name.starts_with("hkFpMath"))
		{
			FullCode.push_back("	" + ConvertClassName(Class["SubType"].get<std::string>()) + " m_subTypeArray[" + std::to_string(Class["Size"].get<int>()) + "/sizeof(" + ConvertClassName(Class["SubType"].get<std::string>()) + ")];\n");
			ReadCode.push_back("Reader.ReadStruct(&m_subTypeArray[0], sizeof(m_subTypeArray));");
			WriteCode.push_back("Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_subTypeArray[0]), sizeof(m_subTypeArray));");
			if (Class["SubType"].get<std::string>().starts_with("hk"))
			{
				ClassQuery.push_back(Class["SubType"].get<std::string>());
			}
		}

		if (Class["Kind"] == "Pointer")
		{
			FullCode.insert(FullCode.begin(), "template <typename T>\n");
		}

		for (auto Declaration : Class["Declarations"])
		{
			std::string FieldName = "m_" + Declaration["DeclaredName"].get<std::string>();

			std::string Type = ConvertClassName(Declaration["Parent"].get<std::string>());

			std::string SubClassName = "";
			std::string SubTypeAddress = "";
			int SubClassSize = 0;
			bool IsParentArray = false;
			bool IsParentPointer = false;

			for (auto SubClass : Data)
			{
				if (SubClass["Address"] == Declaration["ParentAddress"])
				{
					if (SubClass["Kind"] == "Array")
					{
						IsParentArray = true;
					}
					else if (SubClass["Kind"] == "Pointer")
					{
						IsParentPointer = true;
					}

					if(!SubClass["SubType"].is_null())
						SubClassName = SubClass["SubType"].get<std::string>();

					if (!SubClass["Size"].is_null())
						SubClassSize = SubClass["Size"].get<int>();

					break;
				}
			}

			if(Type == "T*")
			{
				Type = "T";
			} 
			else if (Type == "T[N]")
			{
				Type = "T";
				FieldName += "[N]";
				FullCode.insert(FullCode.begin(), "template <typename T, int N>\n");
			}
			else if (IsParentPointer)
			{
				Type = Declaration["Parent"].get<std::string>() + "<" + ConvertClassName(SubClassName) + ">";
				if (SubClassName.starts_with("hk"))
				{
					ClassQuery.push_back(SubClassName);
					ClassQuery.push_back(Declaration["Parent"].get<std::string>());
				}
			}
			else if (Type == "hkArray" || Type == "hkRelArray")
			{
				if (ConvertClassName(SubClassName) != "hkRefPtr")
				{
					Type = "std::vector<" + ConvertClassName(SubClassName) + ">";
				}
				else
				{


					Type = "std::vector<" + ConvertClassName(SubClassName) + "<" +  + ">>";
				}
				if (SubClassName.starts_with("hk"))
				{
					ClassQuery.push_back(SubClassName);
				}
			}
			else
			{
				if (Type.starts_with("hk") && Type != "T*")
				{
					ClassQuery.push_back(Declaration["Parent"].get<std::string>());
				}
			}

			FullCode.push_back("	" + Type + " " + FieldName + ";\n");
			if (ConvertClassName(Declaration["Parent"].get<std::string>()).starts_with("hk"))
			{
				if (Declaration["Parent"].get<std::string>() == "hkArray" || Declaration["Parent"].get<std::string>() == "hkRelArray")
				{
					ReadCode.push_back("Reader.ReadHkArray<" + ConvertClassName(SubClassName) + ">(&" + FieldName + ", sizeof(" + ConvertClassName(SubClassName) + "), \"" + ConvertClassName(SubClassName) + "\");");
					WriteCode.push_back("Writer.QueryHkArrayWrite(\"" + GetPointerArrayName() + "\");");
					WriteArrayCode.push_back("Writer.WriteHkArray<" + ConvertClassName(SubClassName) + ">(&" + FieldName + ", sizeof(" + ConvertClassName(SubClassName) + "), \"" + ConvertClassName(SubClassName) + "\", \"" + std::to_string(PointerArrayCount-1) + "\");");
				}
				else if (IsParentArray)
				{
					ReadCode.push_back("Reader.ReadStruct(&" + FieldName + ", sizeof(" + FieldName + "));");
					WriteCode.push_back("Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&" + FieldName + "), sizeof(" + FieldName + "));");
				}
				else if (IsParentPointer)
				{
					ReadCode.push_back(FieldName + ".Read(Reader);");
					WriteCode.push_back("Writer.QueryHkPointerWrite(\"" + GetPointerArrayName() + "\");");
					WriteArrayCode.push_back("Writer.WriteHkPointer<" + ConvertClassName(SubClassName) + ">(&" + FieldName + ".m_ptr, sizeof(" + ConvertClassName(SubClassName) + "), \"" + std::to_string(PointerArrayCount-1) + "\");");
				}
				else
				{
					ReadCode.push_back(FieldName + ".Read(Reader);");
					WriteCode.push_back(FieldName + ".Write(Writer);");
				}
			}
			else
			{
				if (Declaration["Parent"].get<std::string>() == "T*")
				{
					ReadCode.push_back("Reader.ReadHkPointer<T>(&" + FieldName + ", sizeof(T));");
					WriteCode.push_back("Writer.QueryHkPointerWrite(\"" + GetPointerArrayName() + "\");");
					WriteArrayCode.push_back("Writer.WriteHkPointer<T>(&" + FieldName + ", sizeof(T), \"" + std::to_string(PointerArrayCount-1) + "\");");
				}
				else if (Declaration["Parent"].get<std::string>() == "T[N]")
				{
					ReadCode.push_back("Reader.ReadStruct(&m_" + Declaration["DeclaredName"].get<std::string>() + "[0], sizeof(m_" + Declaration["DeclaredName"].get<std::string>() + "));");
					WriteCode.push_back("Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_" + Declaration["DeclaredName"].get<std::string>() + "[0]), sizeof(m_" + Declaration["DeclaredName"].get<std::string>() + "));");
				}
				else
				{
					ReadCode.push_back("Reader.ReadStruct(&" + FieldName + ", sizeof(" + FieldName + "));");
					WriteCode.push_back("Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&" + FieldName + "), sizeof(" + FieldName + "));");
				}
			}
		}

		FullCode.push_back("\n	virtual void Read(PhiveBinaryVectorReader& Reader) override {\n");
		for (const std::string& Line : ReadCode)
		{
			FullCode.push_back("		" + Line + "\n");
		}
		FullCode.push_back("	}\n");

		FullCode.push_back("\n	virtual void Write(PhiveBinaryVectorWriter& Writer) override {\n");
		for (const std::string& Line : WriteCode)
		{
			FullCode.push_back("		" + Line + "\n");
		}
		FullCode.push_back("		\n");
		for (const std::string& Line : WriteArrayCode)
		{
			FullCode.push_back("		" + Line + "\n");
		}
		FullCode.push_back("	}\n");

		FullCode.push_back("};\n");

		Classes.insert({ Class["Name"].get<std::string>(), FullCode });

		break;
	}
	std::vector<std::string> ProcessedClassQuery;

	for (const std::string& Class : ClassQuery)
	{
		if (std::find(ProcessedClassQuery.begin(), ProcessedClassQuery.end(), Class) == ProcessedClassQuery.end())
			ProcessedClassQuery.push_back(Class);
	}

	ClassDependencies.insert({ Name, ProcessedClassQuery });

	for (std::string Class : ProcessedClassQuery)
	{
		GenerateClass(Data, Class);
	}
}

void PhiveClassGenerator::SortClassDependencies(std::string Name, std::vector<std::pair<std::string, std::vector<std::string>>>& Sorted)
{
	for (const std::string& Dependency : ClassDependencies[Name])
	{
		SortClassDependencies(Dependency, Sorted);

		bool Contains = false;
		for (auto& [Key, Val] : Sorted)
		{
			if (Key == Dependency)
			{
				Contains = true;
				break;
			}
		}

		if (!Contains)
			Sorted.push_back({ Dependency, Classes[Dependency] });
	}

	bool Contains = false;
	for (auto& [Key, Val] : Sorted)
	{
		if (Key == Name)
		{
			Contains = true;
			break;
		}
	}
	if(!Contains)
		Sorted.push_back({ Name, Classes[Name] });
}

void PhiveClassGenerator::Generate(std::string Path, std::vector<std::string> Goals)
{
	std::ifstream File(Path);
	json Data = json::parse(File);

	Classes.clear();
	ClassDependencies.clear();
	for (std::string Goal : Goals)
	{
		GenerateClass(Data, Goal);
	}

	std::vector<std::pair<std::string, std::vector<std::string>>> SortedClasses;

	for (std::string Goal : Goals)
	{
		SortClassDependencies(Goal, SortedClasses);
	}

	for (auto& [Key, Val] : SortedClasses)
	{
		for (const std::string& Line : Val)
		{
			/*
			if (Line.starts_with("struct hkcdCompressedAabbCodecs__Aabb4BytesCodec : public hkcdCompressedAabbCodecs__CompressedAabbCodec"))
			{
				std::cout << "struct hkcdCompressedAabbCodecs__Aabb4BytesCodec : public hkcdCompressedAabbCodecs__CompressedAabbCodec<uint8_t, 3> {\n";
				continue;
			}
			if (Line == "    hkFpMath__Detail__MultiWordUint m_storage;")
			{
				std::cout << "    hkFpMath__Detail__MultiWordUint<unsigned long long, 4> m_storage;\n";
				continue;
			}
			*/
			std::cout << Line;
		}
		std::cout << "\n\n";
	}

	File.close();
}