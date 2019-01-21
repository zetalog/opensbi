/*
 * Copyright (c) 2018 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __SBI_PLATFORM_H__
#define __SBI_PLATFORM_H__

/** Offset of name in struct sbi_platform */
#define SBI_PLATFORM_NAME_OFFSET		(0x0)
/** Offset of features in struct sbi_platform */
#define SBI_PLATFORM_FEATURES_OFFSET		(0x40)
/** Offset of hart_count in struct sbi_platform */
#define SBI_PLATFORM_HART_COUNT_OFFSET		(0x48)
/** Offset of hart_stack_size in struct sbi_platform */
#define SBI_PLATFORM_HART_STACK_SIZE_OFFSET	(0x4c)

#ifndef __ASSEMBLY__

#include <sbi/sbi_scratch.h>

/** Possible feature flags of a platform */
enum sbi_platform_features {
	/** Platform has MMIO based timer */
	SBI_PLATFORM_HAS_MMIO_TIMER_VALUE	= (1 << 0),
	/** Platform has HART hotplug support */
	SBI_PLATFORM_HAS_HART_HOTPLUG		= (1 << 1),
	/** Platform has PMP support */
	SBI_PLATFORM_HAS_PMP			= (1 << 2),
	/** Platform has S-mode counter enable */
	SBI_PLATFORM_HAS_SCOUNTEREN		= (1 << 3),
	/** Platform has M-mode counter enable */
	SBI_PLATFORM_HAS_MCOUNTEREN		= (1 << 4),
	/** Platform has fault delegation support */
	SBI_PLATFORM_HAS_MFAULTS_DELEGATION	= (1 << 5),
};

/** Default feature set for a platform */
#define SBI_PLATFORM_DEFAULT_FEATURES	\
	(SBI_PLATFORM_HAS_MMIO_TIMER_VALUE | \
	 SBI_PLATFORM_HAS_PMP | \
	 SBI_PLATFORM_HAS_SCOUNTEREN | \
	 SBI_PLATFORM_HAS_MCOUNTEREN | \
	 SBI_PLATFORM_HAS_MFAULTS_DELEGATION)

/** Representation of a platform */
struct sbi_platform {
	/** Name of the platform */
	char name[64];
	/** Supported features */
	u64 features;
	/** Total number of HARTs */
	u32 hart_count;
	/** Per-HART stack size for exception/interrupt handling */
	u32 hart_stack_size;
	/** Mask representing the set of disabled HARTs */
	u64 disabled_hart_mask;

	/** Platform early initialization */
	int (*early_init)(u32 hartid, bool cold_boot);
	/** Platform final initialization */
	int (*final_init)(u32 hartid, bool cold_boot);

	/** Get number of PMP regions for given HART */
	u32 (*pmp_region_count)(u32 hartid);
	/**
	 * Get PMP regions details (namely: protection, base address,
	 * and size) for given HART
	 */
	int (*pmp_region_info)(u32 hartid, u32 index,
			       ulong *prot, ulong *addr, ulong *log2size);

	/** Write a character to the platform console output */
	void (*console_putc)(char ch);
	/** Read a character from the platform console input */
	char (*console_getc)(void);
	/** Initialize the platform console */
	int (*console_init)(void);

	/** Initialize the platform interrupt controller */
	int (*irqchip_init)(u32 hartid, bool cold_boot);

	/** Inject IPI to a target HART */
	void (*ipi_inject)(u32 target_hart, u32 source_hart);
	/** Wait for target HART to acknowledge IPI */
	void (*ipi_sync)(u32 target_hart, u32 source_hart);
	/** Clear IPI for a target HART */
	void (*ipi_clear)(u32 target_hart);
	/** Initialize IPI for given HART */
	int (*ipi_init)(u32 hartid, bool cold_boot);

	/** Get MMIO timer value */
	u64 (*timer_value)(void);
	/** Start MMIO timer event for a target HART */
	void (*timer_event_start)(u32 target_hart, u64 next_event);
	/** Stop MMIO timer event for a target HART */
	void (*timer_event_stop)(u32 target_hart);
	/** Initialize MMIO timer for given HART */
	int (*timer_init)(u32 hartid, bool cold_boot);

	/** Reboot the platform */
	int (*system_reboot)(u32 type);
	/** Shutdown or poweroff the platform */
	int (*system_shutdown)(u32 type);
} __packed;

/** Get pointer to sbi_platform for sbi_scratch pointer */
#define sbi_platform_ptr(__s)	\
	((struct sbi_platform *)((__s)->platform_addr))
/** Get pointer to sbi_platform for current HART */
#define sbi_platform_thishart_ptr()	\
	((struct sbi_platform *)(sbi_scratch_thishart_ptr()->platform_addr))
/** Check whether the platform supports MMIO timer */
#define sbi_platform_has_mmio_timer_value(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_MMIO_TIMER_VALUE)
/** Check whether the platform supports HART hotplug */
#define sbi_platform_has_hart_hotplug(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_HART_HOTPLUG)
/** Check whether the platform has PMP support */
#define sbi_platform_has_pmp(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_PMP)
/** Check whether the platform supports scounteren CSR */
#define sbi_platform_has_scounteren(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_SCOUNTEREN)
/** Check whether the platform supports mcounteren CSR */
#define sbi_platform_has_mcounteren(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_MCOUNTEREN)
/** Check whether the platform supports fault delegation */
#define sbi_platform_has_mfaults_delegation(__p)	\
	((__p)->features & SBI_PLATFORM_HAS_MFAULTS_DELEGATION)

/**
 * Get name of the platform
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return pointer to platform name on success and NULL on failure
 */
static inline const char *sbi_platform_name(struct sbi_platform *plat)
{
	if (plat)
		return plat->name;
	return NULL;
}

/**
 * Check whether the given HART is disabled
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 *
 * @return TRUE if HART is disabled and FALSE otherwise
 */
static inline bool sbi_platform_hart_disabled(struct sbi_platform *plat, u32 hartid)
{
	if (plat && (plat->disabled_hart_mask & (1 << hartid)))
		return 1;
	else
		return 0;
}

/**
 * Get total number of HARTs supported by the platform
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return total number of HARTs
 */
static inline u32 sbi_platform_hart_count(struct sbi_platform *plat)
{
	if (plat)
		return plat->hart_count;
	return 0;
}

/**
 * Get per-HART stack size for exception/interrupt handling
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return stack size in bytes
 */
static inline u32 sbi_platform_hart_stack_size(struct sbi_platform *plat)
{
	if (plat)
		return plat->hart_stack_size;
	return 0;
}

/**
 * Early initialization of a given HART
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 * @param cold_boot whether cold boot (TRUE) or warm_boot (FALSE)
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_early_init(struct sbi_platform *plat,
					  u32 hartid, bool cold_boot)
{
	if (plat && plat->early_init)
		return plat->early_init(hartid, cold_boot);
	return 0;
}

/**
 * Final initialization of a HART
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 * @param cold_boot whether cold boot (TRUE) or warm_boot (FALSE)
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_final_init(struct sbi_platform *plat,
					  u32 hartid, bool cold_boot)
{
	if (plat && plat->final_init)
		return plat->final_init(hartid, cold_boot);
	return 0;
}

/**
 * Get the number of PMP regions of a HART
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 *
 * @return number of PMP regions
 */
static inline u32 sbi_platform_pmp_region_count(struct sbi_platform *plat,
						u32 hartid)
{
	if (plat && plat->pmp_region_count)
		return plat->pmp_region_count(hartid);
	return 0;
}

/**
 * Get PMP regions details (namely: protection, base address,
 * and size) of a HART
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 * @param index index of PMP region for which we want details
 * @param prot output pointer for PMP region protection
 * @param addr output pointer for PMP region base address
 * @param log2size output pointer for log-of-2 PMP region size
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_pmp_region_info(struct sbi_platform *plat,
					       u32 hartid, u32 index,
					       ulong *prot, ulong *addr,
					       ulong *log2size)
{
	if (plat && plat->pmp_region_info)
		return plat->pmp_region_info(hartid, index,
					     prot, addr, log2size);
	return 0;
}

/**
 * Write a character to the platform console output
 *
 * @param plat pointer to struct sbi_platform
 * @param ch character to write
 */
static inline void sbi_platform_console_putc(struct sbi_platform *plat,
					     char ch)
{
	if (plat && plat->console_putc)
		plat->console_putc(ch);
}

/**
 * Read a character from the platform console input
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return character read from console input
 */
static inline char sbi_platform_console_getc(struct sbi_platform *plat)
{
	if (plat && plat->console_getc)
		return plat->console_getc();
	return 0;
}

/**
 * Initialize the platform console
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_console_init(struct sbi_platform *plat)
{
	if (plat && plat->console_init)
		return plat->console_init();
	return 0;
}

/**
 * Initialize the platform interrupt controller for given HART
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 * @param cold_boot whether cold boot (TRUE) or warm_boot (FALSE)
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_irqchip_init(struct sbi_platform *plat,
					    u32 hartid, bool cold_boot)
{
	if (plat && plat->irqchip_init)
		return plat->irqchip_init(hartid, cold_boot);
	return 0;
}

/**
 * Inject IPI to a target HART
 *
 * @param plat pointer to struct sbi_platform
 * @param target_hart HART ID of IPI target
 */
static inline void sbi_platform_ipi_inject(struct sbi_platform *plat,
					   u32 target_hart, u32 source_hart)
{
	if (plat && plat->ipi_inject)
		plat->ipi_inject(target_hart, source_hart);
}

/**
 * Wait for target HART to acknowledge IPI
 *
 * @param plat pointer to struct sbi_platform
 * @param target_hart HART ID of IPI target
 * @param source_hart HART ID of IPI source
 */
static inline void sbi_platform_ipi_sync(struct sbi_platform *plat,
					 u32 target_hart, u32 source_hart)
{
	if (plat && plat->ipi_sync)
		plat->ipi_sync(target_hart, source_hart);
}

/**
 * Clear IPI for a target HART
 *
 * @param plat pointer to struct sbi_platform
 * @param target_hart HART ID of IPI target
 */
static inline void sbi_platform_ipi_clear(struct sbi_platform *plat,
					  u32 target_hart)
{
	if (plat && plat->ipi_clear)
		plat->ipi_clear(target_hart);
}

/**
 * Initialize the platform IPI support for given HART
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 * @param cold_boot whether cold boot (TRUE) or warm_boot (FALSE)
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_ipi_init(struct sbi_platform *plat,
					u32 hartid, bool cold_boot)
{
	if (plat && plat->ipi_init)
		return plat->ipi_init(hartid, cold_boot);
	return 0;
}

/**
 * Get MMIO timer value
 *
 * @param plat pointer to struct sbi_platform
 *
 * @return 64bit timer value
 */
static inline u64 sbi_platform_timer_value(struct sbi_platform *plat)
{
	if (plat && plat->timer_value)
		return plat->timer_value();
	return 0;
}

/**
 * Start MMIO timer event for a target HART
 *
 * @param plat pointer to struct struct sbi_platform
 * @param target_hart HART ID of timer event target
 * @param next_event timer value when timer event will happen
 */
static inline void sbi_platform_timer_event_start(struct sbi_platform *plat,
						  u32 target_hart,
						  u64 next_event)
{
	if (plat && plat->timer_event_start)
		plat->timer_event_start(target_hart, next_event);
}

/**
 * Stop MMIO timer event for a target HART
 *
 * @param plat pointer to struct sbi_platform
 * @param target_hart HART ID of timer event target
 */
static inline void sbi_platform_timer_event_stop(struct sbi_platform *plat,
						 u32 target_hart)
{
	if (plat && plat->timer_event_stop)
		plat->timer_event_stop(target_hart);
}

/**
 * Initialize the platform MMIO timer for given HART
 *
 * @param plat pointer to struct sbi_platform
 * @param hartid HART ID
 * @param cold_boot whether cold boot (TRUE) or warm_boot (FALSE)
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_timer_init(struct sbi_platform *plat,
					  u32 hartid, bool cold_boot)
{
	if (plat && plat->timer_init)
		return plat->timer_init(hartid, cold_boot);
	return 0;
}

/**
 * Reboot the platform
 *
 * @param plat pointer to struct sbi_platform
 * @param type type of reboot
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_system_reboot(struct sbi_platform *plat,
					     u32 type)
{
	if (plat && plat->system_reboot)
		return plat->system_reboot(type);
	return 0;
}

/**
 * Shutdown or poweroff the platform
 *
 * @param plat pointer to struct sbi_platform
 * @param type type of shutdown or poweroff
 *
 * @return 0 on success and negative error code on failure
 */
static inline int sbi_platform_system_shutdown(struct sbi_platform *plat,
					       u32 type)
{
	if (plat && plat->system_shutdown)
		return plat->system_shutdown(type);
	return 0;
}

#endif

#endif
