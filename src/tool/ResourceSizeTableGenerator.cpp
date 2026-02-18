#include "ResourceSizeTableGenerator.h"

#include <util/FileUtil.h>
#include <file/game/zstd/ZStdBackend.h>
#include <file/game/restbl/ResTblFile.h>
#include <file/game/sarc/SarcFile.h>
#include <filesystem>
#include <util/General.h>
#include <Editor.h>
#include <map>
#include <cmath>

namespace application::tool
{
    std::unordered_map<std::string, uint32_t> ResourceSizeTableGenerator::gResourceSizes =
    {
        { ".ainb", 392 },
        { ".asb", 552 },
        { ".baatarc", 256 },
        { ".baev", 288 },
        { ".bagst", 256 },
        { ".bars", 576 },
        { ".bcul", 256 },
        { ".beco", 256 },
        { ".belnk", 256 },
        { ".bfarc", 256 },
        { ".bfevfl", 288 },
        { ".bfsha", 256 },
        { ".bhtmp", 256 },
        { ".blal", 256 },
        { ".blarc", 256 },
        { ".blwp", 256 },
        { ".bnsh", 256 },
        { ".bntx", 256 },
        { ".bphcl", 256 },
        { ".bphhb", 256 },
        { ".bphnm", 288 },
        { ".bphsh", 368 },
        { ".bslnk", 256 },
        { ".bstar", 288 },
        { ".cai", 256 },
        { ".chunk", 256 },
        { ".crbin", 256 },
        { ".cutinfo", 256 },
        { ".dpi", 256 },
        { ".genvb", 384 },
        { ".jpg", 256 },
        { ".pack", 384 },
        { ".png", 256 },
        { ".quad", 256 },
        { ".sarc", 384 },
        { ".tscb", 256 },
        { ".txtg", 256 },
        { ".vsts", 256 },
        { ".wbr", 256 }
    };

    std::vector<std::string> ResourceSizeTableGenerator::gShaderArchives = 
    {
        "Lib/agl/agl_resource.Nin_NX_NVN.release.sarc",
        "Lib/gsys/gsys_resource.Nin_NX_NVN.release.sarc",
        "Lib/Terrain/tera_resource.Nin_NX_NVN.release.sarc",
        "Shader/ApplicationPackage.Nin_NX_NVN.release.sarc" 
    };

	uint32_t ResourceSizeTableGenerator::CalcSize(const std::string& FilePath)
	{
        uint32_t Size = 0;
        std::filesystem::path FSPath(FilePath);
        std::string Ext = FSPath.extension().string();

        std::vector<unsigned char> Data;

        if ((Ext == ".zs" || Ext == ".zstd") && !FilePath.ends_with(".ta.zs"))
        {
            Data = application::util::FileUtil::ReadFile(application::util::FileUtil::GetRomFSFilePath(FilePath));
            Size = application::file::game::ZStdBackend::GetDecompressedSize(Data);
        }
        else if (Ext == ".mc")
        {
            Size = std::filesystem::file_size(application::util::FileUtil::GetRomFSFilePath(FilePath)) * 5;
        }
        else
        {
            Size = std::filesystem::file_size(application::util::FileUtil::GetRomFSFilePath(FilePath));
        }

        if (Ext == ".bstar" || Ext == ".ainb" || Ext == ".asb")
        {
            Data = application::util::FileUtil::ReadFile(application::util::FileUtil::GetRomFSFilePath(FilePath));
        }

        Size = Size - Size % 0x20 + ((Size % 0x20) > 0 ? 0x20 : 0x00);

        bool IsShaderArchiv = false;
        for (const std::string& Path : gShaderArchives)
        {
            if (FilePath.find(Path) != std::string::npos)
            {
                IsShaderArchiv = true;
                break;
            }
        }
        if (IsShaderArchiv)
        {
            Size += 4096;
        }
        else if (FilePath.find(".ta.zs") != std::string::npos || FilePath.find(".bcett.byml") != std::string::npos || (FilePath.find(".byml") != std::string::npos && FilePath.find(".casset.byml") == std::string::npos))
        {
            Size += 256;
        }
        else if (gResourceSizes.contains(Ext))
        {
            Size += gResourceSizes[Ext];
            if (Ext == ".bstar")
            {
                uint32_t Count = *reinterpret_cast<uint32_t*>(&Data[0x8]);
                Size += Count * 8;
            }
            if (Ext == ".ainb")
            {
                uint32_t HasEXB = *reinterpret_cast<uint32_t*>(&Data[0x44]);
                if (HasEXB > 0)
                {
                    uint32_t SigOffset = *reinterpret_cast<uint32_t*>(&Data[HasEXB + 0x20]);
                    uint32_t Count = *reinterpret_cast<uint32_t*>(&Data[HasEXB + SigOffset]);
                    Size += (16 + 8 * ceil(Count / 2));
                }
            }
            if (Ext == ".asb")
            {
                uint32_t Count = *reinterpret_cast<uint32_t*>(&Data[0x10]);
                Size += Count * 40;
                uint32_t HasEXB = *reinterpret_cast<uint32_t*>(&Data[0x60]);
                if (HasEXB > 0)
                {
                    uint32_t SigOffset = *reinterpret_cast<uint32_t*>(&Data[HasEXB + 0x20]);
                    Count = *reinterpret_cast<uint32_t*>(&Data[HasEXB + SigOffset]);
                    Size += (16 + 8 * ceil(Count / 2));
                }
            }
            if (FilePath.find("Event/EventFlow/Dm_ED_0004.bfevfl") != std::string::npos)
            {
                Size += 192;
            }
            if (Ext == ".bphsh")
            {
                Size = Size * 4;
            }
        }
        else if (Ext == ".bgyml" || Ext == ".byml" || Ext == ".byaml")
        {
            Size = (Size + 1000) * 8;
        }
        else
        {
            Size = (Size + 1500) * 4;
        }
        
        return Size;
	}

	void ResourceSizeTableGenerator::Generate()
	{
        bool Write = false;

        application::file::game::restbl::ResTableFile ResTable(application::file::game::ZStdBackend::Decompress(application::util::FileUtil::GetRomFSFilePath("System/Resource/ResourceSizeTable.Product." + application::Editor::gInternalGameVersion + ".rsizetable.zs")));

        for (const auto& DirEntry : std::filesystem::recursive_directory_iterator(application::util::FileUtil::GetSaveFilePath()))
        {
            if (DirEntry.is_regular_file())
            {
                if (DirEntry.path().string().ends_with(".sarc.zs"))
                    continue;

                if (DirEntry.path().string().ends_with(".pack.zs"))
                {
                    application::file::game::SarcFile Sarc(application::file::game::ZStdBackend::Decompress(DirEntry.path().string()));
                    for (application::file::game::SarcFile::Entry& Entry : Sarc.GetEntries())
                    {
                        if (Entry.mBytes[0] == 'A' && Entry.mBytes[1] == 'I' && Entry.mBytes[2] == 'B')
                        {
                            uint32_t Size = (floor((Entry.mBytes.size() + 0x1F) / 0x20)) * 0x20 + 392;
                            uint32_t HasEXB = *reinterpret_cast<uint32_t*>(&Entry.mBytes[0x44]);
                            if (HasEXB > 0)
                            {
                                uint32_t SigOffset = *reinterpret_cast<uint32_t*>(&Entry.mBytes[HasEXB + 0x20]);
                                uint32_t Count = *reinterpret_cast<uint32_t*>(&Entry.mBytes[HasEXB + SigOffset]);
                                Size += (16 + 8 * ceil(Count / 2));
                            }

                            ResTable.SetFileSize(Entry.mName, Size);
                            continue;
                        }
                        if ((Entry.mBytes[0] == 'Y' && Entry.mBytes[1] == 'B') || (Entry.mBytes[0] == 'B' && Entry.mBytes[1] == 'Y'))
                        {
                            uint32_t Size = Entry.mBytes.size();
                            Size = (floor((Size + 0x1F) / 0x20)) * 0x20 + 256;
                            Size = (Size + 1000) * 8;
                            ResTable.SetFileSize(Entry.mName, Size);
                            continue;
                        }
                    }
                    continue;
                }

                std::string LocalPath = DirEntry.path().string().substr(application::util::FileUtil::GetSaveFilePath().length() + 1, DirEntry.path().string().length() - (DirEntry.path().string().ends_with(".zs") && !DirEntry.path().string().ends_with(".ta.zs") ? 4 : 1) - application::util::FileUtil::GetSaveFilePath().length());
                std::string RomFSPath = DirEntry.path().string().substr(application::util::FileUtil::GetSaveFilePath().length() + 1, DirEntry.path().string().length() - 1 - application::util::FileUtil::GetSaveFilePath().length());
                std::replace(LocalPath.begin(), LocalPath.end(), '\\', '/');
                ResTable.SetFileSize(LocalPath, CalcSize(RomFSPath));
                Write = true;
            }
        }


        std::filesystem::create_directories(application::util::FileUtil::GetSaveFilePath("System/Resource"));
        application::util::FileUtil::WriteFile(application::util::FileUtil::GetSaveFilePath("System/Resource/ResourceSizeTable.Product." + application::Editor::gInternalGameVersion + ".rsizetable.zs"), application::file::game::ZStdBackend::Compress(ResTable.ToBinary(), application::file::game::ZStdBackend::Dictionary::None));
	}
}