/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <debug.h>
#include <board.h>
#include <platform.h>
#include <target.h>
#include <smem.h>
#include <baseband.h>
#include <lib/ptable.h>
#include <qpic_nand.h>
#include <ctype.h>
#include <string.h>
#include <pm8x41.h>
#include <reg.h>
#include <platform/timer.h>

extern void smem_ptable_init(void);
extern void smem_add_modem_partitions(struct ptable *flash_ptable);

static struct ptable flash_ptable;

/* PMIC config data */
#define PMIC_ARB_CHANNEL_NUM    0
#define PMIC_ARB_OWNER_ID       0

/* NANDc BAM pipe numbers */
#define DATA_CONSUMER_PIPE                            0
#define DATA_PRODUCER_PIPE                            1
#define CMD_PIPE                                      2

struct qpic_nand_init_config config;

void update_ptable_names(void)
{
	uint32_t ptn_index;
	struct ptentry *ptentry_ptr = flash_ptable.parts;
	struct ptentry *boot_ptn;
	unsigned i;
	uint32_t len;

	/* Change all names to lower case. */
	for (ptn_index = 0; ptn_index != (uint32_t)flash_ptable.count; ptn_index++)
	{
		len = strlen(ptentry_ptr[ptn_index].name);

		for (i = 0; i < len; i++)
		{
			if (isupper(ptentry_ptr[ptn_index].name[i]))
			{
				ptentry_ptr[ptn_index].name[i] = tolower(ptentry_ptr[ptn_index].name[i]);
			}
		}
	}

	/* Rename apps ptn to boot. */
	boot_ptn = ptable_find(&flash_ptable, "apps");
	strcpy(boot_ptn->name, "boot");
}

/* init */
void target_init(void)
{
	dprintf(INFO, "target_init()\n");

	spmi_init(PMIC_ARB_CHANNEL_NUM, PMIC_ARB_OWNER_ID);

	config.pipes.read_pipe = DATA_PRODUCER_PIPE;
	config.pipes.write_pipe = DATA_CONSUMER_PIPE;
	config.pipes.cmd_pipe = CMD_PIPE;

	config.bam_base = MSM_NAND_BAM_BASE;
	config.nand_base = MSM_NAND_BASE;

	qpic_nand_init(&config);

	ptable_init(&flash_ptable);

	smem_ptable_init();

	smem_add_modem_partitions(&flash_ptable);

	update_ptable_names();

	flash_set_ptable(&flash_ptable);
}

/* reboot */
void reboot_device(unsigned reboot_reason)
{
	/* Write the reboot reason */
	writel(reboot_reason, RESTART_REASON_ADDR);

	/* Configure PMIC for warm reset */
	pm8x41_reset_configure(PON_PSHOLD_WARM_RESET);

	/* Drop PS_HOLD for MSM */
	writel(0x00, MPM2_MPM_PS_HOLD);

	mdelay(5000);

	dprintf(CRITICAL, "Rebooting failed\n");
	return;
}

/* Identify the current target */
void target_detect(struct board_data *board)
{
	/* Not used. set to unknown */
	board->target = LINUX_MACHTYPE_UNKNOWN;
}

unsigned board_machtype(void)
{
	return  board_target_id();
}

/* Identify the baseband being used */
void target_baseband_detect(struct board_data *board)
{
	/* Check for baseband variants. Default to MSM */
	if (board->platform_subtype == HW_PLATFORM_SUBTYPE_UNKNOWN)
	{
			board->baseband = BASEBAND_MSM;
	}
	else
	{
		dprintf(CRITICAL, "Could not identify baseband id (%d)\n",
				board->platform_subtype);
		ASSERT(0);
	}
}
