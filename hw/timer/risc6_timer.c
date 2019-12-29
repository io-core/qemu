/*
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
    int           scalex;
    int           scaley;
    int           mousex;
    int           mousey;
    int           mousebtns;
    bool	  lctrldown;
    bool	  rctrldown;
    MemoryRegion  mmio;
    MemoryRegion  vram_mem;
    uint32_t      vram_size;
    uint32_t      freq_hz;
    uint32_t      milliseconds;
    int           full_update;
    uint16_t      width, height, depth;
    qemu_irq      irq;
    uint32_t      spi_sel;
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
    bool lctrldown;
    bool rctrldown;
    int scan_enabled;
    int translate;
    int scancode_set; // 1=XT, 2=AT, 3=PS/2 
    int ledstate;
    bool need_high_bit;
    unsigned int modifiers; // bitmask of MOD_* constants above 
    int key_count;
    uint8_t  key_buf[16];
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
    uint8_t mouse_buttons;
} PS2MouseState;

char c2scan[] = { 0x00,0x12,0x59,0x11,0x11,0x14,0x14,0x00,0x76,0x16,
                  0x1e,0x26,0x25,0x2e,0x36,0x3d,0x3e,0x46,0x45,0x4e,
                  0x55,0x66,0x0d,0x15,0x1d,0x24,0x2d,0x2c,0x35,0x3c,
                  0x43,0x44,0x4d,0x54,0x5b,0x5a,0x1c,0x1b,0x23,0x2b,
                  0x34,0x33,0x3b,0x42,0x4b,0x4c,0x52,0x0e,0x5d,0x1a,
                  0x22,0x21,0x2a,0x32,0x31,0x3a,0x41,0x49,0x4a,0x00,
                  0x29,0x58,0x05,0x06,0x04,0x0c,0x03,0x0b,0x83,0x0a,
                  0x01,0x09,0x77,0x4a,0x7c,0x00,0x00,0x00,0x00,0x49,
                  0x00,0x45,0x16,0x1e,0x26,0x25,0x2e,0x36,0x3d,0x3e,
                  0x46,0x00,0x78,0x07,0x00,0x6c,0x7d,0x7a,0x69,0x6b,
                  0x75,0x72,0x74,0x70,0x71,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

static void fpga_update_display(void *opaque)
{

    RISC6Timer *s = opaque;
    DisplaySurface *surface = qemu_console_surface(s->con);
    unsigned int width, height;
    const uint8_t *pix;
    uint32_t *data;

    int y, x, b;

    width = s->width;
    height = s->height;
    data = (uint32_t *)surface_data(surface);
    pix = memory_region_get_ram_ptr(&s->vram_mem);
    DirtyBitmapSnapshot *snap = NULL;

    
    snap = memory_region_snapshot_and_clear_dirty(&s->vram_mem, 0x0,
                                              memory_region_size(&s->vram_mem),
                                              DIRTY_MEMORY_VGA);
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x=x+8) {
          for (b = 0; b < 8; b++) {
            data[((height-1)*width-(y*width))+x+b] = (((pix[y*width/8+(x/8)] >> b ) & 1 ) == 1 ) ? 0 : BGCOLOR;
          }
        }
    }

        dpy_gfx_update(s->con, 0, 0, width, height );


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
    bool irq = false; //(t->regs[R_STATUS] & STATUS_TO) &&
                      //(t->regs[R_CONTROL] & CONTROL_ITO);
    return irq;
}

static void read_sector(RISC6Timer *t){
  uint8_t abuf[1024];
  uint32_t * r;

  if (t->disk_size < ((t->disk_index)*512)+512) {
    printf("Disk Read Past End\n");
  }else{

    int alen = blk_pread(t->blk, (t->disk_index)*512, abuf, 512);

    if (alen != 512) {
      printf("Disk Read Error\n");
    }

    r = (uint32_t *) &abuf;
    int i;
    for (i = 0; i < 128; i++) {

      r = (uint32_t *) &(abuf[i*4]);
      t->tx_buf[i+2] = *r;
      r++;

    }
  }

}

static void write_sector(RISC6Timer *t){
  printf("Write Sector\n");

  if (t->disk_size < ((t->disk_index)*512)+512) {
    printf("Disk Write Past End\n");
  }else{
    int alen = blk_pwrite(t->blk, (t->disk_index)*512, (char *)t->rx_buf, 512, 0);
    if (alen != 512) {
      printf("Disk Write Error\n");
    }

  }

}



static void disk_run_command(RISC6Timer *t){
  uint32_t cmd = t->rx_buf[0];
  uint32_t arg = ((t->rx_buf[1] & 0xFF) << 24) | ((t->rx_buf[2] & 0xFF) << 16) | ((t->rx_buf[3] & 0xFF) << 8) | (t->rx_buf[4] & 0xFF);

  switch (cmd) {
    case 81: 
      t->disk_state = diskRead;
      t->tx_buf[0] = 0;
      t->tx_buf[1] = 254;
//      printf("Seek %8u for read\n",arg);
      t->disk_index = ((arg -  t->disk_offset ));//(unsigned int)t->disk_offset) );// * 512;

      read_sector(t);
      t->tx_cnt = 2 + 128;
      break;
    case 88: 
      t->disk_state = diskWrite;
      printf("Seek %8u for write\n",arg);
      t->disk_index = ((arg -  t->disk_offset ));//(unsigned int)t->disk_offset) );// * 512;
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
    PS2MouseState *m;
    PS2KbdState *k;

    uint64_t r = 0;
    uint32_t kcd;
    
    addr >>= 2;

    switch (addr) {
    case R_MILLISECONDS:
//       printf("IO Read Millisecond %d\n",t->milliseconds);
        r = t->milliseconds;
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
        m = (PS2MouseState *)t->mouse;
        k = (PS2KbdState *)t->kbd;
        if (t->mousex != m->mouse_dx || t->mousey != m->mouse_dy){
          t->mousex = m->mouse_dx * t->scalex / 0x7fff;
          t->mousey = 767 - (m->mouse_dy * t->scaley / 0x7fff);
          if(k->lctrldown){kcd = 2 << 24;}else{kcd = 0;}
          if(k->rctrldown){kcd = kcd | 1 << 24;}
        r = (k->key_count > 0 ?  0x10000000 : 0) | 
                        (m->mouse_buttons << 24) |  kcd |
                 ((t->mousey <<12) & 0x00FFF000) | 
                        (t->mousex & 0x00000FFF);
        }
        break;
    case R_KEYBOARD:
        k = (PS2KbdState *)t->kbd;
        if (k->key_count > 0){
          r = k->key_buf[0];
          k->key_count--;
          memmove(&k->key_buf[0], &k->key_buf[1], k->key_count);
        }
        break;

    case R_DEBUG:
        r = (uint64_t)t->width << 20 | (uint64_t)t->height << 8 | ((uint64_t)t->depth & 255);
//        printf("DEBUG READ: %04x %04x %08lx\n",t->width,t->height,r);

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
    t->milliseconds++;
    ptimer_set_limit(t->ptimer, 1, 1); //tvalue + 1, 1);

        ptimer_run(t->ptimer, 1);
}



static void ps2_keyboard_event(DeviceState *dev, QemuConsole *src,
                               InputEvent *evt)
{
    PS2KbdState *k = (PS2KbdState *)dev;
    InputKeyEvent *key = evt->u.key.data;
    int qcode,scan;

    scan = qemu_input_key_value_to_qcode(key->key);
    qcode = c2scan[scan];

    if (key->down) {

      k->key_buf[k->key_count++]=(uint8_t)qcode;
      if (scan==3) {k->lctrldown = true;}
      if (scan==4) {k->rctrldown = true;}
//      printf("Key %d 0x%x\n",scan,qcode);
    } else {
      k->key_buf[k->key_count++]=0xF0;
      k->key_buf[k->key_count++]=(uint8_t)qcode;
      if (scan==3) {k->lctrldown = false;}
      if (scan==4) {k->rctrldown = false;}
    }


}

static void ps2_mouse_event(DeviceState *dev, QemuConsole *src,
                            InputEvent *evt)
{
    PS2MouseState *s = (PS2MouseState *)dev;

    InputMoveEvent *move;

    InputBtnEvent *btn;

    switch (evt->type) {
    case INPUT_EVENT_KIND_ABS:


        move = evt->u.abs.data;
            
            switch (move->axis) {
            case INPUT_AXIS_X:
                
                s->mouse_dx = move->value ;
                break;
            case INPUT_AXIS_Y:
                
                s->mouse_dy = move->value ;
                break;
            default:
               
                break;
            }

        break;

    case INPUT_EVENT_KIND_BTN:

        btn = evt->u.btn.data;
        if (btn->down) {
          switch (btn->button) {
          case 0:
            s->mouse_buttons = s->mouse_buttons | 4 ;
            break;
          case 1:
            s->mouse_buttons = s->mouse_buttons | 2 ;
            break;
          case 2:
            s->mouse_buttons = s->mouse_buttons | 1 ;
            break;
	  default:
            break;
          }
        }else{
          switch (btn->button) {
          case 0:
            s->mouse_buttons = s->mouse_buttons & 3 ;
            break;
          case 1:
            s->mouse_buttons = s->mouse_buttons & 5 ;
            break;
          case 2:
            s->mouse_buttons = s->mouse_buttons & 6 ;
            break;
          default:
            break;
          }
        }
        break;

    default:
        // keep gcc happy 
        break;
    }

}


static void ps2_mouse_sync(DeviceState *dev)
{
    
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
    s->scancode_set = 2;
    vmstate_register(NULL, 0, &vmstate_ps2_keyboard, s);
    qemu_input_handler_register((DeviceState *)s,
                                &ps2_keyboard_handler);
    s->lctrldown = false;
    s->rctrldown = false;
    s->key_count = 0;
    return s;
}





static void kbd_update_kbd_irq(void *opaque, int level)
{

}


static QemuInputHandler ps2_mouse_handler = {
    .name  = "QEMU Oberon Mouse",
    .mask  = INPUT_EVENT_MASK_BTN | INPUT_EVENT_MASK_ABS,
    .event = ps2_mouse_event,
    .sync  = ps2_mouse_sync,
};

void *ps2_mouse_init(void (*update_irq)(void *, int), void *update_arg);
void *ps2_mouse_init(void (*update_irq)(void *, int), void *update_arg)
{
    PS2MouseState *s = (PS2MouseState *)g_malloc0(sizeof(PS2MouseState));

    s->mouse_dx = 0;
    s->mouse_dy = 0;
    vmstate_register(NULL, 0, &vmstate_ps2_mouse, s);
    qemu_input_handler_register((DeviceState *)s,
                                &ps2_mouse_handler);

    return s;
}



static void kbd_update_aux_irq(void *opaque, int level)
{

}


static void risc6_timer_realize(DeviceState *dev, Error **errp)
{
    RISC6Timer *t = RISC6_TIMER(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);


    t->ptimer = ptimer_init(timer_hit, t, PTIMER_POLICY_DEFAULT);
    ptimer_transaction_begin(t->ptimer);
    ptimer_set_freq(t->ptimer, 1000 ); 
    ptimer_transaction_commit(t->ptimer);

    memory_region_init_io(&t->mmio, OBJECT(t), &timer_ops, t,
                          TYPE_RISC6_TIMER, R_MAX * sizeof(uint32_t));
    sysbus_init_mmio(sbd, &t->mmio);

    memory_region_init_ram(&t->vram_mem, NULL, "fpga.vram", t->vram_size,
                           &error_fatal);
    memory_region_set_log(&t->vram_mem, true, DIRTY_MEMORY_VGA);
    sysbus_init_mmio(sbd, &t->vram_mem);

    sysbus_init_irq(sbd, &t->irq);

    t->mousex=0;
    t->mousey=0;
    t->mousebtns = 0;

    t->con = graphic_console_init(DEVICE(dev), 0, &video_ops, t);
    qemu_console_resize(t->con, t->width, t->height);

    t->scalex = surface_width(qemu_console_surface(t->con)) - 1;
    t->scaley = surface_height(qemu_console_surface(t->con)) - 1;


    t->kbd = ps2_kbd_init( kbd_update_kbd_irq, t);
    t->mouse = ps2_mouse_init( kbd_update_aux_irq, t);

       if (t->blk) {
            if (blk_is_read_only(t->blk)) {
                error_setg(errp, "Can't use a read-only drive");
                return;
            }
            int ret = blk_set_perm(t->blk, BLK_PERM_CONSISTENT_READ | BLK_PERM_WRITE,
                               BLK_PERM_ALL, errp);
            if (ret < 0) {
                return;
            }
       }


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
               t->disk_offset =  524290;
            }else{
               t->disk_offset =  0;
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
    ptimer_set_limit(t->ptimer, 1, 1); //0xffffffff, 1);
    ptimer_run(t->ptimer, 1);
    ptimer_transaction_commit(t->ptimer);
    memset(t->regs, 0, sizeof(t->regs));
    t->spi_sel = 0;
    t->disk_state = 0;
    //disk_file;
//    t->disk_offset = 0;
    t->rx_idx = 0;
    t->tx_cnt = 0;
    t->tx_idx = 0;
    t->milliseconds = 0;
    t->lctrldown = false;
    t->rctrldown = false;
    t->debugcount = -1;
}

static Property risc6_timer_properties[] = {
    DEFINE_PROP_UINT32("clock-frequency", RISC6Timer, freq_hz, 1000),
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
