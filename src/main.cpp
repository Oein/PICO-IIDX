#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/types.h"
#include "hardware/timer.h"
#include "hardware/spi.h"
#include "hardware/adc.h"

#include "tusb.h"
#include "tusb_config.h"
#include "./usb_descriptors.h"

#include "bsp/board_api.h"
#include "class/hid/hid.h"

void hid_task(void);

hid_iidxpad_report_t gamepad_report = {0};

#define BUTTON0_PIN 0
#define BUTTON1_PIN 1
#define BUTTON2_PIN 2
#define BUTTON3_PIN 3
#define BUTTON4_PIN 4
#define BUTTON5_PIN 5
#define BUTTON6_PIN 6
#define BUTTON7_PIN 7
#define BUTTON8_PIN 8
#define BUTTON9_PIN 9
#define BUTTON10_PIN 10

#define setup_input_pin(pin)    \
    gpio_init(pin);             \
    gpio_set_dir(pin, GPIO_IN); \
    gpio_pull_up(pin);

int changeRange(int reqMin, int reqMax, int inMin, int inMax, int value)
{
    if (value < inMin)
        value = inMin;
    if (value > inMax)
        value = inMax;
    return (reqMax - reqMin) * (value - inMin) / (inMax - inMin) + reqMin;
}

int main()
{
    board_init();
    tusb_init();
    adc_init();

    adc_gpio_init(29);   // Initialize GPIO 29 for ADC
    adc_select_input(3); // Select ADC input 3 (GPIO 29)

    // Initialize button pins
    setup_input_pin(BUTTON0_PIN);
    setup_input_pin(BUTTON1_PIN);
    setup_input_pin(BUTTON2_PIN);
    setup_input_pin(BUTTON3_PIN);
    setup_input_pin(BUTTON4_PIN);
    setup_input_pin(BUTTON5_PIN);
    setup_input_pin(BUTTON6_PIN);
    setup_input_pin(BUTTON7_PIN);
    setup_input_pin(BUTTON8_PIN);
    setup_input_pin(BUTTON9_PIN);
    setup_input_pin(BUTTON10_PIN);

    srand(time(NULL));

    int setted_min = 115;
    int setted_max = 980;
    int res_min = 0;
    int res_max = 255;

    while (1)
    {
        tud_task();
        hid_task();

        // Read ADC and update gamepad X axis
        int read = adc_read();
        int mapped_value = changeRange(res_min, res_max, setted_min, setted_max, read);
        gamepad_report.x = (uint8_t)mapped_value;

        // read buttons
        gamepad_report.buttons[0] = 0;
        gamepad_report.buttons[1] = 0;
        gamepad_report.buttons[0] |= (gpio_get(BUTTON0_PIN) == 0) ? (1 << 0) : 0;
        gamepad_report.buttons[0] |= (gpio_get(BUTTON1_PIN) == 0) ? (1 << 1) : 0;
        gamepad_report.buttons[0] |= (gpio_get(BUTTON2_PIN) == 0) ? (1 << 2) : 0;
        gamepad_report.buttons[0] |= (gpio_get(BUTTON3_PIN) == 0) ? (1 << 3) : 0;
        gamepad_report.buttons[0] |= (gpio_get(BUTTON4_PIN) == 0) ? (1 << 4) : 0;
        gamepad_report.buttons[0] |= (gpio_get(BUTTON5_PIN) == 0) ? (1 << 5) : 0;
        gamepad_report.buttons[0] |= (gpio_get(BUTTON6_PIN) == 0) ? (1 << 6) : 0;
        gamepad_report.buttons[0] |= (gpio_get(BUTTON7_PIN) == 0) ? (1 << 7) : 0;
        gamepad_report.buttons[1] |= (gpio_get(BUTTON8_PIN) == 0) ? (1 << 0) : 0;
        gamepad_report.buttons[1] |= (gpio_get(BUTTON9_PIN) == 0) ? (1 << 1) : 0;
        gamepad_report.buttons[1] |= (gpio_get(BUTTON10_PIN) == 0) ? (1 << 2) : 0;

        sleep_ms(1);
    }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

void tud_mount_cb(void)
{
}

void tud_umount_cb(void)
{
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
}

void tud_resume_cb(void)
{
}

// ========================
// HID Task
// ========================

void send_hid_report(void)
{
    // skip if hid is not ready
    if (!tud_hid_n_ready(ITF_NUM_GAMEPAD))
    {
        return;
    }

    // Send the report (no report ID in descriptor, so use 0)
    tud_hid_n_report(ITF_NUM_GAMEPAD, 0, &gamepad_report, sizeof(gamepad_report));
}

void hid_task(void)
{
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    uint32_t now = board_millis();
    if (now - start_ms < interval_ms)
        return;
    start_ms = now;

    // Send report if ready
    if (tud_hid_n_ready(ITF_NUM_GAMEPAD))
    {
        send_hid_report();
    }
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)len;
    (void)report;
    (void)instance;
    // Don't send from callback, let hid_task() handle periodic sending
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}