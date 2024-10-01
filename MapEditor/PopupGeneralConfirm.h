#include <string>

namespace PopupGeneralConfirm
{
	extern bool IsOpen;
	extern std::string Text;
	extern void (*Func)();
	extern float SizeX;
	extern float SizeY;
	extern const float OriginalSizeX;
	extern const float OriginalSizeY;

	void UpdateSize(float Scale);

	void Render();
	void Open(std::string Message, void (*Callback)());
};