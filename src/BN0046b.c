#include "pebble.h"

static Window *window;
static Layer *window_layer;

static AppSync sync;
static uint8_t sync_buffer[80];

#define TOTAL_IMAGE_SLOTS 4
#define NUMBER_OF_IMAGES 10

// Configuration
static int show_date = 1;
static int show_seconds = 1;
static int show_moon = 0;
static int date_format = 1;

enum {
  DATE_KEY = 0x0,
  SECONDS_KEY = 0x1,
  MOON_KEY = 0x2,
  DATEFORMAT_KEY = 0x3
};


#define BGCOLOR GColorBlack
#define FGCOLOR GColorWhite
const int IMAGE_RESOURCE_IDS[NUMBER_OF_IMAGES] = {
  RESOURCE_ID_IMAGE_NUM_0, RESOURCE_ID_IMAGE_NUM_1, RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3, RESOURCE_ID_IMAGE_NUM_4, RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6, RESOURCE_ID_IMAGE_NUM_7, RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};
#define COLON RESOURCE_ID_IMAGE_COLON

static GBitmap *images[TOTAL_IMAGE_SLOTS];
static BitmapLayer *image_layers[TOTAL_IMAGE_SLOTS];

GBitmap *cursor_bitmap;
BitmapLayer *cursor_layer;

static TextLayer *month_layer;   // Month Layer
static TextLayer *day_layer;    // Date Layer
static TextLayer *ampm_layer;    // AM/PM Layer
static TextLayer *seconds_layer; // Seconds Layer
// static TextLayer *moon;    // Moon Layer

#define EMPTY_SLOT -1

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);


static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Callback: [key=%lu][value=%u]", key, new_tuple->value->uint8);
  switch (key) {
    case DATE_KEY:
      show_date = new_tuple->value->uint8;
      if(show_date) {
        layer_set_hidden((Layer *) day_layer, false);
        layer_set_hidden((Layer *) month_layer, false);
      }
      else {
        layer_set_hidden((Layer *) day_layer, true);
        layer_set_hidden((Layer *) month_layer, true);
      }
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "show_date:%u", show_date);
      break;
    case SECONDS_KEY:
      show_seconds = new_tuple->value->uint8;
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "show_seconds:%u", show_seconds);
      tick_timer_service_unsubscribe();
      if(show_seconds) {
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
        layer_set_hidden((Layer *) seconds_layer, false);
      }
      else {
        tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
        layer_set_hidden((Layer *) seconds_layer, true);
      }
      break;
    case DATEFORMAT_KEY:
      date_format = new_tuple->value->uint8;
      time_t now = time(NULL);
      struct tm *tick_time = localtime(&now);
      handle_tick(tick_time, DAY_UNIT);
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "date_format:%u", date_format);
      break;
    case MOON_KEY:
      show_moon = new_tuple->value->uint8;
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "show_moon:%u", show_moon);
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
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "show_seconds:%u", show_seconds);
  static char seconds_text[] = "00";
  strftime(seconds_text, sizeof(seconds_text), "%S", tick_time);
  text_layer_set_text(seconds_layer, seconds_text);

}

static void update_display_minutes(struct tm *tick_time) {
  set_container_image(&images[2], image_layers[2], IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(72, 54));
  set_container_image(&images[3], image_layers[3], IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(103, 54));
}

static void update_display_hours(struct tm *tick_time) {
  unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  if(display_hour >= 10) {
    layer_set_hidden((Layer *) image_layers[0], false);
    set_container_image(&images[0], image_layers[0], IMAGE_RESOURCE_IDS[display_hour/10], GPoint(2, 54));
  } else {
    layer_set_hidden((Layer *) image_layers[0], true);
  }
  set_container_image(&images[1], image_layers[1], IMAGE_RESOURCE_IDS[display_hour%10], GPoint(34, 54));

  static char am_pm_text[] = "PM";

  // AM/PM
  strftime(am_pm_text, sizeof(am_pm_text), "%p", tick_time);

  if (!clock_is_24h_style()) {
    text_layer_set_text(ampm_layer, am_pm_text);
  }
}

void update_display_day(struct tm *tick_time) {

  static char date_text[] = "00.00";

  if (date_format == 3) {
    strftime(date_text, sizeof(date_text), "%m-%d", tick_time);
  } else if (date_format == 2) {
    strftime(date_text, sizeof(date_text), "%d-%m", tick_time);
  } else {
    strftime(date_text, sizeof(date_text), "%e", tick_time);
  }

    text_layer_set_text(day_layer, date_text);
}

void update_display_month(struct tm *tick_time) {
  // Month or Weekday
  static char month_text[] = "AAA";

  if (date_format == 2 || date_format == 3) {         // Show Weekday (3 letter abbrev)
      strftime(month_text, sizeof(month_text), "%a", tick_time);
  } else { // Show Month (3 letter abbrev)
      strftime(month_text, sizeof(month_text), "%b", tick_time);
  }

  text_layer_set_text(month_layer, month_text);
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {

  if ((units_changed & SECOND_UNIT) != 0 && show_seconds) {
    update_display_seconds(tick_time);
  }

 if ((units_changed & MINUTE_UNIT) != 0) {
    update_display_minutes(tick_time);
  }

  if ((units_changed & HOUR_UNIT) != 0) {
    update_display_hours(tick_time);
  }

  if ((units_changed & DAY_UNIT) != 0) {
    update_display_day(tick_time);
    update_display_month(tick_time);
    // #if SHOW_MOON
    //   update_display_moon(tick_time);
    // #endif
  }

}

static void init(void) {

  const int inbound_size = 80;
  const int outbound_size = 80;
  app_message_open(inbound_size, outbound_size);

  window = window_create();

  window_set_background_color(window, BGCOLOR);

  window_layer = window_get_root_layer(window);

  memset(&image_layers, 0, sizeof(image_layers));
  memset(&images, 0, sizeof(images));

  GRect dummy_frame = { {0, 0}, {0, 0} };
  for (int i = 0; i < TOTAL_IMAGE_SLOTS; ++i) {
    image_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(image_layers[i]));
  }

  // Load Fonts
  // Digital 21pt
  ResHandle font_date = resource_get_handle(RESOURCE_ID_DIGITAL21);
  // Digital 45
  ResHandle font_seconds = resource_get_handle(RESOURCE_ID_DIGITAL45);
  // Climaicons - Moon
  // ResHandle custom_moonfont30 = resource_get_handle(RESOURCE_ID_CLIMAICONS30);

  seconds_layer = text_layer_create(GRect(90, 90, 60, 60));
  text_layer_set_background_color(seconds_layer, GColorClear);
  text_layer_set_text_color(seconds_layer, FGCOLOR);
  text_layer_set_font(seconds_layer, fonts_load_custom_font(font_seconds));
  layer_add_child(window_layer, text_layer_get_layer(seconds_layer));

  day_layer = text_layer_create(GRect(45, 25, 60, 30));
  text_layer_set_background_color(day_layer, GColorClear);
  text_layer_set_text_color(day_layer, FGCOLOR);
  text_layer_set_font(day_layer, fonts_load_custom_font(font_date));
  layer_add_child(window_layer, text_layer_get_layer(day_layer));

  month_layer = text_layer_create(GRect(5, 25, 60, 30));
  text_layer_set_background_color(month_layer, GColorClear);
  text_layer_set_text_color(month_layer, FGCOLOR);
  text_layer_set_font(month_layer, fonts_load_custom_font(font_date));
  layer_add_child(window_layer, text_layer_get_layer(month_layer));

  ampm_layer = text_layer_create(GRect(5, 102, 30, 30));
  text_layer_set_background_color(ampm_layer, GColorClear);
  text_layer_set_text_color(ampm_layer, FGCOLOR);
  text_layer_set_font(ampm_layer, fonts_load_custom_font(font_date));
  layer_add_child(window_layer, text_layer_get_layer(ampm_layer));

  cursor_bitmap = gbitmap_create_with_resource(COLON);
  cursor_layer = bitmap_layer_create(GRect(65, 64, 6, 46));
  bitmap_layer_set_bitmap(cursor_layer, cursor_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(cursor_layer));

  Tuplet initial_values[] = {
    TupletInteger(DATE_KEY, 1),
    TupletInteger(SECONDS_KEY, 1),
    TupletInteger(MOON_KEY, 0),
    TupletInteger(DATEFORMAT_KEY, 1)
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, sync_error_callback, NULL);

  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  handle_tick(tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  //Subscribe to events
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

  //Finally
  window_stack_push(window, true);
}

static void deinit() {

  app_sync_deinit(&sync);

  for (int i = 0; i < TOTAL_IMAGE_SLOTS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(image_layers[i]));
    gbitmap_destroy(images[i]);
    bitmap_layer_destroy(image_layers[i]);
  }
  gbitmap_destroy(cursor_bitmap);
  bitmap_layer_destroy(cursor_layer);

  text_layer_destroy(seconds_layer);
  text_layer_destroy(day_layer);
  text_layer_destroy(month_layer);
  text_layer_destroy(ampm_layer);

  tick_timer_service_unsubscribe();

  window_destroy(window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
