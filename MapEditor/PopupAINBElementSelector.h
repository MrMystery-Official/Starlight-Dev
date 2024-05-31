#pragma once

#include "AINB.h"
#include <string>

namespace PopupAINBElementSelector {

extern bool IsOpen;
extern std::string Key;
extern AINBFile::Node* Node;
extern void (*Func)(AINBFile::Node*, std::string);
extern std::string PopupTitle;
extern std::string Value;
extern std::string ConfirmButtonText;

void Render();
void Open(std::string Title, std::string TextFieldName, std::string ButtonText, AINBFile::Node* NodePtr, void (*Callback)(AINBFile::Node*, std::string));

}
