#include "UIWindowBase.h"

#include <manager/UIMgr.h>

namespace application::rendering
{
	void UIWindowBase::Initialize()
	{
		UpdateSameWindowCount();

		InitializeImpl();
	}

	void UIWindowBase::UpdateSameWindowCount()
	{
		mSameWindowTypeCount = 0;
		for (auto& Window : application::manager::UIMgr::gWindows)
		{
			if (Window->mWindowId == mWindowId)
			{
				break;
			}
			if (Window->GetWindowType() == GetWindowType())
			{
				mSameWindowTypeCount++;
			}
		}
	}

	void UIWindowBase::Draw()
	{
		if (!mOpen)
			return;

		DrawImpl();
		mFirstFrame = false;
	}

	void UIWindowBase::Delete()
	{
		DeleteImpl();
		mOpen = false;
	}

	bool UIWindowBase::SupportsProjectChange()
	{
		return false;
	}

	std::string UIWindowBase::CreateID(const std::string& Id)
	{
		return Id + "##" + std::to_string(mWindowId);
	}
}