#include "pebble.h"

#include "config.h"

static Window *s_main_window;

// This is a scroll layer
//static ScrollLayer *s_scroll_layer;

// We also use a text layer to scroll in the scroll layer
static TextLayer *s_text_layer;

// The scroll layer can other things in it such as an invert layer
// static InverterLayer *s_inverter_layer;

// Lorum ipsum to have something to scroll
static char s_scroll_text[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam quam tellus, fermentu  m quis vulputate quis, vestibulum interdum sapien. Vestibulum lobortis pellentesque pretium. Quisque ultricies purus e  u orci convallis lacinia. Cras a urna mi. Donec convallis ante id dui dapibus nec ullamcorper erat egestas. Aenean a m  auris a sapien commodo lacinia. Sed posuere mi vel risus congue ornare. Curabitur leo nisi, euismod ut pellentesque se  d, suscipit sit amet lorem. Aliquam eget sem vitae sem aliquam ornare. In sem sapien, imperdiet eget pharetra a, lacin  ia ac justo. Suspendisse at ante nec felis facilisis eleifend.";

/** Width of the screen. */
#define SCREEN_W 144
/** Height of the screen minus the statusbar (168px - 16px). */
#define SCREEN_H 152

/** Height of the input box. */
#define INPUT_BOX_HEIGHT 28

/** Height of the keypad. */
#define KEYPAD_HEIGHT ((SCREEN_H) - (INPUT_BOX_HEIGHT))

/** The current position of the cursor. */
static GPoint s_cursor_position =
{SCREEN_W / 2,
 KEYPAD_HEIGHT / 2};

/** The layer with the cursor. Has identical bounds as @ref s_keypad_layer. */
static Layer *s_cursor_layer;

/** A pointer to the button currently focused with the cursor. Used to
 *  not search for that button multiple times per frame, once in every
 *  hook. It is set in @ref draw_keypad_callback and used in other
 *  callbacks and handlers. <b>Is considered valid only if @ref
 *  s_focused_button_index is not equal -1</b>.
 */
static GRect s_focused_button;

/** Index of the button pointed by @ref s_focused_button.
 *  -1 means it wasn't yet set.
 */
static int s_focused_button_index = -1;

int number_of_pixels;

/** Draw the cursor with an outline. */
static void draw_cursor_callback(Layer *layer, GContext *ctx) {
    /* Draw the cursor. */
    graphics_context_set_fill_color(ctx, COLOR_CURSOR);
    graphics_fill_circle(ctx, s_cursor_position, 3);

    /* Draw the cursor outline for better visibility. */
    graphics_context_set_stroke_color(ctx, COLOR_CURSOR_BORDER);
    graphics_draw_circle(ctx, s_cursor_position, 4);
}

PropertyAnimation *s_box_animation;

// prototype so anim_stopped_handler can compile (implementation below)
void animate_quote(int numnber_of_pixels);

void anim_stopped_handler(Animation *animation, bool finished, void *context) {
  // Free the animation
  property_animation_destroy(s_box_animation);

  // Schedule the reverse animation, unless the app is exiting
  if (finished) {
    number_of_pixels = -number_of_pixels;
    animate_quote(number_of_pixels);
  }
}

void animate_quote(int pixels_to_scroll_by) {
  GRect start_frame = GRect(0, (pixels_to_scroll_by < 0? 0 : -pixels_to_scroll_by), SCREEN_W, SCREEN_H);
  GRect finish_frame =  GRect(0, (pixels_to_scroll_by < 0? pixels_to_scroll_by : 0), SCREEN_W, SCREEN_H);
  
  s_box_animation = property_animation_create_layer_frame(text_layer_get_layer(s_text_layer), &start_frame, &finish_frame);
  animation_set_handlers((Animation*)s_box_animation, (AnimationHandlers) {
    .stopped = anim_stopped_handler
  }, NULL);
  animation_set_duration((Animation*)s_box_animation, abs(pixels_to_scroll_by) * 35); // delay is proportional to text size
  animation_set_curve((Animation*)s_box_animation, AnimationCurveLinear);  // setting equal speed animation
  animation_set_delay((Animation*)s_box_animation, 3000); //initial delay of 3 seconds to let user start reading quote

  animation_schedule((Animation*)s_box_animation);
}

// Setup the scroll layer on window load
// We do this here in order to be able to get the max used text size
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  layer_set_clips((Layer *)window_layer, false);
  //window_set_fullscreen(s_main_window, 1);
  
  //GRect bounds = layer_get_frame(window_layer);
  //GRect max_text_bounds = GRect(0, 0, bounds.size.w, 1000);
  //GRect max_text_bounds = GRect(0, 0, 144, 2000);

  // Initialize the scroll layer
  // s_scroll_layer = scroll_layer_create(bounds);

  // This binds the scroll layer to the window so that up and down map to scrolling
  // You may use scroll_layer_set_callbacks to add or override interactivity
  // scroll_layer_set_click_config_onto_window(s_scroll_layer, window);

  // Initialize the text layer
  //s_text_layer = text_layer_create(max_text_bounds);
  s_text_layer = text_layer_create(GRect(0, 0, 144, 2000));
  layer_set_clips((Layer *)s_text_layer, false);
  text_layer_set_text(s_text_layer, s_scroll_text);
  
  // Change the font to a nice readable one
  // This is system font; you can inspect pebble_fonts.h for all system fonts
  // or you can take a look at feature_custom_font to add your own font
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

  // Trim text layer and scroll content to fit text box
  //GSize max_size = text_layer_get_content_size(s_text_layer);
  //text_layer_set_size(s_text_layer, max_size);
  
  layer_add_child(window_layer, (Layer *)s_text_layer);

  // scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, max_size.h + 4));

  // Add the layers for display
  // scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));

  // The inverter layer will highlight some text
  // s_inverter_layer = inverter_layer_create(GRect(0, 28, bounds.size.w, 28));
  // scroll_layer_add_child(s_scroll_layer, inverter_layer_get_layer(s_inverter_layer));

  // layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
  
  
  /* Create the topmost layer with the cursor. */
  s_cursor_layer = layer_create(
    GRect(0, INPUT_BOX_HEIGHT,
          144, KEYPAD_HEIGHT));
  layer_add_child(
    window_get_root_layer(window),
    s_cursor_layer);
  layer_set_update_proc(
    s_cursor_layer,
    draw_cursor_callback);
  
    // if height of quote > height of window, initiate animation to scroll
  GSize text_size = text_layer_get_content_size(s_text_layer);
  number_of_pixels = SCREEN_H - text_size.h;
  if (number_of_pixels < 0) {
      animate_quote(number_of_pixels);
  }
}

static void main_window_unload(Window *window) {
  // inverter_layer_destroy(s_inverter_layer);
  text_layer_destroy(s_text_layer);
  //scroll_layer_destroy(s_scroll_layer);
}

static void read_accel_and_move_cursor_callback(AccelData *data, uint32_t num_samples) {
    static int samples_until_calibrated = CALIBRATION_SAMPLES;
    static int zero_x = 0;
    static int zero_y = 0;

    /* collect the sample for calibration */
    if (samples_until_calibrated > 0) {
        --samples_until_calibrated;
        zero_x += data[0].x;
        zero_y += data[0].y;
        return;
    }

    /* all samples collected, calculate the average */
    if (samples_until_calibrated == 0) {
        --samples_until_calibrated;
        zero_x /= CALIBRATION_SAMPLES;
        zero_y /= CALIBRATION_SAMPLES;
    }

    /* the button is concave, simulate its steepness */
    int x_slope = 0;
    int y_slope = 0;
    if (s_focused_button_index != -1) {
        GPoint center = grect_center_point(&s_focused_button);
        x_slope = (center.x - s_cursor_position.x);
        y_slope = (center.y - s_cursor_position.y);
    }

    /* apply the new position cursor */
    const float ACCEL_MAX = 4000.f;
    s_cursor_position.x +=
        ((data[0].x - zero_x) * (SCREEN_W / ACCEL_MAX)
         + x_slope / STEEPNESS_FACTOR);
    s_cursor_position.y +=
        (-(data[0].y - zero_y) * (SCREEN_H / ACCEL_MAX)
         + y_slope / STEEPNESS_FACTOR);

    if (s_cursor_position.x < 0) {
        s_cursor_position.x = 0;
    } else if (s_cursor_position.x > SCREEN_W) {
        s_cursor_position.x = SCREEN_W;
    }

    if (s_cursor_position.y < 0) {
        s_cursor_position.y = 0;
    } else if (s_cursor_position.y > KEYPAD_HEIGHT) {
        s_cursor_position.y = KEYPAD_HEIGHT;
    }

    layer_mark_dirty(s_cursor_layer);
}


static void init() {
  // Subscribe to the accelerometer data service
  int num_samples = 1;
  accel_data_service_subscribe(num_samples, read_accel_and_move_cursor_callback);

  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
