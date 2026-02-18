#include "GitIdentityFile.h"

#include <util/FileUtil.h>
#include <util/GitManager.h>
#include <util/BinaryVectorReader.h>
#include <util/BinaryVectorWriter.h>
#include <util/Logger.h>

namespace application::file::tool
{
	void GitIdentityFile::Load(const std::string& Path)
	{
		if (!application::util::FileUtil::FileExists(Path))
			return;

		application::util::BinaryVectorReader Reader(application::util::FileUtil::ReadFile(Path));

		char Magic[5];
		Reader.ReadStruct(&Magic[0], 4); //EGIT
		Magic[4] = '\0';
		if (strcmp(Magic, "EGIT") != 0)
		{
			application::util::Logger::Error("GitIdentityFile", "File magic invalid, expected EGIT");
			return;
		}

		uint8_t Version = Reader.ReadUInt8();
		if (Version != 0x01)
		{
			application::util::Logger::Error("GitIdentityFile", "Version invalid, expected 0x01");
			return;
		}

		uint16_t NameSize = Reader.ReadUInt16();
		uint16_t EMailSize = Reader.ReadUInt16();
		uint16_t AccessTokenSize = Reader.ReadUInt16();

		application::util::GitManager::gUserIdentity.mName.resize(NameSize);
		application::util::GitManager::gUserIdentity.mEmail.resize(EMailSize);
		application::util::GitManager::gUserIdentity.mAccessToken.resize(AccessTokenSize);

		Reader.ReadStruct(application::util::GitManager::gUserIdentity.mName.data(), NameSize);
		Reader.ReadStruct(application::util::GitManager::gUserIdentity.mEmail.data(), EMailSize);
		Reader.ReadStruct(application::util::GitManager::gUserIdentity.mAccessToken.data(), AccessTokenSize);

		application::util::Logger::Info("GitIdentityFile", "Loaded git identity");
	}

	void GitIdentityFile::Save(const std::string& Path)
	{
		application::util::BinaryVectorWriter Writer;

		Writer.WriteBytes("EGIT"); //Magic
		Writer.WriteInteger(0x01, sizeof(uint8_t)); //Version

		Writer.WriteInteger(application::util::GitManager::gUserIdentity.mName.size(), sizeof(uint16_t));
		Writer.WriteInteger(application::util::GitManager::gUserIdentity.mEmail.size(), sizeof(uint16_t));
		Writer.WriteInteger(application::util::GitManager::gUserIdentity.mAccessToken.size(), sizeof(uint16_t));

		Writer.WriteBytes(application::util::GitManager::gUserIdentity.mName.c_str());
		Writer.WriteBytes(application::util::GitManager::gUserIdentity.mEmail.c_str());
		Writer.WriteBytes(application::util::GitManager::gUserIdentity.mAccessToken.c_str());

		application::util::FileUtil::WriteFile(Path, Writer.GetData());

		application::util::Logger::Info("GitIdentityFile", "Saved paths");
	}
}