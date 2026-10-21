#include <gba_base.h>

#include "ff.h"
//---------------------------------------------------------------
void Chip_Reset();
u8 Block_Erase(u32 blockAdd);
u8 Chip_Erase();
u8 FormatNor();
u8 WriteFlash(u32 address,u8 *buffer,u32 size);
u8 WriteFlash_with32word(u32 address,u8 *buffer,u32 size);
u32 Loadfile2NOR(TCHAR *filename, u32 NORaddress,u16 have_patch,u8 SAVEMODE);
u32 GetFileListFromNor(void);
