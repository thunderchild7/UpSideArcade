

#include "pebble.h"
#include "face_logic.h"

#define TOTAL_IMAGE_SLOTS FACE_SLOT_COUNT

static Window *window;

static AppTimer *timer;

static GBitmap *images[TOTAL_IMAGE_SLOTS];
static RotBitmapLayer *image_layers[TOTAL_IMAGE_SLOTS];

#define EMPTY_SLOT -1

static int image_slot_state[TOTAL_IMAGE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

static int deviceOrientation;
static int drawn_orientation = -1;

/* Minutes between short bursts, aligned to the hour. 0 turns them off. */
static int mVibeMinutes = 5;

static const uint32_t DIGIT_RESOURCE_IDS[10] = {
	RESOURCE_ID_IMAGE_NUM_0,
	RESOURCE_ID_IMAGE_NUM_1,
	RESOURCE_ID_IMAGE_NUM_2,
	RESOURCE_ID_IMAGE_NUM_3,
	RESOURCE_ID_IMAGE_NUM_4,
	RESOURCE_ID_IMAGE_NUM_5,
	RESOURCE_ID_IMAGE_NUM_6,
	RESOURCE_ID_IMAGE_NUM_7,
	RESOURCE_ID_IMAGE_NUM_8,
	RESOURCE_ID_IMAGE_NUM_9
};

static int32_t digit_rotation_angle(int orientation) {
	return (int32_t)face_rotation_quarters(orientation) * (TRIG_MAX_ANGLE / 4);
}

/* Basalt's pebble.h defines rot_bitmap_layer_get_layer() as 0. The layer
   object is the first field, same as TextLayer and the other layer types. */
static Layer *rot_layer_as_layer(RotBitmapLayer *rot_layer) {
	return (Layer *)rot_layer;
}

/* set_src_ic grows the layer so the bitmap can spin. Keep that drawing
   centered, and show only the slot, or the digit sits down and to the right. */
static void place_rot_layer_in_slot(RotBitmapLayer *rot_layer, int slot) {
	Layer *layer = rot_layer_as_layer(rot_layer);
	GRect fitted = layer_get_frame(layer);
	FaceRect slot_frame = face_frame_for_slot(slot);

	layer_set_frame(layer, GRect(slot_frame.x, slot_frame.y, slot_frame.w, slot_frame.h));
	/* The rotation center stays put when the frame shrinks, so look up-left. */
	layer_set_bounds(layer, GRect((slot_frame.w - fitted.size.w) / 2,
	                               (slot_frame.h - fitted.size.h) / 2,
	                               slot_frame.w, slot_frame.h));
}

static void load_digit_image_into_slot(int islot_number,int vdeviceOrientation, int digit_value) {
  /*

     Loads the digit image from the application's resources and
     displays it on-screen in the correct location.

     Each slot is a quarter of the screen.

   */
	int nslot_number = islot_number;
	

  // TODO: Signal these error(s)?
    APP_LOG(APP_LOG_LEVEL_DEBUG, "load_digit_image_from_slot! %d %d %d", nslot_number,vdeviceOrientation,digit_value);
  if ((nslot_number < 0) || (nslot_number >= TOTAL_IMAGE_SLOTS)) {
    return;
  }

  if ((digit_value < 0) || (digit_value > 9)) {
    return;
  }

  if (image_slot_state[nslot_number] != EMPTY_SLOT) {
    return;
  }

  image_slot_state[nslot_number] = digit_value;
  images[nslot_number] = gbitmap_create_with_resource(DIGIT_RESOURCE_IDS[digit_value]);

  RotBitmapLayer *rot_layer = rot_bitmap_layer_create(images[nslot_number]);
  image_layers[nslot_number] = rot_layer;
  /* 72 and 76 are even, so the center stays on a pixel boundary at 180 degrees. */
  rot_bitmap_set_src_ic(rot_layer, GPoint(TIME_IMAGE_WIDTH / 2, TIME_IMAGE_HEIGHT / 2));
  rot_bitmap_layer_set_angle(rot_layer, digit_rotation_angle(vdeviceOrientation));
  rot_bitmap_layer_set_corner_clip_color(rot_layer, GColorWhite);

  place_rot_layer_in_slot(rot_layer, islot_number);
  layer_add_child(window_get_root_layer(window), rot_layer_as_layer(rot_layer));
}

static void unload_digit_image_from_slot(int slot_number) {
  /*

     Removes the digit from the display and unloads the image resource
     to free up RAM.

     Can handle being called on an already empty slot.

   */

    APP_LOG(APP_LOG_LEVEL_DEBUG, "unload_digit_image_from_slot! %d ", slot_number);

  if (image_slot_state[slot_number] != EMPTY_SLOT) {
    layer_remove_from_parent(rot_layer_as_layer(image_layers[slot_number]));
    rot_bitmap_layer_destroy(image_layers[slot_number]);
    image_layers[slot_number] = NULL;
    gbitmap_destroy(images[slot_number]);
    images[slot_number] = NULL;
    image_slot_state[slot_number] = EMPTY_SLOT;
  }

}

static void apply_face_slots(const int slots[FACE_SLOT_COUNT]) {
  bool orientation_changed = (drawn_orientation != deviceOrientation);
  for (int slot = 0; slot < FACE_SLOT_COUNT; slot++) {
    if (slots[slot] == FACE_DIGIT_UNCHANGED) {
      continue;
    }
    if (!face_slot_needs_redraw(image_slot_state[slot], slots[slot])) {
      if (orientation_changed && image_layers[slot] != NULL) {
        rot_bitmap_layer_set_angle(image_layers[slot], digit_rotation_angle(deviceOrientation));
        layer_mark_dirty(rot_layer_as_layer(image_layers[slot]));
      }
      continue;
    }
    unload_digit_image_from_slot(slot);
    if (slots[slot] != FACE_DIGIT_BLANK) {
      load_digit_image_into_slot(slot, deviceOrientation, slots[slot]);
    }
  }
  drawn_orientation = deviceOrientation;
}

static void display_time(struct tm* pbltime) {
  time_t now;
  int slots[FACE_SLOT_COUNT];
  if (pbltime == NULL) {
    now = time(NULL);
    pbltime = localtime(&now);
  }

  for (int slot = 0; slot < FACE_SLOT_COUNT; slot++) {
    slots[slot] = FACE_DIGIT_UNCHANGED;
  }
  face_compose_time(slots, (unsigned short)pbltime->tm_hour,
                    (unsigned short)pbltime->tm_min, deviceOrientation,
                    clock_is_24h_style());
  apply_face_slots(slots);
}

static void update_orientation_from_accel(void) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };

  accel_service_peek(&accel);
  if (accel.did_vibrate) {
    return;
  }
  deviceOrientation = face_orientation_from_accel(accel.x, accel.y, accel.z, deviceOrientation);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  int previous_orientation = deviceOrientation;
  bool minute_changed = (units_changed & MINUTE_UNIT) != 0;

  update_orientation_from_accel();
  if (face_should_refresh(previous_orientation, deviceOrientation, minute_changed)) {
    display_time(tick_time);
  }
  if (minute_changed && tick_time != NULL &&
      face_should_vibe(tick_time->tm_min, mVibeMinutes)) {
    vibes_short_pulse();
  }
}

static void handle_accel(AccelData *accel_data, uint32_t num_samples) {
  // do nothing
}

static void init() {
  window = window_create();
  window_stack_push(window, true /* Animated */);

  deviceOrientation = UIInterfaceOrientationPortrait;
  /* Peek only works after subscribe. Read once so the first frame matches
     how the watch is being held. */
  accel_data_service_subscribe(0, NULL);
  update_orientation_from_accel();
  display_time(NULL);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit() {
  accel_data_service_unsubscribe();
  tick_timer_service_unsubscribe();
  for (int i = 0; i < TOTAL_IMAGE_SLOTS; i++) {
    unload_digit_image_from_slot(i);
  }
  window_destroy(window);
}
int main(void) {

  init();
	
  app_event_loop();
  deinit();
}
