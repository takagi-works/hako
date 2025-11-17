/*
 * SPDX-License-Identifier: Apache-2.0
 * HAKO extension linker sections
 */

#include <zephyr/linker/common-rom.ld>

SECTION_PROLOGUE(.hako_extensions,,)
{
	__hako_extensions_start = .;
	KEEP(*(SORT(.hako_extensions*)))
	__hako_extensions_end = .;
} GROUP_ROM_LINK_IN(ROMABLE_REGION, ROMABLE_REGION)
