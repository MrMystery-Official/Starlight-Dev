#pragma once

#include <plugin/PluginBase.h>

namespace application::plugin::impl
{
	class PluginFieldCleaner : public application::plugin::PluginBase
	{
	public:
		PluginFieldCleaner() = default;
		virtual ~PluginFieldCleaner() = default;

		void DrawOptions() override;

		std::string GetName() const override;
		std::string GetAuthor() const override;
		std::string GetDescription() const override;
		uint32_t GetVersion() const override;

	private:
		std::string mFieldSceneName = "";
	};
}