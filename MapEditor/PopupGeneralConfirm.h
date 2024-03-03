#include <string>

namespace PopupGeneralConfirm
{
	extern bool IsOpen;
	extern std::string Text;
	extern void (*Func)();

	void Render();
	void Open(std::string Message, void (*Callback)());
};