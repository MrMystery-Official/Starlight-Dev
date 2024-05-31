#pragma once

#include "AINB.h"
#include "Byml.h"
#include "SARC.h"
#include <map>
#include <string>
#include <vector>

namespace UIActorTool {

struct ActorPackStruct {
    std::string OriginalName = "";
    std::string Name = "";

    std::map<std::string, BymlFile> Bymls;
    std::map<std::string, BymlFile> OriginalBymls;
    std::map<std::string, std::vector<unsigned char>> AINBs;
    std::map<std::string, std::vector<unsigned char>> ChangedAINBs;
    std::vector<std::string> DeletedFiles;

    bool Replace = false;
    SarcFile Pack;
};

extern bool Open;
extern std::string ActorPackFilter;
extern ActorPackStruct ActorPack;
extern std::vector<std::string> ActorList;

void DrawActorToolWindow();
void UpdateActorList();
void Save(std::string Path);

}
