#include <stdio.h>
//
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
//
#include "hw_config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
#include "pico/stdlib.h"
//
#include "f_util.h"
#include "hw_config.h"
#include "sd_card.h"

static bool write_file(const char *pathname)
{
    printf("Writing to %s\n", pathname);
    FIL fil = {};
    FRESULT fr = f_open(&fil, pathname, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
    {
        printf("f_open(%s) error: %s (%d)\n", pathname, FRESULT_str(fr), fr);
        return false;
    }
    if (f_printf(&fil, "Hello, world!\n") < 0)
    {
        printf("f_printf failed\n");
        return false;
    }
    fr = f_close(&fil);
    if (FR_OK != fr)
    {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }
    return true;
}

int main()
{
    stdio_init_all();
    time_init();

    // This must be called before sd_get_drive_prefix:
    sd_init_driver();

    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    for (int i = 0; i < 4; i++)
    {
        gpio_put(LED_PIN, 1);
        sleep_ms(500);
        gpio_put(LED_PIN, 0);
        sleep_ms(500);
    }

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    FATFS fs;
    sd_card_t *sd_card_p = sd_get_by_num(0);
    assert(sd_card_p);
    char const *drive_prefix = sd_get_drive_prefix(sd_card_p);
    FRESULT fr = f_mount(&sd_card_p->state.fatfs, drive_prefix, 1);
    if (FR_OK != fr)
    {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        exit(1);
    }
    char buf[128];
    snprintf(buf, sizeof buf, "%s/file%d.txt", drive_prefix, 0);
    if (!write_file(buf))
        exit(2);

    sd_card_t *sd_card_p2 = sd_get_by_num(1);
    assert(sd_card_p2);
    char const *drive_prefix2 = sd_get_drive_prefix(sd_card_p2);
    FRESULT fr2 = f_mount(&sd_card_p2->state.fatfs, drive_prefix2, 1);
    if (FR_OK != fr2)
    {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr2), fr2);
        exit(1);
    }
    char buf2[128];
    snprintf(buf2, sizeof buf2, "%s/file%d.txt", drive_prefix2, 1);
    if (!write_file(buf2))
        exit(2);

    // Open the source file for reading
    FIL src_file;
    fr = f_open(&src_file, buf, FA_READ);
    if (fr != FR_OK)
    {
        printf("Failed to open source file for reading: %s (%d)\n", FRESULT_str(fr), fr);
        exit(2);
    }

    // Open the destination file for writing
    char buf3[128];
    snprintf(buf3, sizeof buf3, "%s/file%d.txt", drive_prefix2, 0);
    FIL dest_file;
    fr = f_open(&dest_file, buf3, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK)
    {
        printf("Failed to open destination file for writing: %s (%d)\n", FRESULT_str(fr), fr);
        exit(3);
    }

    // Read from the source file and write to the destination file
    UINT br, bw;
    while (f_read(&src_file, buf, sizeof(buf), &br) == FR_OK && br > 0)
    {
        f_write(&dest_file, buf, br, &bw);
        if (bw != br)
        {
            printf("Error writing to destination file\n");
            exit(4);
        }
    }

    // Close the files
    f_close(&src_file);
    f_close(&dest_file);

    f_unmount(drive_prefix);
    f_unmount(drive_prefix2);

    while (true)
    {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}
