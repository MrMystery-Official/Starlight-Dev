#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace application::forever_confidential
{
	namespace PhiveClassGenerator
	{
		struct HavokClass
		{
			struct HavokField
			{
				bool mIsPrimitive = false;
				bool mIsRawArray = false;
				bool mIsWriteLater = false;
				unsigned int mRawArraySize = 0;

				std::string mDataType = "";
				std::string mName = "";

				std::string mParentClassName = "";
				std::string mParentClassAddress = "";
			};

			struct HavokTemplate
			{
				std::string mKey;
				std::string mValue;
				std::string mAddress;
			};

			struct HavokAttribute
			{
				std::string mName;
				int mValue = 0;
			};

			std::string mName;
			std::string mAddress;
			std::string mKind;
			unsigned int mAlignment = 0;
			unsigned int mSize = 0;
			bool mIsWriteLater = false;

			std::optional<std::string> mParentAddress;

			std::vector<HavokField> mFields;
			std::vector<HavokTemplate> mTemplates;
			std::vector<HavokAttribute> mAttributes;
		};

		extern std::optional<json> gJsonData;
		extern std::unordered_set<std::string> gSkipClasses;
		extern std::unordered_set<std::string> gWriteLaterClasses;

		void Initialize(const std::string& HavokClassDumpJsonPath);
		
		json* GetClassByAddress(const std::string& Address);
		json* GetClassByName(std::string Name);
		std::optional<HavokClass> GetDecodedClass(json* Class);
		std::string GetFieldDataType(const HavokClass& Class, HavokClass::HavokField& Field);
		bool IsFieldPrimitive(const std::string& DataType);
		void GeneratePhiveClass(const std::string& Address, std::unordered_map<std::string, std::unordered_set<std::string>>& Depends, std::unordered_map<std::string, HavokClass>& Classes);
		void SortClassDependencies(const std::string& Name, std::unordered_map<std::string, HavokClass>& Classes, std::unordered_map<std::string, std::unordered_set<std::string>>& Depends, std::vector<HavokClass>& Sorted);
		void GeneratePhiveClasses(const std::string& Root);
	}
}