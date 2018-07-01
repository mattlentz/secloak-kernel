#ifndef SECLOAK_ENTRY_H
#define SECLOAK_ENTRY_H

/*
 * Update the SeCloak settings
 *
 * Call register usage:
 * a0 SMC Function ID, OPTEE_SMC_CLOAK_SET
 * a1 Bitfield for enabling/disabling classes of devices
 * a2 Bitfield for enabling/disabling classes of devices (cont.)
 *
 * Normal return register usage:
 * a0 OPTEE_SMC_RETURN_OK
 * a1-7 Preserved
 */
#define OPTEE_SMC_FUNCID_CLOAK_SET	99
#define OPTEE_SMC_CLOAK_SET \
	OPTEE_SMC_STD_CALL_VAL(OPTEE_SMC_FUNCID_CLOAK_SET)

/*
 * Retrieve the current SeCloak settings
 *
 * Call register usage:
 * a0 SMC Function ID, OPTEE_SMC_CLOAK_GET
 *
 * Normal return register usage:
 * a0 OPTEE_SMC_RETURN_OK
 * a1 Bitfield for enabling/disabling classes of devices
 * a2 Bitfield for enabling/disabling classes of devices (cont.)
 * a3-7 Preserved
 */
#define OPTEE_SMC_FUNCID_CLOAK_GET	100
#define OPTEE_SMC_CLOAK_GET \
	OPTEE_SMC_STD_CALL_VAL(OPTEE_SMC_FUNCID_CLOAK_GET)

struct thread_smc_args;
void cloak_entry(struct thread_smc_args *args);

#endif

