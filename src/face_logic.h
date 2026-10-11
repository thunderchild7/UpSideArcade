#ifndef FACE_LOGIC_H
#define FACE_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

/* Original Pebble screen and the 2x2 digit grid. */
#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
#define TIME_IMAGE_WIDTH 72
#define TIME_IMAGE_HEIGHT 76
#define MARGIN 1
#define TIME_SLOT_SPACE 2

#define FACE_SLOT_COUNT 4

/* Written into a slot when that digit is hidden. */
#define FACE_DIGIT_BLANK (-1)
/* Left in a slot the caller owns and this call does not touch. */
#define FACE_DIGIT_UNCHANGED (-2)

enum {
	UIInterfaceOrientationPortrait = 0,
	UIInterfaceOrientationPortraitUpsideDown = 1,
	UIInterfaceOrientationLandscapeRight = 2,
	UIInterfaceOrientationLandscapeLeft = 3
};

typedef struct {
	int16_t x;
	int16_t y;
	int16_t w;
	int16_t h;
} FaceRect;

FaceRect face_frame_for_slot(int slot);

/* 12-hour mode maps 0 to 12. 24-hour mode returns the hour unchanged. */
unsigned short face_display_hour(unsigned short hour, bool is_24h);

int face_hour_row(int orientation);
int face_minute_row(int orientation);

/*
 * Place one 0-99 value on a row.
 * Portrait writes tens on the left. Upside down writes tens on the right
 * so 180-degree glyphs still read left to right. Any other orientation
 * leaves slots untouched. Values above 99 wrap with modulo 100.
 * slots[FACE_SLOT_COUNT] entries that this call does not own stay as they are.
 */
void face_place_value(int slots[FACE_SLOT_COUNT], unsigned short value,
		unsigned short row_number, int orientation,
		bool show_first_leading_zero);

/* Hour row hides a leading zero. Minute row always shows two digits. */
void face_compose_time(int slots[FACE_SLOT_COUNT], unsigned short hour,
		unsigned short minute, int orientation, bool is_24h);

/*
 * x points to the right of the watch, y toward the top, z out of the screen.
 * Positive y means the top of the watch points up. Face-up, face-down, and
 * sideways samples keep the current portrait or upside-down orientation.
 * Any other current value is treated as portrait.
 */
int face_orientation_from_accel(int x, int y, int z, int current_orientation);

/* Quarter-turns clockwise for the digit artwork. 0 is upright, 2 is a half turn. */
int face_rotation_quarters(int orientation);

/* Redraw on a new minute or when the watch flips. Seconds alone do not. */
bool face_should_refresh(int previous_orientation, int orientation, bool minute_changed);

/*
 * Short buzz on clock minutes that are a multiple of vibe_minutes.
 * An interval of 5 buzzes at :00, :05, :10, and so on. Zero and negative
 * intervals stay quiet. Minutes outside 0..59 stay quiet.
 */
bool face_should_vibe(int minute_of_hour, int vibe_minutes);

/* Reload a slot only when its digit changes. A flip reuses the bitmap and rotates it. */
bool face_slot_needs_redraw(int loaded_digit, int wanted_digit);

#endif
