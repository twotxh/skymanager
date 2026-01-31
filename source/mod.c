#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <switch.h>
#include <unistd.h>
#include <dirent.h>
#include "../include/mod.h"
#include "../include/main.h"

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

ModManager toggleMod(int index, ModManager manager)
{
    if (index < 0 || index >= manager.modCount)
        return manager;

    Mod *mod = &manager.mods[index];
    mod->enabled = !mod->enabled;

    // Save changes immediately
    saveCCCFile();
    return manager;
}

ModManager scanForMods(Settings settings, ModManager manager)
{
    manager.modCount = 0;
    printf("Scanning for mods in: %s\n", settings.ATMOSPHERE_LAYEREDFS);
    // printf("TEST");
    //  sleep(10);
    DIR *dir = opendir(settings.ATMOSPHERE_LAYEREDFS);
    if (!dir)
    {
        return manager;
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
        if (isESP || isESM || isESL)
        {
            char prefix[3];
            memcpy(prefix, entry->d_name, 2);
            prefix[2] = '\0';
            // printf("entry: %s", entry->d_name);
            // printf("Prefix: %s \n", prefix);

            /*if (settings.HIDE_CC_CONTENT && (!strcmp(prefix, "cc")))
            {
                // do nothing
            }
            else*/
            if (true)
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
    }

    closedir(dir);
    return manager;
}
