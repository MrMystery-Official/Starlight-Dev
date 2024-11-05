#include "PhiveClassGenerator.h"

#include "Util.h"
#include "Logger.h"
#include <iostream>
#include <fstream>

std::unordered_map<std::string, PhiveClassGenerator::HavokClassRepresentation> PhiveClassGenerator::mClasses;
std::unordered_map<std::string, std::vector<std::string>> PhiveClassGenerator::mCode;
std::unordered_map<std::string, std::vector<std::string>> PhiveClassGenerator::mClassDependencies;

PhiveClassGenerator::HavokClassKind PhiveClassGenerator::StringToKind(const std::string& Name)
{
	if (Name == "Array")
		return HavokClassKind::ARRAY;
	
	if (Name == "Record")
		return HavokClassKind::RECORD;

	if (Name == "Float")
		return HavokClassKind::FLOAT;

	if (Name == "Int")
		return HavokClassKind::INT;

	if (Name == "String")
		return HavokClassKind::STRING;

	if (Name == "Pointer")
		return HavokClassKind::POINTER;

	if (Name == "Bool")
		return HavokClassKind::BOOL;

	Logger::Error("PhiveClassGenerator", "Unknown kind: " + Name);

	return HavokClassKind::RECORD;
}

void PhiveClassGenerator::GenerateClassRepresentation(json& Data)
{
	auto SetPtrByAddress = [](HavokClassRepresentation** Ptr, const std::string& AddrName, json& Class)
		{
			if (!Class[AddrName].is_null())
			{
				std::string Addr = Class[AddrName].get<std::string>();
				if (mClasses.contains(Addr))
				{
					*Ptr = &mClasses[Addr];
					return;
				}
			}
			*Ptr = nullptr;
		};

	for (auto Class : Data)
	{
		if ((!Class["Size"].is_null() && Class["Size"].get<int>() == 0) || (!Class["Kind"].is_null() && Class["Kind"].get<std::string>() == "Opaque"))
			continue;

		HavokClassRepresentation& Rep = mClasses[Class["Address"].get<std::string>()];
		Rep.mName = Class["Name"].get<std::string>();
		Rep.mAddress = Class["Address"].get<std::string>();
		Rep.mSize = Class["Size"].is_null() ? -1 : Class["Size"].get<int>();
		Rep.mAlignment = Class["Alignment"].is_null() ? -1 : Class["Alignment"].get<int>();
		
		Rep.mKind = Class["Kind"].is_null() ? HavokClassKind::NONE : StringToKind(Class["Kind"].get<std::string>());

		Rep.mIsPrimitiveType = Rep.mKind != HavokClassKind::RECORD && Rep.mKind != HavokClassKind::ARRAY && Rep.mKind != HavokClassKind::POINTER && Rep.mKind != HavokClassKind::STRING && Rep.mKind != HavokClassKind::NONE;
	}

	for (auto Class : Data)
	{
		if ((!Class["Size"].is_null() && Class["Size"].get<int>() == 0) || (!Class["Kind"].is_null() && Class["Kind"].get<std::string>() == "Opaque"))
			continue;

		HavokClassRepresentation& Rep = mClasses[Class["Address"].get<std::string>()];

		SetPtrByAddress(&Rep.mSubType, "SubTypeAddress", Class);
		SetPtrByAddress(&Rep.mParent, "ParentAddress", Class);
		
		if (!Class["Declarations"].is_null())
		{
			for (auto JsonDeclaration : Class["Declarations"])
			{
				HavokClassRepresentation::Declaration Decl;
				Decl.mDeclaredName = JsonDeclaration["DeclaredName"].get<std::string>();
				SetPtrByAddress(&Decl.mParent, "ParentAddress", JsonDeclaration);
				Rep.mDeclarations.push_back(Decl);
			}
		}

		if (!Class["Template"].is_null())
		{
			for (auto JsonTemplate : Class["Template"])
			{
				if (JsonTemplate["Name"].is_null())
					continue;

				HavokClassRepresentation::Template Temp;
				Temp.mName = JsonTemplate["Name"].get<std::string>();

				if (Temp.mName == "tAllocator")
					continue;

				if (!JsonTemplate["Address"].is_null())
				{
					SetPtrByAddress(&Temp.mType, "Address", JsonTemplate);
				}
				else if(!JsonTemplate["Value"].is_null())
				{
					Temp.mValue = JsonTemplate["Value"].get<int>();
				}
				else
				{
					continue;
				}

				Rep.mTemplates.push_back(Temp);
			}
		}
	}
}

std::string PhiveClassGenerator::GenerateClassName(HavokClassRepresentation& Rep, std::vector<std::string>& Depends, bool Field, std::string* PreClass)
{
	if (Rep.mIsPrimitiveType)
		return Rep.mName;

	if (!Field && *PreClass == "" && !Rep.mTemplates.empty())
	{
		*PreClass = "template<";
		for (HavokClassRepresentation::Template& Template : Rep.mTemplates)
		{
			std::string Name = Template.mName;
			Name.erase(0, 1);
			*PreClass += "class " + Name + ", ";
		}
		PreClass->pop_back();
		PreClass->pop_back();
		*PreClass += ">\n";
	}

	std::string Name = Rep.mName;
	Util::ReplaceString(Name, "::", "__");
	if (Rep.mName != "hkArray" && 
		Rep.mName != "hkRefPtr")
	{
		Name += "_" + Rep.mAddress;
	}
	if (!Rep.mTemplates.empty() && Field)
	{
		Name += "<";

		for (HavokClassRepresentation::Template& Template : Rep.mTemplates)
		{
			if (Template.mType == nullptr)
				continue;

			Name += GenerateClassName(*Template.mType, Depends);
			if (!Template.mType->mIsPrimitiveType)
			{
				Depends.push_back(Template.mType->mAddress);
			}
		}

		Name += ">";
	}
	return Name;
}

void PhiveClassGenerator::GenerateClassCode(const std::string& Address)
{
	if (!mClasses.contains(Address))
		return;

	std::vector<std::string> Code;
	std::string PreClass = "";
	std::vector<std::string> Depends;

	HavokClassRepresentation& Rep = mClasses[Address];

	if (Rep.mName == "hkArray" || Rep.mName == "hkRefPtr")
		return;

	uint32_t RemainingSize = Rep.mSize;

	unsigned int Size = Rep.mSize;
	if (Rep.mParent != nullptr)
		Size -= Rep.mParent->mSize;

	Code.push_back("struct " + GenerateClassName(Rep, Depends, false, &PreClass) + " : public PhiveBinaryVectorReader::hkReadable, public PhiveBinaryVectorWriter::hkWriteable");

	//Parent processing - Begin
	if (Rep.mParent != nullptr && !Rep.mParent->mIsPrimitiveType)
	{
		Code.push_back(", public " + GenerateClassName(*Rep.mParent, Depends));
		Depends.push_back(Rep.mParent->mAddress);
		RemainingSize -= Rep.mParent->mSize;
	}
	Code.push_back(" {\n");
	//Parent processing - End

	//Information processing - Begin
	if(Rep.mSize > 0){
		Code.push_back("	const unsigned int m_internal_totalSize = " + std::to_string(Rep.mSize) + ";\n");
		Code.push_back("	const unsigned int m_internal_size = " + std::to_string(Size) + ";\n\n");
	}
	//Information processing - End

	//Primitive base processing - Begin
	bool HasPrimitiveBase = false;
	if (Rep.mParent != nullptr && Rep.mParent->mIsPrimitiveType)
	{
		Code.push_back("	" + GenerateClassName(*Rep.mParent, Depends) + " m_internal_primitiveBase;\n");
		HasPrimitiveBase = true;
	}
	//Primitive base processing - End

	//Declarations processing - Begin
	for (HavokClassRepresentation::Declaration& Decl : Rep.mDeclarations)
	{
		if (Decl.mParent == nullptr)
			continue;

		Code.push_back("	" + GenerateClassName(*Decl.mParent, Depends) + " m_" + Decl.mDeclaredName + ";\n");
		if(!Decl.mParent->mIsPrimitiveType)
			Depends.push_back(Decl.mParent->mAddress);

		RemainingSize -= Decl.mParent->mSize;
	}
	//Declarations processing - End

	//SubType processing - Begin
	if (Rep.mSubType != nullptr && RemainingSize >= Rep.mSubType->mSize)
	{
		Code.push_back("	" + GenerateClassName(*Rep.mSubType, Depends) + " m_internal_subTypeData[" + std::to_string((RemainingSize / Rep.mSubType->mSize)) + "];\n");
		if (!Rep.mSubType->mIsPrimitiveType)
			Depends.push_back(Rep.mSubType->mAddress);
	}
	//SubType processing - End

	Code.push_back("\n");

	//Read processing - Begin
	{
		Code.push_back("	virtual void Read(PhiveBinaryVectorReader& Reader) override {\n");

		if (Rep.mParent != nullptr && !HasPrimitiveBase)
			Code.push_back("		" + GenerateClassName(*Rep.mParent, Depends) + "::Read(Reader);\n");

		if(Rep.mAlignment > 0)
			Code.push_back("		Reader.Align(" + std::to_string(Rep.mAlignment) + ");\n");
		else if(HasPrimitiveBase)
			Code.push_back("		Reader.Align(" + std::to_string(Rep.mParent->mAlignment) + ");\n");

		if (HasPrimitiveBase)
			Code.push_back("		Reader.ReadStruct(&m_internal_primitiveBase, sizeof(m_internal_primitiveBase));\n");

		for (HavokClassRepresentation::Declaration& Decl : Rep.mDeclarations)
		{
			if (Decl.mParent == nullptr)
				continue;

			if (Decl.mParent->mIsPrimitiveType)
			{
				if (Decl.mParent->mAlignment > 0)
					Code.push_back("		Reader.Align(" + std::to_string(Decl.mParent->mAlignment) + ");\n");

				Code.push_back("		Reader.ReadStruct(&m_" + Decl.mDeclaredName + ", sizeof(m_" + Decl.mDeclaredName + "));\n");
			}
			else
			{
				Code.push_back("		m_" + Decl.mDeclaredName + ".Read(Reader);\n");
			}
		}

		if (Rep.mSubType != nullptr && RemainingSize >= Rep.mSubType->mSize)
			Code.push_back("		Reader.ReadStruct(&m_internal_subTypeData[0], sizeof(m_internal_subTypeData));\n");

		Code.push_back("	}\n");
	}
	//Read processing - End

	//Write processing - Begin
	{
		Code.push_back("	virtual void Write(PhiveBinaryVectorWriter& Writer, unsigned int Offset) override {\n");
		
		Code.push_back("		unsigned int Position = Writer.GetPosition();\n");

		if (Rep.mParent != nullptr && !HasPrimitiveBase)
			Code.push_back("		" + GenerateClassName(*Rep.mParent, Depends) + "::Write(Writer, 0);\n");

		if (Rep.mAlignment > 0)
			Code.push_back("		Writer.Align(" + std::to_string(Rep.mAlignment) + ");\n");
		else if (HasPrimitiveBase)
			Code.push_back("		Writer.Align(" + std::to_string(Rep.mParent->mAlignment) + ");\n");

		if (HasPrimitiveBase)
			Code.push_back("		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_internal_primitiveBase), sizeof(m_internal_primitiveBase));\n");

		for (size_t i = 0; i < Rep.mDeclarations.size(); i++)
		{
			HavokClassRepresentation::Declaration& Decl = Rep.mDeclarations[i];
			if (Decl.mParent == nullptr)
				continue;

			if (Decl.mParent->mIsPrimitiveType)
			{
				if (Decl.mParent->mAlignment > 0)
					Code.push_back("		Writer.Align(" + std::to_string(Decl.mParent->mAlignment) + ");\n");
				Code.push_back("		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_" + Decl.mDeclaredName + "), sizeof(m_" + Decl.mDeclaredName + "));\n");
			}
			else
			{
				Code.push_back("		m_" + Decl.mDeclaredName + ".Write(Writer, Position + m_internal_size - Writer.GetPosition());\n");
			}
		}

		if (Rep.mSubType != nullptr && RemainingSize >= Rep.mSubType->mSize)
			Code.push_back("		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&m_internal_subTypeData[0]), sizeof(m_internal_subTypeData));\n");

		Code.push_back("	}\n");
	}
	//Write processing - End

	Code.push_back("};\n");

	std::sort(Depends.begin(), Depends.end());
	Depends.erase(std::unique(Depends.begin(), Depends.end()), Depends.end());

	for (auto Iter = Depends.begin(); Iter != Depends.end(); )
	{
		if (*Iter == "hkArray" || *Iter == "hkRefPtr")
		{
			Iter = Depends.erase(Iter);
			continue;
		}

		Iter++;
	}

	if(PreClass != "")
		Code.insert(Code.begin(), PreClass);

	mCode.insert({ Rep.mAddress, Code });
	mClassDependencies.insert({ Rep.mAddress, Depends });

	for (std::string& Depend : Depends)
	{
		GenerateClassCode(Depend);
	}
}

void PhiveClassGenerator::SortClassDependencies(std::string Addr, std::vector<std::pair<std::string, std::vector<std::string>>>& Sorted)
{
	for (const std::string& Dependency : mClassDependencies[Addr])
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
			Sorted.push_back({ Dependency, mCode[Dependency] });
	}

	bool Contains = false;
	for (auto& [Key, Val] : Sorted)
	{
		if (Key == Addr)
		{
			Contains = true;
			break;
		}
	}
	if (!Contains)
		Sorted.push_back({ Addr, mCode[Addr] });
}

void PhiveClassGenerator::Generate(std::string Path, std::vector<std::string> Goals)
{
	std::ifstream File(Path);
	json Data = json::parse(File);

	GenerateClassRepresentation(Data);

	for (const std::string& Goal : Goals)
	{
		GenerateClassCode(Goal);
	}

	std::vector<std::pair<std::string, std::vector<std::string>>> SortedClasses;

	for (std::string& Goal : Goals)
	{
		SortClassDependencies(Goal, SortedClasses);
	}

	for (auto& [Key, Val] : SortedClasses)
	{
		for (const std::string& Line : Val)
		{
			std::cout << Line;
		}
		std::cout << "\n\n";
	}

	File.close();
}