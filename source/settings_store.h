#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <stdint.h>
#include "ff.h"

#define LAUNCHER_SETTINGS_PRIMARY "/SYSTEM/SETTINGS.TXT"
#define LAUNCHER_SETTINGS_TEMP "/SYSTEM/SETTINGS.TMP"
#define LAUNCHER_SETTINGS_BACKUP "/SYSTEM/SETTINGS.BAK"
#define LAUNCHER_SETTINGS_MAX_BYTES 8192U

typedef struct {
	FIL file;
	uint32_t crc;
	uint32_t length;
	uint8_t failed;
} LauncherSettingsWriter;

FRESULT LauncherSettingsStoreOpenRead(FIL *file, const char *legacy_path);
int LauncherSettingsStoreBegin(LauncherSettingsWriter *writer, const char *legacy_path);
int LauncherSettingsStorePrintf(LauncherSettingsWriter *writer, const char *format, ...);
int LauncherSettingsStoreCommit(LauncherSettingsWriter *writer);
void LauncherSettingsStoreAbort(LauncherSettingsWriter *writer);
void LauncherSettingsStoreResetCache(void);

#endif
