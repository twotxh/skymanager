#ifndef MOD_H
#define MOD_H

#define MAX_MODS 256
#define MAX_PATH 512
#define MAX_FILENAME 128
#include "main.h"

typedef struct
{
    char name[MAX_FILENAME];
    char path[MAX_PATH];
    bool enabled;
    bool isESM;
    bool isESP;
    bool isESL;
} Mod;

typedef struct
{
    Mod mods[MAX_MODS];
    int modCount;
    int selectedIndex;
    int scrollOffset;
} ModManager;

ModManager scanForMods(Settings settings, ModManager manager);
bool parse_bool(const char *str);
ModManager toggleMod(int index, ModManager manager);
ModManager enableAll(Settings settings, ModManager manager);
ModManager refreshModList(Settings settings, ModManager manager);
#endif