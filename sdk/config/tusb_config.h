#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

// -----------------------------------------------------------------------------
// COMMON CONFIGURATION
// -----------------------------------------------------------------------------
#define CFG_TUSB_MCU              OPT_MCU_RP2040

#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#ifndef BOARD_TUD_RHPORT
    #define BOARD_TUD_RHPORT          0
#endif

#define CFG_TUD_ENDPOINT0_SIZE    64

// -----------------------------------------------------------------------------
// DEVICE CLASSES CONFIGURATION
// -----------------------------------------------------------------------------
#define CFG_TUD_ENABLED           1
#define CFG_TUD_CDC               1
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

// -----------------------------------------------------------------------------
// CDC CONFIGURATION
// -----------------------------------------------------------------------------
#define CFG_TUD_CDC_RX_BUFSIZE 8192
#define CFG_TUD_CDC_EP_BUFSIZE 4096
#define CFG_TUD_CDC_TX_BUFSIZE 768

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */