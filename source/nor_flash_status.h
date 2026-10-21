#ifndef NOR_FLASH_STATUS_H
#define NOR_FLASH_STATUS_H

#include <stdint.h>

#define NOR_STATUS_DQ6 0x0040u
#define NOR_STATUS_DQ5 0x0020u
#define NOR_STATUS_DQ1 0x0002u

typedef enum NOR_StatusDecision
{
	NOR_STATUS_BUSY,
	NOR_STATUS_VERIFY_DATA,
	NOR_STATUS_CONFIRM_DQ5,
	NOR_STATUS_BUFFER_ABORT,
	NOR_STATUS_OPERATION_FAILED,
	NOR_STATUS_OPERATION_COMPLETE
} NOR_StatusDecision;

/*
 * Classify two successive reads from a manufacturer-valid status address.
 * DQ1 is meaningful for write-buffer operations while DQ6 is toggling.
 * DQ5 needs a confirming pair because it may rise as DQ6 stops toggling.
 */
static __inline__ __attribute__((always_inline)) NOR_StatusDecision NOR_ClassifyStatusPair(uint16_t first,
	uint16_t second, int write_buffer)
{
	int dq6_toggles = ((first ^ second) & NOR_STATUS_DQ6) != 0;

	if(write_buffer && dq6_toggles && (second & NOR_STATUS_DQ1))
		return NOR_STATUS_BUFFER_ABORT;
	if(second & NOR_STATUS_DQ5)
		return NOR_STATUS_CONFIRM_DQ5;
	if(dq6_toggles)
		return NOR_STATUS_BUSY;
	return NOR_STATUS_VERIFY_DATA;
}

/* PPB operations use DQ6/DQ5 polling, then verify the PPB's DQ0 status. */
static __inline__ __attribute__((always_inline)) NOR_StatusDecision NOR_ClassifyPPBStatusPair(uint16_t first,
	uint16_t second)
{
	return NOR_ClassifyStatusPair(first, second, 0);
}

static __inline__ __attribute__((always_inline)) int NOR_PPBStatusIsErased(uint16_t status)
{
	return (status & 0x0001u) != 0;
}

/* After DQ5, continued DQ6 toggling means a terminal device error. */
static __inline__ __attribute__((always_inline)) NOR_StatusDecision NOR_ClassifyDQ5ConfirmPair(uint16_t first,
	uint16_t second)
{
	if((first ^ second) & NOR_STATUS_DQ6)
		return NOR_STATUS_OPERATION_FAILED;
	return NOR_STATUS_VERIFY_DATA;
}

/* Require two stable array reads to match the expected word exactly. */
static __inline__ __attribute__((always_inline)) NOR_StatusDecision NOR_ClassifyReadbackPair(uint16_t first,
	uint16_t second, uint16_t expected)
{
	if(first == expected && second == expected)
		return NOR_STATUS_OPERATION_COMPLETE;
	if((first ^ second) & NOR_STATUS_DQ6)
		return NOR_STATUS_BUSY;
	return NOR_STATUS_OPERATION_FAILED;
}

#endif
