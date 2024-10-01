#include "PopupModifyNodeActionQuery.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"
#include "EventNodeDefMgr.h"

bool PopupModifyNodeActionQuery::IsOpen = false;

float PopupModifyNodeActionQuery::SizeX = 341.0f;
float PopupModifyNodeActionQuery::SizeY = 85.0f;
const float PopupModifyNodeActionQuery::OriginalSizeX = 341.0f;
const float PopupModifyNodeActionQuery::OriginalSizeY = 85.0f;

int16_t* PopupModifyNodeActionQuery::mActorIndex = nullptr;
int16_t* PopupModifyNodeActionQuery::mActionQueryIndex = nullptr;
std::vector<const char*> PopupModifyNodeActionQuery::mActorNames;
std::vector<const char*> PopupModifyNodeActionQuery::mActionQueryNames;
BFEVFLFile* PopupModifyNodeActionQuery::mEventFile = nullptr;
bool PopupModifyNodeActionQuery::mIsAction = false;
BFEVFLFile::Container* PopupModifyNodeActionQuery::mParameters = nullptr;

void PopupModifyNodeActionQuery::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupModifyNodeActionQuery::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Modify Node Action/Query");
		//, NULL, ImGuiWindowFlags_NoResize
		if (ImGui::BeginPopupModal("Modify Node Action/Query"))
		{
			//ImGui::Combo doesn't work, no idea why

			if (ImGui::BeginCombo("Actor##PopupModifyNodeActionQueryActor", *mActorIndex >= 0 ? mActorNames[*mActorIndex] : "Undefined"))
			{
				for (size_t i = 0; i < mActorNames.size(); i++)
				{
					bool IsSelected = (i == *mActorIndex);
					if ((mIsAction && mEventFile->mFlowcharts[0].mActors[i].mActions.empty()) || (!mIsAction && mEventFile->mFlowcharts[0].mActors[i].mQueries.empty()))
						ImGui::BeginDisabled();
					if (ImGui::Selectable(mActorNames[i], IsSelected))
					{
						*mActorIndex = i;
						*mActionQueryIndex = 0;
						CalculateActionQueryNames();
						if (IsSelected)
							ImGui::SetItemDefaultFocus();

						RebuildNodeParameters();
					}
					if ((mIsAction && mEventFile->mFlowcharts[0].mActors[i].mActions.empty()) || (!mIsAction && mEventFile->mFlowcharts[0].mActors[i].mQueries.empty()))
						ImGui::EndDisabled();
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Value##PopupModifyNodeActionQueryValue", (*mActorIndex >= 0 && *mActionQueryIndex >= 0) ? mActionQueryNames[*mActionQueryIndex] : "Undefined"))
			{
				for (size_t i = 0; i < mActionQueryNames.size(); i++)
				{
					bool IsSelected = (i == *mActionQueryIndex);
					if (ImGui::Selectable(mActionQueryNames[i], IsSelected))
					{
						*mActionQueryIndex = i;
						if (IsSelected)
							ImGui::SetItemDefaultFocus();

						RebuildNodeParameters();
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				mActorIndex = nullptr;
				mActionQueryIndex = nullptr;
				mEventFile = nullptr;
				mActorNames.clear();
				mActionQueryNames.clear();
			}
		}
		ImGui::EndPopup();
	}
}

void PopupModifyNodeActionQuery::RebuildNodeParameters()
{
	for (EventNodeDefMgr::NodeDef& Def : EventNodeDefMgr::mEventNodes)
	{
		if (Def.mActorName == mEventFile->mFlowcharts[0].mActors[*mActorIndex].mName)
		{
			if ((mIsAction && Def.mEventName == mEventFile->mFlowcharts[0].mActors[*mActorIndex].mActions[*mActionQueryIndex]) || (!mIsAction && Def.mEventName == mEventFile->mFlowcharts[0].mActors[*mActorIndex].mQueries[*mActionQueryIndex]))
			{
				mParameters->mItems.clear();
				for (auto& [Type, Key] : Def.mParameters)
				{
					switch (Type)
					{
					case BFEVFLFile::ContainerDataType::ActorIdentifier:
					case BFEVFLFile::ContainerDataType::Argument:
					case BFEVFLFile::ContainerDataType::Container:
					{
						Logger::Warning("PopUpModifyNodeActionQuery", "Parameter type not implemented");
						break;
					}
					case BFEVFLFile::ContainerDataType::Int:
					{
						BFEVFLFile::ContainerItem Item;
						Item.mCount = 1;
						Item.mType = Type;
						Item.mData = (int32_t)0;
						mParameters->mItems.push_back(std::make_pair(Key, Item));
						break;
					}
					case BFEVFLFile::ContainerDataType::Bool:
					{
						BFEVFLFile::ContainerItem Item;
						Item.mCount = 1;
						Item.mType = Type;
						Item.mData = false;
						mParameters->mItems.push_back(std::make_pair(Key, Item));
						break;
					}
					case BFEVFLFile::ContainerDataType::Float:
					{
						BFEVFLFile::ContainerItem Item;
						Item.mCount = 1;
						Item.mType = Type;
						Item.mData = 0.0f;
						mParameters->mItems.push_back(std::make_pair(Key, Item));
						break;
					}
					case BFEVFLFile::ContainerDataType::WString:
					case BFEVFLFile::ContainerDataType::String:
					{
						BFEVFLFile::ContainerItem Item;
						Item.mCount = 1;
						Item.mType = Type;
						Item.mData = "";
						mParameters->mItems.push_back(std::make_pair(Key, Item));
						break;
					}
					case BFEVFLFile::ContainerDataType::IntArray:
					{
						BFEVFLFile::ContainerItem Item;
						Item.mCount = 0;
						Item.mType = Type;
						Item.mData = std::vector<int32_t>();
						mParameters->mItems.push_back(std::make_pair(Key, Item));
						break;
					}
					case BFEVFLFile::ContainerDataType::BoolArray:
					{
						BFEVFLFile::ContainerItem Item;
						Item.mCount = 0;
						Item.mType = Type;
						Item.mData = std::deque<bool>();
						mParameters->mItems.push_back(std::make_pair(Key, Item));
						break;
					}
					case BFEVFLFile::ContainerDataType::FloatArray:
					{
						BFEVFLFile::ContainerItem Item;
						Item.mCount = 0;
						Item.mType = Type;
						Item.mData = std::vector<float>();
						mParameters->mItems.push_back(std::make_pair(Key, Item));
						break;
					}
					case BFEVFLFile::ContainerDataType::WStringArray:
					case BFEVFLFile::ContainerDataType::StringArray:
					{
						BFEVFLFile::ContainerItem Item;
						Item.mCount = 0;
						Item.mType = Type;
						Item.mData = std::vector<std::string>();
						mParameters->mItems.push_back(std::make_pair(Key, Item));
						break;
					}
					default:
						break;
					}
				}

				break;
			}
		}
	}
}

void PopupModifyNodeActionQuery::CalculateActorNames()
{
	mActorNames.clear();
	for (BFEVFLFile::Actor& Actor : mEventFile->mFlowcharts[0].mActors)
	{
		mActorNames.push_back(Actor.mName.c_str());
	}
}

void PopupModifyNodeActionQuery::CalculateActionQueryNames()
{
	mActionQueryNames.clear();
	if (*mActorIndex < 0)
		return;

	if (mIsAction)
	{
		for (std::string& Action : mEventFile->mFlowcharts[0].mActors[*mActorIndex].mActions)
		{
			mActionQueryNames.push_back(Action.c_str());
		}
	}
	else
	{
		for (std::string& Query : mEventFile->mFlowcharts[0].mActors[*mActorIndex].mQueries)
		{
			mActionQueryNames.push_back(Query.c_str());
		}
	}
}

void PopupModifyNodeActionQuery::Open(int16_t& ActorIndex, int16_t& ActionQueryIndex, BFEVFLFile::Container& Parameters, bool IsAction, BFEVFLFile& EventFile)
{
	IsOpen = true;

	mActorIndex = &ActorIndex;
	mActionQueryIndex = &ActionQueryIndex;
	mEventFile = &EventFile;
	mIsAction = IsAction;
	mParameters = &Parameters;

	CalculateActorNames();
	CalculateActionQueryNames();
}