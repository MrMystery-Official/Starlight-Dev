#pragma once

#include "FixedSizeStack.h"
#include "imgui.h"
#include <variant>
#include "Vector3F.h"

namespace ActionMgr
{
	enum class ActionDataType : uint8_t
	{
		U32 = 0,
		S32 = 1,
		U64 = 2,
		S64 = 3,
		FLOAT = 4,
		DOUBLE = 5,
		U8 = 6,
		S8 = 7,
		U16 = 8,
		S16 = 9,
		VEC3F = 10
	};

	struct Action
	{
		void* mDataPtr;
		ActionDataType mDataType;
		std::variant<uint32_t, int32_t, uint64_t, int64_t, float, double, uint8_t, int8_t, uint16_t, int16_t, Vector3F> mValue;
	};

	extern FixedSizeStack<Action> mUndoActions;
	extern FixedSizeStack<Action> mRedoActions;

	template <typename T>
	void AddAction(void* DataPtr, T Value)
	{
		ActionDataType Type;
		if (std::is_same<T, uint32_t>::value)
		{
			Type = ActionDataType::U32;
		}
		else if (std::is_same<T, int32_t>::value)
		{
			Type = ActionDataType::S32;
		}
		else if (std::is_same<T, int64_t>::value)
		{
			Type = ActionDataType::S64;
		}
		else if (std::is_same<T, uint64_t>::value)
		{
			Type = ActionDataType::U64;
		}
		else if (std::is_same<T, float>::value)
		{
			Type = ActionDataType::FLOAT;
		}
		else if (std::is_same<T, double>::value)
		{
			Type = ActionDataType::DOUBLE;
		}
		else if (std::is_same<T, uint8_t>::value)
		{
			Type = ActionDataType::U8;
		}
		else if (std::is_same<T, int8_t>::value)
		{
			Type = ActionDataType::S8;
		}
		else if (std::is_same<T, uint16_t>::value)
		{
			Type = ActionDataType::U16;
		}
		else if (std::is_same<T, int16_t>::value)
		{
			Type = ActionDataType::S16;
		}
		else if (std::is_same<T, Vector3F>::value)
		{
			Type = ActionDataType::VEC3F;
		}
		mUndoActions.push(Action{
			.mDataPtr = DataPtr,
			.mDataType = Type,
			.mValue = Value
			});
	}
	void AddAction(void* DataPtr, ImGuiDataType DataType, void* Value);

	void Undo();
	void Redo();
}