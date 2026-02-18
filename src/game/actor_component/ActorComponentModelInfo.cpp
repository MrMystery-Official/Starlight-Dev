#include "ActorComponentModelInfo.h"

#include <file/game/byml/BymlFile.h>
#include <imgui.h>
#include <imgui_stdlib.h>

namespace application::game::actor_component
{
	ActorComponentModelInfo::ActorComponentModelInfo(application::game::ActorPack& ActorPack) : mActorPack(&ActorPack)
	{
		std::string ModelInfoPath = "";
		for (application::file::game::SarcFile::Entry& Entry : ActorPack.mPack.GetEntries())
		{
			if (Entry.mName.rfind("Component/ModelInfo/", 0) == 0)
			{
				ModelInfoPath = Entry.mName;
			}
		}

		application::file::game::byml::BymlFile ModelInfoByml(ActorPack.mPack.GetEntry(ModelInfoPath).mBytes);
		if (ModelInfoByml.GetNodes().empty())
		{
			return;
		}

		if (ModelInfoByml.HasChild("ModelProjectName")) mModelProjectName = ModelInfoByml.GetNode("ModelProjectName")->GetValue<std::string>();
		if (ModelInfoByml.HasChild("FmdbName")) mFmdbName = ModelInfoByml.GetNode("FmdbName")->GetValue<std::string>();
		if (ModelInfoByml.HasChild("EnableModelBake")) mEnableModelBake = ModelInfoByml.GetNode("EnableModelBake")->GetValue<bool>();
	}

	const std::string ActorComponentModelInfo::GetDisplayNameImpl()
	{
		return "ModelInfo";
	}

	const std::string ActorComponentModelInfo::GetInternalNameImpl()
	{
		return "engine__component__ModelInfo";
	}

	bool ActorComponentModelInfo::ContainsComponent(application::game::ActorPack& Pack)
	{
		for (application::file::game::SarcFile::Entry& Entry : Pack.mPack.GetEntries())
		{
			if (Entry.mName.rfind("Component/ModelInfo/", 0) == 0)
			{
				return true;
			}
		}

		return false;
	}

	void ActorComponentModelInfo::DrawEditingMenu()
	{
		if(mModelProjectName.has_value()) ImGui::InputText("ModelProjectName", &mModelProjectName.value());
		if(mFmdbName.has_value()) ImGui::InputText("FmdbName", &mFmdbName.value());
		if(mEnableModelBake.has_value()) ImGui::Checkbox("Enable Model Bake", &mEnableModelBake.value());
	}
}