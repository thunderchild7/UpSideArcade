#include "face_logic.h"

#include <stddef.h>

FaceRect face_frame_for_slot(int slot) {
	FaceRect frame;
	frame.x = (int16_t)((slot % 2) * TIME_IMAGE_WIDTH);
	frame.y = (int16_t)(MARGIN + (slot / 2) * (TIME_IMAGE_HEIGHT + TIME_SLOT_SPACE));
	frame.w = TIME_IMAGE_WIDTH;
	frame.h = TIME_IMAGE_HEIGHT;
	return frame;
}

unsigned short face_display_hour(unsigned short hour, bool is_24h) {
	unsigned short display_hour;

	if (is_24h) {
		return hour;
	}

	display_hour = hour % 12;
	return display_hour ? display_hour : 12;
}

int face_hour_row(int orientation) {
	return (orientation == UIInterfaceOrientationPortrait) ? 0 : 1;
}

int face_minute_row(int orientation) {
	return (orientation == UIInterfaceOrientationPortrait) ? 1 : 0;
}

void face_place_value(int slots[FACE_SLOT_COUNT], unsigned short value,
		unsigned short row_number, int orientation,
		bool show_first_leading_zero) {
	int column_number;
	int slot_number;

	if (row_number > 1) {
		return;
	}

	value = value % 100;

	if (orientation == UIInterfaceOrientationPortrait) {
		for (column_number = 1; column_number >= 0; column_number--) {
			slot_number = (row_number * 2) + column_number;
			slots[slot_number] = FACE_DIGIT_BLANK;
			if (!((value == 0) && (column_number == 0) && !show_first_leading_zero)) {
				slots[slot_number] = value % 10;
			}
			value = value / 10;
		}
	} else if (orientation == UIInterfaceOrientationPortraitUpsideDown) {
		for (column_number = 0; column_number <= 1; column_number++) {
			slot_number = (row_number * 2) + column_number;
			slots[slot_number] = FACE_DIGIT_BLANK;
			if (!((value == 0) && (column_number == 1) && !show_first_leading_zero)) {
				slots[slot_number] = value % 10;
			}
			value = value / 10;
		}
	}
}

void face_compose_time(int slots[FACE_SLOT_COUNT], unsigned short hour,
		unsigned short minute, int orientation, bool is_24h) {
	face_place_value(slots, face_display_hour(hour, is_24h),
			(unsigned short)face_hour_row(orientation), orientation, false);
	face_place_value(slots, minute,
			(unsigned short)face_minute_row(orientation), orientation, true);
}

/* Below this, x and y are noise and must not flip the face. */
#define FACE_ACCEL_MIN_MG 200

static int absolute_value(int value) {
	return value < 0 ? -value : value;
}

static int held_orientation(int current) {
	if (current == UIInterfaceOrientationPortraitUpsideDown) {
		return UIInterfaceOrientationPortraitUpsideDown;
	}
	return UIInterfaceOrientationPortrait;
}

int face_orientation_from_accel(int x, int y, int z, int current_orientation) {
	int ax = absolute_value(x);
	int ay = absolute_value(y);
	int az = absolute_value(z);
	int in_plane = ax > ay ? ax : ay;
	int held = held_orientation(current_orientation);

	if (in_plane < FACE_ACCEL_MIN_MG || az >= in_plane) {
		return held;
	}
	/* y * 4 >= |x| is within about 76 degrees of straight up or straight down.
	   The gap around either side keeps the previous orientation. */
	if (y > 0 && y * 4 >= ax) {
		return UIInterfaceOrientationPortrait;
	}
	if (y < 0 && (-y) * 4 >= ax) {
		return UIInterfaceOrientationPortraitUpsideDown;
	}
	return held;
}

int face_rotation_quarters(int orientation) {
	if (orientation == UIInterfaceOrientationPortraitUpsideDown) {
		return 2;
	}
	return 0;
}

bool face_should_refresh(int previous_orientation, int orientation, bool minute_changed) {
	return minute_changed || previous_orientation != orientation;
}

bool face_should_vibe(int minute_of_hour, int vibe_minutes) {
	if (vibe_minutes <= 0) {
		return false;
	}
	if (minute_of_hour < 0 || minute_of_hour > 59) {
		return false;
	}
	return (minute_of_hour % vibe_minutes) == 0;
}

bool face_slot_needs_redraw(int loaded_digit, int wanted_digit) {
	if (wanted_digit == FACE_DIGIT_UNCHANGED) {
		return false;
	}
	return loaded_digit != wanted_digit;
}

FaceRect face_time_line_frame(int screen_w, int screen_h, int line_height, int line_index) {
	FaceRect frame = {0, 0, 0, 0};
	int block_h;
	int top;

	if (screen_w <= 0 || screen_h <= 0 || line_height <= 0) {
		return frame;
	}
	if (line_index < 0 || line_index > 1) {
		return frame;
	}

	block_h = line_height * 2 + FACE_TIME_LINE_GAP;
	top = (screen_h - block_h) / 2;
	if (top < 0) {
		top = 0;
	}

	frame.x = 0;
	frame.y = (int16_t)(top + line_index * (line_height + FACE_TIME_LINE_GAP));
	frame.w = (int16_t)screen_w;
	frame.h = (int16_t)line_height;
	return frame;
}

FaceRect face_time_draw_box(FaceRect ink, int pad_left, int pad_top) {
	if (ink.w <= 0 || ink.h <= 0) {
		return ink;
	}

	ink.x = (int16_t)(ink.x - pad_left);
	ink.y = (int16_t)(ink.y - pad_top);
	if (pad_top > 0) {
		ink.h = (int16_t)(ink.h + pad_top);
	}
	return ink;
}

static int face_pixel_get(const uint8_t *pixels, int bytes_per_row, int bits_per_pixel,
		int x, int y) {
	if (bits_per_pixel == 8) {
		return pixels[y * bytes_per_row + x];
	}

	return (pixels[y * bytes_per_row + (x / 8)] >> (7 - (x % 8))) & 1;
}

static void face_pixel_set(uint8_t *pixels, int bytes_per_row, int bits_per_pixel,
		int x, int y, int value) {
	int index;
	int shift;

	if (bits_per_pixel == 8) {
		pixels[y * bytes_per_row + x] = (uint8_t)value;
		return;
	}

	index = y * bytes_per_row + (x / 8);
	shift = 7 - (x % 8);
	if (value) {
		pixels[index] = (uint8_t)(pixels[index] | (1 << shift));
	} else {
		pixels[index] = (uint8_t)(pixels[index] & ~(1 << shift));
	}
}

void face_rotate_pixels_180(uint8_t *pixels, int width, int height,
		int bytes_per_row, int bits_per_pixel) {
	int y;

	if (pixels == NULL || width <= 0 || height <= 0 || bytes_per_row <= 0) {
		return;
	}
	if (bits_per_pixel != 1 && bits_per_pixel != 8) {
		return;
	}

	for (y = 0; y < height; y++) {
		int y2 = height - 1 - y;
		int x;

		if (y > y2) {
			break;
		}
		for (x = 0; x < width; x++) {
			int x2 = width - 1 - x;
			int first;
			int second;

			if (y == y2 && x >= x2) {
				break;
			}
			first = face_pixel_get(pixels, bytes_per_row, bits_per_pixel, x, y);
			second = face_pixel_get(pixels, bytes_per_row, bits_per_pixel, x2, y2);
			face_pixel_set(pixels, bytes_per_row, bits_per_pixel, x, y, second);
			face_pixel_set(pixels, bytes_per_row, bits_per_pixel, x2, y2, first);
		}
	}
}
