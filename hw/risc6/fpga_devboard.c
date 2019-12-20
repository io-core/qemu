/*
 * Oberon RISC6 FPGA devboard
 *
 * Changes Copyright (c) 2019 Charles Perkins <chuck@kuracali.com>
 *
 * Adapted from Altera 10M50 Nios2 GHRD
 *
 * Copyright (c) 2016 Marek Vasut <marek.vasut@gmail.com>
 *
 * Based on LabX device code
 *
 * Copyright (c) 2012 Chris Wulff <crwulff@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "cpu.h"
#include "qemu/error-report.h"
#include "hw/sysbus.h"
#include "hw/char/serial.h"
#include "sysemu/sysemu.h"
#include "hw/boards.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "qemu/config-file.h"
#include "hw/qdev-properties.h"

#include "boot.h"

#define ROM_FILE    "risc-boot.bin"


static void risc6_fpga_risc_init(MachineState *machine)
{
    RISC6CPU *cpu;
    DeviceState *dev;
    MemoryRegion *address_space_mem = get_system_memory();
    MemoryRegion *phys_rom = g_new(MemoryRegion, 1);

/*    MemoryRegion *phys_tcm_alias = g_new(MemoryRegion, 1); */
    MemoryRegion *phys_ram = g_new(MemoryRegion, 1);

/*    MemoryRegion *phys_ram_alias = g_new(MemoryRegion, 1); */
    ram_addr_t rom_base  = 0xFFFFF800;
    ram_addr_t rom_size  = 0x2000;    
    ram_addr_t ram_base  = 0x00000000;
    ram_addr_t ram_size  = 0x000e7f00;
    ram_addr_t vram_base = 0x000e7f00;
    ram_addr_t vram_size = 0x00100000;

    qemu_irq *cpu_irq, irq[32];
    int i;


    /* Physical ROM */
    memory_region_init_ram(phys_rom, NULL, "risc6.rom", rom_size, &error_abort);
    memory_region_add_subregion(address_space_mem, rom_base, phys_rom);


    /* Physical DRAM */
    memory_region_init_ram(phys_ram, NULL, "risc6.ram", ram_size, &error_abort);
    memory_region_add_subregion(address_space_mem, ram_base, phys_ram);

//    unsigned int smp_cpus = machine->smp.cpus;

    /* Create CPU -- FIXME */

    void *x = cpu_create(TYPE_RISC6_CPU);


    cpu = RISC6_CPU(x);


/*
    for (n = 0; n < smp_cpus; n++) {
        CPUXtensaState *cenv = NULL;

        cpu = XTENSA_CPU(cpu_create(machine->cpu_type));
        cenv = &cpu->env;
        if (!env) {
            env = cenv;
            freq = env->config->clock_freq_khz * 1000;
        }

        if (mx_pic) {
            MemoryRegion *mx_eri;

            mx_eri = xtensa_mx_pic_register_cpu(mx_pic,
                                                xtensa_get_extints(cenv),
                                                xtensa_get_runstall(cenv));
            memory_region_add_subregion(xtensa_get_er_region(cenv),
                                        0, mx_eri);
        }
        cenv->sregs[PRID] = n;
        xtensa_select_static_vectors(cenv, n != 0);
        qemu_register_reset(xtfpga_reset, cpu);
        // Need MMU initialized prior to ELF loading,
        // so that ELF gets loaded into virtual addresses
        //
        cpu_reset(CPU(cpu));
    }
*/



    /* Register: CPU interrupt controller (PIC) */
    cpu_irq = risc6_cpu_pic_init(cpu);

    /* Register: Internal Interrupt Controller (IIC) */
    dev = qdev_create(NULL, "risc6,iic");
    object_property_add_const_link(OBJECT(dev), "cpu", OBJECT(cpu), &error_abort);
    qdev_init_nofail(dev);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, cpu_irq[0]);
    for (i = 0; i < 32; i++) {
        irq[i] = qdev_get_gpio_in(dev, i);
    }

    /* Register: Altera 16550 UART */
//    serial_mm_init(address_space_mem, 0xf8001600, 2, irq[1], 115200,
//                   serial_hd(0), DEVICE_NATIVE_ENDIAN);

    /* Register: fpga io  */
    dev = qdev_create(NULL, "RISC6,io");
    qdev_prop_set_uint32(dev, "clock-frequency", 750 * 1000000);
    qdev_prop_set_uint32(dev, "vram-size", vram_size);
    qdev_prop_set_uint16(dev, "width", graphic_width);
    qdev_prop_set_uint16(dev, "height", graphic_height);
    qdev_prop_set_uint16(dev, "depth", graphic_depth);


    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0xFFFFFFC0);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 1, vram_base );
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, irq[0]);


    /* Configure new exception vectors and reset CPU for it to take effect. */
    cpu->reset_addr     = 0xFFFFF800;
    cpu->exception_addr = 0xFFFFF800;
    cpu->fast_tlb_miss_addr = 0xFFFFF800;

    risc6_board_reset(cpu, ram_base, rom_base, ram_size, machine->initrd_filename,
                      ROM_FILE, NULL);

    sleep(1);
}

static void risc6_fpga_risc_machine_init(struct MachineClass *mc)
{
    mc->desc = "Oberon RISC6 FPGA Emulated Devboard";
    mc->init = risc6_fpga_risc_init;
    mc->is_default = 1;
    mc->min_cpus = 1;
    mc->max_cpus = 4;



    if (graphic_depth != 8 && graphic_depth != 24) {
        error_report("Unsupported depth: %d", graphic_depth);
        exit (1);
    }
    if (vga_interface_type != VGA_NONE) {
        if (vga_interface_type == VGA_CG3) {
//            if (!(graphic_depth == 1) &&
//                !(graphic_depth == 8)) {
//                error_report("Unsupported depth: %d", graphic_depth);
//                exit(1);
//            }
//
//            if (!(graphic_width == 1024 && graphic_height == 768) &&
//                !(graphic_width == 1152 && graphic_height == 900)) {
//                error_report("Unsupported resolution: %d x %d", graphic_width,
//                             graphic_height);
//                exit(1);
//            }


            /* sbus irq 5 */
//            cg3_init( 0xFFFFF800, 0x00100000, graphic_width, graphic_height, graphic_depth);


        }
    }

}

DEFINE_MACHINE("fpga-risc", risc6_fpga_risc_machine_init);
