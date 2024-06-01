#pragma once

namespace PreferencesConfig {

struct WindowInformationStruct {
    bool ConsoleOpen = true;
    bool ActorToolOpen = true;
    bool ToolsOpen = true;
    bool PropertiesOpen = true;
    bool OutlinerOpen = true;
    bool MSBTEditorOpen = true;
    bool MapViewOpen = true;
    bool AINBEditorOpen = true;
};

extern unsigned int FrameCount;
extern WindowInformationStruct WindowInformation;

void Save();
void Load();

void Frame();
void UpdateWindowInformation();
bool WindowInformationChanged();

}