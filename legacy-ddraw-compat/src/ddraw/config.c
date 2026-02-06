/**
 * @file config.c
 * @brief INI configuration parser and settings for legacy-ddraw-compat
 */

#include "core/Common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * Configuration Structure
 * ============================================================================ */

typedef struct LDC_Config {
    /* Display settings */
    BOOL windowed;              /* Run in windowed mode instead of fullscreen */
    BOOL maintas;               /* Maintain aspect ratio when scaling */
    BOOL vsync;                 /* Enable vertical sync */
    BOOL nonexclusive;          /* Non-exclusive fullscreen mode */
    int windowX;                /* Window X position (-1 = centered) */
    int windowY;                /* Window Y position (-1 = centered) */

    /* Scaling settings */
    int scalingMode;            /* 0=stretch, 1=integer, 2=aspect */
    int customWidth;            /* Custom window width (0 = game resolution) */
    int customHeight;           /* Custom window height (0 = game resolution) */

    /* Rendering settings */
    BOOL bilinearFilter;        /* Bilinear filtering when scaling */
    int maxFps;                 /* Max FPS limit (0 = unlimited) */

    /* Mouse settings */
    BOOL adjmouse;              /* Adjust mouse coordinates for scaling */
    BOOL lockcursor;            /* Lock cursor to window */

    /* Compatibility settings */
    BOOL noactivateapp;         /* Don't send WM_ACTIVATEAPP messages */
    BOOL singlecpu;             /* Use single CPU affinity */
    BOOL fixpitch;              /* Fix surface pitch alignment */

    /* Debug settings */
    BOOL debug;                 /* Enable debug logging */
    char logFile[MAX_PATH];     /* Log file path */

} LDC_Config;

/* Global configuration instance */
LDC_Config g_config = {
    /* Display */
    .windowed = FALSE,
    .maintas = TRUE,
    .vsync = TRUE,
    .nonexclusive = FALSE,
    .windowX = -1,
    .windowY = -1,

    /* Scaling */
    .scalingMode = 0,
    .customWidth = 0,
    .customHeight = 0,

    /* Rendering */
    .bilinearFilter = TRUE,
    .maxFps = 0,

    /* Mouse */
    .adjmouse = TRUE,
    .lockcursor = FALSE,

    /* Compatibility */
    .noactivateapp = FALSE,
    .singlecpu = FALSE,
    .fixpitch = TRUE,

    /* Debug */
    .debug = FALSE,
    .logFile = ""
};

/* ============================================================================
 * INI Parser Helpers
 * ============================================================================ */

static char* TrimWhitespace(char* str)
{
    char* end;

    /* Trim leading whitespace */
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    /* Trim trailing whitespace */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
    return str;
}

static BOOL ParseBool(const char* value)
{
    if (_stricmp(value, "true") == 0 || _stricmp(value, "yes") == 0 ||
        _stricmp(value, "1") == 0 || _stricmp(value, "on") == 0) {
        return TRUE;
    }
    return FALSE;
}

static int ParseInt(const char* value)
{
    return atoi(value);
}

/* ============================================================================
 * Configuration Loading
 * ============================================================================ */

static void ProcessConfigLine(const char* section, const char* key, const char* value)
{
    /* [display] section */
    if (_stricmp(section, "display") == 0 || _stricmp(section, "ddraw") == 0) {
        if (_stricmp(key, "windowed") == 0) {
            g_config.windowed = ParseBool(value);
        }
        else if (_stricmp(key, "maintas") == 0) {
            g_config.maintas = ParseBool(value);
        }
        else if (_stricmp(key, "vsync") == 0) {
            g_config.vsync = ParseBool(value);
        }
        else if (_stricmp(key, "nonexclusive") == 0) {
            g_config.nonexclusive = ParseBool(value);
        }
        else if (_stricmp(key, "posX") == 0 || _stricmp(key, "windowX") == 0) {
            g_config.windowX = ParseInt(value);
        }
        else if (_stricmp(key, "posY") == 0 || _stricmp(key, "windowY") == 0) {
            g_config.windowY = ParseInt(value);
        }
        else if (_stricmp(key, "width") == 0) {
            g_config.customWidth = ParseInt(value);
        }
        else if (_stricmp(key, "height") == 0) {
            g_config.customHeight = ParseInt(value);
        }
    }

    /* [scaling] section */
    else if (_stricmp(section, "scaling") == 0) {
        if (_stricmp(key, "mode") == 0) {
            if (_stricmp(value, "stretch") == 0) g_config.scalingMode = 0;
            else if (_stricmp(value, "integer") == 0) g_config.scalingMode = 1;
            else if (_stricmp(value, "aspect") == 0) g_config.scalingMode = 2;
            else g_config.scalingMode = ParseInt(value);
        }
        else if (_stricmp(key, "filter") == 0 || _stricmp(key, "bilinear") == 0) {
            g_config.bilinearFilter = ParseBool(value);
        }
    }

    /* [rendering] section */
    else if (_stricmp(section, "rendering") == 0 || _stricmp(section, "renderer") == 0) {
        if (_stricmp(key, "maxfps") == 0 || _stricmp(key, "fpslimit") == 0) {
            g_config.maxFps = ParseInt(value);
        }
    }

    /* [mouse] section */
    else if (_stricmp(section, "mouse") == 0) {
        if (_stricmp(key, "adjmouse") == 0 || _stricmp(key, "adjust") == 0) {
            g_config.adjmouse = ParseBool(value);
        }
        else if (_stricmp(key, "lockcursor") == 0 || _stricmp(key, "lock") == 0) {
            g_config.lockcursor = ParseBool(value);
        }
    }

    /* [compatibility] section */
    else if (_stricmp(section, "compatibility") == 0 || _stricmp(section, "compat") == 0) {
        if (_stricmp(key, "noactivateapp") == 0) {
            g_config.noactivateapp = ParseBool(value);
        }
        else if (_stricmp(key, "singlecpu") == 0) {
            g_config.singlecpu = ParseBool(value);
        }
        else if (_stricmp(key, "fixpitch") == 0) {
            g_config.fixpitch = ParseBool(value);
        }
    }

    /* [debug] section */
    else if (_stricmp(section, "debug") == 0) {
        if (_stricmp(key, "enabled") == 0 || _stricmp(key, "debug") == 0) {
            g_config.debug = ParseBool(value);
        }
        else if (_stricmp(key, "logfile") == 0 || _stricmp(key, "log") == 0) {
            strncpy_s(g_config.logFile, sizeof(g_config.logFile), value, _TRUNCATE);
        }
    }
}

void LDC_LoadConfig(void)
{
    char iniPath[MAX_PATH];
    char line[512];
    char currentSection[64] = "";
    FILE* fp;

    /* Get INI path next to DLL */
    if (g_ldc.hModule) {
        GetModuleFileNameA(g_ldc.hModule, iniPath, MAX_PATH);
        char* lastSlash = strrchr(iniPath, '\\');
        if (lastSlash) {
            strcpy_s(lastSlash + 1, MAX_PATH - (lastSlash - iniPath + 1), "ddraw.ini");
        }
    } else {
        strcpy_s(iniPath, MAX_PATH, "ddraw.ini");
    }

    LDC_LOG("Loading config from: %s", iniPath);

    if (fopen_s(&fp, iniPath, "r") != 0 || !fp) {
        LDC_LOG("Config file not found, using defaults");
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = TrimWhitespace(line);

        /* Skip empty lines and comments */
        if (*trimmed == '\0' || *trimmed == ';' || *trimmed == '#') {
            continue;
        }

        /* Section header */
        if (*trimmed == '[') {
            char* end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy_s(currentSection, sizeof(currentSection), trimmed + 1, _TRUNCATE);
            }
            continue;
        }

        /* Key=value pair */
        char* equals = strchr(trimmed, '=');
        if (equals) {
            *equals = '\0';
            char* key = TrimWhitespace(trimmed);
            char* value = TrimWhitespace(equals + 1);

            /* Remove quotes from value */
            size_t len = strlen(value);
            if (len >= 2 && ((value[0] == '"' && value[len-1] == '"') ||
                            (value[0] == '\'' && value[len-1] == '\''))) {
                value[len-1] = '\0';
                value++;
            }

            ProcessConfigLine(currentSection, key, value);
        }
    }

    fclose(fp);

    LDC_LOG("Config loaded: windowed=%d maintas=%d vsync=%d adjmouse=%d",
            g_config.windowed, g_config.maintas, g_config.vsync, g_config.adjmouse);
}

/* ============================================================================
 * Configuration Accessors
 * ============================================================================ */

BOOL LDC_GetConfigBool(const char* key)
{
    if (_stricmp(key, "windowed") == 0) return g_config.windowed;
    if (_stricmp(key, "maintas") == 0) return g_config.maintas;
    if (_stricmp(key, "vsync") == 0) return g_config.vsync;
    if (_stricmp(key, "adjmouse") == 0) return g_config.adjmouse;
    if (_stricmp(key, "lockcursor") == 0) return g_config.lockcursor;
    if (_stricmp(key, "debug") == 0) return g_config.debug;
    if (_stricmp(key, "bilinear") == 0) return g_config.bilinearFilter;
    if (_stricmp(key, "fixpitch") == 0) return g_config.fixpitch;
    return FALSE;
}

int LDC_GetConfigInt(const char* key)
{
    if (_stricmp(key, "windowX") == 0) return g_config.windowX;
    if (_stricmp(key, "windowY") == 0) return g_config.windowY;
    if (_stricmp(key, "width") == 0) return g_config.customWidth;
    if (_stricmp(key, "height") == 0) return g_config.customHeight;
    if (_stricmp(key, "maxfps") == 0) return g_config.maxFps;
    if (_stricmp(key, "scalingMode") == 0) return g_config.scalingMode;
    return 0;
}

/* Apply configuration to global state */
void LDC_ApplyConfig(void)
{
    /* Apply single CPU affinity if requested */
    if (g_config.singlecpu) {
        SetProcessAffinityMask(GetCurrentProcess(), 1);
    }

    /* Store debug flag in global state for LDC_LOG */
    /* (debug logging is controlled via preprocessor, but this could be used for runtime logging) */
}
