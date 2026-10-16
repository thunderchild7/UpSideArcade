#include "pebble.h"
#include "face_logic.h"

static Window *window;
static Layer *time_layer;
static GFont time_font;

static int deviceOrientation;

/* Minutes between short bursts, aligned to the hour. 0 turns them off. */
static int mVibeMinutes = 5;

static char hour_text[3];
static char minute_text[3];

/*
 * Karmatic Arcade, Vic Fieger. The size is the largest the platform can draw.
 * Digits sit below the top of the em box and hang past the advance on the right.
 * The line height is the ink, and the pads move that ink onto the centered line.
 * Measured with the SDK's monochrome FreeType render of karmatic-arcade.ttf.
 */
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
#define TIME_FONT RESOURCE_ID_FONT_KARMATIC_60
#define TIME_LINE_HEIGHT 59
#define TIME_INK_PAD_TOP 13
#define TIME_INK_PAD_LEFT 3
#else
#define TIME_FONT RESOURCE_ID_FONT_KARMATIC_42
#define TIME_LINE_HEIGHT 41
#define TIME_INK_PAD_TOP 9
#define TIME_INK_PAD_LEFT 2
#endif

/* Color watches are green on black. Black-and-white watches stay black on white. */
#if defined(PBL_COLOR)
#define FACE_BACKGROUND GColorBlack
#define FACE_FOREGROUND GColorGreen
#else
#define FACE_BACKGROUND GColorWhite
#define FACE_FOREGROUND GColorBlack
#endif

static void format_digits(char *out, int tens, int units) {
	char *cursor = out;

	if (tens >= 0 && tens <= 9) {
		*cursor++ = (char)('0' + tens);
	}
	if (units >= 0 && units <= 9) {
		*cursor++ = (char)('0' + units);
	}
	*cursor = '\0';
}

static void rotate_framebuffer_180(GBitmap *framebuffer) {
	GRect bounds = gbitmap_get_bounds(framebuffer);
	GBitmapFormat format = gbitmap_get_format(framebuffer);
	int width = bounds.size.w;
	int height = bounds.size.h;
	int y;

	if (format == GBitmapFormat1Bit) {
		face_rotate_pixels_180(gbitmap_get_data(framebuffer), width, height,
				gbitmap_get_bytes_per_row(framebuffer), 1);
		return;
	}
	if (format != GBitmapFormat8Bit && format != GBitmapFormat8BitCircular) {
		return;
	}

	for (y = 0; y < height; y++) {
		int y2 = height - 1 - y;
		int x;
		GBitmapDataRowInfo row;
		GBitmapDataRowInfo opposite;

		if (y > y2) {
			break;
		}
		row = gbitmap_get_data_row_info(framebuffer, (uint16_t)y);
		opposite = gbitmap_get_data_row_info(framebuffer, (uint16_t)y2);
		for (x = 0; x < width; x++) {
			int x2 = width - 1 - x;
			uint8_t first;
			uint8_t second;

			if (y == y2 && x >= x2) {
				break;
			}
			if (x < row.min_x || x > row.max_x || x2 < opposite.min_x || x2 > opposite.max_x) {
				continue;
			}
			first = row.data[x];
			second = opposite.data[x2];
			row.data[x] = second;
			opposite.data[x2] = first;
		}
	}
}

static void time_layer_update(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	FaceRect hour_line = face_time_draw_box(
			face_time_line_frame(bounds.size.w, bounds.size.h, TIME_LINE_HEIGHT, 0),
			TIME_INK_PAD_LEFT, TIME_INK_PAD_TOP);
	FaceRect minute_line = face_time_draw_box(
			face_time_line_frame(bounds.size.w, bounds.size.h, TIME_LINE_HEIGHT, 1),
			TIME_INK_PAD_LEFT, TIME_INK_PAD_TOP);
	GBitmap *framebuffer;

	graphics_context_set_antialiased(ctx, false);
	graphics_context_set_fill_color(ctx, FACE_BACKGROUND);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	graphics_context_set_text_color(ctx, FACE_FOREGROUND);
	graphics_draw_text(ctx, hour_text, time_font,
			GRect(hour_line.x, hour_line.y, hour_line.w, hour_line.h),
			GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	graphics_draw_text(ctx, minute_text, time_font,
			GRect(minute_line.x, minute_line.y, minute_line.w, minute_line.h),
			GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

	if (deviceOrientation != UIInterfaceOrientationPortraitUpsideDown) {
		return;
	}
	framebuffer = graphics_capture_frame_buffer(ctx);
	if (framebuffer == NULL) {
		return;
	}
	rotate_framebuffer_180(framebuffer);
	graphics_release_frame_buffer(ctx, framebuffer);
}

static void display_time(struct tm *pbltime) {
	time_t now;
	int slots[FACE_SLOT_COUNT];
	int slot;

	if (pbltime == NULL) {
		now = time(NULL);
		pbltime = localtime(&now);
	}

	for (slot = 0; slot < FACE_SLOT_COUNT; slot++) {
		slots[slot] = FACE_DIGIT_UNCHANGED;
	}
	/* Draw the readable time, then rotate the whole frame when the watch is upside down. */
	face_compose_time(slots, (unsigned short)pbltime->tm_hour,
			(unsigned short)pbltime->tm_min, UIInterfaceOrientationPortrait,
			clock_is_24h_style());
	format_digits(hour_text, slots[0], slots[1]);
	format_digits(minute_text, slots[2], slots[3]);
	if (time_layer != NULL) {
		layer_mark_dirty(time_layer);
	}
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

static void init(void) {
	Layer *root;

	window = window_create();
	window_set_background_color(window, FACE_BACKGROUND);
	window_stack_push(window, true /* Animated */);

	time_font = fonts_load_custom_font(resource_get_handle(TIME_FONT));
	root = window_get_root_layer(window);
	time_layer = layer_create(layer_get_bounds(root));
	layer_set_update_proc(time_layer, time_layer_update);
	layer_add_child(root, time_layer);

	deviceOrientation = UIInterfaceOrientationPortrait;
	/* Peek only works after subscribe. Read once so the first frame matches
	   how the watch is being held. */
	accel_data_service_subscribe(0, NULL);
	update_orientation_from_accel();
	display_time(NULL);

	tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit(void) {
	accel_data_service_unsubscribe();
	tick_timer_service_unsubscribe();
	fonts_unload_custom_font(time_font);
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
