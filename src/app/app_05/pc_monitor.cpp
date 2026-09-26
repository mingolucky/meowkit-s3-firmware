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
static uint32_t s_lastFrameMs = 0; // Date de la dernière trame complète reçue.

/* L'hôte émet une trame par seconde. Au-delà de ce délai sans rien recevoir, on
 * considère qu'il n'y a plus personne : câble débranché, service arrêté, PC
 * éteint. Trois secondes tolèrent une trame manquée sans clignoter. */
#define PCMON_STALE_MS 3000

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

  /*CPU Core Freq*/
    {
        String mhz = cpuClockString + String("MHz");
        lv_label_set_text(ui_mhz, mhz.c_str());
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


/* Remet l'écran à l'état « aucune donnée » : libellés vides et jauges à zéro.
 * Un libellé vide dit « rien à afficher » ; un « 0 » affirmerait une mesure,
 * ce qui serait faux — un CPU n'est pas à 0 °C parce que le câble est débranché. */
static void pc_monitor_blank()
{
    lv_label_set_text(ui_cpu_name, "");
    lv_label_set_text(ui_gpu_name, "");
    lv_label_set_text(ui_cpu_temp, "");
    lv_label_set_text(ui_cpu_percent, "");
    lv_label_set_text(ui_gpu_temp, "");
    lv_label_set_text(ui_gpu_percent, "");
    lv_label_set_text(ui_RAM, "");
    lv_label_set_text(ui_mhz, "");
    lv_label_set_text(ui_gpu_ram, "");
    ui_pc_monitor_set_gauges(0, 0, 0, 0);
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

        pc_monitor_blank();
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
            s_lastFrameMs = millis();
            style_01(_device);
            inputString = "";
            stringComplete = false;
        }
        else if (s_hasData && (millis() - s_lastFrameMs) > PCMON_STALE_MS) {
            /* Plus rien n'arrive : effacer plutôt que laisser des valeurs
             * périmées, qui ont l'apparence de mesures vivantes. */
            s_hasData = false;
            pc_monitor_blank();
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
