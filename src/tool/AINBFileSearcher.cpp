#include <tool/AINBFileSearcher.h>

#include <util/Logger.h>
#include <filesystem>
#include <util/FileUtil.h>
#include <file/game/sarc/SarcFile.h>
#include <file/game/zstd/ZStdBackend.h>

namespace application::tool
{
	void AINBFileSearcher::Start(const std::function<bool(application::file::game::ainb::AINBFile& File)>& Func)
	{
        using directory_iterator = std::filesystem::directory_iterator;
        for (const auto& DirEntry : directory_iterator(application::util::FileUtil::GetRomFSFilePath("Logic", false)))
        {
            if (DirEntry.is_regular_file())
            {
                std::ifstream File(DirEntry.path().string(), std::ios::binary);

                if (!File.eof() && !File.fail())
                {
                    File.seekg(0, std::ios_base::end);
                    std::streampos FileSize = File.tellg();

                    std::vector<unsigned char> Bytes(FileSize);

                    File.seekg(0, std::ios_base::beg);
                    File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

                    File.close();

                    application::file::game::ainb::AINBFile AINB;
                    AINB.Initialize(Bytes, true);

                    std::string Path = "Logic/" + DirEntry.path().filename().string();

                    if (Func(AINB))
                    {
                        application::util::Logger::Info("AINBFileSearcher", "AINB matches requirement: %s", Path.c_str());
                    }
                }
                else
                {
                    application::util::Logger::Error("AINBNodeMgr", "Could not open file \"%s\"", DirEntry.path().string().c_str());
                }
            }
        }

        for (const auto& DirEntry : directory_iterator(application::util::FileUtil::GetRomFSFilePath("AI", false)))
        {
            if (DirEntry.is_regular_file())
            {
                std::ifstream File(DirEntry.path().string(), std::ios::binary);

                if (!File.eof() && !File.fail())
                {
                    File.seekg(0, std::ios_base::end);
                    std::streampos FileSize = File.tellg();

                    std::vector<unsigned char> Bytes(FileSize);

                    File.seekg(0, std::ios_base::beg);
                    File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

                    File.close();

                    application::file::game::ainb::AINBFile AINB;
                    AINB.Initialize(Bytes, true);

                    std::string Path = "AI/" + DirEntry.path().filename().string();

                    if (Func(AINB))
                    {
                        application::util::Logger::Info("AINBFileSearcher", "AINB matches requirement: %s", Path.c_str());
                    }
                }
                else
                {
                    application::util::Logger::Error("AINBNodeMgr", "Could not open file \"%s\"", DirEntry.path().string().c_str());
                }
            }
        }

        for (const auto& DirEntry : directory_iterator(application::util::FileUtil::GetRomFSFilePath("Sequence", false)))
        {
            if (DirEntry.is_regular_file())
            {
                std::ifstream File(DirEntry.path().string(), std::ios::binary);

                if (!File.eof() && !File.fail())
                {
                    File.seekg(0, std::ios_base::end);
                    std::streampos FileSize = File.tellg();

                    std::vector<unsigned char> Bytes(FileSize);

                    File.seekg(0, std::ios_base::beg);
                    File.read(reinterpret_cast<char*>(Bytes.data()), FileSize);

                    File.close();

                    application::file::game::ainb::AINBFile AINB;
                    AINB.Initialize(Bytes, true);

                    std::string Path = "Sequence/" + DirEntry.path().filename().string();

                    if (Func(AINB))
                    {
                        application::util::Logger::Info("AINBFileSearcher", "AINB matches requirement: %s", Path.c_str());
                    }
                }
                else
                {
                    application::util::Logger::Error("AINBNodeMgr", "Could not open file \"%s\"", DirEntry.path().string().c_str());
                }
            }
        }

        for (const auto& DirEntry : directory_iterator(application::util::FileUtil::GetRomFSFilePath("Pack/Actor", false)))
        {
            if (DirEntry.is_regular_file())
            {
                if (DirEntry.path().string().ends_with(".pack.zs"))
                {
                    std::vector<unsigned char> Data = application::file::game::ZStdBackend::Decompress(DirEntry.path().string());
                    if (!Data.empty())
                    {
                        application::file::game::SarcFile File(Data);
                        for (application::file::game::SarcFile::Entry& Entry : File.GetEntries())
                        {
                            if (Entry.mBytes[0] == 'A' && Entry.mBytes[1] == 'I' && Entry.mBytes[2] == 'B')
                            {
                                application::file::game::ainb::AINBFile AINB;
                                AINB.Initialize(Entry.mBytes, true);

                                std::string Path = "Pack/Actor/" + DirEntry.path().filename().string() + "/" + Entry.mName;

                                if (Func(AINB))
                                {
                                    application::util::Logger::Info("AINBFileSearcher", "AINB matches requirement: %s", Path.c_str());
                                }
                            }
                        }
                    }
                }
            }
        }
	}
}