/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: not checked in
 *
 * Copyright (C) 2002 Markus Braun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <lcd.h>

#include "widgets.h"

#ifdef HAVE_LCD_BITMAP

/* Valid dimensions return true, invalid false */
bool valid_dimensions(int x, int y, int width, int height) 
{
    if((x < 0) || (x + width > LCD_WIDTH) || 
       (y < 0) || (y + height > LCD_HEIGHT))
    {
        return false;
    }

    return true;
}

void init_bar(int x, int y, int width, int height)
{
    /* draw box */
    lcd_drawrect(x, y, width, height);

    /* clear edge pixels */
    lcd_clearpixel(x, y);
    lcd_clearpixel((x + width - 1), y);
    lcd_clearpixel(x, (y + height - 1));
    lcd_clearpixel((x + width - 1), (y + height - 1));

    /* clear pixels in progress bar */
    lcd_clearrect(x + 1, y + 1, width - 2, height - 2);
}

/*
 * Print a progress bar
 */
void progressbar(int x, int y, int width, int height, int percent, 
                 int direction)
{
    int pos;

    /* check position and dimensions */
    if (!valid_dimensions(x, y, width, height))
        return;

    init_bar(x, y, width, height);

    /* draw bar */
    pos = percent;
    if(pos < 0)
        pos = 0;
    if(pos > 100)
        pos = 100;

    switch (direction)
    {
        case Grow_Right:
            pos=(width - 2) * pos / 100;
            lcd_fillrect(x + 1, y + 1, pos, height - 2);
            break;
        case Grow_Left:
            pos=(width - 2) * (100 - pos) / 100;
            lcd_fillrect(x + pos, y + 1, width - 1 - pos, height - 2);
            break;
        case Grow_Down:
            pos=(height - 2) * pos / 100;
            lcd_fillrect(x + 1, y + 1, width - 2, pos);
            break;
        case Grow_Up:
            pos=(height - 2) * (100 - pos) / 100;
            lcd_fillrect(x + 1, y + pos, width - 2, height - 1 - pos);
            break;
    }
}


/*
 * Print a slidebar bar
 */
void slidebar(int x, int y, int width, int height, int percent, int direction)
{
    int pos;

    /* check position and dimensions */
    if (!valid_dimensions(x, y, width, height))
        return;

    init_bar(x, y, width, height);

    /* draw knob */
    pos = percent;
    if(pos < 0)
        pos = 0;
    if(pos > 100)
        pos = 100;

    switch (direction)
    {
        case Grow_Right:
            pos = (width - height) * pos / 100;
            break;
        case Grow_Left:
            pos=(width - height) * (100 - pos) / 100;
            break;
        case Grow_Down:
            pos=(height - width) * pos / 100;
            break;
        case Grow_Up:
            pos=(height - width) * (100 - pos) / 100;
            break;
    }

    if(direction == Grow_Left || direction == Grow_Right)
        lcd_fillrect(x + pos + 1, y + 1, height - 2, height - 2);
    else
        lcd_fillrect(x + 1, y + pos + 1, width - 2, width - 2);
}


/*
 * Print a scroll bar
 */
void scrollbar(int x, int y, int width, int height, int items, int min_shown, 
               int max_shown, int orientation)
{
    int min;
    int max;
    int start;
    int size;

    /* check position and dimensions */
    if (!valid_dimensions(x, y, width, height))
        return;

    init_bar(x, y, width, height);

    /* min should be min */
    if(min_shown < max_shown) {
        min = min_shown;
        max = max_shown;
    }
    else {
        min = max_shown;
        max = min_shown;
    }

    /* limit min and max */
    if(min < 0)
        min = 0;
    if(min > items)
        min = items;

    if(max < 0)
        max = 0;
    if(max > items)
        max = items;

    /* calc start and end of the knob */
    if(items > 0 && items > (max - min)) {
        if(orientation == VERTICAL) {
            size = (height - 2) * (max - min) / items;
            start = (height - 2 - size) * min / (items - (max - min));
        }
        else {
            size = (width - 2) * (max - min) / items;
            start = (width - 2 - size) * min / (items - (max - min));
        }
    }
    else { /* if null draw a full bar */
        start = 0;
        if(orientation == VERTICAL) 
            size = (height - 2);
        else
            size = (width - 2);
    }

    /* knob has a width */
    if(size != 0) {
        if(orientation == VERTICAL)
            lcd_fillrect(x + 1, y + start + 1, width - 2, size);
        else
            lcd_fillrect(x + start + 1, y + 1, size, height - 2);
    }
    else { /* width of knob is null */
        if(orientation == VERTICAL) {
            start = (height - 2 - 1) * min / items;
            lcd_fillrect(x + 1, y + start + 1, width - 2, 1);
        }
        else {
            start = (width - 2 - 1) * min / items;
            lcd_fillrect(x + start + 1, y + 1, 1, height - 2);
        }
    }
}

/*
 * Print a checkbox
 */
void checkbox(int x, int y, int width, int height, bool checked)
{
    /* check position and dimensions */
    if((x < 0) || (x + width > LCD_WIDTH) || 
       (y < 0) || (y + height > LCD_HEIGHT) ||
       (width < 4 ) || (height < 4 ))
    {
        return;
    }

    lcd_drawrect(x, y, width, height);

    if (checked){
	lcd_drawline(x + 2, y + 2, x + width - 2 - 1 , y + height - 2 - 1);
	lcd_drawline(x + 2, y + height - 2 - 1, x + width - 2 - 1, y + 2);
    } else {
	/* be sure to clear box */
	lcd_clearrect(x + 1, y + 1, width - 2, height - 2);
    }
}

#endif
