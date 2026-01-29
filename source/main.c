#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <switch.h>
#include "../include/ini.h"

#define MAX_MODS 256
#define MAX_PATH 512
#define MAX_FILENAME 128
#define CONFIG_INI_PATH "/switch/skymanager/config.ini"

// Settings structure
typedef struct
{
    // char MODS_FOLDER[MAX_PATH];
    char SKYRIM_CCC_PATH[MAX_PATH];
    char ATMOSPHERE_LAYEREDFS[MAX_PATH];
    // char TITLE_ID[MAX_PATH];
    bool IGNORE_INVALID_FILES;
    bool HIDE_CC_CONTENT;
} Settings;

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

// Global instances
Settings settings;
ModManager manager = {0};
PadState pad;
bool running = true;

// Function prototypes
void initDefaultSettings();
void loadSettings();
void scanForMods();
void loadCCCFile();
void saveCCCFile();
void toggleMod(int index);
void drawUI();
void handleInput();
bool isModInCCC(const char *modName);
void initializeDirectories();

// bool parser
bool parse_bool(const char *str)
{
    if (str == NULL)
    {
        return false;
    }

    // Case-insensitive comparison for "true" and "false"
    if (strcasecmp(str, "true") == 0)
    {
        return true;
    }
    if (strcasecmp(str, "false") == 0)
    {
        return false;
    }

    // Check for numeric values
    if (strcmp(str, "1") == 0)
    {
        return true;
    }
    if (strcmp(str, "0") == 0)
    {
        return false;
    }

    // Default to false for any other input (or define other logic)
    return false;
}

// INI handler function
static int iniHandler(void *user, const char *section, const char *name, const char *value)
{
    Settings *pconfig = (Settings *)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    /*if (MATCH("paths", "mods_folder")) {
        strncpy(pconfig->MODS_FOLDER, value, MAX_PATH - 1);
        pconfig->MODS_FOLDER[MAX_PATH - 1] = '\0';
    } else*/
    if (MATCH("paths", "skyrim_ccc"))
    {
        strncpy(pconfig->SKYRIM_CCC_PATH, value, MAX_PATH - 1);
        pconfig->SKYRIM_CCC_PATH[MAX_PATH - 1] = '\0';
    }
    else if (MATCH("paths", "atmosphere_layeredfs"))
    {
        strncpy(pconfig->ATMOSPHERE_LAYEREDFS, value, MAX_PATH - 1);
        pconfig->ATMOSPHERE_LAYEREDFS[MAX_PATH - 1] = '\0';
    }
    else if (MATCH("settings", "ignore_invalid_files"))
    {
        pconfig->IGNORE_INVALID_FILES = parse_bool(value);
    }
    else if (MATCH("settings", "hide_cc_content"))
    {
        pconfig->HIDE_CC_CONTENT = parse_bool(value);
    }
    else
    {
        return 0; // Unknown section/name
    }

    return 1; // Success
}

void initDefaultSettings()
{
    // Set default paths
    // strncpy(settings.MODS_FOLDER, "/switch/skymanager/mods", MAX_PATH - 1);
    strncpy(settings.SKYRIM_CCC_PATH, "/atmosphere/contents/01000A10041EA000/romfs/Data/Skyrim.ccc", MAX_PATH - 1);
    strncpy(settings.ATMOSPHERE_LAYEREDFS, "/atmosphere/contents/01000A10041EA000/romfs/Data", MAX_PATH - 1);
    settings.IGNORE_INVALID_FILES = false;
    settings.HIDE_CC_CONTENT = true;
}

void loadSettings()
{
    const char *config_file = "/switch/skymanager/config.ini";

    // Initialize with defaults first
    initDefaultSettings();

    // Try to load from INI file
    if (ini_parse(config_file, iniHandler, &settings) < 0)
    {
        // Config file doesn't exist or has errors - use defaults
        // Silently continue with defaults
    }
}

void initializeDirectories()
{
    // Create necessary directories based on settings
    mkdir("/switch", 0777);
    mkdir("/switch/skymanager", 0777);
    // mkdir(settings.MODS_FOLDER, 0777);

    // Create atmosphere directories
    mkdir("/atmosphere", 0777);
    mkdir("/atmosphere/contents", 0777);
    mkdir("/atmosphere/contents/01000A10041EA000", 0777);
    mkdir("/atmosphere/contents/01000A10041EA000/romfs", 0777);
    mkdir(settings.ATMOSPHERE_LAYEREDFS, 0777);

    FILE *fileptr;
    fileptr = fopen(CONFIG_INI_PATH, "w");
    fclose(fileptr);
}

void scanForMods()
{
    manager.modCount = 0;

    DIR *dir = opendir(settings.ATMOSPHERE_LAYEREDFS);
    if (!dir)
    {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && manager.modCount < MAX_MODS)
    {
        if (entry->d_type != DT_REG)
            continue;

        size_t len = strlen(entry->d_name);
        bool isESP = (len > 4 && strcasecmp(entry->d_name + len - 4, ".esp") == 0);
        bool isESM = (len > 4 && strcasecmp(entry->d_name + len - 4, ".esm") == 0);
        bool isESL = (len > 4 && strcasecmp(entry->d_name + len - 4, ".esl") == 0);
        bool isRESOURCE = ((len > 4 && strcasecmp(entry->d_name + len - 4, ".bsa") == 0) || (len > 4 && strcasecmp(entry->d_name + len - 4, ".ini") == 0));
        if (isESP || isESM || isESL)
        {
            char prefix[2];
            memcpy(prefix, entry->d_name, 2);
            prefix[2] = '\0';
            // printf("entry: %s", entry->d_name);
            // printf("Prefix: %s \n", prefix);

            if (settings.HIDE_CC_CONTENT && (!strcmp(prefix, "cc")))
            {
                // do nothing
            }
            else
            {
                Mod *mod = &manager.mods[manager.modCount];
                strncpy(mod->name, entry->d_name, MAX_FILENAME - 1);
                snprintf(mod->path, MAX_PATH, "%s/%s", settings.ATMOSPHERE_LAYEREDFS, entry->d_name);
                mod->isESM = isESM;
                mod->isESP = isESP;
                mod->isESL = isESL;
                mod->enabled = isModInCCC(mod->name);
                manager.modCount++;
            }
        }
        else if ((!(isESP || isESM || isRESOURCE || isESL)) && !settings.IGNORE_INVALID_FILES)
        {
            printf("Invalid file found: %s", entry);
        }
    }

    closedir(dir);
}

bool isModInCCC(const char *modName)
{
    FILE *f = fopen(settings.SKYRIM_CCC_PATH, "r");
    if (!f)
        return false;

    char line[MAX_PATH];
    bool found = false;

    while (fgets(line, sizeof(line), f))
    {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;

        if (strcmp(line, modName) == 0)
        {
            found = true;
            break;
        }
    }

    fclose(f);
    return found;
}

void loadCCCFile()
{
    // Update enabled status for all mods
    for (int i = 0; i < manager.modCount; i++)
    {
        manager.mods[i].enabled = isModInCCC(manager.mods[i].name);
    }
}

void saveCCCFile()
{
    FILE *f = fopen(settings.SKYRIM_CCC_PATH, "w");
    if (!f)
    {
        // Create the file if it doesn't exist
        f = fopen(settings.SKYRIM_CCC_PATH, "w");
        if (!f)
            return;
    }
    for (int i = 0; i < manager.modCount; i++)
    {
        if (manager.mods[i].enabled)
        {
            fprintf(f, "%s\n", manager.mods[i].name);
        }
    }

    fclose(f);
}

void toggleMod(int index)
{
    if (index < 0 || index >= manager.modCount)
        return;

    Mod *mod = &manager.mods[index];
    mod->enabled = !mod->enabled;

    // Save changes immediately
    saveCCCFile();
}

void enableAll()
{

    int mods = sizeof(manager.mods);
    printf("%d", mods);
    for (int i = 0; i < mods; i++)
    {

        Mod *mod = &manager.mods[i];
        mod->enabled = true;
    }
    printf("saving");
    saveCCCFile();
}

void drawUI()
{
    consoleClear();

    printf("\x1b[1;1H"); // Move cursor to top
    printf("\x1b[32m");  // Green color
    printf("                        SKYRIM SWITCH MOD MANAGER\n\n");

    printf("\x1b[0m"); // Reset color

    printf("\LayeredFS Folder: %s\n", settings.ATMOSPHERE_LAYEREDFS);
    printf("Total mods: %d\n\n", manager.modCount);

    if (manager.modCount == 0)
    {
        printf("\x1b[33m"); // Yellow
        printf("No mods found!\n\n");
        printf("Place your .esp, .esm, or .esl files in:\n");
        printf("  %s\n\n", settings.ATMOSPHERE_LAYEREDFS);
        // printf("Or edit /switch/skymanager/config.ini to change paths.\n\n");
        printf("\x1b[0m");
    }
    else
    {
        printf("Controls:\n");
        printf("  Up/Down: Select mod  |  A: Toggle mod  |  X: Enable All  |  +: Exit\n\n");
        printf("-----------------------------------------------------\n");

        // Display mods (with scrolling support)
        int displayStart = manager.scrollOffset;
        int displayEnd = displayStart + 20; // Show 20 mods at a time
        if (displayEnd > manager.modCount)
            displayEnd = manager.modCount;

        for (int i = displayStart; i < displayEnd; i++)
        {
            Mod *mod = &manager.mods[i];

            // Highlight selected mod
            if (i == manager.selectedIndex)
            {
                printf("\x1b[7m"); // Reverse video
            }

            // Color based on status
            if (mod->enabled)
            {
                printf("\x1b[32m[X]"); // Green checkmark
            }
            else
            {
                printf("\x1b[31m[ ]"); // Red empty
            }

            // Display mod type
            if (mod->isESM)
            {
                printf("\x1b[36m[ESM]\x1b[0m "); // Cyan for ESM
            }
            else if (mod->isESP)
            {
                printf("\x1b[35m[ESP]\x1b[0m "); // Magenta for ESP
            }
            else if (mod->isESL)
            {
                printf("\x1b[33m[ESP]\x1b[0m "); // Yellow for ESL
            }
            else
            {
                printf("[Unknown] \x1b[0m "); // default for error
            }

            // Display mod name
            printf("%s", mod->name);
            printf("                         "); // prevents overlap

            printf("\x1b[0m\n"); // Reset formatting
        }

        if (manager.modCount > 20)
        {
            printf("\n");
            printf("Showing %d-%d of %d mods\n", displayStart + 1, displayEnd, manager.modCount);
        }
    }

    printf("\n------------------\n");
    printf("\x1b[33mEnabled mods will be added to: %s\x1b[0m\n", settings.SKYRIM_CCC_PATH);

    consoleUpdate(NULL);
}

void handleInput()
{
    padUpdate(&pad);
    u64 kDown = padGetButtonsDown(&pad);

    if (kDown & HidNpadButton_Plus)
    {
        saveCCCFile();
        running = false;
    }

    if (manager.modCount > 0)
    {
        if (kDown & HidNpadButton_Down)
        {
            manager.selectedIndex++;
            if (manager.selectedIndex >= manager.modCount)
            {
                manager.selectedIndex = 0;
                manager.scrollOffset = 0;
            }
            else if (manager.selectedIndex >= manager.scrollOffset + 20)
            {
                manager.scrollOffset++;
            }
        }

        if (kDown & HidNpadButton_Up)
        {
            manager.selectedIndex--;
            if (manager.selectedIndex < 0)
            {
                manager.selectedIndex = manager.modCount - 1;
                manager.scrollOffset = manager.modCount - 20;
                if (manager.scrollOffset < 0)
                    manager.scrollOffset = 0;
            }
            else if (manager.selectedIndex < manager.scrollOffset)
            {
                manager.scrollOffset--;
            }
        }

        if (kDown & HidNpadButton_A)
        {
            toggleMod(manager.selectedIndex);
        }
    }

    if (kDown & HidNpadButton_X)
    {
        /*scanForMods();
        loadCCCFile();
        if (manager.selectedIndex >= manager.modCount) {
            manager.selectedIndex = manager.modCount - 1;
            if (manager.selectedIndex < 0) manager.selectedIndex = 0;
        }*/
        enableAll();
        scanForMods();
        loadCCCFile();
    }
}

void toggleCCC(void)
{

    settings.HIDE_CC_CONTENT = !settings.HIDE_CC_CONTENT;
    scanForMods();
    loadCCCFile();
}

int main(int argc, char *argv[])
{
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    // Load settings from INI file
    loadSettings();

    // Initialize directories based on settings
    initializeDirectories();

    // Scan for mods and load enabled state
    scanForMods();
    loadCCCFile();

    while (running && appletMainLoop())
    {
        drawUI();
        handleInput();
    }

    consoleExit(NULL);
    return 0;
}