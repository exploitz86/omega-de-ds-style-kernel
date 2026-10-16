#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"
#include "diskio.h"
#include "savefile_policy.h"

typedef uint32_t u32;
typedef uint8_t u8;
#define IWRAM_CODE
static u8 pReadCache[0x20000];

/* The host test includes the exact SavefileWrite() body extracted from firmware. */
#include SAVEFILEWRITE_IMPL

#define TEST_SECTORS 4096U
#define SECTOR_SIZE 512U
static BYTE disk[TEST_SECTORS * SECTOR_SIZE];
static BYTE formatted_disk[TEST_SECTORS * SECTOR_SIZE];
static FATFS fs;
static long fail_at = -1;
static long write_calls;
static int injected_failure;
static int inject_next_sram_write;
static int failures;
static const char long_save_path[] = "0:/Omega Definitive Save Creation Regression.sav";

#define SRAMSaver 0U
void SetRampage(uint16_t page) { (void)page; }
void ReadSram(uint32_t address, uint8_t *data, uint32_t size)
{
	(void)address;
	memset(data, 0x5A, size);
	if(inject_next_sram_write)
	{
		fail_at = write_calls;
		inject_next_sram_write = 0;
	}
}

/* The host test also includes the exact Save_savefile() body. */
#include SAVESAVEFILE_IMPL

#define CHECK(condition, message) do { \
	if(condition) printf("  ok    %s\n", message); \
	else { printf("  FAIL  %s\n", message); failures++; } \
} while(0)

DSTATUS disk_initialize(BYTE pdrv) { return pdrv == 0 ? 0 : STA_NOINIT; }
DSTATUS disk_status(BYTE pdrv) { return pdrv == 0 ? 0 : STA_NOINIT; }

DRESULT disk_read(BYTE pdrv, BYTE *buffer, LBA_t sector, UINT count)
{
	if(pdrv != 0 || sector > TEST_SECTORS || count > TEST_SECTORS - sector)
		return RES_PARERR;
	memcpy(buffer, disk + (size_t)sector * SECTOR_SIZE, (size_t)count * SECTOR_SIZE);
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buffer, LBA_t sector, UINT count)
{
	long call = write_calls++;
	if(pdrv != 0 || sector > TEST_SECTORS || count > TEST_SECTORS - sector)
		return RES_PARERR;
	if(call == fail_at)
	{
		injected_failure = 1;
		return RES_ERROR;
	}
	memcpy(disk + (size_t)sector * SECTOR_SIZE, buffer, (size_t)count * SECTOR_SIZE);
	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE command, void *buffer)
{
	if(pdrv != 0)
		return RES_PARERR;
	switch(command)
	{
		case CTRL_SYNC: return RES_OK;
		case GET_SECTOR_COUNT: *(LBA_t *)buffer = TEST_SECTORS; return RES_OK;
		case GET_SECTOR_SIZE: *(WORD *)buffer = SECTOR_SIZE; return RES_OK;
		case GET_BLOCK_SIZE: *(DWORD *)buffer = 1; return RES_OK;
		default: return RES_PARERR;
	}
}

DWORD get_fattime(void)
{
	return ((DWORD)(2026 - 1980) << 25) | ((DWORD)10 << 21) | ((DWORD)16 << 16);
}

static void check(FRESULT result, const char *message)
{
	if(result != FR_OK)
	{
		fprintf(stderr, "FAIL: %s (FatFs result %u)\n", message, (unsigned)result);
		exit(2);
	}
}

static void mount_fresh_formatted_volume(void)
{
	check(f_mount(NULL, "0:", 0), "unmount before restoring test volume");
	memcpy(disk, formatted_disk, sizeof(disk));
	check(f_mount(&fs, "0:", 1), "mount test volume");
}

static void format_volume(void)
{
	MKFS_PARM options;
	BYTE work[4096];
	memset(disk, 0, sizeof(disk));
	memset(&options, 0, sizeof(options));
	options.fmt = FM_FAT | FM_SFD;
	check(f_mkfs("0:", &options, work, sizeof(work)), "format in-memory FAT volume");
	check(f_mount(&fs, "0:", 1), "mount formatted volume");
	memcpy(formatted_disk, disk, sizeof(disk));
}

static void test_new_save_creation(void)
{
	FIL file;
	FILINFO info;
	FRESULT result;
	BYTE contents[0x8000];
	UINT bytes_read = 0;

	result = f_open(&file, "0:/MISSING.SAV", FA_OPEN_EXISTING);
	CHECK(result == FR_NO_FILE && SaveFileOpenMeansMissing(result),
		"missing file is the only open result that permits save creation");
	result = f_open(&file, "0:/MISSING_DIR/MISSING.SAV", FA_OPEN_EXISTING);
	CHECK(result == FR_NO_PATH && !SaveFileOpenMeansMissing(result),
		"missing directory is not mistaken for a missing save");
	CHECK(!SaveFileOpenMeansMissing(FR_DISK_ERR),
		"disk I/O error is not mistaken for a missing save");

	CHECK(SavefileWrite("0:/OK.SAV", sizeof(contents)) == 1,
		"successful first-time save returns success");
	result = f_stat("0:/OK.SAV", &info);
	CHECK(result == FR_OK && info.fsize == sizeof(contents),
		"successful save has the requested full size");
	result = f_open(&file, "0:/OK.SAV", FA_READ);
	if(result == FR_OK)
	{
		result = f_read(&file, contents, sizeof(contents), &bytes_read);
		CHECK(result == FR_OK && bytes_read == sizeof(contents),
			"read back the complete newly created save");
		if(result == FR_OK && bytes_read == sizeof(contents))
		{
			size_t i;
			int all_erased = 1;
			for(i = 0; i < sizeof(contents); i++)
				if(contents[i] != 0xFF) { all_erased = 0; break; }
			CHECK(all_erased, "new save contents are initialized to 0xFF");
		}
		check(f_close(&file), "close successful save read");
	}
	else
	{
		CHECK(0, "open successful save for readback");
	}

	CHECK(SavefileWrite((TCHAR*)long_save_path, sizeof(contents)) == 1,
		"successful first-time save supports a long filename");
	result = f_stat(long_save_path, &info);
	CHECK(result == FR_OK && info.fsize == sizeof(contents),
		"long-filename save is discoverable at its complete size");
}

static void fill_card_leaving_clusters(UINT keep_free)
{
	FIL file;
	FATFS *mounted_fs;
	DWORD free_clusters;
	BYTE block[SECTOR_SIZE];
	UINT written;
	DWORD i;

	memset(block, 0xA5, sizeof(block));
	check(f_getfree("0:", &free_clusters, &mounted_fs), "query free clusters");
	check(f_open(&file, "0:/FILLER.BIN", FA_WRITE | FA_CREATE_ALWAYS), "create filler file");
	for(i = 0; i < (free_clusters - keep_free) * mounted_fs->csize; i++)
	{
		FRESULT result = f_write(&file, block, sizeof(block), &written);
		if(result != FR_OK || written != sizeof(block))
		{
			fprintf(stderr, "FAIL: fill test card (FatFs result %u, wrote %u)\n",
				(unsigned)result, written);
			exit(2);
		}
	}
	check(f_close(&file), "close filler file");
}

static void test_full_card_cleanup(void)
{
	FILINFO info;
	FRESULT result;

	mount_fresh_formatted_volume();
	fill_card_leaving_clusters(20);
	CHECK(SavefileWrite("0:/FULL.SAV", 0x8000) == 0,
		"full card causes first-time save creation to fail");
	result = f_stat("0:/FULL.SAV", &info);
	CHECK(result == FR_NO_FILE, "full-card failure leaves no partial save file");
}

static void test_write_failure_cleanup(void)
{
	long clean_write_count;
	long position;
	int failures_with_partial_files = 0;
	int failures_seen = 0;

	mount_fresh_formatted_volume();
	write_calls = 0;
	fail_at = -1;
	injected_failure = 0;
	CHECK(SavefileWrite("0:/COUNT.SAV", 0x8000) == 1,
		"baseline save succeeds before write-failure sweep");
	clean_write_count = write_calls;

	for(position = 0; position < clean_write_count; position++)
	{
		FILINFO info;
		FRESULT result;
		UINT index;

		mount_fresh_formatted_volume();
		write_calls = 0;
		fail_at = position;
		injected_failure = 0;
		if(SavefileWrite((TCHAR*)long_save_path, 0x8000) == 0)
			failures_seen++;
		fail_at = -1;
		if(injected_failure)
		{
			index = (UINT)position;
			result = f_stat(long_save_path, &info);
			if(result != FR_NO_FILE)
			{
				printf("  FAIL  injected disk_write #%u left a %lu-byte save\n",
					index, (unsigned long)info.fsize);
				failures_with_partial_files++;
			}
		}
	}
	CHECK(clean_write_count > 0, "save operation performs disk writes");
	CHECK(failures_seen > 0, "write-error sweep triggers SavefileWrite failures");
	CHECK(failures_with_partial_files == 0,
		"each injected write error leaves no discoverable partial save");
}

/* Characterize the unresolved behavior: failed replacement removes the old file. */
static void test_existing_save_overwrite_failure(void)
{
	FIL file;
	FILINFO info;
	BYTE old_contents[0x800];
	UINT written = 0;
	FRESULT result;
	u32 save_result;

	mount_fresh_formatted_volume();
	memset(old_contents, 0xA5, sizeof(old_contents));
	check(f_open(&file, long_save_path, FA_WRITE | FA_CREATE_ALWAYS),
		"create pre-existing LFN save fixture");
	check(f_write(&file, old_contents, sizeof(old_contents), &written),
		"write pre-existing save contents");
	CHECK(written == sizeof(old_contents), "pre-existing save fixture is complete");
	check(f_close(&file), "close pre-existing save fixture");
	result = f_stat(long_save_path, &info);
	CHECK(result == FR_OK && info.fsize == sizeof(old_contents),
		"precondition: old LFN save exists before overwrite");

	write_calls = 0;
	fail_at = -1;
	injected_failure = 0;
	inject_next_sram_write = 1;
	save_result = Save_savefile((TCHAR*)long_save_path, sizeof(old_contents));
	inject_next_sram_write = 0;
	fail_at = -1;
	CHECK(injected_failure, "overwrite fault is injected after Save_savefile opens the old path");
	CHECK(save_result == 0, "failed overwrite is reported to the caller");
	result = f_stat(long_save_path, &info);
	CHECK(result == FR_NO_FILE,
		"failed overwrite removes the replacement and leaves the previous save unavailable");
}

int main(void)
{
	printf("[save-file creation, overwrite, and failure handling]\n");
	format_volume();
	test_new_save_creation();
	test_full_card_cleanup();
	test_write_failure_cleanup();
	test_existing_save_overwrite_failure();
	printf("%s (%d failure%s)\n", failures ? "FAILED" : "PASSED",
		failures, failures == 1 ? "" : "s");
	return failures ? 1 : 0;
}
