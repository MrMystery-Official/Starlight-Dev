#pragma once

#include <string>
#include <unordered_set>

namespace application::util
{
	namespace General
	{
		void ReplaceString(std::string& Str, const std::string& From, const std::string& To);

		template <typename T>
		void UnionWith(std::vector<T>& Destination, const std::vector<T>& Source) {
			std::unordered_set<T> UniqueElements(Destination.begin(), Destination.end());
			
			for (const auto& Element : Source) 
			{
				if (UniqueElements.insert(Element).second) 
				{
					Destination.push_back(Element);
				}
			}
		}
	}
}