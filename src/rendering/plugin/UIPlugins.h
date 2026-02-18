#pragma once

#include <rendering/UIWindowBase.h>

namespace application::rendering::plugin
{
    class UIPlugins : public UIWindowBase
    {
    public:
        UIPlugins() = default;

        void InitializeImpl() override;
        void DrawImpl() override;
        void DeleteImpl() override;
        WindowType GetWindowType() override;
        bool SupportsProjectChange() override;

    private:
        std::string GetWindowTitle() const;

        int mSelectedPluginIndex = -1;
    };
} // namespace application::rendering::plugin
