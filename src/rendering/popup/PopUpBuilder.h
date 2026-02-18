#pragma once

#include <functional>
#include <string>
#include <vector>
#include "imgui.h"

namespace application::rendering::popup
{
    class PopUpBuilder
    {
    public:
        struct DataStorageEntry
        {
            void* mPtr = nullptr;
            uint32_t mSize = 0;

            bool mIsInstanced = false;
            std::function<void(void*)> mResetFunction;

            void Clear();
        };

        enum class CloseResult : uint8_t
        {
            CANCLE = 0,
            OKAY = 1
        };

        PopUpBuilder() = default;
        ~PopUpBuilder();

        PopUpBuilder& ContentDrawingFunction(const std::function<void(PopUpBuilder&)>& Draw);
        PopUpBuilder& Title(const std::string& Title);
        PopUpBuilder& Register();
        PopUpBuilder& Flag(ImGuiWindowFlags_ Flag);
        PopUpBuilder& Width(float Width);
        PopUpBuilder& Height(float Height);

        PopUpBuilder& AddDataStorage(uint32_t Size);

        template <typename T>
        PopUpBuilder& AddDataStorageInstanced(const std::function<void(void*)>& ResetFunction)
        {
            DataStorageEntry Entry;
            Entry.mPtr = new T;
            Entry.mSize = 0;
            Entry.mIsInstanced = true;
            Entry.mResetFunction = ResetFunction;
            ResetFunction(Entry.mPtr);
            mDataStorage.push_back(Entry);
            return *this;
        }

        DataStorageEntry& GetDataStorage(uint32_t Index);

        PopUpBuilder& NeedsConfirmation(bool Val);
        PopUpBuilder& InformationalPopUp();

        void Open(const std::function<void(PopUpBuilder&)>& Close);
        void Render();

        bool& IsOpen();

    private:
        std::function<void(PopUpBuilder&)> mDrawFunc;
        std::function<void(PopUpBuilder&)> mCloseFunc;
        std::string mTitle = "No Title Set";
        std::vector<DataStorageEntry> mDataStorage;
        bool mNeedsConfirmation = false;
        bool mIsInformationPopUp = false;
        bool mOpen = false;
        ImGuiWindowFlags mFlags;
        float mPopUpWidth = 200.0f;
        float mPopUpHeight = 0.0f;
    };
} // namespace application::rendering::popup
