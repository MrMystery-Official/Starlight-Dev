#pragma once

#include <rendering/UIWindowBase.h>
#include <manager/ActorInfoMgr.h>
#include <optional>
#include <map>
#include <file/game/sarc/SarcFile.h>
#include <file/game/byml/BymlFile.h>

namespace application::rendering::actor
{
	class UIActorTool : public UIWindowBase
	{
	public:
		static void InitializeGlobal();

		UIActorTool() = default;

		void InitializeImpl() override;
		void DrawImpl() override;
		void DeleteImpl() override;
		WindowType GetWindowType() override;
	private:
		struct ActorPackStruct
		{
			std::string mName = "";
			std::string mOriginalName = "";

			std::map<std::string, std::pair<application::file::game::byml::BymlFile, bool>> mBymls;
			std::map<std::string, std::pair<std::vector<unsigned char>, bool>> mAINBs;

			application::file::game::SarcFile mActorPack;
		};

		std::string GetWindowTitle() const;

		std::optional<std::string> mActorGyml = std::nullopt;
		std::optional<application::manager::ActorInfoMgr::ActorInfoEntry> mActorInfo = std::nullopt;
		std::optional<ActorPackStruct> mActor;

		void* mPopUpDataStructPtr = nullptr;
	};
}