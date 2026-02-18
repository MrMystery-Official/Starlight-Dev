#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <rendering/UIWindowBase.h>
#include <rendering/popup/PopUpBuilder.h>
#include "imgui.h"

namespace application::manager
{
	namespace UIMgr
	{
		extern GLFWwindow* gWindow;
		extern const ImVec4 gClearColor;
		extern std::vector<std::unique_ptr<application::rendering::UIWindowBase>> gWindows;
		extern std::vector<std::unique_ptr<application::rendering::UIWindowBase>> gWaitingWindows;
		extern unsigned int gWindowId;
		extern bool gFirstFrame;
		extern bool gBlockProjectSwitch;
		extern bool gASTCSupported;

		extern ImGuiID gDockMain;
		extern ImGuiID gDockBottom;

		extern application::rendering::popup::PopUpBuilder gSettingsPopUp;
		extern application::rendering::popup::PopUpBuilder gAddProjectPopUp;
		extern application::rendering::popup::PopUpBuilder gExportProjectPopUp;
		extern application::rendering::popup::PopUpBuilder gEnterLicenseKeyPopUp;
		extern application::rendering::popup::PopUpBuilder gGenerateLicenseKeyPopUp;

		void GLFWErrorCallback(int error, const char* description);
		bool Initialize();
		void Render();
		void Cleanup();
		bool ShouldWindowClose();

		std::unique_ptr<application::rendering::UIWindowBase>& OpenWindow(std::unique_ptr<application::rendering::UIWindowBase> Window);
		void UpdateWaitingWindows();
	}
}