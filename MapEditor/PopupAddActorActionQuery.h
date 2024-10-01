#pragma once

#include <string>
#include <functional>

namespace PopupAddActorActionQuery
{
	extern bool IsOpen;
	extern std::string mActorName;
	extern std::string mActionName;
	extern bool mIsAction;
	extern std::function<void(std::string)> mCallback;
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(std::string ActorName, bool IsAction, std::function<void(std::string)> Callback);
};