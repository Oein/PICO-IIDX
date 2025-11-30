#define CFG_TUD_HID 2

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
hid_keyboard_report_t keyboard_report = {0};

// Key mapping for IIDX buttons (USB HID keycodes)
#define KEY_A 0x04
#define KEY_S 0x16
#define KEY_D 0x07
#define KEY_F 0x09
#define KEY_G 0x0A
#define KEY_H 0x0B
#define KEY_J 0x0D
#define KEY_K 0x0E
#define KEY_L 0x0F
#define KEY_Z 0x1D
#define KEY_X 0x1B

// Key mapping array for buttons 0-10
// Button 0 -> A, Button 1 -> S, Button 2 -> D, Button 3 -> F, Button 4 -> G,
// Button 5 -> H, Button 6 -> J, Button 7 -> K, Button 8 -> L, Button 9 -> Z, Button 10 -> X
const uint8_t button_keys[11] = {
    KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_Z, KEY_X};

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

bool mode = false; // false: gamepad mode, true: keyboard mode

int changeRange(int reqMin, int reqMax, int inMin, int inMax, int value)
{
    if (value < inMin)
        value = inMin;
    if (value > inMax)
        value = inMax;
    return (reqMax - reqMin) * (value - inMin) / (inMax - inMin) + reqMin;
}

// 6-Key Rollover implementation
void update_keyboard_report(bool button_states[11])
{
    // Clear all keys first
    memset(keyboard_report.keycode, 0, sizeof(keyboard_report.keycode));

    // Add pressed keys to the report (up to 6 keys)
    uint8_t key_count = 0;
    for (int i = 0; i < 11 && key_count < 6; i++)
    {
        if (button_states[i])
        {
            keyboard_report.keycode[key_count] = button_keys[i];
            key_count++;
        }
    }
}

void send_keyboard_report(void)
{
    if (tud_hid_n_ready(ITF_NUM_KEYBOARD))
    {
        // Always send the current keyboard report state
        // In keyboard mode, it contains the pressed keys
        // In gamepad mode, it should be empty (keys cleared in main loop)
        tud_hid_n_report(ITF_NUM_KEYBOARD, 0, &keyboard_report, sizeof(keyboard_report));
    }
}

bool mode_key_pressed = false;

void write_response(const char *response)
{
    // // split by 64 bytes
    size_t len = strlen(response);
    size_t offset = 0;
    while (offset < len)
    {
        size_t chunk_size = (len - offset > 64) ? 64 : (len - offset);
        tud_cdc_write(response + offset, chunk_size);
        tud_cdc_write_flush();
        offset += chunk_size;
    }
}

int main()
{
    board_init();
    tusb_init();
    adc_init();

    adc_gpio_init(26);   // Initialize GPIO 26 for ADC
    adc_select_input(0); // Select ADC input 0 (GPIO 26)

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

    int setted_min = 4096;
    int setted_max = 0;
    int res_min = 0;
    int res_max = 255;

    const int DEADZONE = 4;

    while (1)
    {
        tud_task();
        hid_task();

        // Read ADC and update gamepad X axis
        int read = adc_read();
        if (read < setted_min)
            setted_min = read;
        if (read > setted_max)
            setted_max = read;
        int mapped_value = changeRange(res_min, res_max, setted_min, setted_max, read);
        static int prev_mapped_value = -1;

        if (prev_mapped_value == -1)
            prev_mapped_value = mapped_value;
        else
        {
            bool isLarge2MinTurnPoint = prev_mapped_value > 255 - DEADZONE && mapped_value < DEADZONE;
            bool isMin2LargeTurnPoint = prev_mapped_value < DEADZONE && mapped_value > 255 - DEADZONE;
            static int last_setted_min = read;
            static int last_setted_max = read;
            if (isLarge2MinTurnPoint || isMin2LargeTurnPoint)
            {
                if (isLarge2MinTurnPoint)
                {
                    // 0 값으로 돌아올때 그떄의 최소값을 기준으로 천천히 맞춤
                    setted_min = (last_setted_min * 9 + read) / 10;
                }
                else
                {
                    // 255 값으로 돌아올때 그떄의 최대값을 기준으로 천천히 맞춤
                    setted_max = (last_setted_max * 9 + read) / 10;
                }
            }
            last_setted_min = setted_min;
            last_setted_max = setted_max;

            if (abs(prev_mapped_value - mapped_value) < DEADZONE)
                mapped_value = prev_mapped_value;
            else
                prev_mapped_value = mapped_value;
        }

        gamepad_report.x = (uint8_t)mapped_value;

        // write_response("ADC Read: ");
        // char buffer[16];
        // snprintf(buffer, sizeof(buffer), "%d", read);
        // write_response(buffer);
        // write_response("\r\n");

        static int report_counter = 0;
        report_counter++;
        if (report_counter >= 100)
        {
            report_counter = 0;
            write_response("ADC Min: ");
            char buffer_min[16];
            snprintf(buffer_min, sizeof(buffer_min), "%d", setted_min);
            write_response(buffer_min);
            write_response(" Max: ");
            char buffer_max[16];
            snprintf(buffer_max, sizeof(buffer_max), "%d", setted_max);
            write_response(buffer_max);
            write_response(" ADC Read: ");
            char buffer_read[16];
            snprintf(buffer_read, sizeof(buffer_read), "%d", read);
            write_response(buffer_read);
            write_response(" Mapped: ");
            char buffer_mapped[16];
            snprintf(buffer_mapped, sizeof(buffer_mapped), "%d", mapped_value);
            write_response(buffer_mapped);
            write_response("\r\n");
        }

        // read buttons
        bool gpioRead[] = {
            gpio_get(BUTTON0_PIN) == 0,
            gpio_get(BUTTON1_PIN) == 0,
            gpio_get(BUTTON2_PIN) == 0,
            gpio_get(BUTTON3_PIN) == 0,
            gpio_get(BUTTON4_PIN) == 0,
            gpio_get(BUTTON5_PIN) == 0,
            gpio_get(BUTTON6_PIN) == 0,
            gpio_get(BUTTON7_PIN) == 0,
            gpio_get(BUTTON8_PIN) == 0,
            gpio_get(BUTTON9_PIN) == 0,
            gpio_get(BUTTON10_PIN) == 0};

        gamepad_report.buttons[0] = 0;
        gamepad_report.buttons[1] = 0;
        gamepad_report.buttons[0] |= (gpioRead[0]) ? (1 << 0) : 0;
        gamepad_report.buttons[0] |= (gpioRead[1]) ? (1 << 1) : 0;
        gamepad_report.buttons[0] |= (gpioRead[2]) ? (1 << 2) : 0;
        gamepad_report.buttons[0] |= (gpioRead[3]) ? (1 << 3) : 0;
        gamepad_report.buttons[0] |= (gpioRead[4]) ? (1 << 4) : 0;
        gamepad_report.buttons[0] |= (gpioRead[5]) ? (1 << 5) : 0;
        gamepad_report.buttons[0] |= (gpioRead[6]) ? (1 << 6) : 0;
        gamepad_report.buttons[0] |= (gpioRead[7]) ? (1 << 7) : 0;
        gamepad_report.buttons[1] |= (gpioRead[8]) ? (1 << 0) : 0;
        gamepad_report.buttons[1] |= (gpioRead[9]) ? (1 << 1) : 0;
        gamepad_report.buttons[1] |= (gpioRead[10]) ? (1 << 2) : 0;

        // Update keyboard report based on current mode
        if (mode) // keyboard mode
        {
            update_keyboard_report(gpioRead);
        }
        else // gamepad mode - clear keyboard
        {
            memset(keyboard_report.keycode, 0, sizeof(keyboard_report.keycode));
        }

        // BUTTON0, BUTTON3, BUTTON5 to switch mode
        bool current_mode_key = gpioRead[7] && gpioRead[10] && gpioRead[1];  // gamepad mode
        bool current_mode_key2 = gpioRead[7] && gpioRead[10] && gpioRead[3]; // keyboard mode
        bool current_mode_key3 = gpioRead[0] && gpioRead[3] && gpioRead[5];  // calibrate mode
        if ((current_mode_key || current_mode_key2 || current_mode_key3) && !mode_key_pressed)
        {
            if (current_mode_key || current_mode_key2)
            {
                mode = current_mode_key ? false : true;
            }
            else
            {
                setted_max = read;
                setted_min = read;
            }

            mode_key_pressed = true;
        }
        else if (!current_mode_key && !current_mode_key2)
        {
            mode_key_pressed = false;
        }

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

void send_gamepad_report(void)
{
    // skip if hid is not ready
    if (tud_hid_n_ready(ITF_NUM_GAMEPAD))
    {
        if (!mode)
        {
            tud_hid_n_report(ITF_NUM_GAMEPAD, 0, &gamepad_report, sizeof(gamepad_report));
        }
        else
        {
            // In keyboard mode, send empty gamepad report
            hid_iidxpad_report_t empty_report = {0};
            tud_hid_n_report(ITF_NUM_GAMEPAD, 0, &empty_report, sizeof(empty_report));
        }
    }
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

    send_gamepad_report();
    send_keyboard_report();
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)len;
    (void)report;
    (void)instance;
    // Don't send from callback, let hid_task() and keyboard_task() handle periodic sending
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

// CDC Callbacks

// void tud_cdc_line_state_cb(uint8_t instance, bool dtr, bool rts)
// {
//     (void)instance;
//     (void)dtr;
//     (void)rts;
// }

// void tud_cdc_rx_cb(uint8_t instance)
// {
//     (void)instance;
// }
