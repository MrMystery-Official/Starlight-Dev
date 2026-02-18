#include <manager/PluginMgr.h>

#include <plugin/impl/PluginTerrainModificationUtil.h>
#include <plugin/impl/PluginFieldCleaner.h>

namespace application::manager
{
	std::vector<std::unique_ptr<application::plugin::PluginBase>> PluginMgr::mPlugins;

	void PluginMgr::Initialize()
	{
		RegisterPlugin(std::make_unique<application::plugin::impl::PluginTerrainModificationUtil>());
		RegisterPlugin(std::make_unique<application::plugin::impl::PluginFieldCleaner>());
	}

	void PluginMgr::RegisterPlugin(std::unique_ptr<application::plugin::PluginBase> Plugin)
	{
		if (Plugin)
		{
			mPlugins.push_back(std::move(Plugin));
		}
	}

	const std::vector<std::unique_ptr<application::plugin::PluginBase>>& PluginMgr::GetPlugins()
	{
		return mPlugins;
	}

	size_t PluginMgr::GetPluginCount()
	{
		return mPlugins.size();
	}

	application::plugin::PluginBase* PluginMgr::GetPlugin(size_t index)
	{
		if (index < mPlugins.size())
		{
			return mPlugins[index].get();
		}
		return nullptr;
	}

	application::plugin::PluginBase* PluginMgr::FindPluginByName(const std::string& name)
	{
		auto it = std::find_if(mPlugins.begin(), mPlugins.end(),
			[&name](const std::unique_ptr<application::plugin::PluginBase>& plugin) {
				return plugin->GetName() == name;
			});

		return (it != mPlugins.end()) ? it->get() : nullptr;
	}

	void PluginMgr::ClearPlugins()
	{
		mPlugins.clear();
	}
}