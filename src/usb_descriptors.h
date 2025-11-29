#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "tusb.h"

enum
{
  ITF_NUM_GAMEPAD = 0,
  ITF_NUM_KEYBOARD,
  ITF_NUM_TOTAL
};

typedef struct TU_ATTR_PACKED
{
  uint8_t buttons[2]; // 16 buttons
  uint8_t x;          // X axis
  uint8_t y;          // Y axis
} hid_iidxpad_report_t;

// Use TinyUSB's standard keyboard report
// hid_keyboard_report_t is already defined in TinyUSB

#endif /* USB_DESCRIPTORS_H_ */