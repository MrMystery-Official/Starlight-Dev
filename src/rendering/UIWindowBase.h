#pragma once

#include <string>

namespace application::rendering
{
	class UIWindowBase
	{
	public:
		enum class WindowType : uint8_t
		{
			EDITOR_AINB = 0,
			EDITOR_COLLISION = 1,
			EDITOR_MAP = 2,
			EDITOR_ACTOR = 3,
			GENERAL_CONTENT_BROWSER = 4,
			EDITOR_PLUGINS = 5
		};

		virtual ~UIWindowBase() = default;

		void Initialize();
		void Draw();
		void Delete();
		void UpdateSameWindowCount();

		virtual bool SupportsProjectChange();
		virtual WindowType GetWindowType() = 0;

		std::string CreateID(const std::string& Id);

	private:
		virtual void InitializeImpl() = 0;
		virtual void DrawImpl() = 0;
		virtual void DeleteImpl() = 0;

	public:
		bool mOpen = true;
		bool mFirstFrame = true;
		unsigned int mWindowId = 0;
		unsigned int mSameWindowTypeCount = 0;
	};
}