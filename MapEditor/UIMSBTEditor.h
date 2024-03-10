#pragma once

#include "MSBT.h"
#include <unordered_map>
#include "SARC.h"

namespace UIMSBTEditor
{
	enum class Language : uint8_t
	{
		CNzh = 0,
		EUde = 1,
		EUen = 2,
		EUes = 3,
		EUfr = 4,
		EUit = 5,
		EUnl = 6,
		EUru = 7,
		JPja = 8,
		KRko = 9,
		TWzh = 10,
		USen = 11,
		USes = 12,
		USfr = 13,
		_Count = 14
	};

	extern bool Open;
	extern std::unordered_map<Language, MSBTFile> MSBT;
	extern std::unordered_map<Language, SarcFile> LanguagePacks;
	extern std::string PackFileFilter;
	extern std::string EntryFileFilter;

	void Initialize();
	void DrawMSBTEditorWindow();
};