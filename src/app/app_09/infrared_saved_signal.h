#pragma once
#include "infrared_signal.h"
#include <strings.h>
namespace meow_ir {
inline bool appendSavedButton(const std::vector<Signal>& current,const Signal& selected,
                              const std::string& name,std::vector<Signal>& result,std::string& error) {
    if (current.size()>=MaxButtons) { error="Remote full (256 buttons)"; return false; }
    if (!infrared_file::validButtonName(name.c_str()) || name.size()>MaxName) {
        error="Invalid button name"; return false;
    }
    for (const auto& button:current) if (!strcasecmp(button.name.c_str(),name.c_str())) {
        error="Button name exists; choose another"; return false;
    }
    Signal copy=selected; copy.name=name;
    if (!validSignal(copy)) { error="Invalid library signal"; return false; }
    result=current; result.push_back(std::move(copy)); return true;
}
}
