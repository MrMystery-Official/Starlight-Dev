#include "PopupAddDynamicData.h"

#include "imgui.h"
#include "imgui_stdlib.h"
#include "UIMapView.h"
#include "Logger.h"
#include "StarImGui.h"

bool PopupAddDynamicData::IsOpen = false;
std::string PopupAddDynamicData::Key = "";
int PopupAddDynamicData::DataType = 0;
std::variant<std::string, bool, int32_t, int64_t, uint32_t, uint64_t, float, Vector3F> PopupAddDynamicData::Value = "";
void (*PopupAddDynamicData::Func)(std::string, Actor::DynamicData) = nullptr;

float PopupAddDynamicData::SizeX = 334.0f;
float PopupAddDynamicData::SizeY = 126.0f;
const float PopupAddDynamicData::OriginalSizeX = 334.0f;
const float PopupAddDynamicData::OriginalSizeY = 126.0f;

void PopupAddDynamicData::UpdateSize(float Scale)
{
	SizeX = OriginalSizeX * Scale;
	SizeY = OriginalSizeY * Scale;
}

void PopupAddDynamicData::Render()
{
	if (IsOpen)
	{
		UIMapView::RenderSettings.AllowSelectingActor = false;
		ImGui::SetNextWindowSize(ImVec2(SizeX, SizeY));
		ImGui::OpenPopup("Add dynamic data");
		if (ImGui::BeginPopupModal("Add dynamic data", NULL, ImGuiWindowFlags_NoResize))
		{
			const char* DataTypes[]{ "String", "Bool", "S32", "S64", "U32", "U64", "Float", "Vec3f"};
			if (ImGui::Combo("Data type", &DataType, DataTypes, IM_ARRAYSIZE(DataTypes)))
			{
				switch (DataType)
				{
				case 0:
					Value = "";
					break;
				case 1:
					Value = false;
					break;
				case 2:
					Value = (int32_t)0;
					break;
				case 3:
					Value = (int64_t)0;
					break;
				case 4:
					Value = (uint32_t)0;
					break;
				case 5:
					Value = (uint64_t)0;
					break;
				case 6:
					Value = (float)0;
					break;
				case 7:
					Value = Vector3F(0, 0, 0);
					break;
				default:
					Logger::Error("PopupAddDynamicData", "Invalid dynamic data type");
					break;
				}
			}

			ImGui::InputText("Key", &Key);

			switch (DataType)
			{
			case 0:
				ImGui::InputText("Value", reinterpret_cast<std::string*>(&Value));
				break;
			case 1:
				ImGui::Checkbox("Value", reinterpret_cast<bool*>(&Value));
				break;
			case 2:
				ImGui::InputScalar("Value", ImGuiDataType_S32, reinterpret_cast<int32_t*>(&Value));
				break;
			case 3:
				ImGui::InputScalar("Value", ImGuiDataType_S64, reinterpret_cast<int64_t*>(&Value));
				break;
			case 4:
				ImGui::InputScalar("Value", ImGuiDataType_U32, reinterpret_cast<uint32_t*>(&Value));
				break;
			case 5:
				ImGui::InputScalar("Value", ImGuiDataType_U64, reinterpret_cast<uint64_t*>(&Value));
				break;
			case 6:
				ImGui::InputScalar("Value", ImGuiDataType_Float, reinterpret_cast<float*>(&Value));
				break;
			case 7:
				StarImGui::InputFloat3Colored("Value", reinterpret_cast<Vector3F*>(&Value)->GetRawData(), ImVec4(0.26f, 0.06f, 0.07f, 1.0f), ImVec4(0.06f, 0.26f, 0.07f, 1.0f), ImVec4(0.06f, 0.07f, 0.26f, 1.0f), true);
				break;
			default:
				Logger::Error("PopupAddDynamicData", "Invalid dynamic data type");
				break;
			}

			if (ImGui::Button("Add"))
			{
				BymlFile::Type BymlType = BymlFile::Type::StringIndex;
				switch (DataType)
				{
				case 0:
					BymlType = BymlFile::Type::StringIndex;
					break;
				case 1:
					BymlType = BymlFile::Type::Bool;
					break;
				case 2:
					BymlType = BymlFile::Type::Int32;
					break;
				case 3:
					BymlType = BymlFile::Type::Int64;
					break;
				case 4:
					BymlType = BymlFile::Type::UInt32;
					break;
				case 5:
					BymlType = BymlFile::Type::UInt64;
					break;
				case 6:
					BymlType = BymlFile::Type::Float;
					break;
				case 7:
					BymlType = BymlFile::Type::Array;
					break;
				default:
					Logger::Error("PopupAddDynamicData", "Invalid dynamic data type");
					break;
				}

				Func(Key, Actor::DynamicData{
					.Data = Value,
					.Type = BymlType
					});

				IsOpen = false;
				Func = nullptr;
				Key = "";
				Value = "";
				DataType = 0;
			}
			ImGui::SameLine();
			if (ImGui::Button("Return"))
			{
				IsOpen = false;
				Func = nullptr;
				Key = "";
				Value = "";
				DataType = 0;
			}
		}
		//ImGui::SameLine();
		//ImGui::Text(std::string("Size: " + std::to_string(ImGui::GetWindowSize().x) + "x" + std::to_string(ImGui::GetWindowSize().y)).c_str());
		ImGui::EndPopup();
	}
}

void PopupAddDynamicData::Open(void (*Callback)(std::string, Actor::DynamicData))
{
	Func = Callback;
	Key = "";
	Value = "";
	DataType = 0;
	IsOpen = true;
}