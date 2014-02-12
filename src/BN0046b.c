#include "pebble.h"

static Window *window;

static AppSync sync;
static uint8_t sync_buffer[64];


#define TOTAL_IMAGE_SLOTS 4

#define NUMBER_OF_IMAGES 10

enum {
  KEY_SHOW_SECONDS = 0x0,
  KEY_SHOW_DATE = 0x1,
  KEY_WEEKDAY_US_MM_DD = 0x2,
  KEY_WEEKDAY_NON_US_DD_MM = 0x3,
  KEY_SHOW_MOON = 0x4,
  KEY_INVERSE = 0x5
};

// Configuration
static int SHOW_SECONDS;
static int SHOW_DATE;
static int WEEKDAY_US_MM_DD;
static int WEEKDAY_NON_US_DD_MM;
static int SHOW_MOON;
static int INVERSE;

#if (INVERSE == 1)
  #define BGCOLOR GColorWhite
  #define FGCOLOR GColorBlack
  const int IMAGE_RESOURCE_IDS[NUMBER_OF_IMAGES] = {
    RESOURCE_ID_IMAGE_NUM_0i, RESOURCE_ID_IMAGE_NUM_1i, RESOURCE_ID_IMAGE_NUM_2i,
    RESOURCE_ID_IMAGE_NUM_3i, RESOURCE_ID_IMAGE_NUM_4i, RESOURCE_ID_IMAGE_NUM_5i,
    RESOURCE_ID_IMAGE_NUM_6i, RESOURCE_ID_IMAGE_NUM_7i, RESOURCE_ID_IMAGE_NUM_8i,
    RESOURCE_ID_IMAGE_NUM_9i
  };
  #define COLON RESOURCE_ID_IMAGE_COLONi
#else
  #define BGCOLOR GColorBlack
  #define FGCOLOR GColorWhite
  const int IMAGE_RESOURCE_IDS[NUMBER_OF_IMAGES] = {
    RESOURCE_ID_IMAGE_NUM_0, RESOURCE_ID_IMAGE_NUM_1, RESOURCE_ID_IMAGE_NUM_2,
    RESOURCE_ID_IMAGE_NUM_3, RESOURCE_ID_IMAGE_NUM_4, RESOURCE_ID_IMAGE_NUM_5,
    RESOURCE_ID_IMAGE_NUM_6, RESOURCE_ID_IMAGE_NUM_7, RESOURCE_ID_IMAGE_NUM_8,
    RESOURCE_ID_IMAGE_NUM_9
  };
  #define COLON RESOURCE_ID_IMAGE_COLON
#endif

static GBitmap *images[TOTAL_IMAGE_SLOTS];
static BitmapLayer *image_layers[TOTAL_IMAGE_SLOTS];

GBitmap *cursor_bitmap;
BitmapLayer *cursor_layer;

static TextLayer *month;   // Month Layer
static TextLayer *day;    // Date Layer
// static TextLayer *ampm;    // AM/PM Layer
static TextLayer *seconds; // Seconds Layer
// static TextLayer *moon;    // Moon Layer

#define EMPTY_SLOT -1

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case KEY_SHOW_SECONDS:
      SHOW_SECONDS = new_tuple->value->uint8;
      break;
    case KEY_SHOW_DATE:
      SHOW_DATE = new_tuple->value->uint8;
      break;
    case KEY_WEEKDAY_US_MM_DD:
      WEEKDAY_US_MM_DD = new_tuple->value->uint8;
      break;
    case KEY_WEEKDAY_NON_US_DD_MM:
      WEEKDAY_NON_US_DD_MM = new_tuple->value->uint8;
      break;
    case KEY_SHOW_MOON:
      SHOW_MOON = new_tuple->value->uint8;
      break;
    case KEY_INVERSE:
      INVERSE = new_tuple->value->uint8;
      break;
  }
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;

  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

  if (old_image != NULL) {
    gbitmap_destroy(old_image);
  }
}

static unsigned short get_display_hour(unsigned short hour) {

  if (clock_is_24h_style()) {
    return hour;
  }

  unsigned short display_hour = hour % 12;

  // Converts "0" to "12"
  return display_hour ? display_hour : 12;

}

static void update_display_seconds(struct tm *tick_time) {
  static char seconds_text[] = "00";
  strftime(seconds_text, sizeof(seconds_text), "%S", tick_time);
  text_layer_set_text(seconds, seconds_text);
}

static void update_display_minutes(struct tm *tick_time) {
  set_container_image(&images[2], image_layers[2], IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(72, 54));
  set_container_image(&images[3], image_layers[3], IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(103, 54));
}

static void update_display_hours(struct tm *tick_time) {
  unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&images[0], image_layers[0], IMAGE_RESOURCE_IDS[display_hour/10], GPoint(2, 54));
  set_container_image(&images[1], image_layers[1], IMAGE_RESOURCE_IDS[display_hour%10], GPoint(34, 54));
}

void update_display_day(struct tm *tick_time) {
  // Day
  #if (WEEKDAY_US_MM_DD || WEEKDAY_NON_US_DD_MM)
      static char date_text[] = "00.00";
  #else
      static char date_text[] = "00";
  #endif

  #if   WEEKDAY_US_MM_DD           // Show Date in "MM-DD" (US date format)
      strftime(date_text, sizeof(date_text), "%m-%d", tick_time);
  #elif WEEKDAY_NON_US_DD_MM       // Show Date in "DD-MM" (INTL_DD_MM)
      strftime(date_text, sizeof(date_text), "%d-%m", tick_time);
  #else                            // Show Day of Month (no Leading 0)
      strftime(date_text, sizeof(date_text), "%e", tick_time);
  #endif

    text_layer_set_text(day, date_text);
}

void update_display_month(struct tm *tick_time) {
  // Month or Weekday
  static char month_text[] = "AAA";

  #if (WEEKDAY_US_MM_DD || WEEKDAY_NON_US_DD_MM) // Show Weekday (3 letter abbrev)
      strftime(month_text, sizeof(month_text), "%a", tick_time);
  #else // Show Month (3 letter abbrev)
      strftime(month_text, sizeof(month_text), "%b", tick_time);
  #endif

  text_layer_set_text(month, month_text);
}

static void layer_setup(struct tm *tick_time) {

  #if SHOW_DATE
    update_display_day(tick_time);
    update_display_month(tick_time);
  #endif
  update_display_hours(tick_time);
  update_display_minutes(tick_time);
  #if SHOW_SECONDS
    update_display_seconds(tick_time);
  #endif

}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {

  if ((units_changed & SECOND_UNIT) != 0 && SHOW_SECONDS) {
    update_display_seconds(tick_time);
  }

 if ((units_changed & MINUTE_UNIT) != 0) {
    update_display_minutes(tick_time);
  }

  if ((units_changed & HOUR_UNIT) != 0) {
    update_display_hours(tick_time);
  }

  if ((units_changed & DAY_UNIT) != 0) {
    #if SHOW_DATE
      update_display_day(tick_time);
      update_display_month(tick_time);
    #endif
    #if SHOW_MOON
      // update_display_moon(tick_time);
    #endif
  }

  if ((units_changed & MONTH_UNIT) != 0 && SHOW_DATE) {
    update_display_month(tick_time);
  }

}

static void window_load(Window *window) {

  window_set_background_color(window, BGCOLOR);

  Layer *parent = window_get_root_layer(window);

  memset(&image_layers, 0, sizeof(image_layers));
  memset(&images, 0, sizeof(images));

  GRect dummy_frame = { {0, 0}, {0, 0} };
  for (int i = 0; i < TOTAL_IMAGE_SLOTS; ++i) {
    image_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(parent, bitmap_layer_get_layer(image_layers[i]));
  }

  // Load Fonts
  // Digital 21pt
  ResHandle font_date = resource_get_handle(RESOURCE_ID_DIGITAL21);
  // Digital 45
  ResHandle font_seconds = resource_get_handle(RESOURCE_ID_DIGITAL45);
  // Climaicons - Moon
  // ResHandle custom_moonfont30 = resource_get_handle(RESOURCE_ID_CLIMAICONS30);

  seconds = text_layer_create(GRect(90, 92, 60, 60));
  text_layer_set_background_color(seconds, GColorClear);
  text_layer_set_text_color(seconds, FGCOLOR);
  text_layer_set_font(seconds, fonts_load_custom_font(font_seconds));
  layer_add_child(parent, text_layer_get_layer(seconds));

  day = text_layer_create(GRect(45, 25, 60, 30));
  text_layer_set_background_color(day, GColorClear);
  text_layer_set_text_color(day, FGCOLOR);
  text_layer_set_font(day, fonts_load_custom_font(font_date));
  layer_add_child(parent, text_layer_get_layer(day));

  month = text_layer_create(GRect(0, 25, 60, 30));
  text_layer_set_background_color(month, GColorClear);
  text_layer_set_text_color(month, FGCOLOR);
  text_layer_set_font(month, fonts_load_custom_font(font_date));
  layer_add_child(parent, text_layer_get_layer(month));

  //Load bitmaps into GBitmap structures
  //The ID you chose when uploading is prefixed with 'RESOURCE_ID_'

  cursor_bitmap = gbitmap_create_with_resource(COLON);
  cursor_layer = bitmap_layer_create(GRect(64, 64, 6, 46));
  bitmap_layer_set_bitmap(cursor_layer, cursor_bitmap);
  layer_add_child(parent, bitmap_layer_get_layer(cursor_layer));

  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  layer_setup(tick_time);
}

static void window_unload(Window *window) {
  for (int i = 0; i < TOTAL_IMAGE_SLOTS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(image_layers[i]));
    gbitmap_destroy(images[i]);
    bitmap_layer_destroy(image_layers[i]);
  }
  gbitmap_destroy(cursor_bitmap);
  bitmap_layer_destroy(cursor_layer);

  text_layer_destroy(seconds);
  text_layer_destroy(day);
  text_layer_destroy(month);

  tick_timer_service_unsubscribe();
}

static void init(void) {

  window = window_create();
  WindowHandlers handlers = {
    .load = window_load,
    .unload = window_unload
  };
  window_set_window_handlers(window, (WindowHandlers) handlers);

  Tuplet initial_values[] = {
    TupletInteger(KEY_SHOW_SECONDS, 1),
    TupletInteger(KEY_SHOW_DATE, 1),
    TupletInteger(KEY_WEEKDAY_US_MM_DD, 0),
    TupletInteger(KEY_WEEKDAY_NON_US_DD_MM, 0),
    TupletInteger(KEY_SHOW_MOON, 0)
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, NULL, NULL);

  //Subscribe to events
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

  //Finally
  window_stack_push(window, true);
}

static void deinit() {

  window_destroy(window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
