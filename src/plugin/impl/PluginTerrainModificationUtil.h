#pragma once

#include <plugin/PluginBase.h>

namespace application::plugin::impl
{
	class PluginTerrainModificationUtil : public application::plugin::PluginBase
	{
	public:
		PluginTerrainModificationUtil() = default;
		virtual ~PluginTerrainModificationUtil() = default;

		void DrawOptions() override;

		std::string GetName() const override;
		std::string GetAuthor() const override;
		std::string GetDescription() const override;
		uint32_t GetVersion() const override;

	private:
		bool mModifyMaterialMaps = false;
		int mCurrentTextureLayer = 0;
		std::string mFieldSceneName = "";

		bool mPurgeGrass = false;
	};
}