/**
 * @file config.c
 * @brief INI configuration for legacy-ddraw-compat
 *
 * Simple configuration for application compatibility.
 */

#include "core/Common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================================
 * Configuration
 * ============================================================================ */

typedef struct {
    BOOL windowed;
    BOOL maintas;
    BOOL smooth;
    BOOL adjmouse;
    BOOL fixpitch;
    BOOL debug;
    int width;
    int height;
} Config;

static Config s_config = {
    TRUE,   /* windowed */
    TRUE,   /* maintas */
    TRUE,   /* smooth */
    TRUE,   /* adjmouse */
    TRUE,   /* fixpitch */
    FALSE,  /* debug */
    0,      /* width */
    0       /* height */
};

/* ============================================================================
 * INI Parsing
 * ============================================================================ */

static char* TrimSpace(char* s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char* end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static BOOL ParseBool(const char* val) {
    return (_stricmp(val, "true") == 0 ||
            _stricmp(val, "yes") == 0 ||
            _stricmp(val, "1") == 0 ||
            _stricmp(val, "on") == 0);
}

static void ParseLine(const char* key, const char* val) {
    if (_stricmp(key, "windowed") == 0)
        s_config.windowed = ParseBool(val);
    else if (_stricmp(key, "maintas") == 0)
        s_config.maintas = ParseBool(val);
    else if (_stricmp(key, "smooth") == 0)
        s_config.smooth = ParseBool(val);
    else if (_stricmp(key, "adjmouse") == 0)
        s_config.adjmouse = ParseBool(val);
    else if (_stricmp(key, "fixpitch") == 0)
        s_config.fixpitch = ParseBool(val);
    else if (_stricmp(key, "log") == 0 || _stricmp(key, "debug") == 0)
        s_config.debug = ParseBool(val);
    else if (_stricmp(key, "width") == 0)
        s_config.width = atoi(val);
    else if (_stricmp(key, "height") == 0)
        s_config.height = atoi(val);
}

void LDC_LoadConfig(void) {
    char path[MAX_PATH];
    FILE* fp;

    /* Get INI path next to DLL */
    if (g_ldc.hModule) {
        GetModuleFileNameA(g_ldc.hModule, path, MAX_PATH);
        char* slash = strrchr(path, '\\');
        if (slash) strcpy_s(slash + 1, MAX_PATH - (slash - path + 1), "ddraw.ini");
    } else {
        strcpy_s(path, MAX_PATH, "ddraw.ini");
    }

    if (fopen_s(&fp, path, "r") != 0 || !fp) {
        LDC_LOG("Config not found, using defaults");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char* s = TrimSpace(line);
        if (*s == '\0' || *s == ';' || *s == '#' || *s == '[') continue;

        char* eq = strchr(s, '=');
        if (eq) {
            *eq = '\0';
            char* key = TrimSpace(s);
            char* val = TrimSpace(eq + 1);
            ParseLine(key, val);
        }
    }

    fclose(fp);
    LDC_LOG("Config loaded: windowed=%d maintas=%d smooth=%d adjmouse=%d",
            s_config.windowed, s_config.maintas, s_config.smooth, s_config.adjmouse);
}

void LDC_ApplyConfig(void) {
    /* Nothing to apply at startup for now */
}

BOOL LDC_GetConfigBool(const char* key) {
    if (_stricmp(key, "windowed") == 0) return s_config.windowed;
    if (_stricmp(key, "maintas") == 0) return s_config.maintas;
    if (_stricmp(key, "smooth") == 0 || _stricmp(key, "bilinear") == 0) return s_config.smooth;
    if (_stricmp(key, "adjmouse") == 0) return s_config.adjmouse;
    if (_stricmp(key, "fixpitch") == 0) return s_config.fixpitch;
    if (_stricmp(key, "debug") == 0) return s_config.debug;
    return FALSE;
}

int LDC_GetConfigInt(const char* key) {
    if (_stricmp(key, "width") == 0) return s_config.width;
    if (_stricmp(key, "height") == 0) return s_config.height;
    return 0;
}
