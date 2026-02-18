#include "ImGuiExt.h"

#include "imgui_internal.h"
#include <manager/UIMgr.h>
#include <util/Logger.h>
#include <glm/glm.hpp>
#include <string>
#include <sstream>

application::gl::Texture* ImGuiExt::gAddButtonTexture = nullptr;
application::gl::Texture* ImGuiExt::gDeleteButtonTexture = nullptr;
std::unordered_map<std::string, bool> ImGuiExt::mInputFloatSliderCombos;

bool ImGuiExt::Initialize()
{
    gAddButtonTexture = application::manager::TextureMgr::GetAssetTexture("AddButton");
    gDeleteButtonTexture = application::manager::TextureMgr::GetAssetTexture("DeleteButton");

    return gAddButtonTexture && gDeleteButtonTexture;
}

void ImGuiExt::DrawButtonArray(const char* Names[], uint8_t NameSize, uint8_t MaxColumnCount, int* Index, const std::optional<std::function<void()>>& OnButtonPress)
{
    const uint8_t RowCount = std::ceil((float)NameSize / (float)MaxColumnCount);
    int ButtonIndex = 0;

    for (uint8_t i = 0; i < RowCount; i++)
    {
        const uint8_t Count = std::min(MaxColumnCount, (uint8_t)NameSize);
        const float ButtonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * (Count - 1)) / Count;
        for (uint8_t j = 0; j < Count; j++)
        {
            bool Selected = *Index == ButtonIndex;

            if (Selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);

            if (ImGui::Button(Names[ButtonIndex], ImVec2(ButtonWidth, 0)))
            {
                *Index = ButtonIndex;
                if (OnButtonPress.has_value()) OnButtonPress.value()();
            }

            if (Selected)
                ImGui::PopStyleColor();

            if (j != Count - 1)
            {
                ImGui::SameLine();
            }

            ButtonIndex++;
        }

        NameSize -= Count;
    }
}

bool ImGuiExt::InputFloat3Colored(const char* Label, float Values[3], ImVec4 ColorA, ImVec4 ColorB, ImVec4 ColorC, bool CopyPastePopUp)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    void* Data = Values;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    ImGui::BeginGroup();
    ImGui::PushID(Label);
    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    size_t type_size = sizeof(float);
    for (int i = 0; i < 3; i++)
    {
        ImGui::PushID(i);
        if (i > 0)
            ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);

        switch (i)
        {
        case 0:
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorA);
            break;
        }
        case 1:
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorB);
            break;
        }
        case 2:
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorC);
            break;
        }
        default:
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ColorA);
            break;
        }
        }

        value_changed |= ImGui::InputScalar("", ImGuiDataType_Float, Data, NULL, NULL, "%.3f", 0);
        ImGui::PopID();
        ImGui::PopItemWidth();
        ImGui::PopStyleColor();
        Data = (void*)((char*)Data + type_size);
    }
    ImGui::PopID();

    const char* label_end = ImGui::FindRenderedTextEnd(Label);
    if (Label != label_end)
    {
        ImGui::SameLine(0.0f, g.Style.ItemInnerSpacing.x);
        ImGui::TextEx(Label, label_end);
        if (CopyPastePopUp)
            CheckVec3fInputRightClick(std::string(Label), Values);
    }

    ImGui::EndGroup();
    return value_changed;
}

void ImGuiExt::CheckVec3fInputRightClick(const std::string& Id, float Values[3], bool ConvertAngles)
{
    ImGui::OpenPopupOnItemClick(("CopyPasteVec3f" + Id).c_str(), ImGuiPopupFlags_MouseButtonRight);

    if (ImGui::BeginPopup(("CopyPasteVec3f" + Id).c_str()))
    {
        if (ImGui::MenuItem("Copy"))
        {
            const std::string Clipboard = std::to_string(Values[0]) + "," + std::to_string(Values[1]) + "," + std::to_string(Values[2]);
            glfwSetClipboardString(application::manager::UIMgr::gWindow, Clipboard.c_str());
        }

        if(ConvertAngles)
        {
            if (ImGui::MenuItem("Copy (to degrees)"))
            {
                const std::string Clipboard = std::to_string(glm::degrees(Values[0])) + "," + std::to_string(glm::degrees(Values[1])) + "," + std::to_string(glm::degrees(Values[2]));
                glfwSetClipboardString(application::manager::UIMgr::gWindow, Clipboard.c_str());
            }
            if (ImGui::MenuItem("Copy (to radians)"))
            {
                const std::string Clipboard = std::to_string(glm::radians(Values[0])) + "," + std::to_string(glm::radians(Values[1])) + "," + std::to_string(glm::radians(Values[2]));
                glfwSetClipboardString(application::manager::UIMgr::gWindow, Clipboard.c_str());
            }
        }
        if (ImGui::MenuItem("Paste"))
        {
            std::vector<std::string> Result;
            Result.reserve(3);

            std::stringstream Data(glfwGetClipboardString(application::manager::UIMgr::gWindow));
            std::string Line;
            while (std::getline(Data, Line, ','))
            {
                Result.push_back(Line);
            }

            if (Result.size() != 3)
            {
                application::util::Logger::Error("ImGuiExt", "Can't import vector data from clipboard, wrong length");
                goto EndPopUp;
            }

            Values[0] = std::stof(Result[0]);
            Values[1] = std::stof(Result[1]);
            Values[2] = std::stof(Result[2]);
        }
    EndPopUp:
        ImGui::EndPopup();
    }
}

bool ImGuiExt::SelectableTextOffset(const char* label, float offsetX, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // Submit label or explicit size to ItemSize(), whereas ItemAdd() will submit a larger/spanning rectangle.
    ImGuiID id = window->GetID(label);
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
    ImVec2 pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    ImGui::ItemSize(size, 0.0f);

    // Fill horizontal space
    // We don't support (size < 0.0f) in Selectable() because the ItemSpacing extension would make explicitly right-aligned sizes not visibly match other widgets.
    const bool span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
    const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
    const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
    if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth))
        size.x = ImMax(label_size.x, max_x - min_x);

    // Text stays at the submission position, but bounding box may be extended on both sides
    ImVec2 text_min = pos;
    ImVec2 text_max(min_x + size.x, pos.y + size.y);

    // Selectables are meant to be tightly packed together with no click-gap, so we extend their box to cover spacing between selectable.
    // FIXME: Not part of layout so not included in clipper calculation, but ItemSize currently doesn't allow offsetting CursorPos.
    ImRect bb(min_x, pos.y, text_max.x, text_max.y);
    if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
    {
        const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
        const float spacing_y = style.ItemSpacing.y;
        const float spacing_L = IM_TRUNC(spacing_x * 0.50f);
        const float spacing_U = IM_TRUNC(spacing_y * 0.50f);
        bb.Min.x -= spacing_L;
        bb.Min.y -= spacing_U;
        bb.Max.x += (spacing_x - spacing_L);
        bb.Max.y += (spacing_y - spacing_U);
    }
    //if (g.IO.KeyCtrl) { GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(0, 255, 0, 255)); }

    const bool disabled_item = (flags & ImGuiSelectableFlags_Disabled) != 0;
    const ImGuiItemFlags extra_item_flags = disabled_item ? (ImGuiItemFlags)ImGuiItemFlags_Disabled : ImGuiItemFlags_None;
    bool is_visible;
    if (span_all_columns)
    {
        // Modify ClipRect for the ItemAdd(), faster than doing a PushColumnsBackground/PushTableBackgroundChannel for every Selectable..
        const float backup_clip_rect_min_x = window->ClipRect.Min.x;
        const float backup_clip_rect_max_x = window->ClipRect.Max.x;
        window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
        window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
        is_visible = ImGui::ItemAdd(bb, id, NULL, extra_item_flags);
        window->ClipRect.Min.x = backup_clip_rect_min_x;
        window->ClipRect.Max.x = backup_clip_rect_max_x;
    }
    else
    {
        is_visible = ImGui::ItemAdd(bb, id, NULL, extra_item_flags);
    }

    const bool is_multi_select = (g.LastItemData.ItemFlags & ImGuiItemFlags_IsMultiSelect) != 0;
    if (!is_visible)
        if (!is_multi_select || !g.BoxSelectState.UnclipMode || !g.BoxSelectState.UnclipRect.Overlaps(bb)) // Extra layer of "no logic clip" for box-select support (would be more overhead to add to ItemAdd)
            return false;

    const bool disabled_global = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    if (disabled_item && !disabled_global) // Only testing this as an optimization
        ImGui::BeginDisabled();

    // FIXME: We can standardize the behavior of those two, we could also keep the fast path of override ClipRect + full push on render only,
    // which would be advantageous since most selectable are not selected.
    if (span_all_columns)
    {
        if (g.CurrentTable)
            ImGui::TablePushBackgroundChannel();
        else if (window->DC.CurrentColumns)
            ImGui::PushColumnsBackground();
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasClipRect;
        g.LastItemData.ClipRect = window->ClipRect;
    }

    // We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
    ImGuiButtonFlags button_flags = 0;
    if (flags & ImGuiSelectableFlags_NoHoldingActiveID) { button_flags |= ImGuiButtonFlags_NoHoldingActiveId; }
    if (flags & ImGuiSelectableFlags_NoSetKeyOwner) { button_flags |= ImGuiButtonFlags_NoSetKeyOwner; }
    if (flags & ImGuiSelectableFlags_SelectOnClick) { button_flags |= ImGuiButtonFlags_PressedOnClick; }
    if (flags & ImGuiSelectableFlags_SelectOnRelease) { button_flags |= ImGuiButtonFlags_PressedOnRelease; }
    if (flags & ImGuiSelectableFlags_AllowDoubleClick) { button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick; }
    if ((flags & ImGuiSelectableFlags_AllowOverlap) || (g.LastItemData.ItemFlags & ImGuiItemFlags_AllowOverlap)) { button_flags |= ImGuiButtonFlags_AllowOverlap; }

    // Multi-selection support (header)
    const bool was_selected = selected;
    if (is_multi_select)
    {
        // Handle multi-select + alter button flags for it
        ImGui::MultiSelectItemHeader(id, &selected, &button_flags);
    }

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, button_flags);

    // Multi-selection support (footer)
    if (is_multi_select)
    {
        ImGui::MultiSelectItemFooter(id, &selected, &pressed);
    }
    else
    {
        // Auto-select when moved into
        // - This will be more fully fleshed in the range-select branch
        // - This is not exposed as it won't nicely work with some user side handling of shift/control
        // - We cannot do 'if (g.NavJustMovedToId != id) { selected = false; pressed = was_selected; }' for two reasons
        //   - (1) it would require focus scope to be set, need exposing PushFocusScope() or equivalent (e.g. BeginSelection() calling PushFocusScope())
        //   - (2) usage will fail with clipped items
        //   The multi-select API aim to fix those issues, e.g. may be replaced with a BeginSelection() API.
        if ((flags & ImGuiSelectableFlags_SelectOnNav) && g.NavJustMovedToId != 0 && g.NavJustMovedToFocusScopeId == g.CurrentFocusScopeId)
            if (g.NavJustMovedToId == id)
                selected = pressed = true;
    }

    // Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with keyboard/gamepad
    if (pressed || (hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover)))
    {
        if (!g.NavHighlightItemUnderNav && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
        {
            ImGui::SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, ImGui::WindowRectAbsToRel(window, bb)); // (bb == NavRect)
            if (g.IO.ConfigNavCursorVisibleAuto)
                g.NavCursorVisible = false;
        }
    }
    if (pressed)
        ImGui::MarkItemEdited(id);

    if (selected != was_selected)
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

    // Render
    if (is_visible)
    {
        const bool highlighted = hovered || (flags & ImGuiSelectableFlags_Highlight);
        if (highlighted || selected)
        {
            // Between 1.91.0 and 1.91.4 we made selected Selectable use an arbitrary lerp between _Header and _HeaderHovered. Removed that now. (#8106)
            ImU32 col = ImGui::GetColorU32((held && highlighted) ? ImGuiCol_HeaderActive : highlighted ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
            ImGui::RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
        }
        if (g.NavId == id)
        {
            ImGuiNavRenderCursorFlags nav_render_cursor_flags = ImGuiNavRenderCursorFlags_Compact | ImGuiNavRenderCursorFlags_NoRounding;
            if (is_multi_select)
                nav_render_cursor_flags |= ImGuiNavRenderCursorFlags_AlwaysDraw; // Always show the nav rectangle
            ImGui::RenderNavCursor(bb, id, nav_render_cursor_flags);
        }
    }

    if (span_all_columns)
    {
        if (g.CurrentTable)
            ImGui::TablePopBackgroundChannel();
        else if (window->DC.CurrentColumns)
            ImGui::PopColumnsBackground();
    }

    if (is_visible)
    {
        text_min.x += offsetX;
        text_max.x += offsetX;
        ImGui::RenderTextClipped(text_min, text_max, label, NULL, &label_size, style.SelectableTextAlign, &bb);
    }

    // Automatically close popups
    if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_NoAutoClosePopups) && (g.LastItemData.ItemFlags & ImGuiItemFlags_AutoClosePopups))
        ImGui::CloseCurrentPopup();

    if (disabled_item && !disabled_global)
        ImGui::EndDisabled();

    // Selectable() always returns a pressed state!
    // Users of BeginMultiSelect()/EndMultiSelect() scope: you may call ImGui::IsItemToggledSelection() to retrieve
    // selection toggle, only useful if you need that state updated (e.g. for rendering purpose) before reaching EndMultiSelect().
    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed; //-V1020
}

bool ImGuiExt::SelectableTextOffsetWithDeleteButton(const char* label, float offsetX, std::function<void()> onDeletePressed, bool selected, ImGuiSelectableFlags flags, const ImVec2& size)
{
    ImVec2 Pos = ImGui::GetCursorPos();
    ImGui::SetNextItemAllowOverlap();
    bool Result = SelectableTextOffset(label, offsetX, selected, flags, size);
    ImVec2 Size = ImGui::GetItemRectSize();
    ImVec2 ReturnPos = ImGui::GetCursorPos();

    float SizeHeight = Size.y - ImGui::GetStyle().FramePadding.y * 2.0f;

    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorPosY(Pos.y - ImGui::GetStyle().ItemSpacing.y * 0.5f);
    ImGui::SetCursorPosX(ImGui::GetCurrentWindow()->ParentWorkRect.Max.x - SizeHeight - ImGui::GetStyle().ItemInnerSpacing.x * 2 - ImGui::GetStyle().ItemSpacing.x);

    if(ImGui::ImageButton(("-##" + std::string(label)).c_str(), (ImTextureID)gDeleteButtonTexture->mID, ImVec2(SizeHeight, SizeHeight)))
    {
        onDeletePressed();
        ImGui::SetCursorPos(ReturnPos);
        return false;
    }

    ImGui::SetCursorPos(ReturnPos);
    return Result;
}

bool ImGuiExt::InputScalarNWidth(const char* label, ImGuiDataType data_type, void* p_data, int components, float ItemWidth, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    ImGui::BeginGroup();
    ImGui::PushID(label);
    ImGui::PushMultiItemsWidths(components, ItemWidth);
    size_t type_size = ImGui::DataTypeGetInfo(data_type)->Size;
    for (int i = 0; i < components; i++)
    {
        ImGui::PushID(i);
        if (i > 0)
            ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= ImGui::InputScalar("", data_type, p_data, p_step, p_step_fast, format, flags);
        ImGui::PopID();
        ImGui::PopItemWidth();
        p_data = (void*)((char*)p_data + type_size);
    }
    ImGui::PopID();

    const char* label_end = ImGui::FindRenderedTextEnd(label);
    if (label != label_end)
    {
        ImGui::SameLine(0.0f, g.Style.ItemInnerSpacing.x);
        ImGui::TextEx(label, label_end);
    }

    ImGui::EndGroup();
    return value_changed;
}

bool ImGuiExt::CollapsingHeaderWithAddButton(const char* label, ImGuiTreeNodeFlags flags, std::function<void()> onButtonPressed)
{
    ImVec2 Pos = ImGui::GetCursorPos();
    ImGui::SetNextItemAllowOverlap();
    bool Open = ImGui::CollapsingHeader(label, flags);
    ImGui::SetCursorPosY(Pos.y);

    const float Height = ImGui::GetCurrentContext()->FontSize + ImGui::GetStyle().FramePadding.y * 2;
    float Size = Height - ImGui::GetStyle().FramePadding.y * 2.0f;

    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorPosX(ImGui::GetItemRectSize().x - ImGui::GetStyle().ItemInnerSpacing.x * 2 - ImGui::GetStyle().ItemSpacing.x * 2);
    
    /*
    if(ImGui::Button(("+##" + std::string(label)).c_str()))
    {
        onButtonPressed();
        ImGui::GetStateStorage()->SetInt(ImGui::GetID(label), 1);
        return true;
    }
    */

   if(ImGui::ImageButton(("+##" + std::string(label)).c_str(), (ImTextureID)gAddButtonTexture->mID, ImVec2(Size, Size), ImVec2(0, 0)))
   {
        onButtonPressed();
        ImGui::GetStateStorage()->SetInt(ImGui::GetID(label), 1);
        return true;
   }

    return Open;
}

bool ImGuiExt::ButtonDelete(const char* Label, const ImVec2& Size)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0.14, 0.14, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.91, 0.12, 0.15, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0, 0, 1));
    bool Changed = ImGui::Button(Label, Size);
    ImGui::PopStyleColor(3);
    return Changed;
}

bool ImGuiExt::InputFloatSliderCombo(const char* label, const std::string& id, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    bool ValueChanged = false;
    ImGui::PushID(id.c_str());

    if (mInputFloatSliderCombos[id])
    {
        ImGui::SetKeyboardFocusHere();
        ValueChanged = ImGui::InputFloat(("##Field" + id).c_str(), v, 0.0f, 0.0f, format, ImGuiInputTextFlags_None | flags);


        if (ImGui::IsItemDeactivated())
        {
            mInputFloatSliderCombos[id] = false;
        }
    }
    else
    {
        ValueChanged |= ImGui::SliderFloat(("##Slider" + std::string(id)).c_str(), v, v_min, v_max, "%.3f");

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            mInputFloatSliderCombos[id] = true;
        }
    }

    ImGui::SameLine();
    ImGui::Text("%s", label);
    ImGui::PopID();

    return ValueChanged;
}

bool ImGuiExt::InputIntSliderCombo(const char* label, const std::string& id, int* v, int v_min, int v_max)
{
    bool ValueChanged = false;
    ImGui::PushID(id.c_str());

    if (mInputFloatSliderCombos[id])
    {
        ImGui::SetKeyboardFocusHere();
        ValueChanged = ImGui::InputInt(("##Field" + id).c_str(), v, v_min, v_max, ImGuiInputTextFlags_None);

        if (ImGui::IsItemDeactivated())
        {
            mInputFloatSliderCombos[id] = false;
        }
    }
    else
    {
        ValueChanged |= ImGui::SliderInt(("##Slider" + std::string(id)).c_str(), v, v_min, v_max);

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            mInputFloatSliderCombos[id] = true;
        }
    }

    ImGui::SameLine();
    ImGui::Text("%s", label);
    ImGui::PopID();

    return ValueChanged;
}