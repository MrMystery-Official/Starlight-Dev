#pragma once

#include <rendering/popup/PopUpBuilder.h>
#include <vector>

namespace application::manager
{
	namespace PopUpMgr
	{
        extern std::vector<application::rendering::popup::PopUpBuilder*> gPopUps;

        void AddPopUp(application::rendering::popup::PopUpBuilder* PopUp);
        void RemovePopUp(application::rendering::popup::PopUpBuilder* PopUp);

        void Render();
    }
}