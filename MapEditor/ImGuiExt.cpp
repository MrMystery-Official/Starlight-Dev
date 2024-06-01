#include "ImGuiExt.h"
#include "imgui_internal.h"

namespace {

static const ImGuiDataTypeInfo GDataTypeInfo[] = {
    { sizeof(char), "S8", "%d", "%d" }, // ImGuiDataType_S8
    { sizeof(unsigned char), "U8", "%u", "%u" },
    { sizeof(short), "S16", "%d", "%d" }, // ImGuiDataType_S16
    { sizeof(unsigned short), "U16", "%u", "%u" },
    { sizeof(int), "S32", "%d", "%d" }, // ImGuiDataType_S32
    { sizeof(unsigned int), "U32", "%u", "%u" },
#ifdef _MSC_VER
    { sizeof(ImS64), "S64", "%I64d", "%I64d" }, // ImGuiDataType_S64
    { sizeof(ImU64), "U64", "%I64u", "%I64u" },
#else
    { sizeof(ImS64), "S64", "%lld", "%lld" }, // ImGuiDataType_S64
    { sizeof(ImU64), "U64", "%llu", "%llu" },
#endif
    { sizeof(float), "float", "%.3f", "%f" }, // ImGuiDataType_Float (float are promoted to double in va_arg)
    { sizeof(double), "double", "%f", "%lf" }, // ImGuiDataType_Double
};
IM_STATIC_ASSERT(IM_ARRAYSIZE(GDataTypeInfo) == ImGuiDataType_COUNT);

}

bool ImGui::InputScalarNWidth(const char* label, ImGuiDataType data_type, void* p_data, int components, float ItemWidth, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, ItemWidth);
    size_t type_size = GDataTypeInfo[data_type].Size;
    for (int i = 0; i < components; i++) {
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        value_changed |= InputScalar("", data_type, p_data, p_step, p_step_fast, format, flags);
        PopID();
        PopItemWidth();
        p_data = (void*)((char*)p_data + type_size);
    }
    PopID();

    const char* label_end = FindRenderedTextEnd(label);
    if (label != label_end) {
        SameLine(0.0f, g.Style.ItemInnerSpacing.x);
        TextEx(label, label_end);
    }

    EndGroup();
    return value_changed;
}
