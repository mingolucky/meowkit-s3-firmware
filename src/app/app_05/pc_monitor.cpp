/**
 * @file pc_monitor.cpp
 * @author Mingo
 * @brief PC Monitor app implementation for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#include "pc_monitor.h"
#include "lvgl.h"

// Use LVGL UI assets (fonts + images + positioned widgets)
extern "C" {
#include "asset/ui.h"
}

String inputString = "";
bool stringComplete = false;
const int Serial0_eventDelay = 15;
static bool s_hasData = false; // The value will not be displayed until the first complete frame of data is received.

void style_01(DEVICES* _device) {
    //------------------------------------------- Update UI widgets (LVGL) ----------------------------------------------------//
    // Background and bars are already positioned by SquareLine (3 images + 2 fonts)

 //------------------------------------------- CPU & GPU name ----------------------------------------------------//
  String cpuName, gpuName;

  /* Extract CPU name */
  int cpuIndex = inputString.indexOf("CPU:");
  if (cpuIndex != -1) {
      cpuIndex += (inputString.indexOf("Intel", cpuIndex) != -1) ? 10 : 8; // Handling Intel offsets
      int gpuIndex = inputString.indexOf("GPU:");
      cpuName = (gpuIndex != -1) ? inputString.substring(cpuIndex, gpuIndex) : inputString.substring(cpuIndex);
  }

  /*  Extract GPU name */
  int gpuIndex = inputString.indexOf("GPU:");
  if (gpuIndex != -1) {
      gpuIndex += (inputString.indexOf("NVIDIA", gpuIndex) != -1) ? 18 : 8; // Dealing with NVIDIA offset
      int gpuEnd = inputString.indexOf("|", gpuIndex);
      gpuName = (gpuEnd != -1) ? inputString.substring(gpuIndex, gpuEnd) : inputString.substring(gpuIndex);
  }

    /* Display CPU & GPU names */
#ifdef Manual_cpuName
    lv_label_set_text(ui_cpu_name, set_CPUname);
#else
    lv_label_set_text(ui_cpu_name, cpuName.c_str());
#endif

#ifdef Manual_gpuName
    lv_label_set_text(ui_gpu_name, set_GPUname);
#else
    lv_label_set_text(ui_gpu_name, gpuName.c_str());
#endif

    //------------------------------------------ CPU Load/Temp -------------------------------------------------//

  /*CPU Display String*/
  int cpuStringStart = inputString.indexOf("C");
  int cpuDegree = inputString.indexOf("c");
  int cpuStringLimit = inputString.indexOf("|");
  String cpuString1 = inputString.substring(cpuStringStart + 1, cpuDegree);
  String cpuString2 = inputString.substring(cpuDegree + 1, cpuStringLimit - 1);

    /*CPU TEMPERATURE*/
    lv_label_set_text(ui_cpu_temp, cpuString1.c_str());
    /*CPU LOAD, ALL CORES*/
    lv_label_set_text(ui_cpu_percent, cpuString2.c_str());

  //------------------------------------------ GPU Load/Temp -------------------------------------------------//

  /*GPU Display String*/
  int gpuStringStart = inputString.indexOf("G", cpuStringLimit);
  int gpuDegree = inputString.indexOf("c", gpuStringStart);
  int gpuStringLimit = inputString.indexOf("|", gpuStringStart);
  String gpuString1 = inputString.substring(gpuStringStart + 1, gpuDegree);
  String gpuString2 = inputString.substring(gpuDegree + 1, gpuStringLimit - 1);

    /*GPU TEMPERATURE*/
    lv_label_set_text(ui_gpu_temp, gpuString1.c_str());
    /*GPU LOAD*/
    lv_label_set_text(ui_gpu_percent, gpuString2.c_str());

    /* Les quatre barres dégradées suivent désormais les valeurs affichées ;
     * elles montraient jusqu'ici le dégradé complet en permanence. */
    ui_pc_monitor_set_gauges(cpuString1.toInt(), cpuString2.toInt(),
                             gpuString1.toInt(), gpuString2.toInt());

  //----------------------------------------SYSTEM  RAM TOTAL---------------------------------------------------//
  /*SYSTEM RAM String*/
  int ramStringStart = inputString.indexOf("R", gpuStringLimit);
  int ramStringLimit = inputString.indexOf("|", ramStringStart);
  String ramString = inputString.substring(ramStringStart + 1 , ramStringLimit);


  /*SYSTEM RAM AVALABLE String*/
  int AramStringStart = inputString.indexOf("RA", ramStringLimit);
  int AramStringLimit = inputString.indexOf("|", AramStringStart);
  String AramString = inputString.substring(AramStringStart + 2 , AramStringLimit);

  /*SYSTEM RAM TOTAL String*/
  double intRam = atof(ramString.c_str());
  double intAram = atof(AramString.c_str());
  //double  intRamSum = intRam + intAram;
  float  intRamSum = intRam + intAram; //float to handle the decimal point when printed (intRamSum,0)

  /*RAM USED/TOTAL*/
    {
        char ramBuf[24];
        snprintf(ramBuf, sizeof(ramBuf), "%s/%.0fGB", ramString.c_str(), intRamSum);
        lv_label_set_text(ui_RAM, ramBuf);
    }



    //------------------------------------------ CPU Freq -------------------------------------------------//

  /*CPU Freq Display String*/
  int cpuCoreClockStart = inputString.indexOf("CHC") + 3;
  int cpuCoreClockEnd = inputString.indexOf("|", cpuCoreClockStart);
  String cpuClockString = inputString.substring(cpuCoreClockStart, cpuCoreClockEnd);

  /* Bloc « USED MEMORY » : l'en-tête imprimé dans l'image de fond annonce la
   * mémoire utilisée, mais c'est la fréquence CPU qui y était affichée. Le
   * libellé et la jauge verticale suivent désormais ce qui est écrit. La
   * fréquence reste parsée ci-dessus ; la maquette ne lui offre aucun bloc. */
    {
        int usedPct = (intRamSum > 0.0f) ? (int)((intRam * 100.0) / intRamSum) : 0;
        char memBuf[16];
        snprintf(memBuf, sizeof(memBuf), "%d", usedPct);
        lv_label_set_text(ui_mhz, memBuf);
        ui_pc_monitor_set_memory_gauge(usedPct);
    }

   //---------------------------------------------Total GPU Memory-----------------------------------------------------------

   int gpuMemoryStart = inputString.indexOf("GMT") + 3;
   int gpuMemoryEnd = inputString.indexOf("|", gpuMemoryStart);
   String gpuMemoryString = inputString.substring(gpuMemoryStart, gpuMemoryEnd);
 
   double totalGPUmem = atof(gpuMemoryString.c_str());
   double totalGPUmemSum = totalGPUmem / 1024;    // divide by 1024 to get the correct value
   float  totalGPUmemSumDP = totalGPUmemSum ;     // float to handle the decimal point when printed (totalGPUmemSumDP, 0)

    {
        char vramBuf[16];
#ifdef Manual_gpuRam
        snprintf(vramBuf, sizeof(vramBuf), "%s", set_GPUram);
#else
        snprintf(vramBuf, sizeof(vramBuf), "%.0fGB", totalGPUmemSumDP);
#endif
        lv_label_set_text(ui_gpu_ram, vramBuf);
    }
}


namespace MOONCAKE::APPS
{
    PCMonitor::PCMonitor(DEVICES* device)
        : _device(device)
    {
        setAppInfo().name = "PCMonitor";
    }

    void PCMonitor::onOpen()
    {
        // Initialize PC Monitor
        inputString.reserve(200); // Reserve string space
        s_hasData = false;        // Mark: No valid data has been received

        // Loading the LVGL interface (two fonts, three images)
        ui_pc_monitor_init();

        // Clear all numerical display when entering for the first time to avoid displaying old values ​​or placeholder text
        // CPU/GPU name
        lv_label_set_text(ui_cpu_name, "");
        lv_label_set_text(ui_gpu_name, "");
        // CPU temperature/occupancy
        lv_label_set_text(ui_cpu_temp, "");
        lv_label_set_text(ui_cpu_percent, "");
        // GPU temperature/occupancy
        lv_label_set_text(ui_gpu_temp, "");
        lv_label_set_text(ui_gpu_percent, "");
        // RAM used/total
        lv_label_set_text(ui_RAM, "");
        // CPU frequency
        lv_label_set_text(ui_mhz, "");
        // Total video memory
        lv_label_set_text(ui_gpu_ram, "");
    }

    void PCMonitor::onRunning()
    {
        // Update PC Monitor
        while (Serial.available()) {          
            char inChar = (char)Serial.read();  
            inputString += inChar;
            if (inChar == '|') {
                stringComplete = true;
                delay(Serial0_eventDelay);
            }
        }

        if (stringComplete) {
            s_hasData = true; // Receive the first complete frame of data
            style_01(_device);
            inputString = "";
            stringComplete = false;
        }
        // Let LVGL process tasks to refresh the screen
        lv_timer_handler();
        // Shorten delays to improve response and avoid long blocking times (more robust when host computer is disconnected)
        delay(200);
    }

    void PCMonitor::onClose()
    {
        // Cleanup PC Monitor
        _device->Lcd.fillScreen(TFT_BLACK);
    }
}
