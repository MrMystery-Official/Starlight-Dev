#pragma once

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <cstdint>
#include "json.hpp"

using json = nlohmann::json;

namespace PhiveClassGenerator
{
	enum class HavokClassKind : uint8_t
	{
		NONE = 0,
		ARRAY = 1,
		RECORD = 2,
		FLOAT = 3,
		INT = 4,
		STRING = 5,
		POINTER = 6,
		BOOL = 7
	};

	struct HavokClassRepresentation
	{
		struct Declaration
		{
			std::string mDeclaredName;
			HavokClassRepresentation* mParent = nullptr;
		};

		struct Template
		{
			std::string mName;
			HavokClassRepresentation* mType = nullptr;
			int32_t mValue = -1;
		};

		std::string mName;
		std::string mAddress;
		int32_t mSize;
		int32_t mAlignment;
		HavokClassKind mKind;

		HavokClassRepresentation* mParent = nullptr;
		HavokClassRepresentation* mSubType = nullptr;
		
		std::vector<Declaration> mDeclarations;
		std::vector<Template> mTemplates;

		bool mIsPrimitiveType = false;
	};

	extern std::unordered_map<std::string, HavokClassRepresentation> mClasses;
	extern std::unordered_map<std::string, std::vector<std::string>> mCode;
	extern std::unordered_map<std::string, std::vector<std::string>> mClassDependencies;

	HavokClassKind StringToKind(const std::string& Name);
	std::string GenerateClassName(HavokClassRepresentation& Rep, std::vector<std::string>& Depends, bool Field = true, std::string* Code = nullptr);
	void GenerateClassRepresentation(json& Data);
	void GenerateClassCode(const std::string& Address);
	void SortClassDependencies(std::string Addr, std::vector<std::pair<std::string, std::vector<std::string>>>& Sorted);
	void Generate(std::string Path, std::vector<std::string> Goals);
}