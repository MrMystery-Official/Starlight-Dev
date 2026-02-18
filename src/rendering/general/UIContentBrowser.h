#pragma once

#include <rendering/UIWindowBase.h>

namespace application::rendering::general
{
    class UIContentBrowser : public UIWindowBase
	{
    public:
        UIContentBrowser() = default;

        void InitializeImpl() override;
		void DrawImpl() override;
		void DeleteImpl() override;
        WindowType GetWindowType() override;
        bool SupportsProjectChange() override;
    };
} // namespace application::rendering::general
