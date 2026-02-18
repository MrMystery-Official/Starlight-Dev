#include "PhiveClassGenerator.h"

#include <fstream>
#include <util/Logger.h>
#include <util/General.h>

namespace application::forever_confidential
{
	std::optional<json> PhiveClassGenerator::gJsonData;
	std::unordered_set<std::string> PhiveClassGenerator::gSkipClasses = { "hkEnum", "hkFlags", "hkRelPtr", "hkRelArray", "hkRelArrayView", "hkBaseObject", "hkVector4f", "hkInt64", "hknpShapeType__Enum", "hkUlong", "hkReferencedObject", "hkUint8", "hknpCollisionDispatchType__Enum", "hkUint16", "hknpShape__FlagsEnum", "hkReal", "hkUint64", "hknpShapePropertyBase", "hknpShapeProperties__Entry", "hknpShapeProperties", "hknpShape", "hkUint32", "hknpCompositeShape", "hkVector4", "hknpMeshShapeVertexConversionUtil", "hknpTransposedFourAabbs8", "hknpAabb8TreeNode", "hknpMeshShape__GeometrySection__Primitive", "hkFloat3", "hkInt16", "hknpMeshShape__GeometrySection", "hknpMeshShape__ShapeTagTableEntry", "hkcdFourAabb", "hkcdSimdTree__Node", "hkcdSimdTree", "hknpMeshShapePrimitiveMapping", "hknpMeshShape", "hkInt32", "hkaiIndex", "hkaiNavMesh__Face", "T*", "hkArray", "hkaiPackedKey_", "hkHalf16", "hkaiNavMesh__EdgeFlagBits", "hkaiNavMesh__Edge", "hkaiUpVector", "hkaiAnnotatedStreamingSet__Side", "hkaiStreamingSet__VolumeConnection", "hkaiStreamingSet__ClusterGraphConnection", "hkPair", "hkHashMapDetail__Index", "hkHashBase", "hkHashMap", "hkaiFaceEdgeIndexPair", "hkaiStreamingSet__NavMeshConnection", "hkAabb", "hkaiStreamingSet", "hkRefPtr", "hkaiAnnotatedStreamingSet", "hkaiNavMeshFaceIterator", "hkcdCompressedAabbCodecs__AabbCodecBase", "hkcdCompressedAabbCodecs__CompressedAabbCodec", "hkcdCompressedAabbCodecs__Aabb6BytesCodec", "hkcdStaticTree__AabbTreeBase", "hkcdStaticTree__AabbTree", "hkcdStaticTree__Aabb6BytesTree", "hkcdStaticAabbTree__Impl", "hkcdStaticAabbTree", "hkaiNavMeshStaticTreeFaceIterator", "hkaiNavMeshClearanceCache__McpDataInteger", "hkaiNavMeshClearanceCache__EdgeDataInteger", "hkaiNavMeshClearanceCache", "hkaiNavMeshClearanceCacheSeeding__CacheData", "hkaiNavMeshClearanceCacheSeeding__CacheDataSet", "hkaiNavMesh", "char*", "hkStringPtr", "hkRefVariant", "hkRootLevelContainer__NamedVariant", "hkRootLevelContainer" };
	std::unordered_set<std::string> PhiveClassGenerator::gWriteLaterClasses = { "hkRelPtr", "hkRelArray", "hkRelArrayView" };

	void PhiveClassGenerator::Initialize(const std::string& HavokClassDumpJsonPath)
	{
		if (gJsonData.has_value())
			return;

		std::fstream File(HavokClassDumpJsonPath);
		gJsonData = json::parse(File);
		File.close();
	}

	json* PhiveClassGenerator::GetClassByAddress(const std::string& Address)
	{
		if (!gJsonData.has_value())
			return nullptr;

		for (auto& Class : gJsonData.value()) 
		{
			if (Class.contains("Address") && Class["Address"] == Address)
			{
				return &Class;
			}
		}

		application::util::Logger::Error("PhiveClassGenerator", "Class with address %s not found", Address.c_str());
		return nullptr;
	}

	json* PhiveClassGenerator::GetClassByName(std::string Name)
	{
		application::util::General::ReplaceString(Name, "__", "::"); //Re-transform

		if (!gJsonData.has_value())
			return nullptr;

		for (auto& Class : gJsonData.value())
		{
			if (Class.contains("Name") && Class["Name"] == Name)
			{
				return &Class;
			}
		}

		application::util::Logger::Error("PhiveClassGenerator", "Class with re-transformed name %s not found", Name.c_str());
		return nullptr;
	}

	std::optional<PhiveClassGenerator::HavokClass> PhiveClassGenerator::GetDecodedClass(json* Class)
	{
		if (!Class)
			return std::nullopt;

		if (!Class->contains("Name"))
			return std::nullopt;

		HavokClass DecodedClass;
		DecodedClass.mName = (*Class)["Name"];
		DecodedClass.mKind = Class->contains("Kind") ? (*Class)["Kind"] : "Record";

		application::util::General::ReplaceString(DecodedClass.mName, "::", "__");

		if (Class->contains("ParentAddress"))
		{
			DecodedClass.mParentAddress = (*Class)["ParentAddress"];
		}

		if (Class->contains("Alignment"))
		{
			DecodedClass.mAlignment = (*Class)["Alignment"].get<int>();
		}

		if (Class->contains("Size"))
		{
			DecodedClass.mSize = (*Class)["Size"].get<int>();
		}

		DecodedClass.mIsWriteLater = gWriteLaterClasses.contains(DecodedClass.mName);

		if (DecodedClass.mParentAddress.has_value())
		{
			HavokClass::HavokField Field;
			Field.mDataType = GetFieldDataType(GetDecodedClass(GetClassByAddress(DecodedClass.mParentAddress.value())).value(), Field);
			Field.mName = "mParent";
			Field.mParentClassName = (*Class)["Parent"];
			Field.mParentClassAddress = (*Class)["ParentAddress"];

			Field.mIsPrimitive = IsFieldPrimitive(Field.mDataType);
			DecodedClass.mFields.push_back(Field);
		}

		if (Class->contains("Template"))
		{
			for (auto& Template : (*Class)["Template"])
			{
				HavokClass::HavokTemplate HTemplate;
				HTemplate.mAddress = Template.contains("Address") ? Template["Address"] : "";
				HTemplate.mKey = Template["Name"];
				HTemplate.mValue = Template.contains("Type") ? Template["Type"].get<std::string>() : (Template["Value"].is_number() ? std::to_string(Template["Value"].get<int>()) : Template["Value"].get<std::string>());

				application::util::General::ReplaceString(HTemplate.mValue, "::", "__");

				DecodedClass.mTemplates.push_back(HTemplate);
			}
		}

		if (Class->contains("Declarations"))
		{
			for (auto& Declaration : (*Class)["Declarations"])
			{
				HavokClass::HavokField Field;
				Field.mDataType = GetFieldDataType(GetDecodedClass(GetClassByAddress(Declaration["ParentAddress"])).value(), Field);

				Field.mName = Declaration["DeclaredName"];
				Field.mName[0] = std::toupper(Field.mName[0]);
				Field.mName = "m" + Field.mName;

				Field.mParentClassName = Declaration["Parent"];
				Field.mParentClassAddress = Declaration["ParentAddress"];


				Field.mIsPrimitive = IsFieldPrimitive(Field.mDataType);
				DecodedClass.mFields.push_back(Field);
			}
		}

		if (Class->contains("Attributes") && DecodedClass.mKind == "Int")
		{
			for (auto& Attribute : (*Class)["Attributes"])
			{
				for (auto& Item : Attribute["Items"])
				{
					HavokClass::HavokAttribute HAttribute;
					HAttribute.mName = Item["Name"];
					HAttribute.mValue = Item["Value"];

					DecodedClass.mAttributes.push_back(HAttribute);
				}
			}
		}

		return DecodedClass;
	}

	std::string PhiveClassGenerator::GetFieldDataType(const HavokClass& Class, HavokClass::HavokField& Field)
	{
		std::string Type = Class.mName;

		if (Type == "T[N]" && Class.mTemplates.size() == 2)
		{
			Field.mIsRawArray = true;
			Field.mRawArraySize = std::stoi(Class.mTemplates[1].mValue);
			return Class.mTemplates[0].mValue;
		}

		if (!Class.mTemplates.empty())
		{
			Type += "<";

			for (size_t i = 0; i < Class.mTemplates.size(); i++)
			{
				Type += Class.mTemplates[i].mValue;
				if (i < Class.mTemplates.size() - 1) Type += ", ";
			}

			Type += ">";
		}

		Field.mIsWriteLater = Class.mIsWriteLater;

		return Type;
	}

	bool PhiveClassGenerator::IsFieldPrimitive(const std::string& DataType)
	{
		return DataType == "void" || DataType == "int" || DataType == "unsigned int" || DataType == "bool" || DataType == "long" || DataType == "long long" || DataType == "float" || DataType == "unsigned short" || DataType == "unsigned char" || DataType == "unsigned long" || DataType == "unsigned long long" || DataType == "short";
	}

	void PhiveClassGenerator::GeneratePhiveClass(const std::string& Address, std::unordered_map<std::string, std::unordered_set<std::string>>& Depends, std::unordered_map<std::string, HavokClass>& Classes)
	{
		if (!gJsonData.has_value())
			return;

		const json& Json = gJsonData.value();

		json* Class = GetClassByAddress(Address);
		if (!Class)
			return;

		std::optional<HavokClass> DecodedClass = GetDecodedClass(Class);
		if (!DecodedClass.has_value())
			return;

		if (Depends.contains(DecodedClass.value().mName))
			return;

		DecodedClass.value().mAddress = Address;

		for (HavokClass::HavokField& Field : DecodedClass.value().mFields)
		{
			Depends[DecodedClass.value().mName].insert(Field.mParentClassAddress);

			HavokClass ParentClass = GetDecodedClass(GetClassByAddress(Field.mParentClassAddress)).value();

			for (HavokClass::HavokTemplate& Template : ParentClass.mTemplates)
			{
				if (Template.mKey == "tAllocator" || IsFieldPrimitive(Template.mValue))
					continue;

				json* TemplateJson = !Template.mAddress.empty() ? GetClassByAddress(Template.mAddress) : GetClassByName(Template.mValue);

				if (!TemplateJson)
					continue;

				if (!TemplateJson->contains("Address"))
					continue;

				Depends[DecodedClass.value().mName].insert((*TemplateJson)["Address"].get<std::string>());
			}
		}

		Classes[DecodedClass.value().mName] = DecodedClass.value();

		for (HavokClass::HavokField& Field : DecodedClass.value().mFields)
		{
			if (Field.mIsPrimitive)
				continue;

			GeneratePhiveClass(Field.mParentClassAddress, Depends, Classes);

			HavokClass ParentClass = GetDecodedClass(GetClassByAddress(Field.mParentClassAddress)).value();

			for (HavokClass::HavokTemplate& Template : ParentClass.mTemplates)
			{
				if (Template.mKey == "tAllocator" || IsFieldPrimitive(Template.mValue))
					continue;

				json* JsonTemplateClass = !Template.mAddress.empty() ? GetClassByAddress(Template.mAddress) : GetClassByName(Template.mValue);
				if (!JsonTemplateClass)
					continue;

				GeneratePhiveClass((*JsonTemplateClass)["Address"], Depends, Classes);
			}
		}
	}

	void PhiveClassGenerator::SortClassDependencies(const std::string& Name, std::unordered_map<std::string, HavokClass>& Classes, std::unordered_map<std::string, std::unordered_set<std::string>>& Depends, std::vector<HavokClass>& Sorted)
	{
		for (const std::string& Dependency : Depends[Name])
		{
			std::string DependencyName = "";
			for (auto& [Name, Class] : Classes)
			{
				if (Class.mAddress == Dependency)
				{
					DependencyName = Class.mName;
					break;
				}
			}

			SortClassDependencies(DependencyName, Classes, Depends, Sorted);

			bool Contains = false;
			for (HavokClass& Class : Sorted)
			{
				if (Class.mName == DependencyName)
				{
					Contains = true;
					break;
				}
			}

			if (!Contains)
				Sorted.push_back(Classes[DependencyName]);
		}

		std::string DependencyName = "";
		for (auto& [Key, Class] : Classes)
		{
			if (Class.mAddress == Name)
			{
				DependencyName = Class.mName;
				break;
			}
		}

		bool Contains = false;
		for (HavokClass& Class : Sorted)
		{
			if (Class.mName == DependencyName)
			{
				Contains = true;
				break;
			}
		}
		if (!Contains)
			Sorted.push_back(Classes[DependencyName]);
	}

	void PhiveClassGenerator::GeneratePhiveClasses(const std::string& Root)
	{
		if (!gJsonData.has_value())
			return;

		std::unordered_map<std::string, std::unordered_set<std::string>> Depends;
		std::unordered_map<std::string, HavokClass> Classes;

		GeneratePhiveClass(Root, Depends, Classes);

		HavokClass* RootClass = nullptr;

		for (auto& [Key, Class] : Classes)
		{
			if (Class.mAddress == Root)
			{
				RootClass = &Class;
				break;
			}
		}

		if (!RootClass)
		{
			application::util::Logger::Error("PhiveClassGenerator", "Couldn't find root class ptr");
			return;
		}

		std::vector<HavokClass> Sorted;
		SortClassDependencies(RootClass->mName, Classes, Depends, Sorted);

		Sorted.push_back(*RootClass);

		for (HavokClass& Class : Sorted)
		{
			if (Class.mName.empty() || Class.mName == "T[N]")
				continue;

			if (gSkipClasses.contains(Class.mName))
				continue;

			if (!(Class.mKind == "Int" && Class.mFields.empty()))
			{
				if (!Class.mTemplates.empty())
				{
					std::cout << "template<";

					for (size_t i = 0; i < Class.mTemplates.size(); i++)
					{
						std::cout << "typename " << Class.mTemplates[i].mKey;

						if (i != Class.mTemplates.size() - 1) std::cout << ", ";
					}

					std::cout << ">\n";
				}

				std::cout << "struct " << Class.mName << " : public hkReadableWriteableObject\n";
				std::cout << "{\n";

				for (HavokClass::HavokField& Field : Class.mFields)
				{
					if (Field.mIsRawArray)
					{
						std::cout << "	" << Field.mDataType << " " << Field.mName << "[" << Field.mRawArraySize <<"];\n";
						continue;
					}

					std::cout << "	" << Field.mDataType << " " << Field.mName << ";\n";
				}

				std::cout << "\n	virtual void Read(application::file::game::phive::classes::HavokClassBinaryVectorReader& Reader) override \n	{\n";
				if (Class.mAlignment > 0)
				{
					std::cout << "		Reader.Align(" << Class.mAlignment << ");\n";
				}
				if (Class.mSize > 0)
				{
					std::cout << "		unsigned int Jumpback = Reader.GetPosition();\n";
				}
				for (HavokClass::HavokField& Field : Class.mFields)
				{
					if (Field.mIsRawArray)
					{
						std::cout << "		Reader.ReadRawArray(&" << Field.mName << ", sizeof(" << Field.mName << "), " << Field.mRawArraySize << ", " << (Field.mIsPrimitive ? "true" : "false") << ");\n";
					}
					else if (Field.mIsPrimitive)
					{
						std::cout << "		Reader.ReadStruct(&" << Field.mName << ", sizeof(" << Field.mName << "));\n";
					}
					else
					{
						std::cout << "		" << Field.mName << ".Read(Reader);\n";
					}
				}
				if (Class.mSize > 0)
				{
					std::cout << "		Reader.Seek(Jumpback + " << Class.mSize << ", application::file::game::phive::classes::HavokClassBinaryVectorReader::Position::Begin);\n";
				}
				std::cout << "	}\n";




				std::cout << "\n	virtual void Skip(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override \n	{\n";
				if (Class.mAlignment > 0)
				{
					std::cout << "		Writer.Align(" << Class.mAlignment << ");\n";
				}
				if (Class.mSize > 0)
				{
					std::cout << "		unsigned int Jumpback = Writer.GetPosition();\n";
				}
				std::cout << "		if(mIsItem) Writer.WriteItemCallback(this);\n";
				for (HavokClass::HavokField& Field : Class.mFields)
				{
					if (Field.mIsRawArray)
					{
						std::cout << "		Writer.WriteRawArray(&" << Field.mName << ", sizeof(" << Field.mName << "), " << Field.mRawArraySize << ", " << (Field.mIsPrimitive ? "true" : "false") << ");\n";
					}
					else if (Field.mIsPrimitive)
					{
						std::cout << "		Writer.WriteRawUnsafeFixed(reinterpret_cast<const char*>(&" << Field.mName << "), sizeof(" << Field.mName << "));\n";
					}
					else
					{
						std::cout << "		" << Field.mName << ".Skip(Writer);\n";
					}
				}
				if (Class.mSize > 0)
				{
					std::cout << "		Writer.Seek(Jumpback + " << Class.mSize << ", application::file::game::phive::classes::HavokClassBinaryVectorWriter::Position::Begin);\n";
				}
				std::cout << "	}\n";






				std::cout << "\n	virtual void Write(application::file::game::phive::classes::HavokClassBinaryVectorWriter& Writer) override \n	{\n";
				for (HavokClass::HavokField& Field : Class.mFields)
				{
					if (Field.mIsPrimitive)
						continue;

					if (!Field.mIsRawArray)
					{
						std::cout << "		" << Field.mName << ".Write(Writer);\n";
					}
					else
					{
						std::cout << "		//" << Field.mName << ".Write(Writer);\n";
					}
				}
				std::cout << "	}\n";



				std::string RetransformedName = Class.mName;
				application::util::General::ReplaceString(RetransformedName, "__", "::");

				std::cout << "\n	virtual std::string GetHavokClassName() override \n	{\n";
				std::cout << "		return \"" << RetransformedName << "\";\n";
				std::cout << "	}\n";

				if (Class.mAlignment > 1)
				{
					std::cout << "\n	virtual unsigned int GetHavokClassAlignment() override \n	{\n";
					std::cout << "		return " << Class.mAlignment << ";\n";
					std::cout << "	}\n";
				}





				std::cout << "};\n";
			}
			else
			{
				std::cout << "enum class " << Class.mName << " : int\n";
				std::cout << "{\n";
				for (size_t i = 0; i < Class.mAttributes.size(); i++)
				{
					std::cout << "	" << Class.mAttributes[i].mName << " = " << Class.mAttributes[i].mValue;
					
					if (i != Class.mAttributes.size() - 1) std::cout << ",";

					std::cout << "\n";
				}

				std::cout << "};\n";
			}

			std::cout << "\n\n\n";
		}

		for (HavokClass& Class : Sorted)
		{
			if (Class.mName.empty() || Class.mName == "T[N]")
				continue;

			if (gSkipClasses.contains(Class.mName))
				continue;

			std::cout << "\"" << Class.mName << "\", ";
		}
		std::cout << std::endl;
	}
}