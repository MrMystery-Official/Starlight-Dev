#include "UIMgr.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"
#include <rendering/ImGuizmo.h>
#include <cassert>
#include <util/Logger.h>
#include <manager/PopUpMgr.h>
#include <manager/TextureMgr.h>
#include <manager/BfresRendererMgr.h>
#include <manager/FramebufferMgr.h>
#include <manager/ShaderMgr.h>
#include <manager/ProjectMgr.h>
#include <util/FileUtil.h>
#include <util/Math.h>
#include <rendering/ainb/UIAINBEditor.h>
#include <rendering/collision/UICollisionGenerator.h>
#include <rendering/mapeditor/UIMapEditor.h>
#include <rendering/actor/UIActorTool.h>
#include <rendering/plugin/UIPlugins.h>
#include <Editor.h>
#include <file/tool/PathConfigFile.h>
#include <file/tool/LicenseFile.h>
#include <util/IconsFontAwesome6.h>
#include <util/fa-solid-900.h>
#include <util/ImGuiNotify.h>

#define TOOL_IMGUI_VIEWPORTS_ENABLED 0
#define TOOL_GL_DEBUG 0

namespace application::manager
{
	GLFWwindow* UIMgr::gWindow = nullptr;
    const ImVec4 UIMgr::gClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    std::vector<std::unique_ptr<application::rendering::UIWindowBase>> UIMgr::gWindows;
    std::vector<std::unique_ptr<application::rendering::UIWindowBase>> UIMgr::gWaitingWindows;
    unsigned int UIMgr::gWindowId = 0;
    bool UIMgr::gFirstFrame = true;
    bool UIMgr::gASTCSupported = false;
    ImGuiID UIMgr::gDockMain;
	ImGuiID UIMgr::gDockBottom;
    application::rendering::popup::PopUpBuilder UIMgr::gSettingsPopUp;
    application::rendering::popup::PopUpBuilder UIMgr::gAddProjectPopUp;
    application::rendering::popup::PopUpBuilder UIMgr::gExportProjectPopUp;
    application::rendering::popup::PopUpBuilder UIMgr::gEnterLicenseKeyPopUp;
    application::rendering::popup::PopUpBuilder UIMgr::gGenerateLicenseKeyPopUp;
    bool UIMgr::gBlockProjectSwitch = false;

    void APIENTRY glDebugOutput(GLenum source,
        GLenum type,
        unsigned int id,
        GLenum severity,
        GLsizei length,
        const char* message,
        const void* userParam)
    {
        // ignore non-significant error/warning codes
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

        std::cout << "---------------" << std::endl;
        std::cout << "Debug message (" << id << "): " << message << std::endl;

        switch (source)
        {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
        } std::cout << std::endl;

        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
        } std::cout << std::endl;

        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
        } std::cout << std::endl;
        std::cout << std::endl;
    }

	void UIMgr::GLFWErrorCallback(int error, const char* description)
	{
        application::util::Logger::Error("GLFW", "Code: %i, Description: %s", error, description);
	}

	bool UIMgr::Initialize()
	{
		assert(gWindow == nullptr && "UI was already initialized");

		glfwSetErrorCallback(GLFWErrorCallback);
		if (!glfwInit())
		{
            application::util::Logger::Error("UIMgr", "Could not initialize GLFW");
			return false;
		}

#ifdef __APPLE__
        const char* glsl_version = "#version 150";
#else
        const char* glsl_version = "#version 130";
#endif

        {

            const int maxMajor = 4;
            const int maxMinor = 5;

            // Try versions from highest to lowest
            for (int major = maxMajor; major >= 3 && gWindow == nullptr; major--)
            {
                int minorStart = (major == maxMajor) ? maxMinor : 9; // If trying max major, start with max minor

                for (int minor = minorStart; minor >= 0 && gWindow == nullptr; minor--)
                {
                    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
                    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
                    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
                    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
                    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE); // Change this
#endif

#if TOOL_GL_DEBUG == 1
                    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

                    // Try to create window with these settings
                    gWindow = glfwCreateWindow(1280, 720, "Starlight", NULL, NULL);
                }
            }
        }

        // Create window with graphics context
        if (gWindow == nullptr)
        {
            application::util::Logger::Error("UIMgr", "Could not create window");
            return false;
        }
        glfwMakeContextCurrent(gWindow);

        gladLoadGL();

        glfwSwapInterval(1); // Enable vsync

        int ActualMajor, ActualMinor;
        glGetIntegerv(GL_MAJOR_VERSION, &ActualMajor);
        glGetIntegerv(GL_MINOR_VERSION, &ActualMinor);

        application::util::Logger::Info("UIMgr", "Created OpenGL context version: %i.%i", ActualMajor, ActualMinor);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
#if TOOL_IMGUI_VIEWPORTS_ENABLED == 1
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
#endif
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;


        int fbWidth, fbHeight;
        int winWidth, winHeight;
        glfwGetFramebufferSize(gWindow, &fbWidth, &fbHeight);
        glfwGetWindowSize(gWindow, &winWidth, &winHeight);

        float retinaScaleX = (float)fbWidth / (float)winWidth;
        float retinaScaleY = (float)fbHeight / (float)winHeight;
        io.FontGlobalScale = 1.0f / retinaScaleX;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();
        
        ImGuiStyle& Style = ImGui::GetStyle();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
#if TOOL_IMGUI_VIEWPORTS_ENABLED == 1
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            Style.WindowRounding = 0.0f;
            Style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
#endif

        Style.Alpha = 1.0;
        Style.DisabledAlpha = 0.6000000238418579;
        Style.WindowPadding = ImVec2(8.0, 8.0);
        Style.WindowRounding = 0.0;
        Style.WindowBorderSize = 1.0;
        Style.WindowMinSize = ImVec2(32.0, 32.0);
        Style.WindowTitleAlign = ImVec2(0.0, 0.5);
        Style.WindowMenuButtonPosition = ImGuiDir_Left;
        Style.ChildRounding = 0.0;
        Style.ChildBorderSize = 1.0;
        Style.PopupRounding = 0.0;
        Style.PopupBorderSize = 1.0;
        Style.FramePadding = ImVec2(4.0, 3.0);
        Style.FrameRounding = 0.0;
        Style.FrameBorderSize = 0.0;
        Style.ItemSpacing = ImVec2(8.0, 4.0);
        Style.ItemInnerSpacing = ImVec2(4.0, 4.0);
        Style.CellPadding = ImVec2(4.0, 2.0);
        Style.IndentSpacing = 21.0;
        Style.ColumnsMinSpacing = 6.0;
        Style.ScrollbarSize = 14.0;
        Style.ScrollbarRounding = 0.0;
        Style.GrabMinSize = 10.0;
        Style.GrabRounding = 0.0;
        Style.TabRounding = 0.0;
        Style.TabBorderSize = 0.0;
        Style.ColorButtonPosition = ImGuiDir_Right;
        Style.ButtonTextAlign = ImVec2(0.5, 0.5);
        Style.SelectableTextAlign = ImVec2(0.0, 0.0);

        Style.Colors[ImGuiCol_Text] = ImVec4(1.0, 1.0, 1.0, 1.0);
        Style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.592156862745098, 0.592156862745098, 0.592156862745098, 1.0);
        Style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_Border] = ImVec4(0.3058823529411765, 0.3058823529411765, 0.3058823529411765, 1.0);
        Style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.3058823529411765, 0.3058823529411765, 0.3058823529411765, 1.0);
        Style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2, 0.2, 0.21568627450980393, 1.0);
        Style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.11372549019607843, 0.592156862745098, 0.9254901960784314, 1.0);
        Style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2, 0.2, 0.21568627450980393, 1.0);
        Style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2, 0.2, 0.21568627450980393, 1.0);
        Style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3215686274509804, 0.3215686274509804, 0.3333333333333333, 1.0);
        Style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35294117647058826, 0.35294117647058826, 0.37254901960784315, 1.0);
        Style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35294117647058826, 0.35294117647058826, 0.37254901960784315, 1.0);
        Style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11372549019607843, 0.592156862745098, 0.9254901960784314, 1.0);
        Style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_Button] = ImVec4(0.2, 0.2, 0.21568627450980393, 1.0);
        Style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.11372549019607843, 0.592156862745098, 0.9254901960784314, 1.0);
        Style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.11372549019607843, 0.592156862745098, 0.9254901960784314, 1.0);
        Style.Colors[ImGuiCol_Header] = ImVec4(0.2, 0.2, 0.21568627450980393, 1.0);
        Style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.11372549019607843, 0.592156862745098, 0.9254901960784314, 1.0);
        Style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_Separator] = ImVec4(0.3058823529411765, 0.3058823529411765, 0.3058823529411765, 1.0);
        Style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3058823529411765, 0.3058823529411765, 0.3058823529411765, 1.0);
        Style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3058823529411765, 0.3058823529411765, 0.3058823529411765, 1.0);
        Style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2, 0.2, 0.21568627450980393, 1.0);
        Style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.3215686274509804, 0.3215686274509804, 0.3333333333333333, 1.0);
        Style.Colors[ImGuiCol_Tab] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_TabHovered] = ImVec4(0.11372549019607843, 0.592156862745098, 0.9254901960784314, 1.0);
        Style.Colors[ImGuiCol_TabActive] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.11372549019607843, 0.592156862745098, 0.9254901960784314, 1.0);
        Style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.11372549019607843, 0.592156862745098, 0.9254901960784314, 1.0);
        Style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18823529411764706, 0.18823529411764706, 0.2, 1.0);
        Style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30980392156862746, 0.30980392156862746, 0.34901960784313724, 1.0);
        Style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.22745098039215686, 0.22745098039215686, 0.24705882352941178, 1.0);
        Style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        Style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0, 1.0, 1.0, 0.05999999865889549);
        Style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0, 0.4666666666666667, 0.7843137254901961, 1.0);
        Style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);
        Style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0, 1.0, 1.0, 0.699999988079071);
        Style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8, 0.8, 0.8, 0.2000000029802322);
        //Style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.1450980392156863, 0.1450980392156863, 0.14901960784313725, 1.0);

        /*
        style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.40f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 1.00f, 1.00f, 0.65f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.80f, 0.80f, 0.18f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.44f, 0.80f, 0.80f, 0.27f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.81f, 0.86f, 0.66f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.21f, 0.73f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.27f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.22f, 0.29f, 0.30f, 0.71f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.44f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 1.00f, 0.33f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.04f, 0.10f, 0.09f, 0.51f);*/


        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(gWindow, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        io.Fonts->AddFontFromFileTTF(application::util::FileUtil::GetAssetFilePath("Fonts/Regular.ttf").c_str(), 14.0f * ImGui::GetPlatformIO().Monitors[0].DpiScale);

        /**
         * FontAwesome setup START (required for icons)
        */

        float baseFontSize = 16.0f;
        float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

        static constexpr ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
        ImFontConfig iconsConfig;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.GlyphMinAdvanceX = iconFontSize;
        io.Fonts->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data, fa_solid_900_compressed_size, iconFontSize, &iconsConfig, iconsRanges);

        /**
         * FontAwesome setup END
        */


        gSettingsPopUp.Title("Settings").Width(500.0f).Flag(ImGuiWindowFlags_NoResize).NeedsConfirmation(false).ContentDrawingFunction([](application::rendering::popup::PopUpBuilder& Builder)
            {
                ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(189 / 255.0f, 195 / 255.0f, 199 / 255.0f, 1.0f));
                ImGui::Text("Paths");
                ImGui::Separator();
                //ImGui::NewLine();
                ImGui::Columns(2, "Paths");
                ImGui::Indent();

                ImGui::SetColumnWidth(0, ImGui::CalcTextSize("Model Dump Path").x + ImGui::GetStyle().ItemSpacing.x * 2 + ImGui::GetStyle().IndentSpacing);

                ImGui::Text("RomFS Path");
                ImGui::NextColumn();
                ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                bool RomFSValid = !application::util::FileUtil::gRomFSPath.empty();
                if (RomFSValid)
                    RomFSValid = application::util::FileUtil::FileExists(application::util::FileUtil::gRomFSPath + "/Pack/Bootup.Nin_NX_NVN.pack.zs");

                ImGui::PushStyleColor(ImGuiCol_FrameBg, RomFSValid ? ImVec4(0.06f, 0.26f, 0.07f, 1.0f) : ImVec4(0.26f, 0.06f, 0.07f, 1.0f));
                ImGui::InputText("##RomFSPath", &application::util::FileUtil::gRomFSPath);
                ImGui::PopStyleColor();
                ImGui::PopItemWidth();
                ImGui::NextColumn();

                ImGui::Text("Model Dump Path");
                ImGui::NextColumn();
                ImGui::PushItemWidth(ImGui::GetColumnWidth() - ImGui::GetStyle().ScrollbarSize);
                bool ModelValid = !application::util::FileUtil::gBfresPath.empty();
                if (ModelValid)
                    ModelValid = application::util::FileUtil::FileExists(application::util::FileUtil::gBfresPath + "/Weapon_Sword_020.Weapon_Sword_020.bfres");

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ModelValid ? ImVec4(0.06f, 0.26f, 0.07f, 1.0f) : ImVec4(0.26f, 0.06f, 0.07f, 1.0f));
                ImGui::InputText("##ModelPath", &application::util::FileUtil::gBfresPath);
                ImGui::PopStyleColor();
                ImGui::PopItemWidth();

                ImGui::Unindent();
                ImGui::Columns();
                ImGui::PopStyleColor();
            }).Register();

        gAddProjectPopUp.Title("Projects").Width(500.0f).Flag(ImGuiWindowFlags_NoResize).NeedsConfirmation(false).AddDataStorage(512).ContentDrawingFunction([](application::rendering::popup::PopUpBuilder& Builder)
            {
                ImGui::InputText("Name", reinterpret_cast<char*>(Builder.GetDataStorage(0).mPtr), Builder.GetDataStorage(0).mSize);
            }).Register();

        gExportProjectPopUp.Title("Export").Width(500.0f).Flag(ImGuiWindowFlags_NoResize).AddDataStorageInstanced<bool>([](void* Bool) { *reinterpret_cast<bool*>(Bool) = true; }).NeedsConfirmation(false).ContentDrawingFunction([](application::rendering::popup::PopUpBuilder& Builder)
            {
                ImGui::InputText("Path", &application::manager::ProjectMgr::gExportProjectPath);
                ImGui::Checkbox("Generate RSTB", reinterpret_cast<bool*>(Builder.GetDataStorage(0).mPtr));
            }).Register();

        gEnterLicenseKeyPopUp.Title("License").Width(500.0f).NeedsConfirmation(true).AddDataStorageInstanced<std::string>([](void* Str) { *reinterpret_cast<std::string*>(Str) = ""; }).ContentDrawingFunction([](application::rendering::popup::PopUpBuilder& Builder)
            {
                if(application::file::tool::LicenseFile::gLicenseSeed.empty())
				    application::file::tool::LicenseFile::gLicenseSeed = application::util::Math::GenerateRandomString();

                ImGui::Text("Your license seed: %s", application::file::tool::LicenseFile::gLicenseSeed.c_str());
                ImGui::SameLine();
                if(ImGui::Button("Copy"))
                {
                    ImGui::SetClipboardText(application::file::tool::LicenseFile::gLicenseSeed.c_str());
				}
                ImGui::NewLine();
				ImGui::Text("Please enter your license key to continue:");
                ImGui::InputText("License Key", reinterpret_cast<std::string*>(Builder.GetDataStorage(0).mPtr));
            }).Register();

        gGenerateLicenseKeyPopUp.Title("Generate License").Width(500.0f).AddDataStorageInstanced<std::string>([](void* Str) { *reinterpret_cast<std::string*>(Str) = ""; }).ContentDrawingFunction([](application::rendering::popup::PopUpBuilder& Builder)
            {
                ImGui::Text("Please enter the license seed to generate:");
                ImGui::InputText("License Seed", reinterpret_cast<std::string*>(Builder.GetDataStorage(0).mPtr));
            }).Register();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPatchParameteri(GL_PATCH_VERTICES, 4); //4 is for terrain

#if TOOL_GL_DEBUG == 1
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        application::util::Logger::Info("UIMgr", "Enabled GL debugging");
#endif

        GLint NumFormats = 0;
        glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &NumFormats);
        GLint* Formats = new GLint[NumFormats];
        glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, Formats);
        for (int i = 0; i < NumFormats; i++)
        {
            if (Formats[i] == GL_COMPRESSED_RGBA_ASTC_4x4_KHR)
            {
                gASTCSupported = true;
                break;
            }
        }
        delete[] Formats;
        application::util::Logger::Info("UIMgr", "GPU based ASTC decoding supported: %s", gASTCSupported ? "True" : "False");

        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* vendor = glGetString(GL_VENDOR);
		application::util::Logger::Info("UIMgr", "GPU Renderer: %s", renderer);
		application::util::Logger::Info("UIMgr", "GPU Vendor: %s", vendor);

        application::util::Logger::Info("UIMgr", "UI initialized");

        return true;
	}

    void UIMgr::Render()
    {
        assert(gWindow != nullptr && "UI not initialized!");

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        ImGuiID DockSpace = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        if(gFirstFrame)
        {
            ImGui::DockBuilderRemoveNode(DockSpace);
			ImGui::DockBuilderAddNode(DockSpace, ImGuiDockNodeFlags_CentralNode);
			ImGui::DockBuilderSetNodeSize(DockSpace, ImGui::GetMainViewport()->Size);

			//ImGui::DockBuilderSplitNode(DockSpace, ImGuiDir_Down, 0.25f, &gDockBottom, &gDockMain);
            gDockMain = DockSpace;

            //ImGui::DockBuilderDockWindow("Content Browser", gDockBottom);

            ImGui::DockBuilderFinish(DockSpace);
            gFirstFrame = false;
        }

        //ImGui::ShowDemoWindow();

        UpdateWaitingWindows();

        for (auto Iter = gWindows.begin(); Iter != gWindows.end(); )
        {
            Iter->get()->Draw();

            if (!Iter->get()->mOpen)
            {
                Iter->get()->Delete();
                Iter = gWindows.erase(Iter);

                gBlockProjectSwitch = false;
                for (auto& Window : gWindows)
                {
                    Window->UpdateSameWindowCount();
                    gBlockProjectSwitch |= !Window->SupportsProjectChange();
                }

                continue;
            }
            Iter++;
        }

        PopUpMgr::Render();

        if (ImGui::BeginMainMenuBar())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

            if (ImGui::MenuItem("Settings"))
            {
                gSettingsPopUp.Open([](application::rendering::popup::PopUpBuilder& Builder)
                    {
                        application::util::FileUtil::ValidatePaths();
                        application::Editor::InitializeRomFSPathDependant();

                        if (application::util::FileUtil::gPathsValid)
                        {
                            application::file::tool::PathConfigFile::Save(application::util::FileUtil::GetWorkingDirFilePath("Config.epathcfg"));
                        }
                    });
            }

#if defined(TOOL_FORCE_PATHS)
            if (!application::util::FileUtil::gPathsValid)
                ImGui::BeginDisabled();
#endif

            if (ImGui::BeginMenu("Projects"))
            {
                if (application::util::FileUtil::gPathsValid && gBlockProjectSwitch)
                    ImGui::BeginDisabled();

                ImGui::Text("Selected: %s", application::manager::ProjectMgr::IsAnyProjectSelected() ? application::manager::ProjectMgr::gProject.c_str() : "None");
                if (ImGui::MenuItem("Add"))
                {
                    gAddProjectPopUp.Open([](application::rendering::popup::PopUpBuilder& Builder)
                        {
                            application::manager::ProjectMgr::AddProject(std::string(reinterpret_cast<char*>(Builder.GetDataStorage(0).mPtr)));
                        });
                }
                ImGui::Separator();
                for (const std::string& Name : application::manager::ProjectMgr::gProjects)
                {
                    if (ImGui::MenuItem(Name.c_str()))
                    {
                        application::manager::ProjectMgr::SelectProject(Name);
                    }
                }

                if (application::util::FileUtil::gPathsValid && gBlockProjectSwitch)
                    ImGui::EndDisabled();

                if (!application::manager::ProjectMgr::IsAnyProjectSelected())
                    ImGui::BeginDisabled();

                ImGui::Separator();
                if (ImGui::MenuItem("Export"))
                {
                    gExportProjectPopUp.Open([](application::rendering::popup::PopUpBuilder& Builder)
                        {
                            application::manager::ProjectMgr::ExportProject(*reinterpret_cast<bool*>(Builder.GetDataStorage(0).mPtr));
                        });
                }

                if (!application::manager::ProjectMgr::IsAnyProjectSelected())
                    ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if(!application::manager::ProjectMgr::IsAnyProjectSelected()
#if defined(TOOL_FORCE_PATHS)
                && application::util::FileUtil::gPathsValid
#endif
                )
                ImGui::BeginDisabled();

            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Map Editor"))
                {
                    application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::map_editor::UIMapEditor>());
                }
                if (ImGui::MenuItem("AINB Editor"))
                {
                    application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::ainb::UIAINBEditor>());
                }
                if (ImGui::MenuItem("Actor Editor"))
                {
                    application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::actor::UIActorTool>());
                }
                if (ImGui::MenuItem("Collision Generator"))
                {
                    application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::collision::UICollisionGenerator>());
                }
                if (ImGui::MenuItem("Plugins"))
                {
                    application::manager::UIMgr::OpenWindow(std::make_unique<application::rendering::plugin::UIPlugins>());
                }
                ImGui::EndMenu();
            }

#if defined(TOOL_FORCE_PATHS)
            if (!application::util::FileUtil::gPathsValid || (application::util::FileUtil::gPathsValid && !application::manager::ProjectMgr::IsAnyProjectSelected()))
#else
            if (!application::manager::ProjectMgr::IsAnyProjectSelected())
#endif
                ImGui::EndDisabled();

            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::Text("PUBLIC BUILD");
            ImGui::PopStyleColor();

            ImGui::EndMainMenuBar();
        }

        // Notifications style setup
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f); // Disable round borders
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f); // Disable borders

        // Notifications color setup
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f)); // Background color


        // Main rendering function
        ImGui::RenderNotifications();


        //������������������������������� WARNING �������������������������������
        // Argument MUST match the amount of ImGui::PushStyleVar() calls 
        ImGui::PopStyleVar(2);
        // Argument MUST match the amount of ImGui::PushStyleColor() calls 
        ImGui::PopStyleColor(1);

        // Rendering
        ImGui::Render();
        int DisplayW, DisplayH;
        glfwGetFramebufferSize(gWindow, &DisplayW, &DisplayH);
        glViewport(0, 0, DisplayW, DisplayH);
        glClearColor(gClearColor.x, gClearColor.y, gClearColor.z, gClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
#if TOOL_IMGUI_VIEWPORTS_ENABLED == 1
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
#endif

        glfwSwapBuffers(gWindow);
    }

    bool UIMgr::ShouldWindowClose()
    {
        assert(gWindow != nullptr && "UI not initialized!");

        return glfwWindowShouldClose(gWindow);
    }

    void UIMgr::Cleanup()
    {
        assert(gWindow != nullptr && "UI not initialized!");

        for (std::unique_ptr<application::rendering::UIWindowBase>& Window : gWindows)
        {
            Window->Delete();
        }
        gWindows.clear();

        // Cleanup
        application::manager::BfresRendererMgr::Cleanup();
        application::manager::FramebufferMgr::Cleanup();
        application::manager::TextureMgr::Cleanup();
        application::manager::ShaderMgr::Cleanup();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(gWindow);
        glfwTerminate();

        application::util::Logger::Info("UIMgr", "UIMgr cleaned up");
    }

    std::unique_ptr<application::rendering::UIWindowBase>& UIMgr::OpenWindow(std::unique_ptr<application::rendering::UIWindowBase> Window)
    {
        if (!Window->SupportsProjectChange())
            gBlockProjectSwitch = true;

        gWaitingWindows.push_back(std::move(Window));
        return gWaitingWindows.back();
    }

    void UIMgr::UpdateWaitingWindows()
    {
        if(gWaitingWindows.empty())
            return;

        for(auto& Window : gWaitingWindows)
        {
            Window->mWindowId = gWindowId++;
            Window->Initialize();
            gWindows.push_back(std::move(Window));
        }
        gWaitingWindows.clear();
    }
}