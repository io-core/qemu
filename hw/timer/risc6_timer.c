/*
 * QEMU model of the RISC6 timer and io memory mapped registers.
 *
 * Copyright (c) 2019 Charles Perkins <charlesap@gmail.com>
 *
 * Adapted from the 
 *
 * QEMU model of the Altera timer.
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
#include "qemu/module.h"
#include "qapi/error.h"

#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/ptimer.h"
#include "hw/qdev-properties.h"
#include "sysemu/block-backend.h"
#include "sysemu/blockdev.h"

#define R_STATUS      11
#define R_CONTROL     12
#define R_PERIODL     13
#define R_PERIODH     14
#define R_SNAPL       15
#define R_SNAPH       16

// FFFFFFC0		Millisecond Counter
// FFFFFFC4             Switches / LED
// FFFFFFC8             RS232 Data
// FFFFFFCC             RS232 Status
// FFFFFFD0             SPI Data / SPI Write
// FFFFFFD4             SPI Status / SPI Control
// FFFFFFD8             Mouse Input
// FFFFFFDC             Keyboard Input
// FFFFFFE0             DEBUG 
// FFFFFFE4             
// FFFFFFE8             Clipboard A
// FFFFFFEC             Clipboard B

#define R_MILLISECONDS	0
#define R_LED		1
#define R_RS232DATA	2
#define R_RS232STATUS	3
#define R_SPIDATA	4
#define R_SPICONTROL	5
#define R_MOUSE		6
#define R_KEYBOARD	7
#define R_DEBUG         8
#define R_DEBUG2        9
#define R_DEBUG3        10


#define R_MAX         17

#define STATUS_TO     0x0001
#define STATUS_RUN    0x0002

#define CONTROL_ITO   0x0001
#define CONTROL_CONT  0x0002
#define CONTROL_START 0x0004
#define CONTROL_STOP  0x0008

#define TYPE_RISC6_TIMER "RISC6,io"
#define RISC6_TIMER(obj) \
    OBJECT_CHECK(RISC6Timer, (obj), TYPE_RISC6_TIMER)

#define  diskCommand 0
#define  diskRead    1
#define  diskWrite   2
#define  diskWriting 3


typedef struct RISC6Timer {
    SysBusDevice  busdev;
    MemoryRegion  mmio;
    qemu_irq      irq;
    uint32_t      spi_sel;
    uint32_t      freq_hz;
    ptimer_state *ptimer;
    BlockBackend *blk;
    uint8_t      *buf;
    uint32_t      regs[R_MAX];

    uint32_t      disk_state;
    FILE *        disk_file;
    uint32_t      disk_offset;
    unsigned int  disk_index;
    unsigned int  disk_size;

    uint32_t      rx_buf[128];
    int           rx_idx;

    uint32_t      tx_buf[130];
    int           tx_cnt;
    int           tx_idx;

    int           debugcount;

} RISC6Timer;

static int timer_irq_state(RISC6Timer *t)
{
    bool irq = (t->regs[R_STATUS] & STATUS_TO) &&
               (t->regs[R_CONTROL] & CONTROL_ITO);
    return irq;
}

static void read_sector(RISC6Timer *t){
//  printf("Read Sector\n");
  uint8_t abuf[1024];
  uint32_t * r;

//  printf("index is %d byte offset is %d\n",t->disk_index,t->disk_index*512);

  int alen = blk_pread(t->blk, (t->disk_index)*512, abuf, 512);

  if (alen != 512) {
    printf("Disk Read Error\n");
  }

  r = (uint32_t *) &abuf;
  int i;
  for (i = 0; i < 128; i++) {
//    printf("%08x\n",*r);
    r = (uint32_t *) &(abuf[i*4]);
    t->tx_buf[i+2] = *r;
    r++;
//    t->tx_buf[i+2] = abuf[i*4+0] | (abuf[i*4+1] << 8) | (abuf[i*4+2] << 16) | (abuf[i*4+3] << 24);  
  }

}

static void write_sector(RISC6Timer *t){
  printf("Write Sector\n");
}

static void disk_run_command(RISC6Timer *t){
  uint32_t cmd = t->rx_buf[0];
  uint32_t arg = ((t->rx_buf[1] & 0xFF) << 24) | ((t->rx_buf[2] & 0xFF) << 16) | ((t->rx_buf[3] & 0xFF) << 8) | (t->rx_buf[4] & 0xFF);

  switch (cmd) {
    case 81: 
      t->disk_state = diskRead;
      t->tx_buf[0] = 0;
      t->tx_buf[1] = 254;
      printf("Seek %8u for read\n",arg);
      t->disk_index = ((arg -  524290 ));//(unsigned int)t->disk_offset) );// * 512;
//      printf("Sector index %8u for read\n",t->disk_index);
      read_sector(t);
      t->tx_cnt = 2 + 128;
      break;
    case 88: 
      t->disk_state = diskWrite;
      printf("Seek %8u for write\n",arg);
      t->disk_index = ((arg -  524290));//(unsigned int)t->disk_offset) );// * 512;
//      printf("Sector index %8u for write\n",t->disk_index);
      t->tx_buf[0] = 0;
      t->tx_cnt = 1;
      break;
    default: 
      t->tx_buf[0] = 0;
      t->tx_cnt = 1;
      
  }
  t->tx_idx = -1;
}


static uint32_t spi_read(RISC6Timer *t){
    uint32_t r;

    r = 255;
    if (t->spi_sel == 1){
      if ( t->tx_idx >= 0 && t->tx_idx < t->tx_cnt) {
        r = t->tx_buf[t->tx_idx];
      }
    }
    return r;
}

static void spi_write(RISC6Timer *t, uint32_t value){
    if (t->spi_sel == 1 ){
      t->tx_idx++;

      switch (t->disk_state) {
      case diskCommand: 
        if ((uint8_t)value != 0xFF || t->rx_idx != 0) {
          t->rx_buf[t->rx_idx] = value;
          t->rx_idx++;
          if (t->rx_idx == 6) {
//            if (t->rx_buf[0]|t->rx_buf[1]|t->rx_buf[2]|t->rx_buf[3]|t->rx_buf[4]|t->rx_buf[5]) {
//              printf(" %d %d %d %d %d %d\n",t->rx_buf[0],t->rx_buf[1],t->rx_buf[2],t->rx_buf[3],t->rx_buf[4],t->rx_buf[5]);
//            }
            disk_run_command(t);
            t->rx_idx = 0;
          }
        }
        break;  
      case diskRead: 
        if (t->tx_idx == t->tx_cnt) {
          t->disk_state = diskCommand;
          t->tx_cnt = 0;
          t->tx_idx = 0;
        }
        break;
      case diskWrite: 
        if (value == 254) {
          t->disk_state = diskWriting;
        }
        break;
      case diskWriting: 
        if (t->rx_idx < 128) {
          t->rx_buf[t->rx_idx] = value;
        }
        t->rx_idx++;
        if (t->rx_idx == 128) {
       //        write_sector(disk.file, &disk.rx_buf[0]);
          write_sector(t);
        }
        if (t->rx_idx == 130) {
          t->tx_buf[0] = 5;
          t->tx_cnt = 1;
          t->tx_idx = -1;
          t->rx_idx = 0;
          t->disk_state = diskCommand;
        }
        break;
      }

    }
}


static uint64_t timer_read(void *opaque, hwaddr addr,
                           unsigned int size)
{
    RISC6Timer *t = opaque;
    uint64_t r = 0;

    addr >>= 2;

//    printf("RISC6 IO READ OF: %ld\n",addr);

    switch (addr) {
    case R_CONTROL:
        r = t->regs[R_CONTROL] & (CONTROL_ITO | CONTROL_CONT);
        break;
    case R_SPIDATA:
	r = spi_read(t);
//        printf("SPI D< %lx\n",r);
	break;

    case R_SPICONTROL:
        r = 1;
//        printf("SPI C< %lx\n",r);
        break;

    default:
        if (addr < ARRAY_SIZE(t->regs)) {
            r = t->regs[addr];
        }
        break;
    }

    return r;
}

static void timer_write(void *opaque, hwaddr addr,
                        uint64_t value, unsigned int size)
{
    RISC6Timer *t = opaque;
    uint64_t tvalue;
    uint32_t count = 0;
    int irqState = timer_irq_state(t);

    addr >>= 2;

    switch (addr) {
    case R_LED:
        printf("LED:%ld\n",value);
        break;

    case R_SPIDATA:
        spi_write(t,value);
//        printf("SPI D> %lx\n",value);
        break;

    case R_SPICONTROL:
//        printf("SPI C> %ld\n",value & 3);
        t->spi_sel = value & 3;
        break;

    case R_DEBUG:
//        if((t->debugcount > 10000)&&(t->debugcount < 50000)){
//          printf("\nDEBUG:%08x:%08lx ",t->debugcount,value);
//        }
//        t->debugcount++;
        break;

    case R_DEBUG2:
//        if((t->debugcount < 10000)&&(t->debugcount < 50000)){
//          printf("%lx ",value);
//        }
        break;

    case R_DEBUG3:
//        if((t->debugcount < 10000)&&(t->debugcount < 50000)){
//          printf("%08lx ",value);
//        }
        break;

    case R_STATUS:
        /* The timeout bit is cleared by writing the status register. */
        t->regs[R_STATUS] &= ~STATUS_TO;
        break;

    case R_CONTROL:
        ptimer_transaction_begin(t->ptimer);
        t->regs[R_CONTROL] = value & (CONTROL_ITO | CONTROL_CONT);
        if ((value & CONTROL_START) &&
            !(t->regs[R_STATUS] & STATUS_RUN)) {
            ptimer_run(t->ptimer, 1);
            t->regs[R_STATUS] |= STATUS_RUN;
        }
        if ((value & CONTROL_STOP) && (t->regs[R_STATUS] & STATUS_RUN)) {
            ptimer_stop(t->ptimer);
            t->regs[R_STATUS] &= ~STATUS_RUN;
        }
        ptimer_transaction_commit(t->ptimer);
        break;

    case R_PERIODL:
    case R_PERIODH:
        ptimer_transaction_begin(t->ptimer);
        t->regs[addr] = value & 0xFFFF;
        if (t->regs[R_STATUS] & STATUS_RUN) {
            ptimer_stop(t->ptimer);
            t->regs[R_STATUS] &= ~STATUS_RUN;
        }
        tvalue = (t->regs[R_PERIODH] << 16) | t->regs[R_PERIODL];
        ptimer_set_limit(t->ptimer, tvalue + 1, 1);
        ptimer_transaction_commit(t->ptimer);
        break;

    case R_SNAPL:
    case R_SNAPH:
        count = ptimer_get_count(t->ptimer);
        t->regs[R_SNAPL] = count & 0xFFFF;
        t->regs[R_SNAPH] = count >> 16;
        break;

    default:
        break;
    }

    if (irqState != timer_irq_state(t)) {
        qemu_set_irq(t->irq, timer_irq_state(t));
    }
}

static const MemoryRegionOps timer_ops = {
    .read = timer_read,
    .write = timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4
    }
};

static void timer_hit(void *opaque)
{
    RISC6Timer *t = opaque;
    const uint64_t tvalue = (t->regs[R_PERIODH] << 16) | t->regs[R_PERIODL];

    t->regs[R_STATUS] |= STATUS_TO;

    ptimer_set_limit(t->ptimer, tvalue + 1, 1);

    if (!(t->regs[R_CONTROL] & CONTROL_CONT)) {
        t->regs[R_STATUS] &= ~STATUS_RUN;
        ptimer_set_count(t->ptimer, tvalue);
    } else {
        ptimer_run(t->ptimer, 1);
    }

    qemu_set_irq(t->irq, timer_irq_state(t));
}

static void risc6_timer_realize(DeviceState *dev, Error **errp)
{
    RISC6Timer *t = RISC6_TIMER(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    if (t->freq_hz == 0) {
        error_setg(errp, "\"clock-frequency\" property must be provided.");
        return;
    }

    t->ptimer = ptimer_init(timer_hit, t, PTIMER_POLICY_DEFAULT);
    ptimer_transaction_begin(t->ptimer);
    ptimer_set_freq(t->ptimer, t->freq_hz);
    ptimer_transaction_commit(t->ptimer);

    memory_region_init_io(&t->mmio, OBJECT(t), &timer_ops, t,
                          TYPE_RISC6_TIMER, R_MAX * sizeof(uint32_t));
    sysbus_init_mmio(sbd, &t->mmio);
}

static void risc6_timer_init(Object *obj)
{
    DriveInfo *dinfo;
    int64_t size;


    RISC6Timer *t = RISC6_TIMER(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    sysbus_init_irq(sbd, &t->irq);

    dinfo = drive_get(IF_NONE, 0, 0);
    if (dinfo) {
       t->blk = blk_by_legacy_dinfo(dinfo);
       printf("Have dinfo\n");
       size = blk_getlength(t->blk);
       t->disk_size=0;
       if (size <= 0) {
           printf("failed to get disk size\n");
       }else{
           printf("disk size: %ld bytes\n",size);
           t->disk_size=size;
       }

       t->buf = g_malloc0(512);

       int alen = blk_pread(t->blk, 0, t->buf, 512);

       t->disk_index = 0;
       t->disk_offset = 0;

       if (alen != 512) {
            printf("Couldn't read disk block zero\n");
       }else{
            printf("Read disk block zero success, signature %d %d %d %d\n",t->buf[0],t->buf[1],t->buf[2],t->buf[3]);
            if (t->buf[0]==141 && t->buf[1]==163 && t->buf[2]==30 && t->buf[3]==155){
               t->disk_offset = 0x80002; // 524290
            }

       }


    }else{
      printf("N. dinfo for drive.\n");
    }

}

static void risc6_timer_reset(DeviceState *dev)
{
    RISC6Timer *t = RISC6_TIMER(dev);

    ptimer_transaction_begin(t->ptimer);
    ptimer_stop(t->ptimer);
    ptimer_set_limit(t->ptimer, 0xffffffff, 1);
    ptimer_transaction_commit(t->ptimer);
    memset(t->regs, 0, sizeof(t->regs));
    t->spi_sel = 0;
    t->disk_state = 0;
    //disk_file;
    t->disk_offset = 0;
    t->rx_idx = 0;
    t->tx_cnt = 0;
    t->tx_idx = 0;

    t->debugcount = 0;
}

static Property risc6_timer_properties[] = {
    DEFINE_PROP_UINT32("clock-frequency", RISC6Timer, freq_hz, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void risc6_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = risc6_timer_realize;
    dc->props = risc6_timer_properties;
    dc->reset = risc6_timer_reset;
}

static const TypeInfo risc6_timer_info = {
    .name          = TYPE_RISC6_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RISC6Timer),
    .instance_init = risc6_timer_init,
    .class_init    = risc6_timer_class_init,
};

static void risc6_timer_register(void)
{
    type_register_static(&risc6_timer_info);
}

type_init(risc6_timer_register)
