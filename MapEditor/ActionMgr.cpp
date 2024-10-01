#include "ActionMgr.h"

#include "Logger.h"
#include <iostream>

FixedSizeStack<ActionMgr::Action> ActionMgr::mUndoActions(100);
FixedSizeStack<ActionMgr::Action> ActionMgr::mRedoActions(100);

void ActionMgr::AddAction(void* DataPtr, ImGuiDataType DataType, void* Value)
{	
	switch (DataType)
	{
	case ImGuiDataType_U32:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::U32,
			.mValue = *reinterpret_cast<uint32_t*>(Value)
		});
		break;
	case ImGuiDataType_U64:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::U64,
			.mValue = *reinterpret_cast<uint64_t*>(Value)
		});
		break;
	case ImGuiDataType_S32:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::S32,
			.mValue = *reinterpret_cast<int32_t*>(Value)
		});
		break;
	case ImGuiDataType_S64:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::S64,
			.mValue = *reinterpret_cast<int64_t*>(Value)
		});
		break;
	case ImGuiDataType_Float:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::FLOAT,
			.mValue = *reinterpret_cast<float*>(Value)
		});
		break;
	case ImGuiDataType_Double:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::DOUBLE,
			.mValue = *reinterpret_cast<double*>(Value)
		});
		break;
	case ImGuiDataType_U8:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::U8,
			.mValue = *reinterpret_cast<int64_t*>(Value)
			});
		break;
	case ImGuiDataType_S8:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::S8,
			.mValue = *reinterpret_cast<int64_t*>(Value)
			});
		break;
	case ImGuiDataType_S16:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::S16,
			.mValue = *reinterpret_cast<int64_t*>(Value)
			});
		break;
	case ImGuiDataType_U16:
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = ActionDataType::U16,
			.mValue = *reinterpret_cast<int64_t*>(Value)
			});
		break;
	default:
		Logger::Warning("ActionMgr", "Unsupported imgui data type: " + std::to_string(DataType));
		break;
	}
}

void ActionMgr::Undo()
{
	if (mUndoActions.empty())
		return;

	Action& Action = mUndoActions.top();

	switch (Action.mDataType)
	{
	case ActionDataType::U32:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<uint32_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<uint32_t*>(Action.mDataPtr) = std::get<uint32_t>(Action.mValue);
		break;
	case ActionDataType::S32:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<int32_t*>(Action.mDataPtr)
		});
		*reinterpret_cast<int32_t*>(Action.mDataPtr) = std::get<int32_t>(Action.mValue);
		break;
	case ActionDataType::U64:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<uint64_t*>(Action.mDataPtr)
		});
		*reinterpret_cast<uint64_t*>(Action.mDataPtr) = std::get<uint64_t>(Action.mValue);
		break;
	case ActionDataType::S64:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<int64_t*>(Action.mDataPtr)
		});
		*reinterpret_cast<int64_t*>(Action.mDataPtr) = std::get<int64_t>(Action.mValue);
		break;
	case ActionDataType::FLOAT:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<float*>(Action.mDataPtr)
		});
		*reinterpret_cast<float*>(Action.mDataPtr) = std::get<float>(Action.mValue);
		break;
	case ActionDataType::DOUBLE:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<double*>(Action.mDataPtr)
		});
		*reinterpret_cast<double*>(Action.mDataPtr) = std::get<double>(Action.mValue);
		break;
	case ActionDataType::U8:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<uint8_t*>(Action.mDataPtr)
		});
		*reinterpret_cast<uint8_t*>(Action.mDataPtr) = std::get<uint8_t>(Action.mValue);
		break;
	case ActionDataType::S8:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<int8_t*>(Action.mDataPtr)
		});
		*reinterpret_cast<int8_t*>(Action.mDataPtr) = std::get<int8_t>(Action.mValue);
		break;
	case ActionDataType::U16:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<uint16_t*>(Action.mDataPtr)
		});
		*reinterpret_cast<uint16_t*>(Action.mDataPtr) = std::get<uint16_t>(Action.mValue);
		break;
	case ActionDataType::S16:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<int16_t*>(Action.mDataPtr)
		});
		*reinterpret_cast<int16_t*>(Action.mDataPtr) = std::get<int16_t>(Action.mValue);
		break;
	case ActionDataType::VEC3F:
		mRedoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<Vector3F*>(Action.mDataPtr)
			});
		*reinterpret_cast<Vector3F*>(Action.mDataPtr) = std::get<Vector3F>(Action.mValue);
		break;
	default:
		break;
	}

	mUndoActions.pop();
}

void ActionMgr::Redo()
{
	if (mRedoActions.empty())
		return;

	Action& Action = mRedoActions.top();

	switch (Action.mDataType)
	{
	case ActionDataType::U32:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<uint32_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<uint32_t*>(Action.mDataPtr) = std::get<uint32_t>(Action.mValue);
		break;
	case ActionDataType::S32:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<int32_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<int32_t*>(Action.mDataPtr) = std::get<int32_t>(Action.mValue);
		break;
	case ActionDataType::U64:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<uint64_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<uint64_t*>(Action.mDataPtr) = std::get<uint64_t>(Action.mValue);
		break;
	case ActionDataType::S64:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<int64_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<int64_t*>(Action.mDataPtr) = std::get<int64_t>(Action.mValue);
		break;
	case ActionDataType::FLOAT:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<float*>(Action.mDataPtr)
			});
		*reinterpret_cast<float*>(Action.mDataPtr) = std::get<float>(Action.mValue);
		break;
	case ActionDataType::DOUBLE:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<double*>(Action.mDataPtr)
			});
		*reinterpret_cast<double*>(Action.mDataPtr) = std::get<double>(Action.mValue);
		break;
	case ActionDataType::U8:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<uint8_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<uint8_t*>(Action.mDataPtr) = std::get<uint8_t>(Action.mValue);
		break;
	case ActionDataType::S8:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<int8_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<int8_t*>(Action.mDataPtr) = std::get<int8_t>(Action.mValue);
		break;
	case ActionDataType::U16:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<uint16_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<uint16_t*>(Action.mDataPtr) = std::get<uint16_t>(Action.mValue);
		break;
	case ActionDataType::S16:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<int16_t*>(Action.mDataPtr)
			});
		*reinterpret_cast<int16_t*>(Action.mDataPtr) = std::get<int16_t>(Action.mValue);
		break;
	case ActionDataType::VEC3F:
		mUndoActions.push(ActionMgr::Action{
			.mDataPtr = Action.mDataPtr,
			.mDataType = Action.mDataType,
			.mValue = *reinterpret_cast<Vector3F*>(Action.mDataPtr)
			});
		*reinterpret_cast<Vector3F*>(Action.mDataPtr) = std::get<Vector3F>(Action.mValue);
		break;
	default:
		break;
	}

	mRedoActions.pop();
}