#include "LicenseFile.h"

#include <util/Logger.h>
#include <util/Math.h>
#include <util/HardwareInfo.h>
#include <util/FileUtil.h>
#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>

namespace application::file::tool
{
	bool LicenseFile::gNoLicenseFileFound = true;
	std::string LicenseFile::gLicenseSeed = ""; //Set by UIMgr

	bool LicenseFile::Load(const std::string& Path)
	{
		if (!application::util::FileUtil::FileExists(Path))
			return false;

		application::util::BinaryVectorReader Reader(application::util::FileUtil::ReadFile(Path));

		char Magic[9];
		Reader.ReadStruct(&Magic[0], 8); //ELICENSE
		Magic[8] = '\0';

		if (strcmp(Magic, "ELICENSE") != 0)
		{
			application::util::Logger::Error("LicenseFile", "File magic invalid, expected ELICENSE");
			return false;
		}

		uint8_t Version = Reader.ReadUInt8();
		if (Version != 0x01)
		{
			application::util::Logger::Error("LicenseFile", "File version invalid, expected 0x01");
			return false;
		}

		uint64_t HardwareIdHash = Reader.ReadUInt64();
		uint64_t Hash = application::util::Math::StringHashMurMur3(application::util::HardwareInfo::GetUniqueHardwareID());

		if (Hash != HardwareIdHash)
		{
			application::util::Logger::Error("LicenseFile", "Hardware ID mismatch found. This is an unlicensed installation of Starlight. Aborting");
			return false;
		}

		return true;
	}

	void LicenseFile::Save(const std::string& Path)
	{
		uint64_t Hash = application::util::Math::StringHashMurMur3(application::util::HardwareInfo::GetUniqueHardwareID());
		if (Hash == 0)
			return;

		application::util::BinaryVectorWriter Writer;

		Writer.WriteBytes("ELICENSE"); //Magic
		Writer.WriteInteger(0x01, sizeof(uint8_t)); //Version

		Writer.WriteInteger(Hash, sizeof(uint64_t));

		application::util::FileUtil::WriteFile(Path, Writer.GetData());

		application::util::Logger::Info("LicenseFile", "Successfully generated LicenseFile");
	}
}