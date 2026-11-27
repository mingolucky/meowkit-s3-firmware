#include "../../src/app/app_09/infrared_library_file.h"
#include "../../src/system/storage_directory.h"
#include "../../src/system/sd_write_file.h"
#include <cassert>
#include <cstdio>

int main() {
    for (const char* name:{"tv","projectors","audio","ac","leds","fans","bluray_dvd","monitor","digital_sign"}) {
        meow_ir::LibraryFile file;
        const String path((std::string("builtin:")+name).c_str());
        assert(file.open(path) && !file.isDirectory() && file.size()>0);
        assert(file.readStringUntil('\n').find("IR library file")!=std::string::npos);
        size_t count=0,last=0; String expected;
        while (file.available()) {
            const auto at=file.position(); auto line=file.readStringUntil('\n');
            if (line.startsWith("name:")) { ++count; last=at; expected=line; }
        }
        assert(count>0 && file.position()==file.size());
        assert(file.seek(last) && file.readStringUntil('\n')==expected);
        assert(!file.seek(file.size()+1));
        file.close(); assert(!file);
        printf("Universal %s: %zu records indexed and seek verified\n",name,count);
    }
    meow_ir::LibraryFile missing; assert(!missing.open("builtin:missing"));
    char directory[]="/tmp/meowkit-ir-storage-XXXXXX"; assert(mkdtemp(directory));
    SD_MMC.root=directory;
    std::string error;
    assert(meow_storage::ensureDirectory(SD_MMC,"/infrared/remote",error));
    assert(meow_storage::ensureDirectory(SD_MMC,"/infrared/remote",error));
    {
        std::ofstream custom(std::string(directory)+"/infrared/custom.ir"); custom<<"CUSTOM\nname: My Button\n";
        std::ofstream blocked(std::string(directory)+"/blocked"); blocked<<"PRESERVE";
    }
    meow_ir::LibraryFile custom; assert(custom.open("/infrared/custom.ir"));
    assert(custom.readStringUntil('\n')=="CUSTOM"); custom.close();
    assert(!meow_storage::ensureDirectory(SD_MMC,"/blocked/child",error));
    assert(std::filesystem::is_regular_file(std::string(directory)+"/blocked"));
    char probeError[96]{};
    assert(meow_storage::probe(directory,"unique-probe.tmp",probeError,sizeof(probeError)));
    assert(!std::filesystem::exists(std::string(directory)+"/unique-probe.tmp"));
    meow_storage::WriteFile exclusive;
    assert(!exclusive.open(directory,"blocked") && exclusive.errorCode()==EEXIST);
    std::ifstream preserved(std::string(directory)+"/blocked"); std::string value; preserved>>value; assert(value=="PRESERVE");
    assert(!meow_storage::probe((std::string(directory)+"/blocked").c_str(),"probe",probeError,sizeof(probeError)));
    assert(probeError[0]);
    std::filesystem::remove_all(directory);
    puts("IR storage checks passed (recursive folders, flash fallback, SD override, checked write and preservation)");
}
