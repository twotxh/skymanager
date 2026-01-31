#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <switch.h>
#include "../include/ini.h"
#include "../include/mod.h"
#include "../include/main.h"

#define CONFIG_INI_PATH "/config/skymanager/config.ini"
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

// Settings structure

// Global instances
Settings settings;
ModManager manager = {0};
PadState pad;
bool running = true;
bool draw_bool = true;

int file_exists(const char *filename)
{
    FILE *file;
    /* Try to open the file in read mode ("r") */
    if ((file = fopen(filename, "r")) != NULL)
    {
        /* The file was successfully opened, so it exists and is readable */
        fclose(file); // Close the file immediately
        return 1;     // Return true (exists)
    }
    else
    {
        /* fopen returned NULL, indicating the file doesn't exist or is inaccessible */
        return 0; // Return false (does not exist/inaccessible)
    }
}

// INI handler function
static int iniHandler(void *user, const char *section, const char *name, const char *value)
{
    Settings *pconfig = (Settings *)user;

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
    else if (MATCH("settings", "hide_cc_content"))
    {
        pconfig->HIDE_CC_CONTENT = parse_bool(value);
    }
    else if (MATCH("settings", "skip_init"))
    {
        pconfig->SKIP_INIT = parse_bool(value);
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
    settings.HIDE_CC_CONTENT = false;
}

void loadSettings()
{
    const char *config_file = CONFIG_INI_PATH;

    // Initialize with defaults first
    initDefaultSettings();

    // Try to load from INI file
    if (ini_parse(config_file, iniHandler, &settings) < 0)
    {
        // Config file doesn't exist or has errors - use defaults
        // Silently continue with defaults
        perror("Could not load config.ini, using default settings.\n");
    }
}

void initializeDirectories()
{
    // Create necessary directories based on settings
    mkdir("/config", 0777);
    mkdir("/config/skymanager", 0777);
    // mkdir(settings.MODS_FOLDER, 0777);

    // Create atmosphere directories
    mkdir("/atmosphere", 0777);
    mkdir("/atmosphere/contents", 0777);
    mkdir("/atmosphere/contents/01000A10041EA000/romfs", 0777);
    mkdir(settings.ATMOSPHERE_LAYEREDFS, 0777);

    // FILE *fileptr;
    // fileptr = fopen(CONFIG_INI_PATH, "w");
    // fclose(fileptr);
    if (!file_exists(CONFIG_INI_PATH))
    // if (true)
    {
        Result rc = romfsInit();
        if (R_FAILED(rc))
        {
            printf("Failed to mount romfs: 0x%x\n", rc);
            abort();
            // return;
        }

        char buffer[1024];
        size_t bytes_read, bytes_written;
        FILE *source_file = fopen("romfs:/config.ini", "r");
        if (!source_file)
        {
            perror("Failed to open romfs default config\n");
            return;
        }
        FILE *dest_file = fopen(CONFIG_INI_PATH, "w");
        if (!dest_file)
        {
            perror("Failed to create config.ini");
            fclose(source_file);
            return;
        }

        while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
        {
            bytes_written = fwrite(buffer, 1, bytes_read, dest_file);
            if (bytes_written != bytes_read)
            {
                perror("Error writing to destination file");
                fclose(source_file);
                fclose(dest_file);
                return;
            }
        }
        fclose(source_file);
        fclose(dest_file);
    }
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
        if (manager.mods[i].isCC && settings.HIDE_CC_CONTENT)
        {
            // manager.mods[i].enabled = false;
        }
        else
        {
            manager.mods[i].enabled = isModInCCC(manager.mods[i].name);
        }
    }
    drawUI();
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
void toggleCC(void) // Toggle CC is NOT functional - wip
{
    settings.HIDE_CC_CONTENT = !settings.HIDE_CC_CONTENT;
    saveCCCFile();
    manager = scanForMods(settings, manager);
    manager.selectedIndex = 0;
    manager.scrollOffset = 0; // Reset indices
    loadCCCFile();
    printf(CONSOLE_ESC(2J));
    draw_bool = false;
    drawUI();
    draw_bool = true;
}

void drawUI()
{

    // loadCCCFile();
    printf("\x1b[1;1H"); // Move cursor to top
    printf("\x1b[32m");  // Green color
    printf("                        SKYMANAGER - Skyrim Nintendo Switch Mod Manager\n\n");

    printf("\x1b[0m"); // Reset color

    printf("LayeredFS Folder: %s\n", settings.ATMOSPHERE_LAYEREDFS);
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
        printf("  Up/Down: Select mod  |  A: Toggle mod | +: Exit\n\n");
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
                printf("[Unknown] \x1b[0m "); // default for other
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
            manager = toggleMod(manager.selectedIndex, manager);
        }
    }

    if (kDown & HidNpadButton_X)
    {
        /* manager = enableAll(settings, manager);
        manager = scanForMods(settings, manager);
        manager.selectedIndex = 0;
        manager.scrollOffset = 0; // Reset indices
        loadCCCFile();
        printf(CONSOLE_ESC(2J));
        draw_bool = false;
        drawUI();
        draw_bool = true;
        */
    }
    if (kDown & HidNpadButton_Y)
    {

        // toggleCC();
    }
}

int main(int argc, char *argv[])
{
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    // Load settings from INI file
    loadSettings();

    // Initialize directories based on settings if skip_init = false
    if (!settings.SKIP_INIT)
    {
        initializeDirectories();
    }
    // Scan for mods and load enabled state
    manager = scanForMods(settings, manager);
    loadCCCFile();

    while (running && appletMainLoop())
    {
        if (draw_bool)
        {
            drawUI();
        }
        handleInput();
    }
    consoleExit(NULL);
    return 0;
}