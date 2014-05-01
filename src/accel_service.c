#include "pebble.h"

#define MATH_PI 3.141592653589793238462
#define NUM_DISCS 1
#define DISC_DENSITY 0.25
#define ACCEL_RATIO 0.05
#define ACCEL_STEP_MS 50

/*
* 2d vector struct for position of ball.
*/
typedef struct Vec2d {
  int x;
  int y;
} Vec2d;

/*
* Disc struct to display on the watchapp
*/
typedef struct Disc {
  Vec2d pos;
  Vec2d vel;
  double mass;
  double radius;
} Disc;

static Disc discs[NUM_DISCS];

static double radius = 5;

static Window *window;

static GRect window_frame;

static Layer *disc_layer;

static AppTimer *timer;

static TextLayer *text_layer;

static DictionaryIterator *iter;


void out_sent_handler(DictionaryIterator *sent, void *context) {
  // outgoing message was delivered YAY!!
  text_layer_set_text(text_layer,"Debug: Succeeded to send AppMessage to Pebble");
  if(app_message_outbox_begin(&iter) != APP_MSG_OK){
    return;
  }
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);
  Vec2d force;
  force.x = accel.x * ACCEL_RATIO;
  force.y = -accel.y * ACCEL_RATIO;
  Tuplet value_1 = TupletInteger(1, force.x);
  Tuplet value_2 = TupletInteger(2, force.y);
  dict_write_tuplet(iter, &value_1);
  dict_write_tuplet(iter, &value_2);
  app_message_outbox_send();
   
 }


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
  text_layer_set_text(text_layer,"Debug: Failed to send AppMessage to Pebble");
  if(app_message_outbox_begin(&iter) != APP_MSG_OK){
    return;
  }
  
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);
  Vec2d force;
  force.x = accel.x * ACCEL_RATIO;
  force.y = -accel.y * ACCEL_RATIO;
  Tuplet value_1 = TupletInteger(1, force.x);
  Tuplet value_2 = TupletInteger(2, force.y);
  dict_write_tuplet(iter, &value_1);
  dict_write_tuplet(iter, &value_2);
  app_message_outbox_send();
   
 }


 void in_received_handler(DictionaryIterator *received, void *context) {
   // incoming message received
 }


 void in_dropped_handler(AppMessageResult reason, void *context) {
   // incoming message dropped
 }

/*
* Loading the window on the watchapp
*/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = window_frame = layer_get_frame(window_layer);
  
  //disc_layer = layer_create(frame);
  //layer_set_update_proc(disc_layer, disc_layer_update_callback);
  //layer_add_child(window_layer, disc_layer);
  
  text_layer = text_layer_create(GRect(0, 20, 90, 90));
	text_layer_set_text_color(text_layer, GColorBlack);
	text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, (Layer*) text_layer);

  /*for (int i = 0; i < NUM_DISCS; i++) {
    disc_init(&discs[i]);
  }*/
}

/*
* On ending app we unload the window.
*/
static void window_unload(Window *window) {
  //layer_destroy(disc_layer);
  text_layer_destroy(text_layer);
}


static void app_message_init(){
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
}

static void message_sending_init(void){
  
  if(app_message_outbox_begin(&iter) != APP_MSG_OK){
    APP_LOG(APP_LOG_LEVEL_INFO, "OUTBOX FAILED");
    return;
  }
  Tuplet value_1 = TupletInteger(1, 0);
  Tuplet value_2 = TupletInteger(2, 0);
  dict_write_tuplet(iter, &value_1);
  dict_write_tuplet(iter, &value_2);
  
  app_message_outbox_send();
}

/*
* Main init function. Creates app window. Subscribes to accel service and initializes timer. 
*/
static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);

  accel_data_service_subscribe(0, NULL);
  app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
}

static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  app_message_init();
  message_sending_init();
  app_event_loop();
  deinit();
}
