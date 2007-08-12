/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: jackpot.c 14034 2007-07-28 05:42:55Z kevin $
 *
 * Copyright (C) 2007 Copyright K�vin Ferrare based on Zakk Roberts's work
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "clock_draw_analog.h"
#include "xlcd.h"
#include "fixedpoint.h"
#include "clock_bitmaps.h"
#include "clock_bitmap_strings.h"

#define ANALOG_SECOND_RADIUS(screen, round) \
    ANALOG_MINUTE_RADIUS(screen, round)
#define ANALOG_MINUTE_RADIUS(screen, round) \
    (round?MIN(screen->height/2 -10, screen->width/2 -10):screen->height/2)
#define ANALOG_HOUR_RADIUS(screen, round) \
    (2*ANALOG_MINUTE_RADIUS(screen, round)/3)

#define HOUR_ANGLE(hour, minute, second) (30*(hour) +(minute)/2)
#define MINUTE_ANGLE(minute, second) (6*(minute)+(second)/10)
#define SECOND_ANGLE(second) (6 * (second))

/* Note that the given angle's origin is midday and not 3 o'clock */
void polar_to_cartesian(int a, int r, int* x, int* y)
{
#if CONFIG_LCD == LCD_SSD1815
    /* Correct non-square pixel aspect of archos recorder LCD */
    *x = (sin_int(a) * 5 / 4 * r) >> 14;
#else
    *x = (sin_int(a) * r) >> 14;
#endif
    *y = (sin_int(a-90) * r) >> 14;
}

void polar_to_cartesian_screen_centered(struct screen * display, 
                                        int a, int r, int* x, int* y)
{
    polar_to_cartesian(a, r, x, y);
    *x+=display->width/2;
    *y+=display->height/2;
}

void angle_to_square(int square_width, int square_height,
                     int a, int* x, int* y)
{
    a = (a+360-90)%360;
    if(a>45 && a<=135){/* top line */
        a-=45;
        *x=square_width-(square_width*2*a)/90;
        *y=square_height;
    }else if(a>135 && a<=225){/* left line */
        a-=135;
        *x=-square_width;
        *y=square_height-(square_height*2*a)/90;
    }else if(a>225 && a<=315){/* bottom line */
        a-=225;
        *x=(square_width*2*a)/90-square_width;
        *y=-square_height;
    }else if(a>315 || a<=45){/* right line */
        if(a>315)
            a-=315;
        else
            a+=45;
        *x=square_width;
        *y=(square_height*2*a)/90-square_height;
    }
}

void angle_to_square_screen_centered(struct screen * display,
                     int square_width, int square_height,
                     int a, int* x, int* y)
{
    angle_to_square(square_width, square_height, a, x, y);
    *x+=display->width/2;
    *y+=display->height/2;
}

void draw_hand(struct screen* display, int angle,
               int radius, int thickness, bool round)
{
    int x1, y1; /* the longest */
    int x2, y2, x3, y3; /* the base */
    if(round){/* round clock */
        polar_to_cartesian_screen_centered(display, angle, radius, &x1, &y1);
    }else{/* fullscreen clock, hands describes square motions */
        int square_width, square_height;
        /* radius is defined smallest between width and height */
        square_height=radius;
        square_width=(radius*display->width)/display->height;
        angle_to_square_screen_centered(
            display, square_width, square_height, angle, &x1, &y1);
    }
    polar_to_cartesian_screen_centered(display, (angle+120)%360,
        radius/40+thickness, &x2, &y2);
    polar_to_cartesian_screen_centered(display, (angle+240)%360,
        radius/40+thickness, &x3, &y3);
    xlcd_filltriangle_screen(display, x1, y1, x2, y2, x3, y3);
    rb->lcd_drawline(x1, y1, x2, y2);
    rb->lcd_drawline(x1, y1, x3, y3);
}

void draw_hands(struct screen* display, int hour, int minute, int second,
                int thickness, bool round, bool draw_seconds)
{
    if(draw_seconds){
        draw_hand(display, SECOND_ANGLE(second),
                  ANALOG_SECOND_RADIUS(display, round), thickness, round);
    }
    draw_hand(display, MINUTE_ANGLE(minute, second),
              ANALOG_MINUTE_RADIUS(display, round), thickness+2, round);
    draw_hand(display, HOUR_ANGLE(hour, minute, second),
              ANALOG_HOUR_RADIUS(display, round), thickness+2, round);
}

void draw_counter(struct screen* display, struct counter* counter)
{
    char buffer[10];
    int second_str_w, hour_str_w, str_h;
    const struct picture* smalldigits_bitmaps =
        &(smalldigits[display->screen_type]);
    struct time counter_time;
    counter_get_elapsed_time(counter, &counter_time);
    rb->snprintf(buffer, 10, "%02d:%02d",
                    counter_time.hour, counter_time.minute);
    getstringsize(smalldigits_bitmaps, buffer, &hour_str_w, &str_h);
    draw_string(display, smalldigits_bitmaps, buffer,
                display->width-hour_str_w,
                display->height-2*str_h);

    rb->snprintf(buffer, 10, "%02d", counter_time.second);
    getstringsize(smalldigits_bitmaps, buffer, &second_str_w, &str_h);
    draw_string(display, smalldigits_bitmaps, buffer,
                display->width-(hour_str_w+second_str_w)/2,
                display->height-str_h);
}

void draw_date(struct screen* display, struct time* time, int date_format)
{
    char buffer[10];
    int year_str_w, monthday_str_w, str_h;
    int year_line=date_format==JAPANESE?1:2;
    int monthday_line=date_format==JAPANESE?2:1;
    const struct picture* smalldigits_bitmaps =
        &(smalldigits[display->screen_type]);
    if(date_format==ENGLISH || date_format==JAPANESE){
        rb->snprintf(buffer, 10, "%02d/%02d", time->month, time->day);
    }else{
        rb->snprintf(buffer, 10, "%02d/%02d", time->day, time->month);
    }
    /* draws month and day */
    getstringsize(smalldigits_bitmaps, buffer, &monthday_str_w, &str_h);
    draw_string(display, smalldigits_bitmaps, buffer,
                0, display->height-year_line*str_h);
    rb->snprintf(buffer, 10, "%04d", time->year);

    /* draws year */
    getstringsize(smalldigits_bitmaps, buffer, &year_str_w, &str_h);
    draw_string(display, smalldigits_bitmaps, buffer,
                (monthday_str_w-year_str_w)/2,
                display->height-monthday_line*str_h);
}

void draw_border(struct screen* display, int skin)
{
    /* Draws square dots every 5 minutes */
    int i;
    int x, y;
    int size=display->height/50;/* size of the square dots */
    if(size%2)/* a pair number */
        size++;
    for(i=0; i < 60; i+=5){
        if(skin){
            polar_to_cartesian_screen_centered(display, MINUTE_ANGLE(i, 0),
                ANALOG_MINUTE_RADIUS(display, skin), &x, &y);
        }else{
            angle_to_square_screen_centered(
                display, display->width/2-size/2, display->height/2-size/2,
                MINUTE_ANGLE(i, 0), &x, &y);
        }
        display->fillrect(x-size/2, y-size/2, size, size);
    }
}

void draw_hour(struct screen* display, struct time* time,
               bool show_seconds, int skin)
{
    int hour=time->hour;
    if(hour >= 12)
        hour -= 12;

    /* Crappy fake antialiasing (color LCDs only)!
     * how this works is we draw a large mid-gray hr/min/sec hand,
     * then the actual (slightly smaller) hand on top of those.
     * End result: mid-gray edges to the black hands, smooths them out. */
#ifdef HAVE_LCD_COLOR
    if(display->is_color){
        display->set_foreground(LCD_RGBPACK(100,110,125));
        draw_hands(display, hour, time->minute, time->second,
                   1, skin, show_seconds);
        display->set_foreground(LCD_BLACK);
    }
#endif
    draw_hands(display, hour, time->minute, time->second,
               0, skin, show_seconds);
}

void draw_center_cover(struct screen* display)
{
    display->drawline((display->width/2)-1, (display->height/2)+3,
                      (display->width/2)+1, (display->height/2)+3);
    display->drawline((display->width/2)-3, (display->height/2)+2,
                      (display->width/2)+3, (display->height/2)+2);
    display->drawline((display->width/2)-4, (display->height/2)+1,
                      (display->width/2)+4, (display->height/2)+1);
    display->drawline((display->width/2)-4, display->height/2,
                      (display->width/2)+4, display->height/2);
    display->drawline((display->width/2)-4, (display->height/2)-1,
                      (display->width/2)+4, (display->height/2)-1);
    display->drawline((display->width/2)-3, (display->height/2)-2,
                      (display->width/2)+3, (display->height/2)-2);
    display->drawline((display->width/2)-1, (display->height/2)-3,
                      (display->width/2)+1, (display->height/2)-3);
}

void analog_clock_draw(struct screen* display, struct time* time,
                       struct clock_settings* settings,
                       struct counter* counter,
                       int skin)
{

    draw_hour(display, time, settings->analog.show_seconds, skin);
    if(settings->analog.show_border)
        draw_border(display, skin);
    if(counter)
        draw_counter(display, counter);
    if(settings->analog.show_date && settings->general.date_format!=NONE)
        draw_date(display, time, settings->general.date_format);
    draw_center_cover(display);
}
