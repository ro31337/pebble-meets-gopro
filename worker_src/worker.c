#include <pebble_worker.h>

#define HANDLER_WORKER_TICKS 0
#define HANDLER_APP_BUTTONS 1
#define MY_BUTTON_UP 0
#define MY_BUTTON_DOWN 1

#define REC_LENGTH 60 // default recording length in seconds

static uint16_t ticks_recorded = 0;
static uint16_t ticks_left = REC_LENGTH;

static void app_message_handler(uint16_t type, AppWorkerMessage *data) {
  if(type == HANDLER_APP_BUTTONS) {
    if(data->data0 == MY_BUTTON_UP) {
      ticks_recorded += 5;
    }
    if(data->data0 == MY_BUTTON_DOWN) {
      ticks_left += 5;
    }
  }
}

static void tick_handler(struct tm *tick_timer, TimeUnits units_changed) {
  // Update value
  ticks_recorded++;
  ticks_left--;

  // Construct a data packet
  AppWorkerMessage msg_data = {
    .data0 = ticks_recorded,
    .data1 = ticks_left
  };

  // Send the data to the foreground app
  app_worker_send_message(HANDLER_WORKER_TICKS, &msg_data);
}

static void worker_init() {
  // Use the TickTimer Service as a data source
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  app_worker_message_subscribe(app_message_handler);
}

static void worker_deinit() {
  // Stop using the TickTimerService
  tick_timer_service_unsubscribe();
}

int main(void) {
  worker_init();
  worker_event_loop();
  worker_deinit();
}
