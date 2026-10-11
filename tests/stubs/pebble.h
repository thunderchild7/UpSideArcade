#ifndef PEBBLE_STUB_H
#define PEBBLE_STUB_H

/* Host stand-in so the watchface translation unit can be typechecked
   without the Pebble SDK. Declarations only. */

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct Window Window;
typedef struct Layer Layer;
typedef struct AppTimer AppTimer;
typedef struct GBitmap GBitmap;
typedef struct BitmapLayer BitmapLayer;

typedef struct { int16_t x; int16_t y; } GPoint;
typedef struct { int16_t w; int16_t h; } GSize;
typedef struct { GPoint origin; GSize size; } GRect;

#define GPoint(x, y) ((GPoint){(x), (y)})
#define GRect(x, y, w, h) ((GRect){{(x), (y)}, {(w), (h)}})

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
  bool did_vibrate;
  uint64_t timestamp;
} AccelData;

typedef enum {
  SECOND_UNIT = 1 << 0,
  MINUTE_UNIT = 1 << 1,
  HOUR_UNIT = 1 << 2,
  DAY_UNIT = 1 << 3,
  MONTH_UNIT = 1 << 4,
  YEAR_UNIT = 1 << 5
} TimeUnits;

#define TRIG_MAX_ANGLE 0x10000
#define APP_LOG_LEVEL_DEBUG 0
#define APP_LOG(level, fmt, ...) ((void)0)

#define RESOURCE_ID_IMAGE_NUM_0 100
#define RESOURCE_ID_IMAGE_NUM_1 101
#define RESOURCE_ID_IMAGE_NUM_2 102
#define RESOURCE_ID_IMAGE_NUM_3 103
#define RESOURCE_ID_IMAGE_NUM_4 104
#define RESOURCE_ID_IMAGE_NUM_5 105
#define RESOURCE_ID_IMAGE_NUM_6 106
#define RESOURCE_ID_IMAGE_NUM_7 107
#define RESOURCE_ID_IMAGE_NUM_8 108
#define RESOURCE_ID_IMAGE_NUM_9 109

typedef enum {
  GColorClear = 0,
  GColorBlack = 1,
  GColorWhite = 2
} GColor;

typedef struct RotBitmapLayer RotBitmapLayer;

Window *window_create(void);
void window_stack_push(Window *window, bool animated);
void window_destroy(Window *window);
Layer *window_get_root_layer(Window *window);

void layer_add_child(Layer *parent, Layer *child);
void layer_set_frame(Layer *layer, GRect frame);
GRect layer_get_frame(const Layer *layer);
void layer_set_bounds(Layer *layer, GRect bounds);
void layer_mark_dirty(Layer *layer);
void layer_remove_from_parent(Layer *child);
void layer_destroy(Layer *layer);

RotBitmapLayer *rot_bitmap_layer_create(GBitmap *bitmap);
void rot_bitmap_layer_destroy(RotBitmapLayer *bitmap_layer);
void rot_bitmap_layer_set_angle(RotBitmapLayer *bitmap_layer, int32_t angle);
void rot_bitmap_set_src_ic(RotBitmapLayer *bitmap_layer, GPoint ic);
void rot_bitmap_layer_set_corner_clip_color(RotBitmapLayer *bitmap_layer, GColor color);
Layer *rot_bitmap_layer_get_layer(RotBitmapLayer *bitmap_layer);

GBitmap *gbitmap_create_with_resource(uint32_t resource_id);
void gbitmap_destroy(GBitmap *bitmap);

BitmapLayer *bitmap_layer_create(GRect frame);
void bitmap_layer_set_bitmap(BitmapLayer *bitmap_layer, GBitmap *bitmap);
void bitmap_layer_destroy(BitmapLayer *bitmap_layer);
Layer *bitmap_layer_get_layer(BitmapLayer *bitmap_layer);

int32_t atan2_lookup(int16_t y, int16_t x);
void accel_service_peek(AccelData *data);
void accel_data_service_subscribe(uint32_t samples_per_update, void *handler);
void accel_data_service_unsubscribe(void);

bool clock_is_24h_style(void);

typedef void (*TickHandler)(struct tm *tick_time, TimeUnits units_changed);
void tick_timer_service_subscribe(TimeUnits units, TickHandler handler);
void tick_timer_service_unsubscribe(void);
void vibes_short_pulse(void);

void app_event_loop(void);

#endif
