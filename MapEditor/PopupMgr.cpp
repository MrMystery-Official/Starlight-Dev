#include "PopupMgr.h"

#include "Editor.h"
#include "imgui_internal.h"

#include "PopupAddActor.h"
#include "PopupGeneralInputPair.h"
#include "PopupAddRail.h"
#include "PopupAddLink.h"
#include "PopupGeneralConfirm.h"
#include "PopupGeneralInputString.h"
#include "PopupLoadScene.h"
#include "PopupExportMod.h"
#include "PopupAddAINBNode.h"
#include "PopupEditorAINBActorLinks.h"
#include "PopupAINBElementSelector.h"
#include "PopupCredits.h"
#include "PopupStackActors.h"
#include "PopupSettings.h"
#include "PopupCreateScene.h"
#include "PopupAddDynamicData.h"
#include "PopupModifyNodeActionQuery.h"
#include "PopupAddActorActionQuery.h"

void PopupMgr::OnViewportChanged(ImGuiViewport* ViewPortPtr)
{
	if (Editor::UIScale != ViewPortPtr->DpiScale) {
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(ViewPortPtr->DpiScale);
		Editor::UIScale = ViewPortPtr->DpiScale;
		ImGui::SetCurrentFont(Editor::Fonts[ViewPortPtr->DpiScale]);

		PopupAddActor::UpdateSize(Editor::UIScale * 1.2f);
		PopupAddDynamicData::UpdateSize(Editor::UIScale * 1.2f);
		PopupAddLink::UpdateSize(Editor::UIScale * 1.2f);
		PopupAddRail::UpdateSize(Editor::UIScale * 1.2f);
		PopupCreateScene::UpdateSize(Editor::UIScale * 1.2f);
		PopupCredits::UpdateSize(Editor::UIScale * 1.2f);
		PopupExportMod::UpdateSize(Editor::UIScale * 1.2f);
		PopupGeneralConfirm::UpdateSize(Editor::UIScale * 1.2f);
		PopupGeneralInputPair::UpdateSize(Editor::UIScale * 1.2f);
		PopupGeneralInputString::UpdateSize(Editor::UIScale * 1.2f);
		PopupLoadScene::UpdateSize(Editor::UIScale * 1.2f);
		PopupSettings::UpdateSize(Editor::UIScale * 1.2f);
		PopupStackActors::UpdateSize(Editor::UIScale * 1.2f);

		PopupAddAINBNode::UpdateSize(Editor::UIScale * 1.2f);
		PopupAINBElementSelector::UpdateSize(Editor::UIScale * 1.2f);
		PopupEditorAINBActorLinks::UpdateSize(Editor::UIScale * 1.2f);

		PopupModifyNodeActionQuery::UpdateSize(Editor::UIScale * 1.2f);
		PopupAddActorActionQuery::UpdateSize(Editor::UIScale * 1.2f);
	}
}