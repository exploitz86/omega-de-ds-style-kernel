#include <stdio.h>
#include <stdlib.h>

#include "nor_flash_status.h"

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
	check(NOR_ClassifyStatusPair(0x0000, NOR_STATUS_DQ6, 0) == NOR_STATUS_BUSY,
		"DQ6 toggle without DQ5 remains busy");
	check(NOR_ClassifyPPBStatusPair(0x0000, NOR_STATUS_DQ6) == NOR_STATUS_BUSY,
		"PPB DQ6 toggle without DQ5 remains busy");
	check(NOR_ClassifyPPBStatusPair(0x0000, NOR_STATUS_DQ5 | NOR_STATUS_DQ6) == NOR_STATUS_CONFIRM_DQ5,
		"PPB DQ5 requests a confirming DQ6 pair");
	check(NOR_ClassifyPPBStatusPair(0x0000, 0x0000) == NOR_STATUS_VERIFY_DATA,
		"stable PPB status proceeds to DQ0 verification");
	check(NOR_PPBStatusIsErased(0xFFFF),
		"PPB DQ0 set means the protection bit is erased");
	check(!NOR_PPBStatusIsErased(0xFFFE),
		"PPB DQ0 clear means the protection bit remains programmed");
	check(NOR_ClassifyStatusPair(0x0010, 0x0010, 0) == NOR_STATUS_VERIFY_DATA,
		"stable status pair proceeds to readback verification");
	check(NOR_ClassifyStatusPair(0x0000, NOR_STATUS_DQ5, 0) == NOR_STATUS_CONFIRM_DQ5,
		"DQ5 requests a confirming pair even when DQ6 stopped");
	check(NOR_ClassifyStatusPair(0x0000, NOR_STATUS_DQ5 | NOR_STATUS_DQ6, 0) == NOR_STATUS_CONFIRM_DQ5,
		"DQ5 requests confirmation while DQ6 still toggles");
	check(NOR_ClassifyStatusPair(0x0000, NOR_STATUS_DQ6 | NOR_STATUS_DQ1, 1) == NOR_STATUS_BUFFER_ABORT,
		"DQ1 with a toggling buffer operation is an abort");
	check(NOR_ClassifyStatusPair(0x0000, NOR_STATUS_DQ6 | NOR_STATUS_DQ1, 0) == NOR_STATUS_BUSY,
		"DQ1 is not treated as buffer abort for a non-buffer operation");
	check(NOR_ClassifyStatusPair(NOR_STATUS_DQ1, NOR_STATUS_DQ1, 1) == NOR_STATUS_VERIFY_DATA,
		"stable array DQ1 data is not misread as a buffer abort");
	check(NOR_ClassifyDQ5ConfirmPair(0x0000, NOR_STATUS_DQ6) == NOR_STATUS_OPERATION_FAILED,
		"continued DQ6 toggling after DQ5 is a terminal failure");
	check(NOR_ClassifyDQ5ConfirmPair(NOR_STATUS_DQ5, NOR_STATUS_DQ5) == NOR_STATUS_VERIFY_DATA,
		"stopped DQ6 after DQ5 proceeds to data verification");
	check(NOR_ClassifyReadbackPair(0xFFFF, 0xFFFF, 0xFFFF) == NOR_STATUS_OPERATION_COMPLETE,
		"two matching erased reads complete successfully");
	check(NOR_ClassifyReadbackPair(0x1234, 0x1234, 0x1234) == NOR_STATUS_OPERATION_COMPLETE,
		"two matching programmed reads complete successfully");
	check(NOR_ClassifyReadbackPair(0x1234, 0x1274, 0x1234) == NOR_STATUS_BUSY,
		"readback pair with DQ6 toggling remains busy");
	check(NOR_ClassifyReadbackPair(0x1234, 0x1235, 0x1234) == NOR_STATUS_OPERATION_FAILED,
		"stable data mismatch fails instead of reporting success");
	puts("NOR flash status scenarios passed.");
	return 0;
}
