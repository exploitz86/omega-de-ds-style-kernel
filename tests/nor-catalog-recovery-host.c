#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nor_catalog_recovery.h"

#define CATALOG_SIZE 96U
#define IMAGE_SIZE (CATALOG_SIZE + NOR_CATALOG_RECOVERY_HEADER_SIZE)

static void check(int condition, const char *message)
{
	if(!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

static void fill_catalog(uint8_t *catalog, uint8_t seed)
{
	size_t i;
	for(i = 0; i < CATALOG_SIZE; ++i)
		catalog[i] = (uint8_t)(seed + (uint8_t)i);
}

static void build_image(uint8_t *image, const uint8_t *catalog)
{
	memcpy(image, catalog, CATALOG_SIZE);
	check(NOR_CatalogBuildHeader(image + CATALOG_SIZE, image, CATALOG_SIZE),
		"header builds for non-empty catalog");
}

static NOR_CatalogHeaderState image_state(const uint8_t *image)
{
	return NOR_CatalogInspectHeader(image, CATALOG_SIZE, image + CATALOG_SIZE);
}

static void program_prefix(uint8_t *slot, const uint8_t *image, size_t count)
{
	size_t i;
	memset(slot, 0xFF, IMAGE_SIZE);
	for(i = 0; i < count; ++i)
		slot[i] &= image[i];
}

static NOR_CatalogSource choose(const uint8_t *primary, const uint8_t *shadow,
	uint32_t primary_matches, uint32_t shadow_matches)
{
	return NOR_CatalogChooseSource(image_state(primary), image_state(shadow),
		primary_matches, shadow_matches);
}

static void test_header_validation(const uint8_t *old_image)
{
	uint8_t damaged[IMAGE_SIZE];

	check(image_state(old_image) == NOR_CATALOG_HEADER_ACTIVE,
		"new header is active before retirement");
	memcpy(damaged, old_image, IMAGE_SIZE);
	damaged[CATALOG_SIZE + 32] = 0;
	check(image_state(damaged) == NOR_CATALOG_HEADER_INVALID,
		"non-erased reserved header byte invalidates the image");
	memcpy(damaged, old_image, IMAGE_SIZE);
	damaged[0] ^= 1;
	check(image_state(damaged) == NOR_CATALOG_HEADER_INVALID,
		"catalog data checksum rejects changed payload");
	memset(damaged + CATALOG_SIZE, 0xFF,
		NOR_CATALOG_RECOVERY_HEADER_SIZE);
	check(image_state(damaged) == NOR_CATALOG_HEADER_LEGACY,
		"all-erased header is accepted as a legacy raw catalog");
}

static void test_shadow_write_cuts(const uint8_t *old_image)
{
	uint8_t primary[IMAGE_SIZE];
	uint8_t shadow[IMAGE_SIZE];
	size_t cut;

	for(cut = 0; cut <= IMAGE_SIZE; ++cut)
	{
		memcpy(primary, old_image, IMAGE_SIZE);
		program_prefix(shadow, old_image, cut);
		check(choose(primary, shadow, 3, 3) == NOR_CATALOG_SOURCE_PRIMARY,
			"interrupted shadow write leaves the valid primary selected");
		check(memcmp(primary, old_image, CATALOG_SIZE) == 0,
			"interrupted shadow write preserves the primary catalog");
	}
}

static void test_primary_write_cuts(const uint8_t *old_image,
	const uint8_t *new_image)
{
	uint8_t primary[IMAGE_SIZE];
	uint8_t shadow[IMAGE_SIZE];
	size_t cut;
	size_t active_end = CATALOG_SIZE + NOR_CATALOG_RECOVERY_ACTIVE_OFFSET + 2;

	for(cut = 0; cut <= IMAGE_SIZE; ++cut)
	{
		uint32_t primary_matches = (cut >= CATALOG_SIZE) ? 4 : 0;
		program_prefix(shadow, old_image, IMAGE_SIZE);
		program_prefix(primary, new_image, cut);
		if(cut == CATALOG_SIZE || cut >= active_end)
		{
			check(choose(primary, shadow, primary_matches, 3) == NOR_CATALOG_SOURCE_PRIMARY,
				"complete primary active marker selects the new catalog");
			check(memcmp(primary, new_image, CATALOG_SIZE) == 0,
				"selected committed primary is the full replacement");
		}
		else
		{
			check(choose(primary, shadow, primary_matches, 3) == NOR_CATALOG_SOURCE_SHADOW,
				"interrupted primary write recovers the old shadow");
			check(memcmp(shadow, old_image, CATALOG_SIZE) == 0,
				"selected recovery shadow retains the previous catalog");
		}
	}
}

static void test_retirement_states(const uint8_t *old_image,
	const uint8_t *new_image, const uint8_t *legacy_image)
{
	uint8_t primary[IMAGE_SIZE];
	uint8_t shadow[IMAGE_SIZE];
	uint16_t partial_values[] = { 0xFFFE, 0x7FFF, 0xA55A, 0x0001, 0x0000 };
	size_t i;

	memcpy(primary, new_image, IMAGE_SIZE);
	for(i = 0; i < sizeof(partial_values)/sizeof(partial_values[0]); ++i)
	{
		memcpy(shadow, old_image, IMAGE_SIZE);
		NOR_CatalogWriteLE16(shadow + CATALOG_SIZE +
			NOR_CATALOG_RECOVERY_RETIRED_OFFSET, partial_values[i]);
		check(choose(primary, shadow, 4, 3) == NOR_CATALOG_SOURCE_PRIMARY,
			"valid primary outranks active, partial, and retired shadows");
	}

	/* A completed retirement makes a later raw legacy write authoritative. */
	memcpy(shadow, old_image, IMAGE_SIZE);
	NOR_CatalogWriteLE16(shadow + CATALOG_SIZE +
		NOR_CATALOG_RECOVERY_RETIRED_OFFSET,
		NOR_CATALOG_RECOVERY_RETIRED_WORD);
	check(image_state(shadow) == NOR_CATALOG_HEADER_RETIRED,
		"fully programmed distinct retirement word retires the shadow");
	program_prefix(primary, legacy_image, CATALOG_SIZE);
	check(image_state(primary) == NOR_CATALOG_HEADER_LEGACY,
		"older firmware raw catalog is recognized as legacy");
	check(choose(primary, shadow, 4, 3) == NOR_CATALOG_SOURCE_PRIMARY,
		"retired shadow does not override an older firmware catalog");
	check(memcmp(primary, legacy_image, CATALOG_SIZE) == 0,
		"older firmware catalog remains readable after retirement");

	/* A failed retirement that changes no bits leaves an ambiguous stale copy. */
	memcpy(primary, new_image, IMAGE_SIZE);
	memcpy(shadow, old_image, IMAGE_SIZE);
	program_prefix(primary, legacy_image, CATALOG_SIZE);
	check(image_state(shadow) == NOR_CATALOG_HEADER_ACTIVE,
		"retirement failure before any programmed bit leaves shadow active");
	check(choose(primary, shadow, 4, 3) == NOR_CATALOG_SOURCE_PRIMARY,
		"legacy append with more hardware matches outranks stale shadow");
	check(choose(primary, shadow, 3, 3) == NOR_CATALOG_SOURCE_PRIMARY,
		"same-count legacy metadata update wins ties");
	check(choose(primary, shadow, 2, 3) == NOR_CATALOG_SOURCE_SHADOW,
		"fewer primary matches prefer the recoverable shadow");
}

static void test_primary_erase_cuts(const uint8_t *old_image)
{
	uint8_t primary[IMAGE_SIZE];
	uint8_t shadow[IMAGE_SIZE];
	size_t cuts[] = { 0, 1, CATALOG_SIZE / 2, CATALOG_SIZE,
		CATALOG_SIZE + 1, IMAGE_SIZE / 2, IMAGE_SIZE - 1, IMAGE_SIZE };
	size_t i, j;

	for(i = 0; i < sizeof(cuts)/sizeof(cuts[0]); ++i)
	{
		memcpy(primary, old_image, IMAGE_SIZE);
		memcpy(shadow, old_image, IMAGE_SIZE);
		for(j = 0; j < cuts[i]; ++j)
			primary[j] = 0xFF;
		check(choose(primary, shadow, 3, 3) != NOR_CATALOG_SOURCE_NONE,
			"every sampled primary erase interruption has a source");
		check(memcmp(shadow, old_image, CATALOG_SIZE) == 0,
			"completed shadow remains intact during primary erase");
	}
}

static void test_unretired_shadow_after_legacy_erase(const uint8_t *old_image,
	const uint8_t *current_image)
{
	uint8_t primary[IMAGE_SIZE];
	uint8_t shadow[IMAGE_SIZE];

	memcpy(primary, current_image, IMAGE_SIZE);
	memcpy(shadow, old_image, IMAGE_SIZE);
	check(choose(primary, shadow, 4, 3) == NOR_CATALOG_SOURCE_PRIMARY,
		"new-format reader uses the committed primary while it remains intact");

	/* An older Save_info erases the primary sector before writing raw catalog bytes. */
	memset(primary, 0xFF, IMAGE_SIZE);
	check(image_state(primary) == NOR_CATALOG_HEADER_LEGACY,
		"legacy primary erase appears as a blank raw catalog");
	check(choose(primary, shadow, 0, 3) == NOR_CATALOG_SOURCE_SHADOW,
		"unretired stale shadow wins during an interrupted legacy raw rewrite");
	check(memcmp(shadow, current_image, CATALOG_SIZE) != 0,
		"selected shadow predates the previously committed primary catalog");
}

int main(void)
{
	uint8_t old_catalog[CATALOG_SIZE];
	uint8_t new_catalog[CATALOG_SIZE];
	uint8_t legacy_catalog[CATALOG_SIZE];
	uint8_t old_image[IMAGE_SIZE];
	uint8_t new_image[IMAGE_SIZE];
	uint8_t legacy_image[IMAGE_SIZE];

	fill_catalog(old_catalog, 0x20);
	fill_catalog(new_catalog, 0x50);
	fill_catalog(legacy_catalog, 0x80);
	build_image(old_image, old_catalog);
	build_image(new_image, new_catalog);
	memcpy(legacy_image, legacy_catalog, CATALOG_SIZE);
	memset(legacy_image + CATALOG_SIZE, 0xFF,
		NOR_CATALOG_RECOVERY_HEADER_SIZE);

	check((8448U + NOR_CATALOG_RECOVERY_HEADER_SIZE) % 32U == 0,
		"target catalog image remains aligned to GL buffer writes");
	test_header_validation(old_image);
	test_shadow_write_cuts(old_image);
	test_primary_write_cuts(old_image, new_image);
	test_primary_erase_cuts(old_image);
	test_retirement_states(old_image, new_image, legacy_image);
	test_unretired_shadow_after_legacy_erase(old_image, new_image);

	puts("NOR catalog recovery transitions and legacy ranking scenarios passed.");
	return 0;
}
