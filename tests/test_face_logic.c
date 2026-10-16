#include "face_logic.h"

#include <stdio.h>
#include <stdlib.h>

static int tests_run = 0;
static int failures = 0;

static void expect_int(const char *name, int got, int want) {
	tests_run++;
	if (got != want) {
		fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
		failures++;
	}
}

static void expect_slots(const char *name, const int *got, const int *want) {
	int i;
	int mismatch = 0;

	tests_run++;
	for (i = 0; i < FACE_SLOT_COUNT; i++) {
		if (got[i] != want[i]) {
			mismatch = 1;
		}
	}
	if (mismatch) {
		fprintf(stderr, "FAIL %s: got [%d, %d, %d, %d] want [%d, %d, %d, %d]\n",
				name,
				got[0], got[1], got[2], got[3],
				want[0], want[1], want[2], want[3]);
		failures++;
	}
}

static void fill_slots(int slots[FACE_SLOT_COUNT], int value) {
	int i;
	for (i = 0; i < FACE_SLOT_COUNT; i++) {
		slots[i] = value;
	}
}

static void test_frames(void) {
	FaceRect slot0 = face_frame_for_slot(0);
	FaceRect slot1 = face_frame_for_slot(1);
	FaceRect slot2 = face_frame_for_slot(2);
	FaceRect slot3 = face_frame_for_slot(3);

	expect_int("slot0.x", slot0.x, 0);
	expect_int("slot0.y", slot0.y, MARGIN);
	expect_int("slot0.w", slot0.w, TIME_IMAGE_WIDTH);
	expect_int("slot0.h", slot0.h, TIME_IMAGE_HEIGHT);

	expect_int("slot1.x", slot1.x, TIME_IMAGE_WIDTH);
	expect_int("slot1.y", slot1.y, MARGIN);
	expect_int("slot2.x", slot2.x, 0);
	expect_int("slot2.y", slot2.y, MARGIN + TIME_IMAGE_HEIGHT + TIME_SLOT_SPACE);
	expect_int("slot3.x", slot3.x, TIME_IMAGE_WIDTH);
	expect_int("slot3.y", slot3.y, MARGIN + TIME_IMAGE_HEIGHT + TIME_SLOT_SPACE);

	expect_int("columns meet", slot1.x, slot0.x + slot0.w);
	expect_int("columns fill the screen", slot0.w + slot1.w, SCREEN_WIDTH);
	expect_int("row gap", slot2.y, slot0.y + slot0.h + TIME_SLOT_SPACE);
	expect_int("bottom fits the screen", slot3.y + slot3.h < SCREEN_HEIGHT, 1);
	expect_int("right edge", slot3.x + slot3.w, SCREEN_WIDTH);
}

static void test_display_hour(void) {
	static const unsigned short want_12h[24] = {
		12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
		12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	};
	unsigned short hour;

	for (hour = 0; hour < 24; hour++) {
		char name[32];
		snprintf(name, sizeof(name), "24h hour %u", hour);
		expect_int(name, face_display_hour(hour, true), hour);
		snprintf(name, sizeof(name), "12h hour %u", hour);
		expect_int(name, face_display_hour(hour, false), want_12h[hour]);
	}
}

static void test_rows(void) {
	expect_int("portrait hour row", face_hour_row(UIInterfaceOrientationPortrait), 0);
	expect_int("portrait minute row", face_minute_row(UIInterfaceOrientationPortrait), 1);
	expect_int("upside down hour row",
			face_hour_row(UIInterfaceOrientationPortraitUpsideDown), 1);
	expect_int("upside down minute row",
			face_minute_row(UIInterfaceOrientationPortraitUpsideDown), 0);
	expect_int("landscape hour row follows the non-portrait path",
			face_hour_row(UIInterfaceOrientationLandscapeLeft), 1);
	expect_int("landscape minute row follows the non-portrait path",
			face_minute_row(UIInterfaceOrientationLandscapeRight), 0);
}

static void test_place_value(void) {
	int slots[FACE_SLOT_COUNT];
	const int portrait_12[] = {1, 2, 42, 42};
	const int portrait_5_blank_tens[] = {FACE_DIGIT_BLANK, 5, 42, 42};
	const int portrait_5_with_zero[] = {0, 5, 42, 42};
	const int portrait_0_blank_tens[] = {FACE_DIGIT_BLANK, 0, 42, 42};
	const int portrait_0_with_zero[] = {0, 0, 42, 42};
	const int portrait_105[] = {FACE_DIGIT_BLANK, 5, 42, 42};
	const int portrait_100[] = {FACE_DIGIT_BLANK, 0, 42, 42};
	const int bottom_30[] = {42, 42, 3, 0};
	const int flipped_12[] = {2, 1, 42, 42};
	const int flipped_5_blank_tens[] = {5, FACE_DIGIT_BLANK, 42, 42};
	const int flipped_0[] = {0, 0, 42, 42};
	const int flipped_bottom_untouched[] = {42, 42, 42, 42};

	fill_slots(slots, 42);
	face_place_value(slots, 12, 0, UIInterfaceOrientationPortrait, false);
	expect_slots("portrait 12", slots, portrait_12);

	fill_slots(slots, 42);
	face_place_value(slots, 5, 0, UIInterfaceOrientationPortrait, false);
	expect_slots("portrait 5 hides the tens digit", slots, portrait_5_blank_tens);

	fill_slots(slots, 42);
	face_place_value(slots, 5, 0, UIInterfaceOrientationPortrait, true);
	expect_slots("portrait 5 keeps the leading zero", slots, portrait_5_with_zero);

	fill_slots(slots, 42);
	face_place_value(slots, 0, 0, UIInterfaceOrientationPortrait, false);
	expect_slots("portrait 0 hides the tens digit", slots, portrait_0_blank_tens);

	fill_slots(slots, 42);
	face_place_value(slots, 0, 0, UIInterfaceOrientationPortrait, true);
	expect_slots("portrait 0 shows both zeros", slots, portrait_0_with_zero);

	fill_slots(slots, 42);
	face_place_value(slots, 105, 0, UIInterfaceOrientationPortrait, false);
	expect_slots("values wrap at 100", slots, portrait_105);

	fill_slots(slots, 42);
	face_place_value(slots, 100, 0, UIInterfaceOrientationPortrait, false);
	expect_slots("100 wraps to 0", slots, portrait_100);

	fill_slots(slots, 42);
	face_place_value(slots, 30, 1, UIInterfaceOrientationPortrait, true);
	expect_slots("portrait row 1", slots, bottom_30);

	fill_slots(slots, 42);
	face_place_value(slots, 12, 0, UIInterfaceOrientationPortraitUpsideDown, false);
	expect_slots("upside down puts the units digit on the left", slots, flipped_12);

	fill_slots(slots, 42);
	face_place_value(slots, 5, 0, UIInterfaceOrientationPortraitUpsideDown, false);
	expect_slots("upside down blanks the physical right tens place", slots, flipped_5_blank_tens);

	fill_slots(slots, 42);
	face_place_value(slots, 0, 0, UIInterfaceOrientationPortraitUpsideDown, true);
	expect_slots("upside down 00", slots, flipped_0);

	fill_slots(slots, 42);
	face_place_value(slots, 12, 0, UIInterfaceOrientationLandscapeLeft, false);
	expect_slots("landscape draws nothing", slots, flipped_bottom_untouched);

	fill_slots(slots, 42);
	face_place_value(slots, 12, 2, UIInterfaceOrientationPortrait, false);
	expect_slots("a row outside the grid is ignored", slots, flipped_bottom_untouched);
}

static void test_compose_time(void) {
	int slots[FACE_SLOT_COUNT];
	const int nine_oh_five_12h[] = {FACE_DIGIT_BLANK, 9, 0, 5};
	const int midnight_24h[] = {FACE_DIGIT_BLANK, 0, 0, 0};
	const int fifteen_thirty[] = {1, 5, 3, 0};
	const int noon[] = {1, 2, 0, 0};
	const int flipped_nine_oh_five[] = {5, 0, 9, FACE_DIGIT_BLANK};
	const int flipped_thirteen_fortyfive[] = {5, 4, 3, 1};
	const int ten_fifteen[] = {1, 0, 1, 5};
	const int untouched[] = {42, 42, 42, 42};

	fill_slots(slots, 42);
	face_compose_time(slots, 9, 5, UIInterfaceOrientationPortrait, false);
	expect_slots("12h 9:05", slots, nine_oh_five_12h);

	fill_slots(slots, 42);
	face_compose_time(slots, 0, 0, UIInterfaceOrientationPortrait, true);
	expect_slots("24h 0:00", slots, midnight_24h);

	fill_slots(slots, 42);
	face_compose_time(slots, 15, 30, UIInterfaceOrientationPortrait, true);
	expect_slots("24h 15:30", slots, fifteen_thirty);

	fill_slots(slots, 42);
	face_compose_time(slots, 0, 0, UIInterfaceOrientationPortrait, false);
	expect_slots("12h midnight is 12:00", slots, noon);

	fill_slots(slots, 42);
	face_compose_time(slots, 12, 0, UIInterfaceOrientationPortrait, false);
	expect_slots("12h noon is 12:00", slots, noon);

	fill_slots(slots, 42);
	face_compose_time(slots, 9, 5, UIInterfaceOrientationPortraitUpsideDown, false);
	expect_slots("upside down 12h 9:05", slots, flipped_nine_oh_five);

	fill_slots(slots, 42);
	face_compose_time(slots, 13, 45, UIInterfaceOrientationPortraitUpsideDown, true);
	expect_slots("upside down 24h 13:45", slots, flipped_thirteen_fortyfive);

	fill_slots(slots, 42);
	face_compose_time(slots, 23, 59, UIInterfaceOrientationPortraitUpsideDown, true);
	{
		const int flipped_2359[] = {9, 5, 3, 2};
		expect_slots("upside down 24h 23:59", slots, flipped_2359);
	}

	fill_slots(slots, FACE_DIGIT_UNCHANGED);
	face_compose_time(slots, 9, 5, UIInterfaceOrientationPortrait, false);
	face_compose_time(slots, 10, 15, UIInterfaceOrientationPortrait, false);
	expect_slots("a later time replaces the earlier digits", slots, ten_fifteen);

	fill_slots(slots, 42);
	face_compose_time(slots, 9, 5, UIInterfaceOrientationLandscapeRight, true);
	expect_slots("landscape leaves the previous digits in place", slots, untouched);
}

static void test_orientation(void) {
	const int portrait = UIInterfaceOrientationPortrait;
	const int upside_down = UIInterfaceOrientationPortraitUpsideDown;

	expect_int("top up selects portrait",
			face_orientation_from_accel(0, 1000, 0, upside_down), portrait);
	expect_int("top down selects upside down",
			face_orientation_from_accel(0, -1000, 0, portrait), upside_down);
	expect_int("tilted upright still selects portrait",
			face_orientation_from_accel(400, 900, 100, upside_down), portrait);
	expect_int("tilted top-down still selects upside down",
			face_orientation_from_accel(-300, -800, 100, portrait), upside_down);

	expect_int("right side up keeps portrait",
			face_orientation_from_accel(1000, 0, 0, portrait), portrait);
	expect_int("right side up keeps upside down",
			face_orientation_from_accel(1000, 40, 0, upside_down), upside_down);
	expect_int("left side up keeps upside down",
			face_orientation_from_accel(-1000, -40, 0, upside_down), upside_down);
	expect_int("about 80 degrees from upright keeps the current side",
			face_orientation_from_accel(940, 174, 0, upside_down), upside_down);
	expect_int("about 110 degrees from upright is upside down",
			face_orientation_from_accel(940, -342, 0, portrait), upside_down);

	expect_int("face up keeps the current orientation",
			face_orientation_from_accel(30, -80, 1000, upside_down), upside_down);
	expect_int("face down keeps the current orientation",
			face_orientation_from_accel(0, 120, -1000, portrait), portrait);
	expect_int("a zero sample keeps the current orientation",
			face_orientation_from_accel(0, 0, 0, upside_down), upside_down);
	expect_int("an unknown orientation falls back to portrait",
			face_orientation_from_accel(1000, 0, 0, UIInterfaceOrientationLandscapeLeft),
			portrait);
}

static void test_redraw_policy(void) {
	const int portrait = UIInterfaceOrientationPortrait;
	const int upside_down = UIInterfaceOrientationPortraitUpsideDown;

	expect_int("a new second does not redraw",
			face_should_refresh(portrait, portrait, false), 0);
	expect_int("a new minute redraws",
			face_should_refresh(portrait, portrait, true), 1);
	expect_int("a flip redraws",
			face_should_refresh(portrait, upside_down, false), 1);

	expect_int("an unchanged digit stays",
			face_slot_needs_redraw(5, 5), 0);
	expect_int("a changed digit reloads",
			face_slot_needs_redraw(5, 6), 1);
	expect_int("a flip keeps the loaded digit",
			face_slot_needs_redraw(5, 5), 0);
	expect_int("a blank slot that stays blank does not reload",
			face_slot_needs_redraw(FACE_DIGIT_BLANK, FACE_DIGIT_BLANK), 0);
	expect_int("a slot this pass does not own is left alone",
			face_slot_needs_redraw(5, FACE_DIGIT_UNCHANGED), 0);

	expect_int("portrait leaves the digit upright",
			face_rotation_quarters(portrait), 0);
	expect_int("upside down is a half turn",
			face_rotation_quarters(upside_down), 2);
	expect_int("landscape leaves the digit upright",
			face_rotation_quarters(UIInterfaceOrientationLandscapeRight), 0);
}

static void test_vibe(void) {
	int minute;

	for (minute = 0; minute < 60; minute++) {
		char name[64];
		int want = (minute % 5) == 0;

		snprintf(name, sizeof name, "five minute burst at minute %d", minute);
		expect_int(name, face_should_vibe(minute, 5), want);
	}
	expect_int("zero minutes disables the burst", face_should_vibe(0, 0), 0);
	expect_int("a negative interval disables the burst", face_should_vibe(10, -5), 0);
	expect_int("every minute includes minute 1", face_should_vibe(1, 1), 1);
	expect_int("quarter hour at minute 45", face_should_vibe(45, 15), 1);
	expect_int("quarter hour skips minute 5", face_should_vibe(5, 15), 0);
	expect_int("a minute past 59 does not buzz", face_should_vibe(60, 5), 0);
	expect_int("a negative minute does not buzz", face_should_vibe(-1, 5), 0);
}

static void expect_frame(const char *name, FaceRect frame, int x, int y, int w, int h) {
	char label[80];

	snprintf(label, sizeof label, "%s x", name);
	expect_int(label, frame.x, x);
	snprintf(label, sizeof label, "%s y", name);
	expect_int(label, frame.y, y);
	snprintf(label, sizeof label, "%s w", name);
	expect_int(label, frame.w, w);
	snprintf(label, sizeof label, "%s h", name);
	expect_int(label, frame.h, h);
}

static void test_centered_time(void) {
	FaceRect hour;
	FaceRect minute;
	FaceRect empty = {0, 0, 0, 0};

	/* 144x168 watches, 42px Karmatic Arcade, 41px digit ink. */
	hour = face_time_line_frame(144, 168, 41, 0);
	minute = face_time_line_frame(144, 168, 41, 1);
	expect_frame("rect hour ink", hour, 0, 41, 144, 41);
	expect_frame("rect minute ink", minute, 0, 86, 144, 41);
	expect_frame("rect hour draw box", face_time_draw_box(hour, 2, 9), -2, 32, 144, 50);
	expect_frame("rect minute draw box", face_time_draw_box(minute, 2, 9), -2, 77, 144, 50);

	/* Pebble Time Round, same 42px font. */
	hour = face_time_line_frame(180, 180, 41, 0);
	minute = face_time_line_frame(180, 180, 41, 1);
	expect_frame("round hour ink", hour, 0, 47, 180, 41);
	expect_frame("round minute ink", minute, 0, 92, 180, 41);

	/* Pebble Time 2 and Pebble Round 2, 60px font, 59px digit ink. */
	hour = face_time_line_frame(200, 228, 59, 0);
	expect_frame("time 2 hour ink", hour, 0, 53, 200, 59);
	expect_frame("time 2 hour draw box", face_time_draw_box(hour, 3, 13), -3, 40, 200, 72);
	expect_frame("round 2 hour ink", face_time_line_frame(260, 260, 59, 0), 0, 69, 260, 59);

	expect_frame("a line past the minutes is empty", face_time_line_frame(144, 168, 41, 2), 0, 0, 0, 0);
	expect_frame("a zero screen is empty", face_time_line_frame(0, 168, 41, 0), 0, 0, 0, 0);
	expect_frame("an empty ink box stays empty", face_time_draw_box(empty, 2, 9), 0, 0, 0, 0);
}

static void test_rotate_180(void) {
	uint8_t color[6] = {1, 2, 3, 4, 5, 6};
	uint8_t bits[8] = {0x80, 0, 0, 0, 0, 0, 0, 0};
	uint8_t single[1] = {7};

	face_rotate_pixels_180(color, 3, 2, 3, 8);
	expect_int("8-bit 180 first pixel", color[0], 6);
	expect_int("8-bit 180 middle pixel", color[1], 5);
	expect_int("8-bit 180 last of first row", color[2], 4);
	expect_int("8-bit 180 first of second row", color[3], 3);
	expect_int("8-bit 180 last pixel", color[5], 1);

	face_rotate_pixels_180(bits, 8, 2, 4, 1);
	expect_int("1-bit source pixel clears", bits[0], 0);
	expect_int("1-bit pixel lands on the opposite corner", bits[4], 0x01);

	face_rotate_pixels_180(single, 1, 1, 1, 8);
	expect_int("a single pixel stays", single[0], 7);
	face_rotate_pixels_180(NULL, 3, 2, 3, 8);
	face_rotate_pixels_180(color, 3, 2, 3, 4);
	expect_int("an unsupported depth is left unchanged", color[0], 6);
}

int main(void) {
	test_frames();
	test_display_hour();
	test_rows();
	test_place_value();
	test_compose_time();
	test_orientation();
	test_redraw_policy();
	test_vibe();
	test_centered_time();
	test_rotate_180();

	if (failures != 0) {
		fprintf(stderr, "%d of %d checks failed\n", failures, tests_run);
		return EXIT_FAILURE;
	}
	printf("%d checks passed\n", tests_run);
	return EXIT_SUCCESS;
}
