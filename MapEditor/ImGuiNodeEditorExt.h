#pragma once
#include <imgui.h>

namespace ax {
    namespace Drawing {

        enum class IconType : ImU32 {
            Flow,
            Circle,
            Square,
            Grid,
            RoundSquare,
            Diamond
        };

        void DrawIcon(ImDrawList* drawList, const ImVec2& a, const ImVec2& b, IconType type, bool filled, ImU32 color, ImU32 innerColor);

    } // namespace Drawing

    namespace Widgets {

        using Drawing::IconType;

        void Icon(const ImVec2& size, IconType type, bool filled, const ImVec4& color = ImVec4(1, 1, 1, 1), const ImVec4& innerColor = ImVec4(0, 0, 0, 0));

    } // namespace Widgets
} // namespace ax