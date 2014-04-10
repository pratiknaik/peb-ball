#include "pebble.h"

#define MATH_PI 3.141592653589793238462
#define NUM_DISCS 1
#define DISC_DENSITY 0.25
#define ACCEL_RATIO 0.05
#define ACCEL_STEP_MS 50
  
/*typedef enum {
  DATA_LOG_SUCCESS = 0,
  DATA_LOG_BUSY,         //! Someone else is writing to this log
  DATA_LOG_FULL,         //! No more space to save data
  DATA_LOG_NOT_FOUND,    //! The log does not exist
  DATA_LOG_CLOSED,        //! The log was made inactive
  DATA_LOG_INVALID_PARAMS
} DataLoggingResult;
*/

/*
* 2d vector struct for position of ball.
*/
typedef struct Vec2d {
  double x;
  double y;
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

//Data logging session reference
static DataLoggingSessionRef my_data_log;

/*
* Helper method to log the accelerometer data
*/
void accel_data_logger(AccelData *data, uint32_t num_samples) {
  DataLoggingResult r = data_logging_log(my_data_log, data, num_samples);
  //text_layer = text_layer_create(GRect(10, 110, 130, 20));
  //layer_add_child(disc_layer, text_layer_get_layer(text_layer));
  //text_layer_set_text(text_layer, "Got here");
}

/*
* Calculation of mass of disc
*/
static double disc_calc_mass(Disc *disc) {
  return MATH_PI * disc->radius * disc->radius * DISC_DENSITY;
}

/*
* Initializing the disc struct
*/
static void disc_init(Disc *disc) {
  GRect frame = window_frame;
  disc->pos.x = frame.size.w/2;
  disc->pos.y = frame.size.h/2;
  disc->vel.x = 0;
  disc->vel.y = 0;
  disc->radius = radius;
  disc->mass = disc_calc_mass(disc);
}

/*
* Updating the disc velocity using accel data
*/
static void disc_apply_force(Disc *disc, Vec2d force) {
  disc->vel.x += force.x / disc->mass;
  disc->vel.y += force.y / disc->mass;
}

/*
* Applying accel data to get force on disc
*/
static void disc_apply_accel(Disc *disc, AccelData accel) {
  Vec2d force;
  force.x = accel.x * ACCEL_RATIO;
  force.y = -accel.y * ACCEL_RATIO;
  disc_apply_force(disc, force);
}

/*
* Updating the position of the disc on the frame
*/
static void disc_update(Disc *disc) {
  const GRect frame = window_frame;
  double e = 0.5;
  
  // Bouncing off x walls effect
  if ((disc->pos.x - disc->radius < 0 && disc->vel.x < 0)
    || (disc->pos.x + disc->radius > frame.size.w && disc->vel.x > 0)) {
    disc->vel.x = -disc->vel.x * e;
  }
  
  //Bouncing off y walls effect
  if ((disc->pos.y - disc->radius < 0 && disc->vel.y < 0)
    || (disc->pos.y + disc->radius > frame.size.h && disc->vel.y > 0)) {
    disc->vel.y = -disc->vel.y * e;
  }
  disc->pos.x += disc->vel.x;
  disc->pos.y += disc->vel.y;
}

/*
* Drawing the disc
*/
static void disc_draw(GContext *ctx, Disc *disc) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(disc->pos.x, disc->pos.y), disc->radius);
}

/*
* Updating the disc and calling the draw function.
*/
static void disc_layer_update_callback(Layer *me, GContext *ctx) {
  for (int i = 0; i < NUM_DISCS; i++) {
    disc_draw(ctx, &discs[i]);
  }
}

/*
* Timer function to ger accel data.
*/
static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };

  accel_service_peek(&accel);
  
  accel_data_logger(&accel, 1);

  for (int i = 0; i < NUM_DISCS; i++) {
    Disc *disc = &discs[i];
    disc_apply_accel(disc, accel);
    disc_update(disc);
  }

  layer_mark_dirty(disc_layer);

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

/*
* Loading the window on the watchapp
*/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = window_frame = layer_get_frame(window_layer);
  
  disc_layer = layer_create(frame);
  layer_set_update_proc(disc_layer, disc_layer_update_callback);
  layer_add_child(window_layer, disc_layer);

  for (int i = 0; i < NUM_DISCS; i++) {
    disc_init(&discs[i]);
  }
}

/*
* Initializing the data logging
*/
static void data_log_init(void){
  my_data_log = data_logging_create(
    /* tag */ 42,
    /* DataLogType */ DATA_LOGGING_BYTE_ARRAY,
    /* length */ sizeof(AccelData),
    /* resume */ true );
}

/*
* On ending app we unload the window.
*/
static void window_unload(Window *window) {
  layer_destroy(disc_layer);
  data_logging_finish(my_data_log);
  text_layer_destroy(text_layer);
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

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();
  data_log_init();
  app_event_loop();
  deinit();
}
