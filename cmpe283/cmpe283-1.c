/*  
 *  cmpe283-1.c - Kernel module for CMPE283 assignment 1
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <asm/msr.h>

#define MAX_MSG 80

/*
 * Model specific registers (MSRs) by the module.
 * See SDM volume 4, section 2.1
 */
#define IA32_VMX_PINBASED_CTLS  0x481
#define IA32_VMX_PROCBASED_CTLS 0x482
#define IA32_VMX_PROCBASED_CTLS2 0x48B
#define IA32_VMX_EXIT_CTLS  0x483
#define IA32_VMX_ENTRY_CTLS  0x484
/*
 * struct caapability_info
 *
 * Represents a single capability (bit number and description).
 * Used by report_capability to output VMX capabilities.
 */
struct capability_info {
	uint8_t bit;
	const char *name;
};

/*
 * IA32_VMX_ENTRY_CTLS capabilities
 * See SDM volume 3, section 24.8.3
 */
struct capability_info vmentry_controls[9] =
{
    { 2, "Load Debug Controls" },
    { 9, "IA-32e Mode Guest" },
    { 10, "Entry to SMM" },
    { 11, "Deactivate Dual-Monitor Treatment" },
    { 12, "Load IA32_PERF_GLOBAL_CTRL" },
    { 14, "Load IA32_PAT" },
    { 15, "Load IA32_EFER" },
    { 16, "Load IA32_BNDCFGS" },
    { 17, "Conceal VMX from PT" }
};

/*
 * IA32_VMX_EXIT_CTLS capabilities
 * See SDM volume 3, section 24.8.2
 */
struct capability_info vmexit_controls[11] =
{
    { 1, "Save Debug Controls" },
    { 2, "Host Address-Space Size" },
    { 9, "Load IA32_PERF_GLOBAL_CTRL" },
    { 12, "Acknowledge Interrupt on Exit" },
    { 15, "Save IA32_PAT" },
    { 18, "Load IA32_PAT" },
    { 19, "Save IA32_EFER" },
    { 20, "Load IA32_EFER" },
    { 21, "Save VMX Preemption Timer" },
    { 22, "Clear IA32_BNDCFGS" },
    { 23, "Conceal VMX from PT" }
};

/*
 * IA32_VMX_PROCBASED_CTLS2 capabilities
 * See SDM volume 3, section 24.6.2
 */
struct capability_info procbased2[9] =
{
    { 0, "Virtualize APIC Accesses" },
    { 1, "Enable EPT" },
    { 2, "Descriptor-table Exiting" },
    { 3, "Enable RDTSCP" },
    { 4, "Virtualize x2APIC Mode" },
    { 5, "Enable VPID" },
    { 6, "WBINVD Exiting" },
    { 7, "Unrestricted Guest" },
    { 8, "APIC Register Virtualization" }
};

/*
 * Processor-based capabilities
 * See SDM volume 3, section 24.6.2
 */
struct capability_info procbased[6] =
{
    { 2, "Interrupt Window Exiting" },
    { 3, "Use TSC Offsetting" },
    { 7, "HLT Exiting" },
    { 9, "INVLPG Exiting" },
    { 10, "MWAIT Exiting" },
    { 11, "RDPMC Exiting" }
};

/*
 * Pinbased capabilities
 * See SDM volume 3, section 24.6.1
 */
struct capability_info pinbased[5] =
{
	{ 0, "External Interrupt Exiting" },
	{ 3, "NMI Exiting" },
	{ 5, "Virtual NMIs" },
	{ 6, "Activate VMX Preemption Timer" },
	{ 7, "Process Posted Interrupts" }
};

/*
 * report_capability
 *
 * Reports capabilities present in 'cap' using the corresponding MSR values
 * provided in 'lo' and 'hi'.
 *
 * Parameters:
 *  cap: capability_info structure for this feature
 *  len: number of entries in 'cap'
 *  lo: low 32 bits of capability MSR value describing this feature
 *  hi: high 32 bits of capability MSR value describing this feature
 */
void
report_capability(struct capability_info *cap, uint8_t len, uint32_t lo,
    uint32_t hi)
{
	uint8_t i;
	struct capability_info *c;
	char msg[MAX_MSG];

	memset(msg, 0, sizeof(msg));

	for (i = 0; i < len; i++) {
		c = &cap[i];
		snprintf(msg, 79, "  %s: Can set=%s, Can clear=%s\n",
		    c->name,
		    (hi & (1 << c->bit)) ? "Yes" : "No",
		    !(lo & (1 << c->bit)) ? "Yes" : "No");
		printk(msg);
	}
}

/*
 * detect_vmx_features
 *
 * Detects and prints VMX capabilities of this host's CPU.
 */
void
detect_vmx_features(void)
{
	uint32_t lo, hi;

	/* Pinbased controls */
	rdmsr(IA32_VMX_PINBASED_CTLS, lo, hi);
	pr_info("Pinbased Controls MSR: 0x%llx\n",
		(uint64_t)(lo | (uint64_t)hi << 32));
	report_capability(pinbased, 5, lo, hi);
	/* Procbased controls */
	rdmsr(IA32_VMX_PROCBASED_CTLS, lo, hi);
	pr_info("Procbased Controls MSR: 0x%llx\n",
		(uint64_t)(lo | (uint64_t)hi << 32));
	report_capability(procbased, 6, lo, hi);
	/* Procbased2 controls */
	rdmsr(IA32_VMX_PROCBASED_CTLS2, lo, hi);
	pr_info("Procbased 2 Controls MSR: 0x%llx\n",
		(uint64_t)(lo | (uint64_t)hi << 32));
	report_capability(procbased2, 9, lo, hi);
	/* Vm Exit controls */
	rdmsr(IA32_VMX_EXIT_CTLS, lo, hi);
	pr_info("VM exit Controls MSR: 0x%llx\n",
		(uint64_t)(lo | (uint64_t)hi << 32));
	report_capability(vmexit_controls, 11, lo, hi);
	/* Vm Entry controls */
	rdmsr(IA32_VMX_ENTRY_CTLS, lo, hi);
	pr_info("VM Entry Controls MSR: 0x%llx\n",
		(uint64_t)(lo | (uint64_t)hi << 32));
	report_capability(vmentry_controls, 9, lo, hi);
}

/*
 * init_module
 *
 * Module entry point
 *
 * Return Values:
 *  Always 0
 */
int
init_module(void)
{
	printk(KERN_INFO "CMPE 283 Assignment 1 Module Start\n");

	detect_vmx_features();

	/* 
	 * A non 0 return means init_module failed; module can't be loaded. 
	 */
	return 0;
}

/*
 * cleanup_module
 *
 * Function called on module unload
 */
void
cleanup_module(void)
{
	printk(KERN_INFO "CMPE 283 Assignment 1 Module Exits\n");
}

MODULE_LICENSE("GPL");
