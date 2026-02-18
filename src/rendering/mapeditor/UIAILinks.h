#pragma once

#include <rendering/popup/PopUpBuilder.h>

namespace application::rendering::map_editor
{
	struct UIAILinks
	{
		static void Initialize();

		static application::rendering::popup::PopUpBuilder gAILinksPopUp;
		static void* gScenePtr;


		void Open(void* ScenePtr);
	};
}