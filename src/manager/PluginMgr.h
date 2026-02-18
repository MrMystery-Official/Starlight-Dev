#pragma once

#include <vector>
#include <memory>

#include <plugin/PluginBase.h>

namespace application::manager
{
	namespace PluginMgr
	{
		extern std::vector<std::unique_ptr<application::plugin::PluginBase>> mPlugins;

		void Initialize();

		void RegisterPlugin(std::unique_ptr<application::plugin::PluginBase> Plugin);
		const std::vector<std::unique_ptr<application::plugin::PluginBase>>& GetPlugins();
		size_t GetPluginCount();
		application::plugin::PluginBase* GetPlugin(size_t index);
		application::plugin::PluginBase* FindPluginByName(const std::string& name);
		void ClearPlugins();
	}
}