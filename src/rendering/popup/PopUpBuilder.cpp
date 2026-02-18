#include "PopUpBuilder.h"

#include <cstring>
#include <manager/PopUpMgr.h>
#include <util/Logger.h>

namespace application::rendering::popup
{
    void PopUpBuilder::DataStorageEntry::Clear()
    {
        if (!mIsInstanced)
        {
            memset(mPtr, 0, mSize);
        }
        else
        {
            mResetFunction(mPtr);
        }
    }

    PopUpBuilder::~PopUpBuilder()
    {
        application::manager::PopUpMgr::RemovePopUp(this);

        for(DataStorageEntry& Entry : mDataStorage)
        {
            if (Entry.mIsInstanced)
            {
                delete Entry.mPtr;
                continue;
            }
            free(Entry.mPtr);
        }

        mDataStorage.clear();
    }

    PopUpBuilder& PopUpBuilder::Register()
    {
        application::manager::PopUpMgr::AddPopUp(this);
        return *this;
    }

    PopUpBuilder& PopUpBuilder::Width(float Width)
    {
        mPopUpWidth = Width;
        return *this;
    }

    PopUpBuilder& PopUpBuilder::Height(float Height)
    {
        mPopUpHeight = Height;
        return *this;
    }

    PopUpBuilder& PopUpBuilder::ContentDrawingFunction(const std::function<void(PopUpBuilder&)>& Draw)
    {
        mDrawFunc = Draw;
        return *this;
    }

    PopUpBuilder& PopUpBuilder::Title(const std::string& Title)
    {
        mTitle = Title;
        return *this;
    }

    PopUpBuilder& PopUpBuilder::AddDataStorage(uint32_t Size)
    {
        DataStorageEntry Entry;
        Entry.mPtr = malloc(Size);
        Entry.mSize = Size;
        Entry.Clear();
        mDataStorage.push_back(Entry);
        return *this;
    }

    PopUpBuilder::DataStorageEntry& PopUpBuilder::GetDataStorage(uint32_t Index)
    {
        return mDataStorage[Index];
    }

    PopUpBuilder& PopUpBuilder::NeedsConfirmation(bool Val)
    {
        mNeedsConfirmation = Val;
        return *this;
    }

    PopUpBuilder& PopUpBuilder::InformationalPopUp()
    {
        mIsInformationPopUp = true;
        return *this;
    }

    PopUpBuilder& PopUpBuilder::Flag(ImGuiWindowFlags_ Flag)
    {
        mFlags |= Flag;
        return *this;
    }

    void PopUpBuilder::Open(const std::function<void(PopUpBuilder&)>& Close)
    {
        for(DataStorageEntry& Entry : mDataStorage)
        {
            Entry.Clear();
        }

        mOpen = true;
        mCloseFunc = Close;
    }

    bool& PopUpBuilder::IsOpen()
    {
        return mOpen;
    }

    void PopUpBuilder::Render()
    {
        if(!mOpen)
            return;

        ImGui::SetNextWindowSize(ImVec2(mPopUpWidth * ImGui::GetPlatformIO().Monitors[0].DpiScale, mPopUpHeight * ImGui::GetPlatformIO().Monitors[0].DpiScale), ImGuiCond_Once);
        ImGui::OpenPopup(mTitle.c_str());
        if (ImGui::BeginPopupModal(mTitle.c_str(), NULL, mFlags))
        {
            mDrawFunc(*this);

            if(ImGui::Button("Okay##PopUpButton"))
            {
                mOpen = false;
                mCloseFunc(*this);
            }
            ImGui::SameLine();

            if (!mIsInformationPopUp)
            {
                if (mNeedsConfirmation)
                    ImGui::BeginDisabled();
                if (ImGui::Button("Cancel##PopUpButton"))
                {
                    mOpen = false;
                }
                if (mNeedsConfirmation)
                    ImGui::EndDisabled();
            }
        }
        ImGui::EndPopup();
    }

} // namespace application::rendering::popup
