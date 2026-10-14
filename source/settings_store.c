#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "settings_store.h"

#define SETTINGS_MAX_FILE_BYTES 8192U
#define SETTINGS_FOOTER_BYTES 25U
#define SETTINGS_FOOTER_PREFIX "# DSSTYLE-CRC32="
#define SETTINGS_FOOTER_PREFIX_BYTES 16U

static int settings_recovery_checked;
static char settings_read_path[32];

void LauncherSettingsStoreResetCache(void)
{
	settings_recovery_checked = 0;
	settings_read_path[0] = '\0';
}

static uint32_t Settings_Crc32Update(uint32_t crc, const uint8_t *data, uint32_t length)
{
	uint32_t i;
	uint32_t bit;
	for(i = 0; i < length; i++)
	{
		crc ^= data[i];
		for(bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320U : 0U);
	}
	return crc;
}

static int Settings_ParseHex32(const char *text, uint32_t *value)
{
	uint32_t parsed = 0;
	uint32_t i;
	for(i = 0; i < 8; i++)
	{
		char ch = text[i];
		uint8_t digit;
		if(ch >= '0' && ch <= '9') digit = (uint8_t)(ch - '0');
		else if(ch >= 'A' && ch <= 'F') digit = (uint8_t)(ch - 'A' + 10);
		else if(ch >= 'a' && ch <= 'f') digit = (uint8_t)(ch - 'a' + 10);
		else return 0;
		parsed = (parsed << 4) | digit;
	}
	*value = parsed;
	return 1;
}

/* Return 0 for missing, 1 for checksum-valid, 2 for pre-journal plain text,
   and 3 for a damaged journal file. */
static int Settings_ValidateFile(const char *path, int require_footer)
{
	FIL file;
	FRESULT result;
	UINT count;
	FSIZE_t size;
	uint8_t footer[SETTINGS_FOOTER_BYTES];
	uint8_t buffer[256];
	uint32_t expected_crc;
	uint32_t crc = 0xFFFFFFFFU;
	uint32_t remaining;
	int footer_present;

	result = f_open(&file, path, FA_READ);
	if(result == FR_NO_FILE || result == FR_NO_PATH)
		return 0;
	if(result != FR_OK)
		return 3;
	size = f_size(&file);
	if((size < SETTINGS_FOOTER_BYTES) || (size > SETTINGS_MAX_FILE_BYTES))
	{
		f_close(&file);
		return require_footer ? 3 : 2;
	}
	result = f_lseek(&file, size - SETTINGS_FOOTER_BYTES);
	if(result == FR_OK)
		result = f_read(&file, footer, sizeof(footer), &count);
	if(result != FR_OK || count != sizeof(footer))
	{
		f_close(&file);
		return 3;
	}
	footer_present = memcmp(footer, SETTINGS_FOOTER_PREFIX, SETTINGS_FOOTER_PREFIX_BYTES) == 0;
	if(!footer_present)
	{
		f_close(&file);
		return require_footer ? 3 : 2;
	}
	if(footer[24] != '\n' || !Settings_ParseHex32((const char*)footer + SETTINGS_FOOTER_PREFIX_BYTES, &expected_crc))
	{
		f_close(&file);
		return 3;
	}
	result = f_lseek(&file, 0);
	if(result != FR_OK)
	{
		f_close(&file);
		return 3;
	}
	remaining = (uint32_t)size - SETTINGS_FOOTER_BYTES;
	while(remaining)
	{
		UINT wanted = (remaining > sizeof(buffer)) ? sizeof(buffer) : (UINT)remaining;
		result = f_read(&file, buffer, wanted, &count);
		if(result != FR_OK || count != wanted)
		{
			f_close(&file);
			return 3;
		}
		crc = Settings_Crc32Update(crc, buffer, count);
		remaining -= count;
	}
	result = f_close(&file);
	return (result == FR_OK && (crc ^ 0xFFFFFFFFU) == expected_crc) ? 1 : 3;
}

static int Settings_CopyToBackup(const char *primary, const char *backup)
{
	FIL src;
	FIL dst;
	FRESULT result;
	UINT read_count;
	UINT write_count;
	uint8_t buffer[256];
	uint32_t crc = 0xFFFFFFFFU;
	FSIZE_t bytes_to_copy;
	char footer[SETTINGS_FOOTER_BYTES + 1];
	int ok = 0;

	if(f_open(&src, primary, FA_READ) != FR_OK)
		return 0;
	bytes_to_copy = f_size(&src);
	if(Settings_ValidateFile(primary, 0) == 1)
		bytes_to_copy -= SETTINGS_FOOTER_BYTES;
	if(bytes_to_copy > SETTINGS_MAX_FILE_BYTES - SETTINGS_FOOTER_BYTES)
	{
		f_close(&src);
		return 0;
	}
	if(f_open(&dst, backup, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
	{
		f_close(&src);
		return 0;
	}
	result = FR_OK;
	while(result == FR_OK && bytes_to_copy)
	{
		UINT wanted = (bytes_to_copy > sizeof(buffer)) ? sizeof(buffer) : (UINT)bytes_to_copy;
		result = f_read(&src, buffer, wanted, &read_count);
		if(result != FR_OK || read_count != wanted)
			break;
		crc = Settings_Crc32Update(crc, buffer, read_count);
		result = f_write(&dst, buffer, read_count, &write_count);
		if(result != FR_OK || write_count != read_count)
			break;
		bytes_to_copy -= read_count;
	}
	if(result == FR_OK && f_error(&src) == 0)
	{
		snprintf(footer, sizeof(footer), SETTINGS_FOOTER_PREFIX "%08lX\n", (unsigned long)(crc ^ 0xFFFFFFFFU));
		result = f_write(&dst, footer, SETTINGS_FOOTER_BYTES, &write_count);
		if(result == FR_OK && write_count == SETTINGS_FOOTER_BYTES)
			result = f_sync(&dst);
		else if(result == FR_OK)
			result = FR_DISK_ERR;
	}
	if(f_close(&src) != FR_OK)
		result = FR_DISK_ERR;
	if(f_close(&dst) != FR_OK)
		result = FR_DISK_ERR;
	if(result == FR_OK && Settings_ValidateFile(backup, 1) == 1)
		ok = 1;
	return ok;
}

static int Settings_RenameToPrimary(const char *source, const char *primary)
{
	if(f_rename(source, primary) == FR_OK)
		return 1;
	return 0;
}

static const char *Settings_Recover(const char *primary)
{
	int primary_state;
	int backup_state;
	int temp_state;
	FILINFO info;
	if(settings_recovery_checked)
		return settings_read_path[0] ? settings_read_path : primary;
	settings_recovery_checked = 1;
	strncpy(settings_read_path, primary, sizeof(settings_read_path) - 1);
	settings_read_path[sizeof(settings_read_path) - 1] = '\0';
	primary_state = Settings_ValidateFile(primary, 0);
	backup_state = Settings_ValidateFile(LAUNCHER_SETTINGS_BACKUP, 1);
	temp_state = Settings_ValidateFile(LAUNCHER_SETTINGS_TEMP, 1);

	if(primary_state == 1 || primary_state == 2)
	{
		/* A complete primary means the transaction had not removed the old file,
		   or the new replacement was fully installed. Keep the backup as the
		   previous-good copy and discard an uncommitted temp. */
		f_unlink(LAUNCHER_SETTINGS_TEMP);
		return settings_read_path;
	}
	if(primary_state == 0 && backup_state == 1)
	{
		if(Settings_RenameToPrimary(LAUNCHER_SETTINGS_BACKUP, primary))
			return settings_read_path;
		strncpy(settings_read_path, LAUNCHER_SETTINGS_BACKUP, sizeof(settings_read_path) - 1);
		settings_read_path[sizeof(settings_read_path) - 1] = '\0';
		return settings_read_path;
	}
	if(primary_state == 3 && backup_state == 1 && temp_state == 1)
	{
		f_unlink(primary);
		if(Settings_RenameToPrimary(LAUNCHER_SETTINGS_BACKUP, primary))
		{
			f_unlink(LAUNCHER_SETTINGS_TEMP);
			return settings_read_path;
		}
		strncpy(settings_read_path, LAUNCHER_SETTINGS_BACKUP, sizeof(settings_read_path) - 1);
		settings_read_path[sizeof(settings_read_path) - 1] = '\0';
		return settings_read_path;
	}
	if(primary_state == 0 && temp_state == 1)
	{
		if(Settings_RenameToPrimary(LAUNCHER_SETTINGS_TEMP, primary))
			return settings_read_path;
		strncpy(settings_read_path, LAUNCHER_SETTINGS_TEMP, sizeof(settings_read_path) - 1);
		settings_read_path[sizeof(settings_read_path) - 1] = '\0';
		return settings_read_path;
	}
	/* An edited plain-text file with a stale checksum is user data. If no
	   transaction artifacts exist, preserve and read it as before. */
	if(primary_state == 3 && backup_state != 1 && temp_state == 1)
	{
		f_unlink(primary);
		if(Settings_RenameToPrimary(LAUNCHER_SETTINGS_TEMP, primary))
			return settings_read_path;
	}
	if(f_stat(primary, &info) == FR_NO_FILE && backup_state == 1)
	{
		strncpy(settings_read_path, LAUNCHER_SETTINGS_BACKUP, sizeof(settings_read_path) - 1);
		settings_read_path[sizeof(settings_read_path) - 1] = '\0';
	}
	return settings_read_path;
}

static int Settings_CommitFailed(void)
{
	LauncherSettingsStoreResetCache();
	return 0;
}

FRESULT LauncherSettingsStoreOpenRead(FIL *file, const char *legacy_path)
{
	const char *path;
	if(!file)
		return FR_INVALID_OBJECT;
	path = Settings_Recover(legacy_path);
	return f_open(file, path, FA_READ);
}

int LauncherSettingsStoreBegin(LauncherSettingsWriter *writer, const char *legacy_path)
{
	(void)legacy_path;
	if(!writer)
		return 0;
	memset(writer, 0, sizeof(*writer));
	if(f_open(&writer->file, LAUNCHER_SETTINGS_TEMP, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
		return 0;
	writer->crc = 0xFFFFFFFFU;
	return 1;
}

int LauncherSettingsStorePrintf(LauncherSettingsWriter *writer, const char *format, ...)
{
	char buffer[256];
	va_list args;
	int length;
	UINT count;
	FRESULT result;
	if(!writer || writer->failed || !format)
		return -1;
	va_start(args, format);
	length = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	if((length < 0) || ((uint32_t)length >= sizeof(buffer)) ||
	   ((uint32_t)length > SETTINGS_MAX_FILE_BYTES - SETTINGS_FOOTER_BYTES - writer->length))
	{
		writer->failed = 1;
		return -1;
	}
	result = f_write(&writer->file, buffer, (UINT)length, &count);
	if(result != FR_OK || count != (UINT)length)
	{
		writer->failed = 1;
		return -1;
	}
	writer->crc = Settings_Crc32Update(writer->crc, (const uint8_t*)buffer, (uint32_t)length);
	writer->length += (uint32_t)length;
	return length;
}

void LauncherSettingsStoreAbort(LauncherSettingsWriter *writer)
{
	if(writer && writer->file.obj.fs)
		f_close(&writer->file);
}

int LauncherSettingsStoreCommit(LauncherSettingsWriter *writer)
{
	FRESULT result;
	UINT count;
	char footer[SETTINGS_FOOTER_BYTES + 1];
	int primary_state;
	if(!writer || writer->failed)
	{
		LauncherSettingsStoreAbort(writer);
		return Settings_CommitFailed();
	}
	snprintf(footer, sizeof(footer), SETTINGS_FOOTER_PREFIX "%08lX\n", (unsigned long)(writer->crc ^ 0xFFFFFFFFU));
	result = f_write(&writer->file, footer, SETTINGS_FOOTER_BYTES, &count);
	if(result == FR_OK && count == SETTINGS_FOOTER_BYTES)
		result = f_sync(&writer->file);
	else if(result == FR_OK)
		result = FR_DISK_ERR;
	if(f_close(&writer->file) != FR_OK)
		result = FR_DISK_ERR;
	if(result != FR_OK || Settings_ValidateFile(LAUNCHER_SETTINGS_TEMP, 1) != 1)
		return Settings_CommitFailed();

	primary_state = Settings_ValidateFile(LAUNCHER_SETTINGS_PRIMARY, 0);
	if(primary_state != 0 && !Settings_CopyToBackup(LAUNCHER_SETTINGS_PRIMARY, LAUNCHER_SETTINGS_BACKUP))
		return Settings_CommitFailed();
	if(primary_state != 0 && f_unlink(LAUNCHER_SETTINGS_PRIMARY) != FR_OK)
		return Settings_CommitFailed();
	if(!Settings_RenameToPrimary(LAUNCHER_SETTINGS_TEMP, LAUNCHER_SETTINGS_PRIMARY))
		return Settings_CommitFailed();
	if(Settings_ValidateFile(LAUNCHER_SETTINGS_PRIMARY, 1) != 1)
		return Settings_CommitFailed();
	settings_recovery_checked = 1;
	strncpy(settings_read_path, LAUNCHER_SETTINGS_PRIMARY, sizeof(settings_read_path) - 1);
	settings_read_path[sizeof(settings_read_path) - 1] = '\0';
	return 1;
}
