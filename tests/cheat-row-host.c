#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"

typedef uint32_t u32;
typedef uint8_t u8;

#define MAX_BUF_LEN 6000
#define MAX_KEY_LEN 50
#define MAX_CHEAT_ENTRIES 128

typedef struct CHT_LINE
{
	char LINEname[MAX_KEY_LEN];
	u8 is_section;
	u8 section_val_count;
	u8 len;
	u8 select;
} FM_CHT_LINE;

typedef struct ST_entry_
{
	u32 address;
	u32 VAL;
} ST_entry;

static u8 lines_storage[4 * sizeof(FM_CHT_LINE)];
u8 *pCHTbuffer = lines_storage;
ST_entry pCHEAT[MAX_CHEAT_ENTRIES];
u32 gl_cheat_count;
u32 gl_cheat_selected_count;
char _paramv[MAX_BUF_LEN];
FIL gfile;

static void CheatSelectionRemember(u32 total)
{
	(void)total;
}

static u32 Get_CHT_val(FIL *file, char *section, char *key)
{
	const char *value;
	(void)file;
	(void)section;
	if(strcmp(key, "First") == 0)
		value = "02000000,FF";
	else if(strcmp(key, "Second") == 0)
		value = "02000001";
	else
		return 0;
	strcpy(_paramv, value);
	return (u32)strlen(value);
}

/* Exact str2hex() and Analyze_KEYVAL() bodies extracted from source/showcht.c. */
#include CHEAT_IMPL

int main(void)
{
	FM_CHT_LINE *lines = (FM_CHT_LINE *)pCHTbuffer;
	memset(lines, 0, sizeof(lines_storage));
	lines[0].is_section = 1;
	strcpy(lines[0].LINEname, "Game");
	strcpy(lines[1].LINEname, "First");
	lines[1].select = 1;
	lines[2].is_section = 1;
	strcpy(lines[2].LINEname, "Game");
	strcpy(lines[3].LINEname, "Second");
	lines[3].select = 1;

	Analyze_KEYVAL(&gfile, 4);
	if(gl_cheat_count != 2 || pCHEAT[0].address != 0x02000000 ||
		pCHEAT[0].VAL != 0xFF || pCHEAT[1].address != 0x02000001 ||
		pCHEAT[1].VAL != 0)
	{
		fprintf(stderr, "FAIL: row values leaked: count=%u first=%08x second=%08x/%08x\n",
			(unsigned)gl_cheat_count, (unsigned)pCHEAT[0].VAL,
			(unsigned)pCHEAT[1].address, (unsigned)pCHEAT[1].VAL);
		return 1;
	}
	puts("PASSED: an address-only row starts with an empty value buffer");
	return 0;
}
