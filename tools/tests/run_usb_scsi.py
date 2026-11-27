#!/usr/bin/env python3
"""Exercise the pinned SDK SCSI callback, patched by the production build hook."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'build_scripts'))
from usb_framework_fixes import patch_msc
parser=argparse.ArgumentParser()
parser.add_argument('--sdk-source',required=True,type=Path)
parser.add_argument('--cxx',default='g++')
args=parser.parse_args()
source=args.sdk_source.read_text()
a=source.index('int32_t tud_msc_read10_cb(')
b=source.index('USBMSC::USBMSC()',a)
callback=patch_msc(source[a:b])
try:
 patch_msc('unrecognized framework implementation')
except AssertionError:
 pass
else:
 raise AssertionError('Unknown framework must fail closed')
preamble='''#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#define log_v(...) ((void)0)
constexpr int SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL=0x1e;
constexpr int SCSI_SENSE_ILLEGAL_REQUEST=5;
constexpr int SCSI_SENSE_NOT_READY=2;
struct Lun {bool media_present;int32_t (*read)(uint32_t,uint32_t,void*,uint32_t);int32_t (*write)(uint32_t,uint32_t,uint8_t*,uint32_t);};
Lun msc_luns[1]={{true,nullptr,nullptr}};
int senses=0;
void tud_msc_set_sense(uint8_t,int key,int asc,int ascq){assert(((key==5 && asc==0x20) || (key==2 && asc==0x3a)) && ascq==0);++senses;}
'''
tests='''
int main(){uint8_t cmd[16]={};uint8_t buffer[64];memset(buffer,0xa5,sizeof(buffer));
for(uint8_t op:{uint8_t(0xff),uint8_t(0x9e),uint8_t(0x5a)}){
 cmd[0]=op;for(uint16_t size:{uint16_t(0),uint16_t(1),uint16_t(64)}){
 senses=0;assert(tud_msc_scsi_cb(0,cmd,buffer,size)==-1);assert(senses==1);
 for(auto byte:buffer)assert(byte==0xa5);
 }}
senses=0;cmd[0]=0x35;assert(tud_msc_scsi_cb(0,cmd,buffer,0)==0);assert(senses==0);
cmd[0]=0x1e;assert(tud_msc_scsi_cb(0,cmd,buffer,0)==0);assert(senses==0);
msc_luns[0].media_present=false;assert(tud_msc_scsi_cb(0,cmd,buffer,64)==-1);
assert(tud_msc_read10_cb(0,0,0,buffer,64)==-1);
assert(tud_msc_write10_cb(0,0,0,buffer,64)==-1);
msc_luns[0].media_present=true;
assert(tud_msc_read10_cb(0,0,0,buffer,64)==-1);
assert(tud_msc_write10_cb(0,0,0,buffer,64)==-1);
msc_luns[0].read=[](uint32_t lba,uint32_t offset,void* p,uint32_t n)->int32_t{assert(lba==7 && offset==32);memset(p,0x23,n);return n;};
msc_luns[0].write=[](uint32_t lba,uint32_t offset,uint8_t* p,uint32_t n)->int32_t{assert(lba==7 && offset==32 && p[0]==0x23);return n;};
assert(tud_msc_read10_cb(0,7,32,buffer,64)==64);
assert(tud_msc_write10_cb(0,7,32,buffer,64)==64);
puts("Pinned USB MSC checks passed (unsupported rejection, flush, removed-media failure, callback dispatch)");}
'''
with tempfile.TemporaryDirectory(prefix='meow-scsi-') as temp:
 cpp=Path(temp)/'scsi.cpp';binary=Path(temp)/'scsi'
 cpp.write_text('#include <initializer_list>\n'+preamble+callback+tests)
 subprocess.run([args.cxx,'-std=c++17','-Wall','-Wextra','-fsanitize=address,undefined',str(cpp),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
