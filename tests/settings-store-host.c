#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "diskio.h"
#include "settings_store.h"

#define TEST_SECTORS 4096U
#define SECTOR_SIZE 512U

static unsigned char disk[TEST_SECTORS * SECTOR_SIZE];
static int write_fail_at = -1;
static int write_count;
static int powered_off;
static int sync_fail;
static FATFS fs;

static void check(int condition, const char *message)
{
	if(!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

DSTATUS disk_initialize(BYTE pdrv) { return (pdrv == 0) ? 0 : STA_NOINIT; }
DSTATUS disk_status(BYTE pdrv) { return (pdrv == 0) ? 0 : STA_NOINIT; }

DRESULT disk_read(BYTE pdrv, BYTE *buffer, LBA_t sector, UINT count)
{
	if((pdrv != 0) || (sector > TEST_SECTORS) || (count > TEST_SECTORS - sector))
		return RES_PARERR;
	memcpy(buffer, disk + (size_t)sector * SECTOR_SIZE, (size_t)count * SECTOR_SIZE);
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buffer, LBA_t sector, UINT count)
{
	UINT i;
	if((pdrv != 0) || (sector > TEST_SECTORS) || (count > TEST_SECTORS - sector))
		return RES_PARERR;
	for(i = 0; i < count; i++)
	{
		if(powered_off || (write_fail_at >= 0 && write_count == write_fail_at))
		{
			powered_off = 1;
			return RES_ERROR;
		}
		memcpy(disk + (size_t)(sector + i) * SECTOR_SIZE,
		       buffer + (size_t)i * SECTOR_SIZE, SECTOR_SIZE);
		write_count++;
	}
	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE command, void *buffer)
{
	if(pdrv != 0)
		return RES_PARERR;
	switch(command)
	{
		case CTRL_SYNC:
			if(powered_off || sync_fail)
			{
				sync_fail = 0;
				return RES_ERROR;
			}
			return RES_OK;
		case GET_SECTOR_COUNT: *(LBA_t*)buffer = TEST_SECTORS; return RES_OK;
		case GET_SECTOR_SIZE: *(WORD*)buffer = SECTOR_SIZE; return RES_OK;
		case GET_BLOCK_SIZE: *(DWORD*)buffer = 1; return RES_OK;
		default: return RES_PARERR;
	}
}

DWORD get_fattime(void) { return ((DWORD)(2026 - 1980) << 25) | ((DWORD)1 << 21) | ((DWORD)1 << 16); }

static void mount_disk(void)
{
	FRESULT result;
	powered_off = 0;
	write_fail_at = -1;
	sync_fail = 0;
	LauncherSettingsStoreResetCache();
	result = f_mount(&fs, "0:", 1);
	check(result == FR_OK, "mount disk");
}

static void format_disk(void)
{
	MKFS_PARM options;
	unsigned char work[4096];
	f_mount(NULL, "0:", 0);
	memset(&options, 0, sizeof(options));
	options.fmt = FM_FAT | FM_SFD;
	check(f_mkfs("0:", &options, work, sizeof(work)) == FR_OK, "format in-memory FAT volume");
	mount_disk();
	check(f_mkdir("0:/SYSTEM") == FR_OK, "create SYSTEM directory");
}

static void write_legacy_file(void)
{
	FIL file;
	UINT written;
	const char text[] = "Version = legacy\n";
	check(f_open(&file, "0:/SYSTEM/SETTINGS.TXT", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK,
	      "create legacy settings file");
	check(f_write(&file, text, sizeof(text) - 1, &written) == FR_OK && written == sizeof(text) - 1,
	      "write legacy settings file");
	check(f_close(&file) == FR_OK, "close legacy settings file");
}

static void replace_primary_with_external_edit(void)
{
	FIL file;
	UINT written;
	const char text[] = "Version = external-edit\n";
	check(f_open(&file, LAUNCHER_SETTINGS_PRIMARY, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK,
	      "open canonical settings file through installer-style write");
	check(f_write(&file, text, sizeof(text) - 1, &written) == FR_OK && written == sizeof(text) - 1,
	      "write installer-style plain-text settings edit");
	check(f_close(&file) == FR_OK, "close installer-style settings edit");
	LauncherSettingsStoreResetCache();
}

static int save_value(const char *value)
{
	LauncherSettingsWriter writer;
	if(!LauncherSettingsStoreBegin(&writer, "0:/SYSTEM/SETTINGS.TXT"))
		return 0;
	LauncherSettingsStorePrintf(&writer, "Version = %s\n", value);
	return LauncherSettingsStoreCommit(&writer);
}

static int read_value(char *out, size_t size)
{
	FIL file;
	char line[64];
	UINT total = 0;
	FRESULT result = LauncherSettingsStoreOpenRead(&file, "0:/SYSTEM/SETTINGS.TXT");
	if(result != FR_OK)
		return 0;
	while((total + 1 < size) && f_read(&file, line, sizeof(line), &total) == FR_OK && total)
	{
		UINT i;
		for(i = 0; i < total && i + 1 < size; i++)
			out[i] = line[i];
		out[i] = 0;
		break;
	}
	f_close(&file);
	return out[0] != 0;
}

static void expect_value(const char *expected)
{
	char value[64] = {0};
	check(read_value(value, sizeof(value)), "read selected settings payload");
	if(!strstr(value, expected))
		fprintf(stderr, "Expected [%s], received [%s]\n", expected, value);
	check(strstr(value, expected) != NULL, "selected settings value matches expected generation");
}

static void test_legacy_and_primary_text(void)
{
	format_disk();
	write_legacy_file();
	expect_value("legacy");
	check(save_value("first"), "commit first transactional settings file");
	expect_value("first");
	replace_primary_with_external_edit();
	expect_value("external-edit");
	check(f_stat(LAUNCHER_SETTINGS_PRIMARY, &(FILINFO){0}) == FR_OK, "keep canonical SETTINGS.TXT path");
	check(save_value("second"), "commit next transactional settings file");
	expect_value("second");
	check(f_stat(LAUNCHER_SETTINGS_BACKUP, &(FILINFO){0}) == FR_OK, "retain verified previous settings backup");
	f_mount(NULL, "0:", 0);
	puts("PASS: legacy read and canonical readable SETTINGS.TXT updates");
}

static unsigned char baseline[TEST_SECTORS * SECTOR_SIZE];

static void test_write_interruption_points(void)
{
	int cut;
	format_disk();
	check(save_value("previous"), "commit previous generation");
	memcpy(baseline, disk, sizeof(disk));
	for(cut = 0; cut < 16; cut++)
	{
		int saved;
		memcpy(disk, baseline, sizeof(disk));
		write_count = 0;
		mount_disk();
		write_fail_at = cut;
		saved = save_value("replacement");
		f_mount(NULL, "0:", 0);
		powered_off = 0;
		write_fail_at = -1;
		mount_disk();
		{
			char value[64] = {0};
			check(read_value(value, sizeof(value)), "settings remain readable after interrupted update");
			check(strstr(value, "previous") || strstr(value, "replacement"),
			      "recovery selects one complete settings generation");
			if(saved)
				check(strstr(value, "replacement") != NULL, "successful update selects replacement generation");
		}
	}
	puts("PASS: injected sector-write interruption recovers old or complete new SETTINGS.TXT");
}

static void test_first_save_interruption_points(void)
{
	int cut;
	for(cut = 0; cut < 16; cut++)
	{
		LauncherSettingsWriter writer;
		int committed;
		int began;
		char value[64] = {0};
		format_disk();
		write_count = 0;
		write_fail_at = cut;
		began = LauncherSettingsStoreBegin(&writer, LAUNCHER_SETTINGS_PRIMARY);
		committed = 0;
		if(began)
		{
			LauncherSettingsStorePrintf(&writer, "Version = first-ever\n");
			committed = LauncherSettingsStoreCommit(&writer);
		}
		f_mount(NULL, "0:", 0);
		mount_disk();
		if(read_value(value, sizeof(value)))
			check(strstr(value, "first-ever") != NULL, "first-save recovery exposes only complete settings");
		else
			check(!committed, "failed first-ever save may leave no settings file");
	}
	puts("PASS: first-save interruption exposes either no file or a complete settings file");
}

static void test_missing_primary_recovers_backup(void)
{
	FILINFO info;
	format_disk();
	check(save_value("valid-old"), "commit old slot");
	check(save_value("valid-new"), "commit new slot");
	check(f_unlink(LAUNCHER_SETTINGS_PRIMARY) == FR_OK, "simulate interruption after old primary removal");
	LauncherSettingsStoreResetCache();
	check(f_stat(LAUNCHER_SETTINGS_BACKUP, &info) == FR_OK, "backup remains available after missing primary");
	expect_value("valid-old");
	f_mount(NULL, "0:", 0);
	puts("PASS: missing canonical file recovers previous verified backup");
}

static void test_flush_failure_and_oversized_write(void)
{
	LauncherSettingsWriter writer;
	char large[300];
	format_disk();
	check(save_value("survivor"), "commit settings before flush-failure test");
	check(LauncherSettingsStoreBegin(&writer, "0:/SYSTEM/SETTINGS.TXT"), "begin flush-failure update");
	LauncherSettingsStorePrintf(&writer, "Version = flush-failure\n");
	sync_fail = 1;
	check(!LauncherSettingsStoreCommit(&writer), "flush failure is reported as failed commit");
	LauncherSettingsStoreResetCache();
	expect_value("survivor");
	memset(large, 'X', sizeof(large) - 1);
	large[sizeof(large) - 1] = 0;
	check(LauncherSettingsStoreBegin(&writer, "0:/SYSTEM/SETTINGS.TXT"), "begin oversized-payload test");
	check(LauncherSettingsStorePrintf(&writer, "%s", large) < 0, "oversized formatted record is rejected");
	check(!LauncherSettingsStoreCommit(&writer), "failed serializer cannot commit a slot");
	LauncherSettingsStoreResetCache();
	expect_value("survivor");
	f_mount(NULL, "0:", 0);
	puts("PASS: flush failures and serializer overflow preserve previous settings");
}

static void test_full_volume(void)
{
	FIL filler;
	BYTE data[512];
	UINT written;
	format_disk();
	check(save_value("survivor"), "commit settings before filling media");
	memset(data, 0x5A, sizeof(data));
	check(f_open(&filler, "0:/FILL.BIN", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK, "open volume filler");
	while(f_write(&filler, data, sizeof(data), &written) == FR_OK && written == sizeof(data)) {}
	f_close(&filler);
	check(!save_value("cannot-fit"), "full media rejects alternate-slot update");
	f_mount(NULL, "0:", 0);
	mount_disk();
	expect_value("survivor");
	f_mount(NULL, "0:", 0);
	puts("PASS: full media preserves previous canonical settings");
}

int main(void)
{
	test_legacy_and_primary_text();
	test_write_interruption_points();
	test_first_save_interruption_points();
	test_missing_primary_recovers_backup();
	test_flush_failure_and_oversized_write();
	test_full_volume();
	puts("All settings-store host checks passed.");
	return 0;
}
