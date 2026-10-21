#ifndef NOR_CATALOG_RECOVERY_H
#define NOR_CATALOG_RECOVERY_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define NOR_CATALOG_RECOVERY_HEADER_SIZE 64U
#define NOR_CATALOG_RECOVERY_ACTIVE_OFFSET 60U
#define NOR_CATALOG_RECOVERY_RETIRED_OFFSET 62U
#define NOR_CATALOG_RECOVERY_ACTIVE_WORD 0xA55AU
#define NOR_CATALOG_RECOVERY_RETIRED_WORD 0x0000U

typedef enum NOR_CatalogHeaderState
{
	NOR_CATALOG_HEADER_LEGACY = 0,
	NOR_CATALOG_HEADER_ACTIVE = 1,
	NOR_CATALOG_HEADER_RETIRED = 2,
	NOR_CATALOG_HEADER_INVALID = 3
} NOR_CatalogHeaderState;

typedef enum NOR_CatalogSource
{
	NOR_CATALOG_SOURCE_NONE = 0,
	NOR_CATALOG_SOURCE_PRIMARY = 1,
	NOR_CATALOG_SOURCE_SHADOW = 2
} NOR_CatalogSource;

static inline uint32_t NOR_CatalogCRC32(const uint8_t *data, size_t size)
{
	uint32_t crc = 0xFFFFFFFFU;
	size_t i;
	unsigned bit;

	for(i = 0; i < size; ++i)
	{
		crc ^= data[i];
		for(bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
	}
	return ~crc;
}

static inline void NOR_CatalogWriteLE16(uint8_t *destination, uint16_t value)
{
	destination[0] = (uint8_t)value;
	destination[1] = (uint8_t)(value >> 8);
}

static inline void NOR_CatalogWriteLE32(uint8_t *destination, uint32_t value)
{
	destination[0] = (uint8_t)value;
	destination[1] = (uint8_t)(value >> 8);
	destination[2] = (uint8_t)(value >> 16);
	destination[3] = (uint8_t)(value >> 24);
}

static inline uint16_t NOR_CatalogReadLE16(const uint8_t *source)
{
	return (uint16_t)((uint16_t)source[0] | ((uint16_t)source[1] << 8));
}

static inline uint32_t NOR_CatalogReadLE32(const uint8_t *source)
{
	return (uint32_t)source[0] |
		((uint32_t)source[1] << 8) |
		((uint32_t)source[2] << 16) |
		((uint32_t)source[3] << 24);
}

static inline int NOR_CatalogHeaderIsBlank(const uint8_t *header)
{
	size_t i;

	for(i = 0; i < NOR_CATALOG_RECOVERY_HEADER_SIZE; ++i)
		if(header[i] != 0xFF)
			return 0;
	return 1;
}

static inline int NOR_CatalogBuildHeader(uint8_t *header,
	const uint8_t *catalog, size_t catalog_size)
{
	const uint32_t magic = 0x5441434EU;
	uint32_t crc;

	if(!header || !catalog || !catalog_size || catalog_size > 0xFFFFFFFFU)
		return 0;

	crc = NOR_CatalogCRC32(catalog, catalog_size);
	memset(header, 0xFF, NOR_CATALOG_RECOVERY_HEADER_SIZE);
	NOR_CatalogWriteLE32(header + 0, magic);
	NOR_CatalogWriteLE16(header + 4, 1);
	NOR_CatalogWriteLE16(header + 6, NOR_CATALOG_RECOVERY_HEADER_SIZE);
	NOR_CatalogWriteLE32(header + 8, (uint32_t)catalog_size);
	NOR_CatalogWriteLE32(header + 12, crc);
	NOR_CatalogWriteLE32(header + 16, ~crc);
	NOR_CatalogWriteLE32(header + 20, ~magic);
	NOR_CatalogWriteLE32(header + 24, ~(uint32_t)catalog_size);
	NOR_CatalogWriteLE32(header + 28, NOR_CatalogCRC32(header, 28));
	NOR_CatalogWriteLE16(header + NOR_CATALOG_RECOVERY_ACTIVE_OFFSET,
		NOR_CATALOG_RECOVERY_ACTIVE_WORD);
	return 1;
}

static inline NOR_CatalogHeaderState NOR_CatalogInspectHeader(
	const uint8_t *catalog, size_t catalog_size, const uint8_t *header)
{
	const uint32_t magic = 0x5441434EU;
	uint32_t crc;
	size_t i;
	uint16_t retired;

	if(!catalog || !header || !catalog_size || catalog_size > 0xFFFFFFFFU)
		return NOR_CATALOG_HEADER_INVALID;
	if(NOR_CatalogHeaderIsBlank(header))
		return NOR_CATALOG_HEADER_LEGACY;

	crc = NOR_CatalogCRC32(catalog, catalog_size);
	if(NOR_CatalogReadLE32(header + 0) != magic ||
		NOR_CatalogReadLE16(header + 4) != 1 ||
		NOR_CatalogReadLE16(header + 6) != NOR_CATALOG_RECOVERY_HEADER_SIZE ||
		NOR_CatalogReadLE32(header + 8) != (uint32_t)catalog_size ||
		NOR_CatalogReadLE32(header + 12) != crc ||
		NOR_CatalogReadLE32(header + 16) != ~crc ||
		NOR_CatalogReadLE32(header + 20) != ~magic ||
		NOR_CatalogReadLE32(header + 24) != ~(uint32_t)catalog_size ||
		NOR_CatalogReadLE32(header + 28) != NOR_CatalogCRC32(header, 28) ||
		NOR_CatalogReadLE16(header + NOR_CATALOG_RECOVERY_ACTIVE_OFFSET) !=
			NOR_CATALOG_RECOVERY_ACTIVE_WORD)
		return NOR_CATALOG_HEADER_INVALID;

	for(i = 32; i < NOR_CATALOG_RECOVERY_ACTIVE_OFFSET; ++i)
		if(header[i] != 0xFF)
			return NOR_CATALOG_HEADER_INVALID;

	retired = NOR_CatalogReadLE16(header + NOR_CATALOG_RECOVERY_RETIRED_OFFSET);
	if(retired == 0xFFFF)
		return NOR_CATALOG_HEADER_ACTIVE;
	if(retired == NOR_CATALOG_RECOVERY_RETIRED_WORD)
		return NOR_CATALOG_HEADER_RETIRED;
	return NOR_CATALOG_HEADER_INVALID;
}

static inline NOR_CatalogSource NOR_CatalogChooseSource(
	NOR_CatalogHeaderState primary_state, NOR_CatalogHeaderState shadow_state,
	uint32_t primary_matches, uint32_t shadow_matches)
{
	if(primary_state == NOR_CATALOG_HEADER_ACTIVE)
		return NOR_CATALOG_SOURCE_PRIMARY;
	if(primary_state == NOR_CATALOG_HEADER_LEGACY &&
		shadow_state == NOR_CATALOG_HEADER_ACTIVE)
		return primary_matches >= shadow_matches ?
			NOR_CATALOG_SOURCE_PRIMARY : NOR_CATALOG_SOURCE_SHADOW;
	if(shadow_state == NOR_CATALOG_HEADER_ACTIVE)
		return NOR_CATALOG_SOURCE_SHADOW;
	if(primary_state == NOR_CATALOG_HEADER_LEGACY)
		return NOR_CATALOG_SOURCE_PRIMARY;
	return NOR_CATALOG_SOURCE_NONE;
}

#endif
