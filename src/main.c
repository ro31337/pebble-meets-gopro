#include <pebble.h>

#define HANDLER_WORKER_TICKS 0
#define HANDLER_APP_BUTTONS 1
#define MY_BUTTON_UP 0
#define MY_BUTTON_DOWN 1

static Window *s_main_window;
static TextLayer *s_output_layer, *s_ticks_layer;

static void toggle_worker(void) {
  // Check to see if the worker is currently active
  bool running = app_worker_is_running();

  // Toggle running state
  AppWorkerResult result;
  if(running) {
    result = app_worker_kill();

    if(result == APP_WORKER_RESULT_SUCCESS) {
      text_layer_set_text(s_ticks_layer, "Click to start...");
    } else {
      text_layer_set_text(s_ticks_layer, "Error killing worker!");
    }
  } else {
    result = app_worker_launch();

    if(result == APP_WORKER_RESULT_SUCCESS) {
      text_layer_set_text(s_ticks_layer, "Recording...");
    } else {
      text_layer_set_text(s_ticks_layer, "Error launching worker!");
    }
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Result: %d", result);
}

static void worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  if(type == HANDLER_WORKER_TICKS) {
    // Read ticks from worker's packet
    int ticks_recorded = data->data0;
    int ticks_left = data->data1;

    int ticks_recorded_minutes = ticks_recorded / 60;
    int ticks_recorded_seconds = ticks_recorded % 60;

    int ticks_left_minutes = ticks_left / 60;
    int ticks_left_seconds = ticks_left % 60;

    // Show to user in TextLayer
    static char s_buffer[32];
    snprintf(s_buffer, sizeof(s_buffer), "%02d:%02d recorded,\n%02d:%02d left",
      ticks_recorded_minutes,
      ticks_recorded_seconds,
      ticks_left_minutes,
      ticks_left_seconds);
    text_layer_set_text(s_ticks_layer, s_buffer);

    if(ticks_left == 0) {
      toggle_worker();
    }
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  bool running = app_worker_is_running();

  if(!running) {
    toggle_worker();
  }

  // Construct a data packet
  AppWorkerMessage msg_data = {
    .data0 = MY_BUTTON_UP
  };

  // Send the data to the foreground app
  app_worker_send_message(HANDLER_APP_BUTTONS, &msg_data);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Construct a data packet
  AppWorkerMessage msg_data = {
    .data0 = MY_BUTTON_DOWN
  };

  // Send the data to the foreground app
  app_worker_send_message(HANDLER_APP_BUTTONS, &msg_data);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  toggle_worker();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  const int inset = 8;

  // Create UI
  s_output_layer = text_layer_create(bounds);
  text_layer_set_text(s_output_layer, "UP: -5 seconds\nSELECT: start/stop\nDOWN: +5 seconds");
  text_layer_set_text_alignment(s_output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
#ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(s_output_layer, inset);
#endif

  s_ticks_layer = text_layer_create(GRect(PBL_IF_RECT_ELSE(15, 10), 80, bounds.size.w, 30));
  text_layer_set_text(s_ticks_layer, "No data yet.");
  text_layer_set_text_alignment(s_ticks_layer, PBL_IF_RECT_ELSE(GTextAlignmentLeft,
                                                                GTextAlignmentCenter));
  layer_add_child(window_layer, text_layer_get_layer(s_ticks_layer));
#ifdef PBL_ROUND
  text_layer_enable_screen_text_flow_and_paging(s_ticks_layer, inset);
#endif
}

static void main_window_unload(Window *window) {
  // Destroy UI
  text_layer_destroy(s_output_layer);
  text_layer_destroy(s_ticks_layer);
}

static void init(void) {
  // Setup main Window
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  // Subscribe to Worker messages
  app_worker_message_subscribe(worker_message_handler);
}

static void deinit(void) {
  // Destroy main Window
  window_destroy(s_main_window);

  // No more worker updates
  app_worker_message_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
