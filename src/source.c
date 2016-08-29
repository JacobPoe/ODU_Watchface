#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;

static GBitmap *s_background_bitmap;
static BitmapLayer *s_background_layer;

static int s_battery_level;
static Layer *s_battery_layer;

static BitmapLayer *s_bt_background_layer, *s_bt_icon_layer;
static GBitmap *s_bt_background_bitmap, *s_bt_icon_bitmap;

/* ------------ Time Functions ------------ */

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), 
					 clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  // Display time
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)	{	
	//Update time
	update_time();
}

/* ------------ Battery Functions ------------ */

static void battery_callback(BatteryChargeState state)	{
	//Record battery level
	s_battery_level = state.charge_percent;
	
	//Update meter
	layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx)	{
	GRect bounds = layer_get_bounds(layer);
	
	//Find width
	int width = (int)(float)(((float)s_battery_level / 100.0F) * 37.0F);
	
	//Draw background
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, GCornersAll);
	
	//Draw Bar
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

/* ------------ Bluetooth Functions ------------ */

static void bluetooth_callback(bool connected)	{
	//Show icon if DC'd
	layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
	
	if(!connected)	{
		//Vibrate
		vibes_double_pulse();
	}
}

/* ------------ Window Functions ------------ */

static void main_window_load(Window *window) {
	
	//Get Window info
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	//Create TextLayer
	s_time_layer = text_layer_create(
		GRect(0, PBL_IF_ROUND_ELSE(58,130), bounds.size.w, 50));
		
	//Optimize layout
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorBlack);
	text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	//Create layer as child to Window
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
		
	//Create GBitmap Layer
	s_background_layer = bitmap_layer_create(
		GRect(0, PBL_IF_ROUND_ELSE(58,-5), bounds.size.w, 150));
	
	//Create GBitmap
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ODU_Flame2);
	
	//Display GBitmap image
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
	bitmap_layer_set_alignment(s_background_layer, GAlignCenter);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
	
	//Create battery meter layer
	s_battery_layer = layer_create(GRect(52,160,37,4));//X, Y, Width, Height
	layer_set_update_proc(s_battery_layer, battery_update_proc);
	layer_add_child(window_get_root_layer(window), s_battery_layer);
	
	//Create BlueTooth Icon Gbitmap
	s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_Icon);
	
	//Create BlueTooth Icon BitmapLayer
	s_bt_icon_layer = bitmap_layer_create(GRect(114, 137, 20, 20));
	bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
	
	//Display correct state of BT connection from Go
	bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {	
	//Destroy TextLayer
	text_layer_destroy(s_time_layer);
	
	//Destroy GBitmap
	gbitmap_destroy(s_background_bitmap);
	bitmap_layer_destroy(s_background_layer);
	
	//Destroy Battery
	layer_destroy(s_battery_layer);
	
	//Destroy BlueTooth
	gbitmap_destroy(s_bt_icon_bitmap);
	bitmap_layer_destroy(s_bt_icon_layer);
}

/* ------------ Init / Deinit ------------ */

static void init()	{
	//Create window & assign to pointer
	s_main_window = window_create();
	
	//Set handler
	window_set_window_handlers(s_main_window, (WindowHandlers)	{
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	//Set window background color
	window_set_background_color(s_main_window, GColorChromeYellow);
		
	//Display the window
	window_stack_push(s_main_window, true);
	
	//Display time from Go
	update_time();
	
	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	// Register for battery level updates
	battery_state_service_subscribe(battery_callback);
	
	//Display battery from Go
	battery_callback(battery_state_service_peek());
	
	//Register for BT Connection service
	connection_service_subscribe((ConnectionHandlers)	{
		.pebble_app_connection_handler = bluetooth_callback
	});
}

static void deinit()	{
	//Destroy window
	window_destroy(s_main_window);
}

/* ------------ Main Method ------------ */

int main(void)	{
	init();
	app_event_loop();
	deinit();
}