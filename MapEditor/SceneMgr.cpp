#include "SceneMgr.h"

#include "ActorMgr.h"
#include "Byml.h"
#include "Editor.h"
#include "HashMgr.h"
#include "Logger.h"
#include "UIMapView.h"
#include "UIOutliner.h"
#include "ZStdFile.h"

SceneMgr::Type SceneMgr::SceneType = SceneMgr::Type::SmallDungeon;

void SceneMgr::LoadScene(SceneMgr::Type Type, std::string Identifier)
{
    SceneType = Type;

    std::string BancPrefix = "Banc/";
    switch (Type) {
    case SceneMgr::Type::SmallDungeon:
        BancPrefix += "SmallDungeon/Dungeon";
        Editor::BancDir = "Banc/SmallDungeon";
        break;
    case SceneMgr::Type::MainField:
        BancPrefix += "MainField/";
        Editor::BancDir = "Banc/MainField";
        break;
    case SceneMgr::Type::MinusField:
        BancPrefix += "MinusField/";
        Editor::BancDir = "Banc/MinusField";
        break;
    case SceneMgr::Type::LargeDungeon:
        BancPrefix += "LargeDungeon/LargeDungeon";
        Editor::BancDir = "Banc/LargeDungeon";
        break;
    case SceneMgr::Type::NormalStage:
        BancPrefix += "NormalStage/";
        Editor::BancDir = "Banc/NormalStage";
        break;
    case SceneMgr::Type::SkyIslands:
        BancPrefix += "MainField/Sky/";
        Editor::BancDir = "Banc/MainField/Sky";
        break;
    case SceneMgr::Type::Custom:
        size_t Found = Identifier.find_last_of("/\\");
        Editor::BancDir = "Banc/" + Identifier.substr(0, Found);
        break;
    }

    Logger::Info("SceneMgr", "Loading map " + Identifier + " of type " + std::to_string((int)Type));

    BymlFile DynamicActorsByml(ZStdFile::Decompress(Editor::GetRomFSFile(BancPrefix + Identifier + "_Dynamic.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);
    BymlFile StaticActorsByml(ZStdFile::Decompress(Editor::GetRomFSFile(BancPrefix + Identifier + "_Static.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);

    ActorMgr::GetActors().clear();
    ActorMgr::OpaqueActors.clear();
    ActorMgr::TransparentActors.clear();
    UIOutliner::SelectedActor = nullptr;

    Editor::DynamicActorsByml = DynamicActorsByml;
    Editor::StaticActorsByml = StaticActorsByml;
    Editor::BancPrefix = BancPrefix;
    Editor::Identifier = Identifier;

    auto DecodeByml = [](BymlFile& File, Actor::Type Type) {
        if (!File.HasChild("Actors"))
            return;
        for (BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren()) {
            Actor* NewActor = ActorMgr::AddActorFromByml(ActorNode);
            if (NewActor == nullptr)
                return;

            NewActor->ActorType = Type;
        }
    };

    DecodeByml(DynamicActorsByml, Actor::Type::Dynamic);
    DecodeByml(StaticActorsByml, Actor::Type::Static);

    ActorMgr::UpdateModelOrder();
    HashMgr::Initialize();

    UIMapView::CameraView.Position.x = ActorMgr::GetActors()[0].Translate.GetX();
    UIMapView::CameraView.Position.y = ActorMgr::GetActors()[0].Translate.GetY();
    UIMapView::CameraView.Position.z = ActorMgr::GetActors()[0].Translate.GetZ();
}

void SceneMgr::Reload()
{
    Logger::Info("SceneMgr", "Reloading map " + Editor::Identifier);

    BymlFile DynamicActorsByml(ZStdFile::Decompress(Editor::GetRomFSFile(Editor::BancPrefix + Editor::Identifier + "_Dynamic.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);
    BymlFile StaticActorsByml(ZStdFile::Decompress(Editor::GetRomFSFile(Editor::BancPrefix + Editor::Identifier + "_Static.bcett.byml.zs"), ZStdFile::Dictionary::BcettByaml).Data);

    ActorMgr::GetActors().clear();
    ActorMgr::OpaqueActors.clear();
    ActorMgr::TransparentActors.clear();
    UIOutliner::SelectedActor = nullptr;

    Editor::DynamicActorsByml = DynamicActorsByml;
    Editor::StaticActorsByml = StaticActorsByml;

    auto DecodeByml = [](BymlFile& File, Actor::Type Type) {
        if (!File.HasChild("Actors"))
            return;
        for (BymlFile::Node& ActorNode : File.GetNode("Actors")->GetChildren()) {
            Actor* NewActor = ActorMgr::AddActorFromByml(ActorNode);
            if (NewActor == nullptr)
                return;

            NewActor->ActorType = Type;
        }
    };

    DecodeByml(DynamicActorsByml, Actor::Type::Dynamic);
    DecodeByml(StaticActorsByml, Actor::Type::Static);

    ActorMgr::UpdateModelOrder();
    HashMgr::Initialize();
    Logger::Info("SceneMgr", "Scene reload completed");
}