#include "../../src/app/app_09/infrared_native_learn.h"
#include <cassert>
int main() {
    for(const auto mask:{uint64_t(0x800),uint64_t(0x10000),uint64_t(0x8000)}) {
        meow_ir::NativeToggleState toggle;
        const uint64_t original=0x23;
        const auto first=toggle.apply(original,mask,false);
        assert((first&mask)!=(original&mask));
        assert(toggle.apply(original,mask,true)==first);
        const auto second=toggle.apply(original,mask,false);
        assert((second^first)==mask);
        assert(toggle.apply(original,mask,true)==second);
    }
    puts("Native toggle checks passed (new presses flip, held repeats stay stable)");
}
