#ifndef MAIN_H
#define MAIN_H
#define MAX_PATH 512

typedef struct
{
    // char MODS_FOLDER[MAX_PATH];
    char SKYRIM_CCC_PATH[MAX_PATH];
    char ATMOSPHERE_LAYEREDFS[MAX_PATH];
    // char TITLE_ID[MAX_PATH];
    bool IGNORE_INVALID_FILES;
    bool HIDE_CC_CONTENT;
} Settings;

void initDefaultSettings();
void loadSettings();
void loadCCCFile();
void saveCCCFile();
void drawUI();
void handleInput();
bool isModInCCC(const char *modName);
void initializeDirectories();
#endif