#pragma once

#include <cstdint>
#include <string>

namespace application::plugin
{
	class PluginBase
	{
	public:
		PluginBase() = default;
		virtual ~PluginBase() = default;

		virtual void DrawOptions() = 0;

		virtual std::string GetName() const = 0;
		virtual std::string GetAuthor() const = 0;
		virtual std::string GetDescription() const = 0;
		virtual uint32_t GetVersion() const = 0;
	};
}