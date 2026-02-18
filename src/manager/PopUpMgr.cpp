#include "PopUpMgr.h"

#include <util/Logger.h>

namespace application::manager
{
    std::vector<application::rendering::popup::PopUpBuilder*> PopUpMgr::gPopUps;

    void PopUpMgr::AddPopUp(application::rendering::popup::PopUpBuilder* PopUp)
    {
        if(std::find(gPopUps.begin(), gPopUps.end(), PopUp) != gPopUps.end())
            return;
        
        gPopUps.push_back(PopUp);
    }

    void PopUpMgr::RemovePopUp(application::rendering::popup::PopUpBuilder* PopUp)
    {
        if (gPopUps.empty())
            return;

        gPopUps.erase(
            std::remove_if(gPopUps.begin(), gPopUps.end(), [PopUp](application::rendering::popup::PopUpBuilder* ManagedPopUp)
                {
                    return ManagedPopUp == PopUp;
                }),
            gPopUps.end());
    }

    void PopUpMgr::Render()
    {
        for(application::rendering::popup::PopUpBuilder* Ptr : gPopUps)
        {
            Ptr->Render();
        }
    }
}