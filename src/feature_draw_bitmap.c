

#include "pebble.h"
#include "face_logic.h"

#define TOTAL_IMAGE_SLOTS FACE_SLOT_COUNT

static Window *window;

static AppTimer *timer;

static GBitmap *images[TOTAL_IMAGE_SLOTS];
static BitmapLayer *image_layers[TOTAL_IMAGE_SLOTS];
static InverterLayer* inverter_layer;

#define EMPTY_SLOT -1

static int image_slot_state[TOTAL_IMAGE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

static int deviceOrientation;
static int drawn_orientation = -1;

static int mInvert = 1;			// Invert colours (0/1)
static int mVibeMinutes = 0;	// Vibrate every X minutes

#define NUMBER_OF_IMAGES 1
	const int IMAGE_RESOURCE_BLACK_IDS[10][2] = {
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_0,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_0
},
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_1,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_1
},
	{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_2,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_2
},
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_3,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_3
},
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_4,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_4
},
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_5,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_5
},
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_6,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_6
},
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_7,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_7
},
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_8,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_8
},
{
	RESOURCE_ID_IMAGE_NUM_BLACK_UP_9,RESOURCE_ID_IMAGE_NUM_BLACK_DOWN_9
}
};

static void set_invert() {
    if (!inverter_layer) {
		inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
		layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_layer));
		layer_mark_dirty(inverter_layer_get_layer(inverter_layer));
    }
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
//	if (mInvert){
//  images[slot_number] = gbitmap_create_with_resource(IMAGE_RESOURCE_WHITE_IDS[digit_value]);/
//}
//	if (!mInvert){
   images[nslot_number] = gbitmap_create_with_resource(
       IMAGE_RESOURCE_BLACK_IDS[digit_value][face_digit_variant(vdeviceOrientation)]);
//	};
	
  FaceRect slot_frame = face_frame_for_slot(islot_number);
  GRect frame = GRect(slot_frame.x, slot_frame.y, slot_frame.w, slot_frame.h);

  BitmapLayer *bitmap_layer = bitmap_layer_create(frame);
  image_layers[nslot_number] = bitmap_layer;
  bitmap_layer_set_bitmap(bitmap_layer, images[nslot_number]);
  Layer *window_layer = window_get_root_layer(window);
  layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));
}

static void unload_digit_image_from_slot(int slot_number) {
  /*

     Removes the digit from the display and unloads the image resource
     to free up RAM.

     Can handle being called on an already empty slot.

   */

    APP_LOG(APP_LOG_LEVEL_DEBUG, "unload_digit_image_from_slot! %d ", slot_number);

  if (image_slot_state[slot_number] != EMPTY_SLOT) {
    layer_remove_from_parent(bitmap_layer_get_layer(image_layers[slot_number]));
    bitmap_layer_destroy(image_layers[slot_number]);
    gbitmap_destroy(images[slot_number]);
    image_slot_state[slot_number] = EMPTY_SLOT;
  }

}

static void apply_face_slots(const int slots[FACE_SLOT_COUNT]) {
  bool orientation_changed = (drawn_orientation != deviceOrientation);
  for (int slot = 0; slot < FACE_SLOT_COUNT; slot++) {
    if (!face_slot_needs_redraw(image_slot_state[slot], slots[slot], orientation_changed)) {
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
  //set_invert();

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

  update_orientation_from_accel();
  if (face_should_refresh(previous_orientation, deviceOrientation,
                          (units_changed & MINUTE_UNIT) != 0)) {
    display_time(tick_time);
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
