/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2009 Karl Kurbjun
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "file.h"
#include "lcd-remote.h"
#include "scroll_engine.h"
#include "uart-target.h"
#include "button.h"

static enum remote_control_states
{
    REMOTE_CONTROL_IDLE,
    REMOTE_CONTROL_NOP,
    REMOTE_CONTROL_POWER,
    REMOTE_CONTROL_MASK,
    REMOTE_CONTROL_DRAW,
    REMOTE_CONTROL_SLEEP
} remote_state_control = REMOTE_CONTROL_NOP, remote_state_control_next;

static enum remote_draw_states
{
    DRAW_TOP,
    DRAW_BOTTOM,
    DRAW_PAUSE,
} remote_state_draw = DRAW_TOP, remote_state_draw_next;

static bool remote_hold_button=false;

bool remote_initialized=true;

static unsigned char remote_contrast=DEFAULT_REMOTE_CONTRAST_SETTING;
static unsigned char remote_power=0x00;
static unsigned char remote_mask=0x00;

/*** hardware configuration ***/

int lcd_remote_default_contrast(void)
{
    return DEFAULT_REMOTE_CONTRAST_SETTING;
}

void lcd_remote_sleep(void)
{
    remote_state_control_next=REMOTE_CONTROL_SLEEP;
}

void lcd_remote_powersave(bool on)
{
    if(on)
    {
        remote_power|=0xC0;
        remote_state_control_next=REMOTE_CONTROL_POWER;
    }
    else
    {
        remote_power&=~(0xC0);
        remote_state_control_next=REMOTE_CONTROL_POWER;
    }
}

void lcd_remote_set_contrast(int val)
{
    remote_contrast=(char)val;
    remote_state_control_next=REMOTE_CONTROL_POWER;
}

void lcd_remote_set_invert_display(bool yesno)
{
    (void)yesno;
}

bool remote_detect(void)
{
    return true;
}

void lcd_remote_on(void)
{
    remote_power|=0x80;
    remote_state_control_next=REMOTE_CONTROL_POWER;
}

void lcd_remote_off(void)
{
    remote_power&=~(0x80);
    remote_state_control_next=REMOTE_CONTROL_POWER;
}

/* This is the maximum transfer size to the remote (op 0x51= 7 bytes setup+79 
 *  bytes screen data+xor+sum 
 */
unsigned char remote_payload[88];
unsigned char remote_payload_size;
bool remote_repeat_draw=false;

unsigned char   remote_draw_x, remote_draw_y, 
                remote_draw_width, remote_draw_height;

/* Monitor remote hotswap */
static void remote_tick(void)
{
    unsigned char i;
    static unsigned char pause_length=0;
    
    if(remote_state_control!=REMOTE_CONTROL_DRAW)
        remote_state_control=remote_state_control_next;
    
    switch (remote_state_control)
    {
        case REMOTE_CONTROL_IDLE:
            remote_payload_size=0;
            remote_state_control=REMOTE_CONTROL_IDLE;
            break;
            
        case REMOTE_CONTROL_NOP:
            remote_payload[0]=0x11;
            remote_payload[1]=0x30;
            
            remote_payload_size=2;
            remote_state_control=REMOTE_CONTROL_NOP;
            break;
            
        case REMOTE_CONTROL_POWER:
            remote_payload[0]=0x31;
            remote_payload[1]=remote_power;
            remote_payload[2]=remote_contrast;
            
            remote_payload_size=3;
            remote_state_control=REMOTE_CONTROL_NOP;
            break;
            
        case REMOTE_CONTROL_MASK:
            remote_payload[0]=0x41;
            remote_payload[1]=remote_mask;
            
            remote_payload_size=2;
            remote_state_control=REMOTE_CONTROL_NOP;
            break;
            
        case REMOTE_CONTROL_DRAW:
            remote_payload[0]=0x51;
            remote_payload[1]=0x80;
            remote_payload[2]=remote_draw_width;
            remote_payload[3]=remote_draw_x;
            
            remote_payload[5]=remote_draw_x+remote_draw_width;
            remote_payload_size=7+remote_payload[2];
            
            switch (remote_state_draw)
            {
                case DRAW_TOP:
                    remote_payload[4]=0;
                    remote_payload[6]=8;
                    
                    pause_length=6;
                    remote_state_draw_next=DRAW_BOTTOM;
                    remote_state_draw=DRAW_PAUSE;
                    break;
                    
                case DRAW_BOTTOM:
                    remote_payload[4]=8;
                    remote_payload[6]=16;
                    
                    pause_length=6;
                    remote_state_draw_next=DRAW_TOP;
                    remote_state_draw=DRAW_PAUSE;
                    break;
                    
                case DRAW_PAUSE:
                    remote_payload_size=0;
                    
                    if(--pause_length==0)
                    {
                        if(remote_state_draw_next==DRAW_TOP)
                            remote_state_control=REMOTE_CONTROL_NOP;
                            
                        remote_state_draw=remote_state_draw_next;
                    }
                    else
                        remote_state_draw=DRAW_PAUSE;

                    break;
                    
                default:
                    remote_payload_size=0;
                    break;
            }
            break;
            
        case REMOTE_CONTROL_SLEEP:
            remote_payload[0]=0x71;
            remote_payload[1]=0x30;

            remote_payload_size=2;
            remote_state_control=REMOTE_CONTROL_IDLE;
            break;
            
        default:
            remote_payload_size=0;
            break;
    }
    
    if(remote_payload_size==0)
    {
        return;
    }
    
    if(remote_payload[0]==0x51)
    {
        for(i=7; i<remote_payload_size; i++)
        {
            remote_payload[i]=
                lcd_remote_framebuffer[remote_payload[4]>>3][i+remote_draw_x-7];
        }
    }
    
    /* Calculate the xor and sum to place in the payload */
    remote_payload[remote_payload_size]=remote_payload[0];
    remote_payload[remote_payload_size+1]=remote_payload[0];
    for(i=1; i<remote_payload_size; i++)
    {
        remote_payload[remote_payload_size]^=remote_payload[i];
        remote_payload[remote_payload_size+1]+=remote_payload[i];
    }

    uart1_puts(remote_payload, remote_payload_size+2);
}

void lcd_remote_init_device(void)
{
    lcd_remote_clear_display();
    if (remote_detect())
        lcd_remote_on();

    lcd_remote_update();

    /* put the remote control in the tick task */
    tick_add_task(remote_tick);
}

/* Update the display.
   This must be called after all other LCD functions that change the display. */
void lcd_remote_update(void)
{
    lcd_remote_update_rect(0, 0, LCD_REMOTE_WIDTH, LCD_REMOTE_HEIGHT);
}

/* Update a fraction of the display. */
void lcd_remote_update_rect(int x, int y, int width, int height)
{
    remote_draw_x=x;
    remote_draw_y=y;
    remote_draw_width=width;
    remote_draw_height=height;
    
    remote_state_control=REMOTE_CONTROL_DRAW;
}

bool remote_button_hold(void)
{
    return remote_hold_button;
}

int remote_read_device(void)
{
    char read_buffer[5];
    int read_button = BUTTON_NONE;
    
    static int oldbutton=BUTTON_NONE;
    
    /* Handle remote buttons */
    if(uart1_gets_queue(read_buffer, 5)>=0)
    {
        int button_location;
        
        for(button_location=0;button_location<4;button_location++)
        {
            if((read_buffer[button_location]&0xF0)==0xF0 
                && (read_buffer[button_location+1]&0xF0)!=0xF0)
                break;
        }
        
        if(button_location==4)
            button_location=0;
        
        button_location++;
            
        read_button |= read_buffer[button_location];
        
        /* Find the hold status location */
        if(button_location==4)
            button_location=0;
        else
            button_location++;
            
        remote_hold_button=((read_buffer[button_location]&0x80)?true:false);
        
        uart1_clear_queue();
        oldbutton=read_button;
    }
    else
        read_button=oldbutton;
        
    return read_button;
}

void _remote_backlight_on(void)
{
    remote_power|=0x40;
    remote_state_control_next=REMOTE_CONTROL_POWER;
}

void _remote_backlight_off(void)
{
    remote_power&=~(0x40);
    remote_state_control_next=REMOTE_CONTROL_POWER;
}
