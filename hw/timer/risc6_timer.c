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

#include "ui/console.h"
#include "ui/input.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "hw/irq.h"
#include "hw/ptimer.h"
#include "hw/qdev-properties.h"
#include "sysemu/block-backend.h"
#include "sysemu/blockdev.h"


#define BGCOLOR		(238 << 16) + (223 << 8) + 204

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


#define PS2_MOUSE_BUTTON_LEFT   0x01
#define PS2_MOUSE_BUTTON_RIGHT  0x02
#define PS2_MOUSE_BUTTON_MIDDLE 0x04
#define PS2_MOUSE_BUTTON_SIDE   0x08
#define PS2_MOUSE_BUTTON_EXTRA  0x10

#define MOUSE_STATUS_REMOTE     0x40
#define MOUSE_STATUS_ENABLED    0x20
#define MOUSE_STATUS_SCALE21    0x10


#define STATUS_TO     0x0001
#define STATUS_RUN    0x0002

#define CONTROL_ITO   0x0001
#define CONTROL_CONT  0x0002
#define CONTROL_START 0x0004
#define CONTROL_STOP  0x0008

#define FPGA_VRAM_SIZE 0x100000


#define TYPE_RISC6_TIMER "RISC6,io"
#define RISC6_TIMER(obj) \
    OBJECT_CHECK(RISC6Timer, (obj), TYPE_RISC6_TIMER)

#define  diskCommand 0
#define  diskRead    1
#define  diskWrite   2
#define  diskWriting 3


typedef struct RISC6Timer {
    SysBusDevice  busdev;
    QemuConsole  *con;
    void         *kbd;
    void         *mouse;
    int           mouse_oldx;
    int           mouse_oldy;
    MemoryRegion  mmio;
    MemoryRegion  vram_mem;
    uint32_t      vram_size;
    int           full_update;
    uint16_t      width, height, depth;
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



typedef struct {
    // Keep the data array 256 bytes long, which compatibility
    //  with older qemu versions. 
    uint8_t data[256];
    int rptr, wptr, count;
} PS2Queue;

typedef struct PS2State {
    PS2Queue queue;
    int32_t write_cmd;
    void (*update_irq)(void *, int);
    void *update_arg;
} PS2State;

typedef struct {
    PS2State common;
    int scan_enabled;
    int translate;
    int scancode_set; // 1=XT, 2=AT, 3=PS/2 
    int ledstate;
    bool need_high_bit;
    unsigned int modifiers; // bitmask of MOD_* constants above 
} PS2KbdState;

typedef struct {
    PS2State common;
    uint8_t mouse_status;
    uint8_t mouse_resolution;
    uint8_t mouse_sample_rate;
    uint8_t mouse_wrap;
    uint8_t mouse_type; // 0 = PS2, 3 = IMPS/2, 4 = IMEX 
    uint8_t mouse_detect_state;
    int mouse_dx; // current values, needed for 'poll' mode 
    int mouse_dy;
    int mouse_dz;
    int mouse_ox;
    int mouse_oy;
    bool mousefixed;
    uint8_t mouse_buttons;
} PS2MouseState;


static void fpga_update_display(void *opaque)
{
    RISC6Timer *s = opaque;
    DisplaySurface *surface = qemu_console_surface(s->con);
    const uint8_t *pix;
    uint32_t *data;
    uint32_t dval;
    int b, x, y, y_start;
    unsigned int width, height;
    ram_addr_t page;
    DirtyBitmapSnapshot *snap = NULL;

//    printf("fpga update display\n");

    if (surface_bits_per_pixel(surface) != 32) {
        return;
    }
    width = s->width;
    height = s->height;

    y_start = -1;
    pix = memory_region_get_ram_ptr(&s->vram_mem);
    data = (uint32_t *)surface_data(surface);

    if (!s->full_update) {
        snap = memory_region_snapshot_and_clear_dirty(&s->vram_mem, 0x0,
                                              memory_region_size(&s->vram_mem),
                                              DIRTY_MEMORY_VGA);
    }
    for (y = 0; y < height; y++) {
        int update;

        page = (ram_addr_t)y * width/8;

        if (s->full_update) {
            update = 1;
        } else {
            update = memory_region_snapshot_get_dirty(&s->vram_mem, snap, page,
                                                      width);
        }

        if (update) {
            if (y_start < 0) {
                y_start = y;
            }

            for (x = 0; x < width/8; x++) {
                dval = *pix++;
                for (b = 0; b < 8; b++) {
                  data[((height-y)*width)+(x*8)+b] = (((dval >> b ) & 1 ) == 1 ) ? 0 : BGCOLOR ; 
               //   *data++ = (((dval >> b ) & 1 ) == 1 ) ? 0 : BGCOLOR ;
                }
            }
        } else {
            if (y_start >= 0) {
                dpy_gfx_update(s->con, 0, y_start, width, y - y_start);
                y_start = -1;
            }
            pix += width/8;
            data += width;
        }
    }
    s->full_update = 0;
    if (y_start >= 0) {
        dpy_gfx_update(s->con, 0, y_start, width, y - y_start);
    }
    
    g_free(snap);
}

static void fpga_invalidate_display(void *opaque)
{
    RISC6Timer *s = opaque;

    printf("fpga invalidate display\n");

    memory_region_set_dirty(&s->vram_mem, 0, FPGA_VRAM_SIZE);
}


static const GraphicHwOps video_ops = {
    .invalidate = fpga_invalidate_display,
    .gfx_update = fpga_update_display,
};



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

  if (t->disk_size < ((t->disk_index)*512)+512) {
    printf("Disk Read Past End\n");
  }else{

//    sleep(.02);

    int alen = blk_pread(t->blk, (t->disk_index)*512, abuf, 512);

    if (alen != 512) {
      printf("Disk Read Error\n");
    }

//    sleep(.02);

    r = (uint32_t *) &abuf;
    int i;
    for (i = 0; i < 128; i++) {
//      printf("%08x\n",*r);
      r = (uint32_t *) &(abuf[i*4]);
      t->tx_buf[i+2] = *r;
      r++;
//      t->tx_buf[i+2] = abuf[i*4+0] | (abuf[i*4+1] << 8) | (abuf[i*4+2] << 16) | (abuf[i*4+3] << 24);  
    }
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
    PS2MouseState *s;

    uint64_t r = 0;

    addr >>= 2;

//    printf("RISC6 IO READ OF: %ld\n",addr);

    switch (addr) {


    case R_MILLISECONDS:
//        printf("IO Read Milliseconds\n");
        break;
    case R_LED:	      
        printf("IO Read LED\n");
        break;
    case R_RS232DATA:   
        printf("IO Read RS232Data\n");
        break;
    case R_RS232STATUS: 
        printf("IO Read RS232Status\n");
        break;

    case R_SPIDATA:
	r = spi_read(t);
	break;

    case R_SPICONTROL:
        r = 1;
        break;

    case R_MOUSE:
        s = (PS2MouseState *)t->mouse;
        if (t->mouse_oldx != s->mouse_dx || t->mouse_oldy != s->mouse_dy){
          t->mouse_oldx = s->mouse_dx;
          t->mouse_oldy = s->mouse_dy;
          printf("IO Read Mouse %d %d\n",t->mouse_oldx,t->mouse_oldy);
        }
        break;
    case R_KEYBOARD:
//        printf("IO Read Keyboard\n");
        break;


    case R_CONTROL:
        r = t->regs[R_CONTROL] & (CONTROL_ITO | CONTROL_CONT);
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

    case R_MILLISECONDS:
        printf("IO Write Milliseconds\n");
        break;
    case R_LED:
        printf("LED:%ld\n",value);
        if(value==132){
          t->debugcount=0;
        }
        break;
    case R_RS232DATA:
        printf("IO Write RS232Data\n");
        break;    
    case R_RS232STATUS:
        printf("IO Write RS232Status\n");
        break;
    case R_SPIDATA:
        spi_write(t,value);
        break;
    case R_SPICONTROL:
        t->spi_sel = value & 3;
        break;
    case R_MOUSE:
        printf("IO Write Mouse\n");
        break;
    case R_KEYBOARD:
        printf("IO Write Keyboard\n");
        break;

    case R_DEBUG:
        if((t->debugcount < 10000000)&&(t->debugcount >= 0)){
          printf("\nDEBUG:%08x:%08lx ",t->debugcount,value);
        }
        t->debugcount++;
        break;

    case R_DEBUG2:
        if((t->debugcount < 10000000)&&(t->debugcount >= 0)){
          printf("%lx ",value);
        }
        break;

    case R_DEBUG3:
        if((t->debugcount < 10000000)&&(t->debugcount >= 0)){
          printf("%08lx ",value);
        }
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



static void ps2_keyboard_event(DeviceState *dev, QemuConsole *src,
                               InputEvent *evt)
{
    printf("Key Event!\n");
}

static void ps2_mouse_event(DeviceState *dev, QemuConsole *src,
                            InputEvent *evt)
{
//    printf("Mouse Event!\n");

/*
    static const int bmap[INPUT_BUTTON__MAX] = {
        [INPUT_BUTTON_LEFT]   = PS2_MOUSE_BUTTON_LEFT,
        [INPUT_BUTTON_MIDDLE] = PS2_MOUSE_BUTTON_MIDDLE,
        [INPUT_BUTTON_RIGHT]  = PS2_MOUSE_BUTTON_RIGHT,
        [INPUT_BUTTON_SIDE]   = PS2_MOUSE_BUTTON_SIDE,
        [INPUT_BUTTON_EXTRA]  = PS2_MOUSE_BUTTON_EXTRA,
    };
*/
    PS2MouseState *s = (PS2MouseState *)dev;

    InputMoveEvent *move;

    InputBtnEvent *btn;

    // check if deltas are recorded when disabled 
//    if (!(s->mouse_status & MOUSE_STATUS_ENABLED))
//        return;



    switch (evt->type) {
    case INPUT_EVENT_KIND_REL:

        move = evt->u.abs.data;
        if (move->axis == INPUT_AXIS_X) {
            s->mouse_dx += move->value;
        } else if (move->axis == INPUT_AXIS_Y) {
            s->mouse_dy -= move->value;
        }
        if (s->mouse_dx > 1023){s->mouse_dx=1023;}
        if (s->mouse_dx < 0){s->mouse_dx=0;}
        if (s->mouse_dy < 0){s->mouse_dy=0;}
        if (s->mouse_dy > 767){s->mouse_dy=767;}

//        printf("Mouse position %d %d\n",s->mouse_dx,s->mouse_dy);

        break;

    case INPUT_EVENT_KIND_BTN:

        btn = evt->u.btn.data;
        if (btn->down) {
          printf("Mouse Btn %d down\n",btn->button);
        }else{
          printf("Mouse Btn %d up\n",btn->button);
        }
/*
        if (btn->down) {
            s->mouse_buttons |= bmap[btn->button];
            if (btn->button == INPUT_BUTTON_WHEEL_UP) {
                s->mouse_dz--;
            } else if (btn->button == INPUT_BUTTON_WHEEL_DOWN) {
                s->mouse_dz++;
            }
        } else {
            s->mouse_buttons &= ~bmap[btn->button];
        }
*/
        break;

    default:
        // keep gcc happy 
        break;
    }

}


static void ps2_mouse_sync(DeviceState *dev)
{
    PS2MouseState *s = (PS2MouseState *)dev;

//    printf("Mouse Sync %d %d %d !\n",s->mouse_dx,s->mouse_dy,s->mouse_dz);


//    /* do not sync while disabled to prevent stream corruption */
//    if (!(s->mouse_status & MOUSE_STATUS_ENABLED)) {
//        return;
//    }

//    if (s->mouse_buttons) {
//        qemu_system_wakeup_request(QEMU_WAKEUP_REASON_OTHER, NULL);
//    }
    if (!(s->mouse_status & MOUSE_STATUS_REMOTE)) {
//        /* if not remote, send event. Multiple events are sent if
//           too big deltas */
//        while (ps2_mouse_send_packet(s)) {
//            if (s->mouse_dx == 0 && s->mouse_dy == 0 && s->mouse_dz == 0)
//                break;
//        }
//      s->mouse_dx = 0 ;
//      s->mouse_dy = 0 ;
    }
}



static const VMStateDescription vmstate_ps2_common = {
    .name = "PS2 Common State",
    .version_id = 3,
    .minimum_version_id = 2,
    .fields = (VMStateField[]) {
        VMSTATE_INT32(write_cmd, PS2State),
        VMSTATE_INT32(queue.rptr, PS2State),
        VMSTATE_INT32(queue.wptr, PS2State),
        VMSTATE_INT32(queue.count, PS2State),
        VMSTATE_BUFFER(queue.data, PS2State),
        VMSTATE_END_OF_LIST()
    }
};



static const VMStateDescription vmstate_ps2_keyboard = {
    .name = "ps2kbd",
    .version_id = 3,
    .minimum_version_id = 2,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT(common, PS2KbdState, 0, vmstate_ps2_common, PS2State),
        VMSTATE_END_OF_LIST()
    }
};

//static const VMStateDescription vmstate_ps2_keyboard = {
//    .name = "ps2kbd",
//    .version_id = 3,
//    .minimum_version_id = 2,
//    .post_load = ps2_kbd_post_load,
//    .pre_save = ps2_kbd_pre_save,
//    .fields = (VMStateField[]) {
//        VMSTATE_STRUCT(common, PS2KbdState, 0, vmstate_ps2_common, PS2State),
//        VMSTATE_INT32(scan_enabled, PS2KbdState),
//        VMSTATE_INT32(translate, PS2KbdState),
//        VMSTATE_INT32_V(scancode_set, PS2KbdState,3),
//        VMSTATE_END_OF_LIST()
//    },
//    .subsections = (const VMStateDescription*[]) {
//        &vmstate_ps2_keyboard_ledstate,
//        &vmstate_ps2_keyboard_need_high_bit,
//        NULL
//    }
//};


static const VMStateDescription vmstate_ps2_mouse = {
    .name = "ps2mouse",
    .version_id = 2,
    .minimum_version_id = 2,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT(common, PS2MouseState, 0, vmstate_ps2_common, PS2State),
        VMSTATE_END_OF_LIST()
    }
};


//static const VMStateDescription vmstate_ps2_mouse = {
//    .name = "ps2mouse",
//    .version_id = 2,
//    .minimum_version_id = 2,
//    .post_load = ps2_mouse_post_load,
//    .pre_save = ps2_mouse_pre_save,
//    .fields = (VMStateField[]) {
//        VMSTATE_STRUCT(common, PS2MouseState, 0, vmstate_ps2_common, PS2State),
//        VMSTATE_UINT8(mouse_status, PS2MouseState),
//        VMSTATE_UINT8(mouse_resolution, PS2MouseState),
//        VMSTATE_UINT8(mouse_sample_rate, PS2MouseState),
//        VMSTATE_UINT8(mouse_wrap, PS2MouseState),
//        VMSTATE_UINT8(mouse_type, PS2MouseState),
//        VMSTATE_UINT8(mouse_detect_state, PS2MouseState),
//        VMSTATE_INT32(mouse_dx, PS2MouseState),
//        VMSTATE_INT32(mouse_dy, PS2MouseState),
//        VMSTATE_INT32(mouse_dz, PS2MouseState),
//        VMSTATE_UINT8(mouse_buttons, PS2MouseState),
//        VMSTATE_END_OF_LIST()
//    }
//};




static QemuInputHandler ps2_keyboard_handler = {
    .name  = "QEMU PS/2 Keyboard",
    .mask  = INPUT_EVENT_MASK_KEY,
    .event = ps2_keyboard_event,
};



void *ps2_kbd_init(void (*update_irq)(void *, int), void *update_arg);

void *ps2_kbd_init(void (*update_irq)(void *, int), void *update_arg)
{
    PS2KbdState *s = (PS2KbdState *)g_malloc0(sizeof(PS2KbdState));

//    trace_ps2_kbd_init(s);
//    s->common.update_irq = update_irq;
//    s->common.update_arg = update_arg;
    s->scancode_set = 2;
    vmstate_register(NULL, 0, &vmstate_ps2_keyboard, s);
    qemu_input_handler_register((DeviceState *)s,
                                &ps2_keyboard_handler);
//    qemu_register_reset(ps2_kbd_reset, s);
    return s;
}





static void kbd_update_kbd_irq(void *opaque, int level)
{
//    KBDState *s = (KBDState *)opaque;

//    if (level)
//        s->pending |= KBD_PENDING_KBD;
//    else
//        s->pending &= ~KBD_PENDING_KBD;
//    kbd_update_irq(s);
}


static QemuInputHandler ps2_mouse_handler = {
    .name  = "QEMU PS/2 Mouse",
    .mask  = INPUT_EVENT_MASK_BTN | INPUT_EVENT_MASK_REL,
    .event = ps2_mouse_event,
    .sync  = ps2_mouse_sync,
};

void *ps2_mouse_init(void (*update_irq)(void *, int), void *update_arg);
void *ps2_mouse_init(void (*update_irq)(void *, int), void *update_arg)
{
    PS2MouseState *s = (PS2MouseState *)g_malloc0(sizeof(PS2MouseState));

//    trace_ps2_mouse_init(s);
//    s->common.update_irq = update_irq;
//    s->common.update_arg = update_arg;
    s->mousefixed = false;
    s->mouse_dx = 0;
    s->mouse_dy = 0;
    vmstate_register(NULL, 0, &vmstate_ps2_mouse, s);
    qemu_input_handler_register((DeviceState *)s,
                                &ps2_mouse_handler);
//    qemu_register_reset(ps2_mouse_reset, s);
    return s;
}



static void kbd_update_aux_irq(void *opaque, int level)
{
//    KBDState *s = (KBDState *)opaque;

//    if (level)
//        s->pending |= KBD_PENDING_AUX;
//    else
//        s->pending &= ~KBD_PENDING_AUX;
//    kbd_update_irq(s);
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

    memory_region_init_ram(&t->vram_mem, NULL, "fpga.vram", t->vram_size,
                           &error_fatal);
    memory_region_set_log(&t->vram_mem, true, DIRTY_MEMORY_VGA);
    sysbus_init_mmio(sbd, &t->vram_mem);

    sysbus_init_irq(sbd, &t->irq);

    t->mouse_oldx=0;
    t->mouse_oldy=0;

    t->con = graphic_console_init(DEVICE(dev), 0, &video_ops, t);
    qemu_console_resize(t->con, t->width, t->height);

    t->kbd = ps2_kbd_init( kbd_update_kbd_irq, t);
    t->mouse = ps2_mouse_init( kbd_update_aux_irq, t);



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
      printf("No dinfo for drive.\n");
    }

}

static int vmstate_fpga_post_load(void *opaque, int version_id)
{
    RISC6Timer *t = opaque;

    fpga_invalidate_display(t);

    return 0;
}

static const VMStateDescription vmstate_fpga = {
    .name = "fpga",
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = vmstate_fpga_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT16(height, RISC6Timer),
        VMSTATE_UINT16(width, RISC6Timer),
        VMSTATE_UINT16(depth, RISC6Timer), 
        VMSTATE_END_OF_LIST()
    }
};



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

    t->debugcount = -1;
}

static Property risc6_timer_properties[] = {
    DEFINE_PROP_UINT32("clock-frequency", RISC6Timer, freq_hz, 0),
    DEFINE_PROP_UINT32("vram-size",       RISC6Timer, vram_size, -1),
    DEFINE_PROP_UINT16("width",           RISC6Timer, width,     -1),
    DEFINE_PROP_UINT16("height",          RISC6Timer, height,    -1),
    DEFINE_PROP_UINT16("depth",           RISC6Timer, depth,     -1),
    DEFINE_PROP_END_OF_LIST(),
};

static void risc6_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = risc6_timer_realize;
    dc->props = risc6_timer_properties;
    dc->vmsd = &vmstate_fpga;
    dc->reset = risc6_timer_reset;
    set_bit(DEVICE_CATEGORY_INPUT, dc->categories);
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
