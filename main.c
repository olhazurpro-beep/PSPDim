#include <pspkernel.h>
#include <pspdisplay_kernel.h>
#include <pspctrl.h>

// Модуль режима ядра
PSP_MODULE_INFO("NightShift", 0x1000, 1, 0); 
PSP_MAIN_THREAD_ATTR(0);

// Флаг состояния
int night_mode = 0; 

int nightshift_thread(SceSize args, void *argp) {
    SceCtrlData pad;
    int debounce = 0;

    while (1) {
        sceCtrlPeekBufferPositive(&pad, 1);

        // Активация: Зажать L + R + Стрелка Вниз
        if ((pad.Buttons & PSP_CTRL_LTRIGGER) &&
            (pad.Buttons & PSP_CTRL_RTRIGGER) &&
            (pad.Buttons & PSP_CTRL_DOWN)) {
            if (!debounce) {
                night_mode = !night_mode;
                debounce = 1;
            }
        } else {
            debounce = 0;
        }

        if (night_mode) {
            // Форсируем минимальную яркость (значения зависят от ревизии PSP)
            // 20 - экспериментальное значение ниже стандарта, 0 может выключить экран
            sceDisplaySetBrightness(20, 0); 
        }

        // Задержка 100мс для разгрузки CPU
        sceKernelDelayThread(100000); 
    }
    return 0;
}

int module_start(SceSize args, void *argp) {
    SceUID thid = sceKernelCreateThread("NightShiftThread", nightshift_thread, 8, 4096, 0, NULL);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, NULL);
    }
    return 0;
}

int module_stop(SceSize args, void *argp) {
    return 0;
}
