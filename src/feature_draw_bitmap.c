

#include "pebble.h"
#include <math.h>
	
#define TOTAL_IMAGE_SLOTS 4

	// Magic numbers
#define SCREEN_WIDTH        144
#define SCREEN_HEIGHT       168

#define TIME_IMAGE_WIDTH    72
#define TIME_IMAGE_HEIGHT   76


#define MARGIN              1
#define TIME_SLOT_SPACE     2
	
static Window *window;

static Layer *layer;
static AppTimer *timer;

static GBitmap *images[TOTAL_IMAGE_SLOTS];
static BitmapLayer *image_layers[TOTAL_IMAGE_SLOTS];
static InverterLayer* inverter_layer;

#define EMPTY_SLOT -1

static int image_slot_state[TOTAL_IMAGE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT, EMPTY_SLOT};

static int deviceOrientation;

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

enum {
	UIInterfaceOrientationPortrait = 0,
	UIInterfaceOrientationPortraitUpsideDown = 1,
	UIInterfaceOrientationLandscapeRight = 2,
	UIInterfaceOrientationLandscapeLeft = 3
};

static void set_invert() {
    if (!inverter_layer) {
		inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
		layer_add_child(window_get_root_layer(window), inverter_layer_get_layer(inverter_layer));
		layer_mark_dirty(inverter_layer_get_layer(inverter_layer));
    }
}
  
GRect frame_for_time_slot(int islot_number) {
  int x = (islot_number % 2) * (TIME_IMAGE_WIDTH );
  int y = MARGIN + (islot_number / 2) * (TIME_IMAGE_HEIGHT + TIME_SLOT_SPACE);

  return GRect(x, y, TIME_IMAGE_WIDTH, TIME_IMAGE_HEIGHT);
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
   images[nslot_number] = gbitmap_create_with_resource(IMAGE_RESOURCE_BLACK_IDS[digit_value][vdeviceOrientation]);
//	};
	
  GRect frame = frame_for_time_slot(islot_number);
	
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

static void display_value(unsigned short value, unsigned short row_number, bool show_first_leading_zero) {
  /*

     Displays a numeric value between 0 and 99 on screen.

     Rows are ordered on screen as:

       Row 0
       Row 1

     Includes optional blanking of first leading zero,
     i.e. displays ' 0' rather than '00'.

   */
  value = value % 100; // Maximum of two digits per row.

  if (deviceOrientation == UIInterfaceOrientationPortrait) {
  // Column order is: | Column 0 | Column 1 |
  // (We process the columns in reverse order because that makes
  // extracting the digits from the value easier.)
  for (int column_number = 1; column_number >= 0; column_number--) {
    int slot_number = (row_number * 2) + column_number;
    unload_digit_image_from_slot(slot_number);
    if (!((value == 0) && (column_number == 0) && !show_first_leading_zero)) {
      load_digit_image_into_slot(slot_number,deviceOrientation, value % 10);
    }
    value = value / 10;
  };
  };
  if (deviceOrientation == UIInterfaceOrientationPortraitUpsideDown) {
  // Column order is: | Column 0 | Column 1 |
  // (We process the columns in reverse order because that makes
  // extracting the digits from the value easier.)
  for (int column_number = 0; column_number <= 1; column_number++) {
    int slot_number = (row_number * 2) + column_number;
    unload_digit_image_from_slot(slot_number);
    if (!((value == 0) && (column_number == 1) && !show_first_leading_zero)) {
      load_digit_image_into_slot(slot_number,deviceOrientation, value % 10);
    }
    value = value / 10;
  };
  };
}

static unsigned short get_display_hour(unsigned short hour) {

  if (clock_is_24h_style()) {
    return hour;
  }

  unsigned short display_hour = hour % 12;

  // Converts "0" to "12"
  return display_hour ? display_hour : 12;

}

static void display_time(struct tm* pbltime) {

  // TODO: Use `units_changed` and more intelligence to reduce
  //       redundant digit unload/load?
  time_t now;
  if (pbltime == NULL) {
    now = time(NULL);
    pbltime = localtime(&now);
  }

  display_value(get_display_hour(pbltime->tm_hour), (deviceOrientation == UIInterfaceOrientationPortrait) ? 0 : 1, false);
  display_value(pbltime->tm_min, (deviceOrientation == UIInterfaceOrientationPortrait) ? 1 : 0, true);
  //set_invert();

}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	
	 AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
     APP_LOG(APP_LOG_LEVEL_DEBUG, "got here ");

  accel_service_peek(&accel);
  // Insert UI code here
	
	     APP_LOG(APP_LOG_LEVEL_DEBUG, "accel data! %d %d %d ", accel.x,accel.y,accel.z);

		// Get the current device angle
	//float xx = -accel.x;
	//float yy = accel.y;
	//float angle = atan2(yy, xx); 
	//bool redraw = false;
	
	float angle = atan2_lookup	(accel.x,accel.y) / (TRIG_MAX_ANGLE/4) ;
	
     APP_LOG(APP_LOG_LEVEL_DEBUG, "angle! %d ", (int)angle * 100);
	
	if( angle >= -0.25 && angle < 0.75)
	{
		APP_LOG(APP_LOG_LEVEL_DEBUG, "landscape left! ");
		//if(deviceOrientation != UIInterfaceOrientationPortrait)
		//{
		//	deviceOrientation = UIInterfaceOrientationLandscapeLeft; // UIInterfaceOrientationPortrait;
         //   redraw = true;
		//}
	}
	else if(angle >= 0.5 && angle < 1.5)
	{
				APP_LOG(APP_LOG_LEVEL_DEBUG, "portrait up!  ");
		//if(deviceOrientation != UIInterfaceOrientationLandscapeRight)
		//{
			deviceOrientation = UIInterfaceOrientationPortrait;
			//redraw = true;
		//}
	}
	else if(angle >= 1.5 && angle < 2.5)
	{
				APP_LOG(APP_LOG_LEVEL_DEBUG, "landscape right! ");
		//if(deviceOrientation != UIInterfaceOrientationPortraitUpsideDown)
		//{
		//	deviceOrientation = UIInterfaceOrientationLandscapeRight  ;
         //   redraw = true;
		//}
	}
	else if(angle >= 2.1 || angle < 3.9)
	{
				APP_LOG(APP_LOG_LEVEL_DEBUG, "portrait down! ");
		//if(deviceOrientation != UIInterfaceOrientationLandscapeLeft)
		//{
			deviceOrientation = UIInterfaceOrientationPortraitUpsideDown;
          //  redraw = true;
		//}
	};

  	display_time(tick_time);
	

}

static void handle_accel(AccelData *accel_data, uint32_t num_samples) {
  // do nothing
}

static void init() {
	  window = window_create();
  window_stack_push(window, true /* Animated */);

	
  // Init the layer for display the image
  Layer *window_layer = window_get_root_layer(window);
  //GRect bounds = layer_get_frame(window_layer);

  
  deviceOrientation = UIInterfaceOrientationPortrait;
  // Avoids a blank screen on watch start.
  display_time(NULL);
	
  tick_timer_service_subscribe(SECOND_UNIT, handle_minute_tick);

 // timer = app_timer_register(100 /* milliseconds */, timer_callback, NULL);
 // Passing 0 to subscribe sets up the accelerometer for peeking
  accel_data_service_subscribe(0, NULL);

}

static void deinit() {
	  accel_data_service_unsubscribe();	
	  tick_timer_service_unsubscribe();
  for (int i = 0; i < TOTAL_IMAGE_SLOTS; i++) {
    unload_digit_image_from_slot(i);
  }
  window_destroy(window);
  layer_destroy(layer);

}
int main(void) {

  init();
	
  app_event_loop();
  deinit();
}
