#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nor_catalog_transaction.h"

typedef struct CatalogEntry
{
	unsigned id;
	char name[12];
} CatalogEntry;

typedef struct PersistContext
{
	CatalogEntry *slot;
	const CatalogEntry *expected;
	int result;
	int called;
	int saw_candidate;
} PersistContext;

static int persist_catalog(void *opaque)
{
	PersistContext *context = (PersistContext*)opaque;
	context->called++;
	context->saw_candidate = memcmp(context->slot, context->expected,
		sizeof(*context->slot)) == 0;
	return context->result;
}

static void check(int condition, const char *message)
{
	if(!condition)
	{
		fprintf(stderr, "FAIL: %s\n", message);
		exit(1);
	}
}

int main(void)
{
	CatalogEntry original = { 7, "old-title" };
	CatalogEntry replacement = { 9, "new-title" };
	CatalogEntry backup;
	PersistContext context;

	memset(&context, 0, sizeof(context));
	context.slot = &original;
	context.expected = &replacement;
	context.result = 0;
	check(!NOR_CommitCatalogSlot(&original, &replacement, &backup,
		sizeof(original), persist_catalog, &context),
		"failed persistence reports failure");
	check(context.called == 1 && context.saw_candidate,
		"persistence callback sees the pending catalog entry");
	check(original.id == 7 && strcmp(original.name, "old-title") == 0,
		"failed persistence restores the previous in-memory entry");

	context.called = 0;
	context.saw_candidate = 0;
	context.result = 1;
	check(NOR_CommitCatalogSlot(&original, &replacement, &backup,
		sizeof(original), persist_catalog, &context),
		"successful persistence reports success");
	check(context.called == 1 && context.saw_candidate,
		"successful persistence sees the pending catalog entry");
	check(original.id == 9 && strcmp(original.name, "new-title") == 0,
		"successful persistence keeps the new in-memory entry");

	puts("NOR catalog transaction scenarios passed.");
	return 0;
}
