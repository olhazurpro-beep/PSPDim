/*
 * NightShift plugin for PSP
 * Затемняет синий/зелёный канал экрана в зависимости от настройки "теплоты".
 *
 * Работает как kernel-плагин, подключаемый через seplugins (PRO CFW / ARK-4).
 * Патчит sceDisplaySetFrameBuf и на лету изменяет цвет каждого кадра.
 *
 * ВНИМАНИЕ: это рабочий скелет, а не готовое коммерческое решение.
 * Его нужно будет протестировать на железе/эмуляторе (PPSSPP с CFW-режимом,
 * либо реальная PSP с CFW) и при необходимости доработать под конкретную
 * версию CFW (адреса/сигнатуры функций могут отличаться).
 */

#include <stdlib.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspsdk.h>
#include <psploadexec_kernel.h>
#include <string.h>
#include <stdio.h>

PSP_MODULE_INFO("NightShift", 0x1007, 1, 0);
PSP_MAIN_THREAD_ATTR(0);

/* ---------- Настройки ---------- */

#define CONFIG_PATH "ms0:/seplugins/nightshift.ini"

static int g_warmth = 40;      /* 0-100: сила теплого фильтра */
static int g_enabled = 1;      /* 0/1: вкл/выкл */
static int g_skip = 1;         /* обрабатывать каждый (skip+1)-й пиксель, для скорости */

/* ---------- Оригинальная функция вывода кадра ---------- */

typedef int (*pSceDisplaySetFrameBuf)(void *topaddr, int bufferwidth,
                                       int pixelformat, int sync);

static pSceDisplaySetFrameBuf sceDisplaySetFrameBuf_Original = NULL;

/* Прототип, объявленный в utils.prx (есть в PRO CFW / ARK-4 SDK).
 * Позволяет находить и патчить системные функции без знания жёстких адресов. */
extern unsigned int sctrlHENFindFunction(char *pModule, char *pLibrary, unsigned int nid);
extern int sctrlHENPatchSyscall(void *addr, void *newaddr);

/* ---------- Загрузка конфига ---------- */

static void load_config(void)
{
    SceUID fd = sceIoOpen(CONFIG_PATH, PSP_O_RDONLY, 0777);
    if (fd < 0) return; /* нет конфига - используем значения по умолчанию */

    char buf[256];
    memset(buf, 0, sizeof(buf));
    int read = sceIoRead(fd, buf, sizeof(buf) - 1);
    sceIoClose(fd);
    if (read <= 0) return;

    int warmth = g_warmth, enabled = g_enabled, skip = g_skip;
    char *line = buf;
    while (line && *line) {
        char *next = strchr(line, '\n');
        if (next) *next = '\0';

        if (!strncmp(line, "warmth=", 7)) warmth = atoi(line + 7);
        else if (!strncmp(line, "enabled=", 8)) enabled = atoi(line + 8);
        else if (!strncmp(line, "skip=", 5)) skip = atoi(line + 5);

        if (!next) break;
        line = next + 1;
    }

    if (warmth < 0) warmth = 0;
    if (warmth > 100) warmth = 100;
    if (skip < 0) skip = 0;
    if (skip > 8) skip = 8;

    g_warmth = warmth;
    g_enabled = enabled;
    g_skip = skip;
}

/* ---------- Обработка кадра ---------- */

static inline void apply_warmth(u32 *fb, int pixel_count)
{
    int warmth = g_warmth;
    int step = g_skip + 1;

    for (int i = 0; i < pixel_count; i += step) {
        u32 px = fb[i];

        int a = (px >> 24) & 0xFF;
        int b = (px >> 16) & 0xFF;
        int g = (px >> 8) & 0xFF;
        int r = px & 0xFF;

        /* уменьшаем синий сильнее, зелёный - слабее (эффект "тёплого" экрана) */
        b -= (b * warmth) / 150;
        g -= (g * warmth) / 400;

        if (b < 0) b = 0;
        if (g < 0) g = 0;

        u32 new_px = (a << 24) | (b << 16) | (g << 8) | r;

        /* заполняем и пропущенные пиксели тем же значением, чтобы не мерцало */
        for (int j = 0; j < step && (i + j) < pixel_count; j++) {
            fb[i + j] = new_px;
        }
    }
}

/* ---------- Патч sceDisplaySetFrameBuf ---------- */

static int sceDisplaySetFrameBuf_Patched(void *topaddr, int bufferwidth,
                                          int pixelformat, int sync)
{
    if (g_enabled && topaddr && pixelformat == PSP_DISPLAY_PIXEL_FORMAT_8888) {
        int pixel_count = bufferwidth * 272; /* высота экрана PSP */
        apply_warmth((u32 *)topaddr, pixel_count);
    }

    return sceDisplaySetFrameBuf_Original(topaddr, bufferwidth, pixelformat, sync);
}

/* ---------- Точки входа модуля ---------- */

int module_start(SceSize args, void *argp)
{
    load_config();

    unsigned int addr = sctrlHENFindFunction("sceDisplay_Service", "sceDisplay",
                                              0x289D82FE /* sceDisplaySetFrameBuf */);
    if (addr) {
        sceDisplaySetFrameBuf_Original = (pSceDisplaySetFrameBuf)addr;
        sctrlHENPatchSyscall((void *)addr, (void *)sceDisplaySetFrameBuf_Patched);
    }

    return 0;
}

int module_stop(SceSize args, void *argp)
{
    return 0;
}
