/*
 * (c) 2015 flabberast <s3+flabbergast@sdfeu.org>
 *
 * Based on the following work:
 *  - Guillaume Duc's raw hid example (MIT License)
 *    https://github.com/guiduc/usb-hid-chibios-example
 *  - PJRC Teensy examples (MIT License)
 *    https://www.pjrc.com/teensy/usb_keyboard.html
 *  - hasu's TMK keyboard code (GPL v2 and some code Modified BSD)
 *    https://github.com/tmk/tmk_keyboard/
 *  - ChibiOS demo code (Apache 2.0 License)
 *    http://www.chibios.org
 *
 * Since some GPL'd code is used, this work is licensed under
 * GPL v2 or later.
 */

#include "ch.h"
#include "hal.h"

#include "usb_main.h"

/* TMK includes */
#include "report.h"
#include "host.h"
#include "host_driver.h"
#include "keyboard.h"
#include "joystick.h"
#include "action.h"
#include "action_util.h"
#include "mousekey.h"
#include "led.h"
#include "sendchar.h"
#include "debug.h"
#ifdef SLEEP_LED_ENABLE
#include "sleep_led.h"
#endif
#include "ps2_mouse.h"
#include "suspend.h"


/* -------------------------
 *   TMK host driver defs
 * -------------------------
 */

/* declarations */
uint8_t keyboard_leds(void);
void send_keyboard(report_keyboard_t *report);
void send_mouse(report_mouse_t *report);
void send_system(uint16_t data);
void send_consumer(uint16_t data);

/* host struct */
host_driver_t chibios_driver = {
  keyboard_leds,
  send_keyboard,
  send_mouse,
  send_system,
  send_consumer
};



/* ps2 mouse thread */
static THD_WORKING_AREA(WA_ps2_mouse, 128);
static THD_FUNCTION(ps2_mouse_thr, arg) {
  (void)arg;
  print("running mouse thread");
  while(true) {
#ifdef PS2_MOUSE_ENABLE
    ps2_mouse_task();
#endif
#ifdef JOYSTICK_MOUSE_ENABLE
  adcAcquireBus(&ADCD1);
    sample_joystick(); 
    chThdSleepMilliseconds(10);
  adcReleaseBus(&ADCD1);
#endif
  }
}

/* Main thread
 */
int main(void) {
  /* ChibiOS/RT init */
  halInit();
  chSysInit();

  /* Init USB */
  init_usb_driver(&USB_DRIVER);

  /* init printf */
  init_printf(NULL,sendchar_pf);

  /* Wait until the USB is active */
  while(USB_DRIVER.state != USB_ACTIVE)
    chThdSleepMilliseconds(50);

  print("USB configured.\n");

  /* init TMK modules */
  keyboard_init();
#ifdef JOYSTICK_MOUSE_ENABLE
  adcAcquireBus(&ADCD1);
  joystick_init(); 
  adcReleaseBus(&ADCD1);
#endif
  host_set_driver(&chibios_driver);

#ifdef SLEEP_LED_ENABLE
  sleep_led_init();
#endif

  print("Keyboard start.\n");

  /* Main loop */
  //chThdCreateStatic(WA_keyboard, sizeof(WA_keyboard), NORMALPRIO, keyboard_thr, NULL);
  chThdCreateStatic(WA_ps2_mouse, sizeof(WA_ps2_mouse), NORMALPRIO+1, ps2_mouse_thr, NULL);
  while(true) {

    if(USB_DRIVER.state == USB_SUSPENDED) {
      print("[s]");
      while(USB_DRIVER.state == USB_SUSPENDED) {
        /* Do this in the suspended state */
        suspend_power_down(); // on AVR this deep sleeps for 15ms
        /* Remote wakeup */
        if((USB_DRIVER.status & 2) && suspend_wakeup_condition()) {
          send_remote_wakeup(&USB_DRIVER);
        }
      }
      /* Woken up */
      // variables has been already cleared by the wakeup hook
      send_keyboard_report();
#ifdef MOUSEKEY_ENABLE
      mousekey_send();
#endif /* MOUSEKEY_ENABLE */
    }

    keyboard_task();
  }
}
