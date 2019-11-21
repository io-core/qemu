#ifndef RISC6_BOOT_H
#define RISC6_BOOT_H

#include "hw/hw.h"
#include "cpu.h"

void risc6_board_reset(RISC6CPU *cpu, hwaddr ddr_base, hwaddr rom_base, uint32_t ramsize,
                       const char *initrd_filename, const char *dtb_filename,
                       void (*machine_cpu_reset)(RISC6CPU *));

#endif /* RISC6_BOOT_H */
