#include "UIMSBTEditor.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include "Editor.h"
#include <filesystem>
#include "ZStdFile.h"
#include "Logger.h"

bool UIMSBTEditor::Open = true;
std::unordered_map<UIMSBTEditor::Language, MSBTFile> UIMSBTEditor::MSBT;
std::unordered_map<UIMSBTEditor::Language, SarcFile> UIMSBTEditor::LanguagePacks;
std::string UIMSBTEditor::PackFileFilter = "";
std::string UIMSBTEditor::EntryFileFilter = "";

std::unordered_map<UIMSBTEditor::Language, MSBTFile::StringEntry*> MSBTEntryPtr;

void UIMSBTEditor::Initialize()
{
	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	for (const auto& DirEntry : recursive_directory_iterator(Editor::GetRomFSFile("Mals", false)))
	{
		if (DirEntry.is_regular_file() && DirEntry.path().filename().string().ends_with(".sarc.zs"))
		{
			UIMSBTEditor::Language Lang;
			if (DirEntry.path().filename().string().starts_with("CNzh")) Lang = UIMSBTEditor::Language::CNzh;
			if (DirEntry.path().filename().string().starts_with("EUde")) Lang = UIMSBTEditor::Language::EUde;
			if (DirEntry.path().filename().string().starts_with("EUen")) Lang = UIMSBTEditor::Language::EUen;
			if (DirEntry.path().filename().string().starts_with("EUes")) Lang = UIMSBTEditor::Language::EUes;
			if (DirEntry.path().filename().string().starts_with("EUfr")) Lang = UIMSBTEditor::Language::EUfr;
			if (DirEntry.path().filename().string().starts_with("EUit")) Lang = UIMSBTEditor::Language::EUit;
			if (DirEntry.path().filename().string().starts_with("EUnl")) Lang = UIMSBTEditor::Language::EUnl;
			if (DirEntry.path().filename().string().starts_with("EUru")) Lang = UIMSBTEditor::Language::EUru;
			if (DirEntry.path().filename().string().starts_with("JPja")) Lang = UIMSBTEditor::Language::JPja;
			if (DirEntry.path().filename().string().starts_with("KRko")) Lang = UIMSBTEditor::Language::KRko;
			if (DirEntry.path().filename().string().starts_with("TWzh")) Lang = UIMSBTEditor::Language::TWzh;
			if (DirEntry.path().filename().string().starts_with("USen")) Lang = UIMSBTEditor::Language::USen;
			if (DirEntry.path().filename().string().starts_with("USes")) Lang = UIMSBTEditor::Language::USes;
			if (DirEntry.path().filename().string().starts_with("USfr")) Lang = UIMSBTEditor::Language::USfr;

			LanguagePacks.insert({ Lang, SarcFile(ZStdFile::Decompress(DirEntry.path().string(), ZStdFile::Dictionary::Base).Data )});
		}
	}

	Logger::Info("MSBTEditor", "Initialized");
}

void UIMSBTEditor::DrawMSBTEditorWindow()
{
	if (!Open) return;

	if (ImGui::Begin("MSBT Editor", &Open))
	{
		if (LanguagePacks.empty()) Initialize();

		ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.75f);
		ImGui::InputTextWithHint("##PackFileFilter", "Search...", &PackFileFilter);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Load##PackFile", ImVec2((ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x * 2 - ImGui::GetStyle().FramePadding.x * 2) * 0.25f, 0)))
		{
			MSBT.clear();
			for (uint8_t i = 0; i < (int)Language::_Count; i++)
			{
				for (SarcFile::Entry& Entry : LanguagePacks[(Language)i].GetEntries())
				{
					if (Entry.Name == PackFileFilter)
					{
						MSBTFile File(Entry.Bytes);
						File.FileName = Entry.Name;
						MSBT.insert({ (Language)i, File });
						break;
					}
				}
			}
		}

		ImGui::PushItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x);
		if (ImGui::BeginListBox("##PackFiles"))
		{
			for (SarcFile::Entry& Entry : LanguagePacks[Language::EUde].GetEntries())
			{
				if (PackFileFilter.length() > 0)
					if (Entry.Name.find(PackFileFilter) == std::string::npos)
						continue;

				if (ImGui::Selectable(Entry.Name.c_str()))
				{
					PackFileFilter = Entry.Name;
				}
			}
			ImGui::EndListBox();
		}
		ImGui::PopItemWidth();

		if (!MSBT.empty())
		{
			ImGui::NewLine();
			ImGui::PushItemWidth((ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.75f);
			ImGui::InputTextWithHint("##MSBTEntryFilter", "Search...", &EntryFileFilter);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			if (ImGui::Button("Load##MSBTEntry", ImVec2((ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x * 2 - ImGui::GetStyle().FramePadding.x * 2) * 0.25f, 0)))
			{
				MSBTEntryPtr.clear();
				for (uint8_t i = 0; i < (int)Language::_Count; i++)
				{
					for (auto& [Key, Val] : MSBT[(Language)i].Text)
					{
						if (Key == EntryFileFilter)
						{
							MSBTEntryPtr.insert({ (Language)i, &Val });
							break;
						}
					}
				}
			}
		

			ImGui::PushItemWidth(ImGui::GetWindowWidth() - ImGui::GetStyle().FramePadding.x);
			if (!MSBT.empty())
			{
				if (ImGui::BeginListBox("##MSBTEntries"))
				{
					for (auto& [Key, Val] : MSBT[Language::EUde].Text)
					{
						if (EntryFileFilter.length() > 0)
							if (Key.find(EntryFileFilter) == std::string::npos)
								continue;

						if (ImGui::Selectable(Key.c_str()))
						{
							EntryFileFilter = Key;
						}
					}
					ImGui::EndListBox();
				}
			}
			ImGui::PopItemWidth();
			ImGui::Separator();

			if (!MSBTEntryPtr.empty())
			{
				ImGui::NewLine();
				/*
				for (auto& [Key, Val] : MSBTEntryPtr)
				{
					ImGui::InputTextMultiline("EUde");
				}
				*/
				ImGui::InputTextMultiline("EUde", &MSBTEntryPtr[Language::EUde]->Text);
			}

			if (ImGui::Button("Save##MSBT"))
			{
				std::vector<unsigned char> MSBTBytes = MSBT[Language::EUde].ToBinary();
				LanguagePacks[Language::EUde].GetEntry(MSBT[Language::EUde].FileName).Bytes = MSBTBytes;
				ZStdFile::Compress(LanguagePacks[Language::EUde].ToBinary(), ZStdFile::Dictionary::Base).WriteToFile(Editor::GetWorkingDirFile("Save/Mals/EUde.Product." + Editor::GetInternalGameVersion() + ".sarc.zs"));
			}
		}

		ImGui::End();
	}
}