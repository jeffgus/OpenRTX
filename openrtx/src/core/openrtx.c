/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/graphics.h>
#include <interfaces/delays.h>
#include <interfaces/filesystem.h>
#include <threads.h>
#include <openrtx.h>
#include <ui.h>
#include <backup.h>

extern void *ui_task(void *arg);

#if !defined(PLATFORM_LINUX) && !defined(PLATFORM_MOD17)
static void _openrtx_backup()
{
    ui_drawBackupScreen();
    gfx_render();
    // Wait 30ms before turning on backlight to hide random pixels on screen
    sleepFor(0u, 30u);
    platform_setBacklightLevel(state.settings.brightness);
    // Wait for backup over serial xmodem
    eflash_dump();
    // Backup completed: Ask user confirmation for flash initialization
    ui_drawFlashInitScreen();
    gfx_render();
    while(true)
    {
        if(platform_getPttStatus() == true)
        {
            int err = filesystem_format();
            // Flash init completed: reboot
            NVIC_SystemReset();
            break;
        }
        if(platform_pwrButtonStatus() == false)
        {
            // User wants to quit: shutdown
            break;
        }
    }
    // Shutdown
    platform_terminate();
    NVIC_SystemReset();
}
#endif

void openrtx_init()
{
    // Initialize platform drivers
    platform_init();

    // Initialize radio state
    state_init();

    // Initialize display and graphics driver
    gfx_init();

    // Set default contrast
    display_setContrast(state.settings.contrast);

    // Initialize filesystem
    int status = filesystem_init();
    if(status == 0)
        state.filesystem_ready = true;

    // Initialize user interface
    ui_init();
}

void *openrtx_run()
{
#if !defined(PLATFORM_LINUX) && !defined(PLATFORM_MOD17)
    // If filesystem initialization failed, enter backup mode (sink state)
    if(state.filesystem_ready == false)
        _openrtx_backup();
#endif

    // Display splash screen
    ui_drawSplashScreen(true);
    gfx_render();

    // Wait 30ms before turning on backlight to hide random pixels on screen
    sleepFor(0u, 30u);
    platform_setBacklightLevel(state.settings.brightness);

    // Start the OpenRTX threads
    create_threads();

    // Jump to the UI task
    ui_task(NULL);

    return NULL;
}
