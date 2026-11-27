#include "../../src/app/app_09/infrared_saved_signal.h"
#include "../../src/app/app_09/infrared_native_learn.h"
#include "../../src/app/app_09/infrared_power.h"
#include "../../src/system/sd_write_file.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

static std::vector<meow_ir::Signal> reload(const std::string& path) {
    std::ifstream file(path); std::string line;
    meow_ir::RecordParser parser; bool started=false;
    std::vector<meow_ir::Signal> result;
    while (std::getline(file,line)) {
        const auto colon=line.find(':'); if (colon==std::string::npos) continue;
        const auto key=line.substr(0,colon); auto value=line.substr(colon+1);
        value.erase(0,value.find_first_not_of(" \t"));
        if (key=="name") {
            if (started) { assert(parser.valid()); result.push_back(parser.signal); }
            parser={}; started=true;
        }
        if (started) parser.field(key,value);
    }
    assert(started && parser.valid()); result.push_back(parser.signal); return result;
}
int main() {
    using namespace meow_ir;
    Signal nativeNec; nativeNec.name="TV Power";
    assert(learnNative(nativeNec,"NEC",32,0x20DF10EF));
    const auto parseNative=[](const Signal& signal) {
        RecordParser parser; std::istringstream stream(serialize(signal)); std::string line;
        while(std::getline(stream,line)) {
            const auto colon=line.find(':'); if(colon==std::string::npos) continue;
            auto value=line.substr(colon+1); value.erase(0,value.find_first_not_of(" \t"));
            parser.field(line.substr(0,colon),value);
        }
        assert(parser.valid()); return parser.signal;
    };
    auto reloadedNative=parseNative(nativeNec);
    assert(reloadedNative.protocol==InfraredProtocolUnknown && reloadedNative.legacyProtocol=="NEC");
    assert(reloadedNative.legacyValue==0x20DF10EF && reloadedNative.legacyBits==32);
    Signal on,off,power; on.name="On"; off.name="Off"; std::string powerError;
    assert(learnNative(on,"COOLIX",24,0xB23F30) && learnNative(off,"COOLIX",24,0xB27BE0));
    assert(makePowerPair(on,off,power,powerError)); power.name="Power";
    assert(power.powerOff && power.powerNextOn && validSignal(power));
    assert(!makePowerPair(on,on,reloadedNative,powerError));
    auto loadedPower=parseNative(power);
    assert(loadedPower.powerOff && loadedPower.legacyValue==0xB23F30 && loadedPower.powerOff->legacyValue==0xB27BE0);
    PowerPress press;
    assert(press.begin(loadedPower,false,false)==nullptr); // held key without fresh edge
    assert(press.begin(loadedPower,true,false)==&loadedPower);
    assert(press.begin(loadedPower,true,false)==nullptr); // pending frame cannot toggle again
    assert(press.finish(loadedPower,true) && !loadedPower.powerNextOn);
    loadedPower=parseNative(loadedPower); // reopening preserves next command
    assert(!loadedPower.powerNextOn && loadedPower.powerOff);
    assert(press.begin(loadedPower,true,false)==loadedPower.powerOff.get());
    assert(!press.finish(loadedPower,false) && !loadedPower.powerNextOn); // failed TX never advances
    assert(press.begin(loadedPower,true,false)==loadedPower.powerOff.get());
    assert(press.finish(loadedPower,true) && loadedPower.powerNextOn);
    assert(press.begin(loadedPower,true,true)==nullptr); // another TX owns worker
    RecordParser incompletePair;
    incompletePair.field("name","Broken"); incompletePair.field("type","raw");
    incompletePair.field("frequency","38000"); incompletePair.field("duty_cycle","0.33"); incompletePair.field("data","9000 4500");
    incompletePair.field("power_toggle","1"); incompletePair.field("power_next","on");
    assert(!incompletePair.valid());
    RecordParser nestedPair; nestedPair.field("off_off_name","Nested"); assert(!nestedPair.ok || !nestedPair.valid());
    for(const auto& name:{"SONY","RC5","SAMSUNG","LG","PANASONIC","MIDEA"}) {
        Signal native; native.name="Power";
        assert(learnNative(native,name,64,UINT64_MAX));
        auto reopened=parseNative(native);
        assert(reopened.protocol==InfraredProtocolUnknown && reopened.legacyProtocol==name);
        assert(reopened.legacyValue==UINT64_MAX && reopened.legacyBits==64);
    }
    uint8_t acState[27]{}; for(size_t i=0;i<sizeof(acState);++i) acState[i]=uint8_t(i*7);
    Signal ac; ac.name="AC On";
    assert(learnNative(ac,"DAIKIN",216,0,acState,sizeof(acState)));
    acState[0]=0xff; // snapshot must own its state, independent of RX buffer reuse
    const auto reopenedAc=parseNative(ac);
    assert(reopenedAc.legacyProtocol=="DAIKIN" && reopenedAc.legacyBits==216);
    assert(reopenedAc.legacyState.size()==27 && reopenedAc.legacyState[0]==0 && reopenedAc.legacyState[26]==182);
    assert(!learnNative(ac,"DAIKIN",216,0,acState,26));
    assert(!learnNative(ac,"DAIKIN",216,0,nullptr,27));
    assert(!learnNative(ac,"UNKNOWN",24,0) && !learnNative(ac,"NEC",0,0));
    Signal truncatedAc=reopenedAc; truncatedAc.legacyState.pop_back(); assert(!validSignal(truncatedAc));
    RecordParser malformedState;
    malformedState.field("state","00 GG"); assert(!malformedState.ok);
    RecordParser shortState; shortState.field("state","0"); assert(!shortState.ok);
    RecordParser reordered;
    reordered.field("name","TV"); reordered.field("type","parsed"); reordered.field("protocol","NEC");
    reordered.field("encoding","native"); reordered.field("address","00 00 00 00"); reordered.field("command","00 00 00 00");
    reordered.field("bits","32"); reordered.field("value","0x20DF10EF");
    assert(reordered.valid() && reordered.signal.protocol==InfraredProtocolUnknown && reordered.signal.legacyValue==0x20DF10EF);
    RecordParser missingValue;
    missingValue.field("name","Broken"); missingValue.field("type","parsed"); missingValue.field("encoding","native");
    missingValue.field("protocol","NEC"); missingValue.field("bits","32");
    missingValue.field("address","00 00 00 00"); missingValue.field("command","00 00 00 00");
    assert(!missingValue.valid());
    Signal coolix; coolix.name="AC Off"; coolix.raw=true; coolix.timings={4428,4464,530,1614};
    assert(learnNative(coolix,"COOLIX",24,0xB27BE0));
    assert(!coolix.raw && coolix.timings.empty() && coolix.legacyProtocol=="COOLIX");
    assert(coolix.legacyBits==24 && coolix.legacyValue==0xB27BE0 && validSignal(coolix));
    RecordParser parsedCoolix;
    std::istringstream coolixText(serialize(coolix)); std::string coolixLine;
    while (std::getline(coolixText,coolixLine)) {
        const auto colon=coolixLine.find(':'); if(colon==std::string::npos) continue;
        auto value=coolixLine.substr(colon+1); value.erase(0,value.find_first_not_of(" \t"));
        parsedCoolix.field(coolixLine.substr(0,colon),value);
    }
    assert(parsedCoolix.valid() && parsedCoolix.signal.legacyValue==0xB27BE0 && parsedCoolix.signal.legacyBits==24);
    assert(!learnNative(coolix,"COOLIX",24,0x1000000));
    Signal selected; selected.name="Power"; selected.protocol=InfraredProtocolNEC;
    selected.address=7; selected.command=0xa2;
    std::vector<Signal> remote; std::string error;
    assert(appendSavedButton({},selected,"Power",remote,error));
    assert(remote.size()==1 && remote[0].address==7 && remote[0].command==0xa2);
    selected.command=0x14; // later scan changes must not change the saved signal
    assert(remote[0].command==0xa2);
    std::vector<Signal> combined;
    assert(appendSavedButton(remote,selected,"Volume Up",combined,error));
    assert(combined.size()==2 && combined[0].command==0xa2 && combined[1].command==0x14);
    const auto before=combined;
    assert(!appendSavedButton(remote,selected,"POWER",combined,error));
    assert(combined.size()==before.size() && combined[0].command==before[0].command);
    assert(!appendSavedButton(remote,selected,"bad\nname",combined,error));
    std::vector<Signal> full(MaxButtons,selected);
    assert(!appendSavedButton(full,selected,"Another",combined,error));
    char directory[]="/tmp/meowkit-ir-save-XXXXXX"; assert(mkdtemp(directory));
    meow_storage::WriteFile file;
    assert(file.open(directory,"TV.ir"));
    const std::string header="Filetype: IR signals file\nVersion: 1\n";
    assert(file.write(reinterpret_cast<const uint8_t*>(header.data()),header.size())==header.size());
    for (const auto& signal:before) {
        const auto bytes=serialize(signal);
        assert(file.write(reinterpret_cast<const uint8_t*>(bytes.data()),bytes.size())==bytes.size());
    }
    assert(file.sync() && file.close());
    const auto saved=reload(file.path());
    assert(saved.size()==2 && saved[0].name=="Power" && saved[1].name=="Volume Up");
    assert(saved[0].protocol==InfraredProtocolNEC && saved[0].command==0xa2 && saved[1].command==0x14);
    assert(file.open(directory,"AC.ir"));
    const auto acBytes=header+serialize(reopenedAc)+serialize(nativeNec)+serialize(coolix)+serialize(loadedPower);
    assert(file.write(reinterpret_cast<const uint8_t*>(acBytes.data()),acBytes.size())==acBytes.size());
    assert(file.sync() && file.close());
    const auto acSaved=reload(file.path());
    assert(acSaved.size()==4 && acSaved[0].legacyState==reopenedAc.legacyState);
    assert(acSaved[1].legacyProtocol=="NEC" && acSaved[1].legacyValue==0x20DF10EF);
    assert(acSaved[2].legacyProtocol=="COOLIX" && acSaved[2].legacyValue==0xB27BE0);
    assert(acSaved[3].powerOff && acSaved[3].powerNextOn && acSaved[3].powerOff->legacyValue==0xB27BE0);
    Signal raw; raw.name="Power"; raw.raw=true; raw.timings={9000,4500,560,1690};
    std::vector<Signal> rawSaved;
    assert(appendSavedButton({},raw,"Raw Power",rawSaved,error));
    assert(rawSaved[0].timings==raw.timings && rawSaved[0].frequency==raw.frequency);
    Signal rawOff=raw; rawOff.timings[1]=4600;
    assert(makePowerPair(raw,rawOff,power,powerError));
    const auto rawPair=parseNative(power); assert(rawPair.raw && rawPair.powerOff->timings==rawOff.timings);
    Signal acOff=reopenedAc; acOff.legacyState[0]^=1;
    assert(makePowerPair(reopenedAc,acOff,power,powerError));
    const auto acPair=parseNative(power); assert(acPair.powerOff->legacyState==acOff.legacyState);
    Signal invalid; invalid.name="Invalid";
    assert(!appendSavedButton({},invalid,"Invalid",combined,error));
    std::filesystem::remove_all(directory);
    puts("IR save checks passed (exact code, append, duplicate protection, raw data, saved-file reload)");
}
