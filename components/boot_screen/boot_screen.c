/*********************
 *      INCLUDES
 *********************/

#include <math.h>
#include "unicode.h"
#include "font_render.h"
#include "display_HAL.h"
#include "system_configuration.h"
#include "sin_table.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern const uint8_t ttf_start[] asm("_binary_Ubuntu_R_ttf_start");
extern const uint8_t ttf_end[] asm("_binary_Ubuntu_R_ttf_end");

/*********************
 *      DEFINES
 *********************/

#define DRAW_EVENT_START 0xfffc
#define DRAW_EVENT_END 0xfffd
#define DRAW_EVENT_FRAME_START 0xfffe
#define DRAW_EVENT_FRAME_END 0xffff
#define DRAW_EVENT_CONTROL DRAW_EVENT_START
#define ST7789_BUFFER_SIZE 20
#define ST7789_DISPLAY_WIDTH 240
#define ST7789_DISPLAY_HEIGHT 240

/*********************
 *   STATIC VARIABLES
 *********************/

static font_render_t font_render;
static font_render_t font_render2;
static font_face_t font_face;

/*********************
 *   VARIABLES
 *********************/
uint8_t initial_dither_table[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


/*********************
 *     TYPEDEFS
 *********************/
typedef struct draw_event_param {
	uint64_t frame;
	uint64_t total_frame;
	uint64_t duration;
	void *user_data;
} draw_event_param_t;

typedef void (*draw_callback)(uint16_t * buffer, uint16_t y, draw_event_param_t *param);

typedef struct draw_element {
	draw_callback callback;
	void *user_data;
} draw_element_t;

typedef struct animation_step {
	const uint64_t duration;
	const draw_element_t *draw_elements;
} animation_step_t;

/**********************
*  STATIC PROTOTYPES
**********************/
static void randomize_dither_table();
static void render_text(const char *text, font_render_t *render, uint16_t *buffer, int src_x, int src_y, int y, uint8_t color_r, uint8_t color_g, uint8_t color_b);
static void draw_gray2_bitmap(uint8_t *src_buf, uint16_t *target_buf, uint8_t r, uint8_t g, uint8_t b, int x, int y, int src_w, int src_h, int target_w, int target_h);
static inline void __attribute__((always_inline)) color_to_rgb(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) ;
static inline uint16_t __attribute__((always_inline)) rgb_to_color_dither(uint8_t r, uint8_t g, uint8_t b, uint16_t x, uint16_t y);
static inline uint8_t __attribute__((always_inline)) fast_sin(int value);
static uint16_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);
static void plasma_animation(uint16_t * buffer, uint16_t y, draw_event_param_t *param);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void boot_screen_free(){
	font_face_destroy(&font_face);
}

void boot_screen_task(){

	// Get the display buffer to save the partial render imagen
	uint16_t * buffer = display_HAL_get_buffer();

	while(1){
		ESP_ERROR_CHECK(font_face_init(&font_face, ttf_start, ttf_end - ttf_start - 1));

		const draw_element_t complex_text_demo_layers[] = {
			{plasma_animation, NULL},
			{NULL, NULL},
		};

		const animation_step_t animation[] = {
			{ 100, complex_text_demo_layers },
			{ 0, NULL },
		};

		const animation_step_t *animation_step = animation;
		draw_event_param_t draw_state = {
			.frame = 0,
			.total_frame = 0,
			.duration = 0,
			.user_data = NULL,
		};

		while (animation_step->draw_elements){
			const draw_element_t *current_layer;

			draw_state.frame = 0;
			draw_state.duration = animation_step->duration;

			// Before draw calls
			current_layer = animation_step->draw_elements;
			while (current_layer->callback) {
				draw_state.user_data = current_layer->user_data;
				plasma_animation(buffer, DRAW_EVENT_START, &draw_state);
				current_layer++;
			}

			while (draw_state.frame < animation_step->duration){
				// Before frame
				current_layer = animation_step->draw_elements;
				bool has_render_layer = (bool)current_layer->callback;
				while (current_layer->callback){
					draw_state.user_data = current_layer->user_data;
					current_layer->callback(buffer, DRAW_EVENT_FRAME_START, &draw_state);
					current_layer++;
				}

				if (has_render_layer){
					randomize_dither_table();
					for (size_t block = 0; block < ST7789_DISPLAY_WIDTH; block += ST7789_BUFFER_SIZE){
						current_layer = animation_step->draw_elements;
						while (current_layer->callback) {
							draw_state.user_data = current_layer->user_data;
							current_layer->callback(buffer, block, &draw_state);
							current_layer++;
						}
						// Send the rendered image to the HAL
						display_HAL_boot_frame(buffer);
					}
				}
				else{
					vTaskDelay(1000 / 40 / portTICK_PERIOD_MS);
				}

				// After frame
				current_layer = animation_step->draw_elements;
				while (current_layer->callback){
					draw_state.user_data = current_layer->user_data;
					current_layer->callback(buffer, DRAW_EVENT_FRAME_END, &draw_state);
					current_layer++;
				}

				draw_state.frame++;
				draw_state.total_frame++;
			}

			// After draw calls
			current_layer = animation_step->draw_elements;
			while (current_layer->callback){
				draw_state.user_data = current_layer->user_data;
				current_layer->callback(buffer, DRAW_EVENT_END, &draw_state);
				current_layer++;
			}

			animation_step++;
		}
		font_face_destroy(&font_face);
	}
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void plasma_animation(uint16_t * buffer, uint16_t y, draw_event_param_t *param){

	if (y >= DRAW_EVENT_CONTROL) {
		if (y == DRAW_EVENT_START) {
			ESP_ERROR_CHECK(font_render_init(&font_render2, &font_face, 30, 48));
		}
		else if (y == DRAW_EVENT_END) {
			font_render_destroy(&font_render2);
		}
		else if (y == DRAW_EVENT_FRAME_START) {
			if (param->frame > 1200 - 240) {
				uint32_t glyph = 0x21 + ((param->frame >> 5) % 0x5d);
				ESP_ERROR_CHECK(font_render_init(&font_render, &font_face, (fast_sin(param->frame << 2) >> 1) + 14, 1));
				font_render_glyph(&font_render, glyph);
			}
		}
		else if (y == DRAW_EVENT_FRAME_END) {
			if (param->frame > 1200 - 240) {
				font_render_destroy(&font_render);
			}
		}
		return;
	}

	int cursor_x = 240 - 1;
	int cursor_y = y - 1;
	const int frame = (int)param->frame;
	const int plasma_shift = frame < 256 ? 1 : 2;

	const int frame_1 = frame << 1;
	const int frame_2 = frame << 2;
	const int frame_7 = frame * 7;
	
	for (size_t i = 0; i < 20*240; ++i){
		cursor_x++;
		if(cursor_x == 240){
			cursor_x = 0;
			cursor_y++;
		}

		if(frame + cursor_y < 1200){
			const int cursor_x_1 = cursor_x << 1;
			const int cursor_x_2 = cursor_x << 2;
			const int cursor_y_1 = cursor_y << 1;
			const int cursor_y_2 = cursor_y << 2;

			uint16_t plasma_value = fast_sin(cursor_x_2 + cursor_y_1 + frame_2);
			plasma_value += fast_sin(fast_sin(((cursor_y_1 + frame) << 1) + cursor_x) + frame_7);
			plasma_value >>= plasma_shift;

			uint16_t color_r = plasma_value;

			plasma_value = fast_sin(cursor_x + cursor_y_2 + frame_1);
			plasma_value += fast_sin(fast_sin(((cursor_x_1 + frame) << 1) + cursor_y) + frame_1);
			plasma_value >>= plasma_shift;

			uint16_t color_b = plasma_value;

			if(frame < 256){
				if(frame < 64){
					color_r = (color_r * frame) >> 6;
					color_b = (color_b * frame) >> 6;
				}
				if(frame > 128){
					color_r = (color_r * (32 + ((256 - frame) >> 2))) >> 6;
					color_b = (color_b * (32 + ((256 - frame) >> 2))) >> 6;
				}
			}
			buffer[i] = rgb_to_color_dither(color_r, (color_r >> 1) + (color_b >> 1), color_b, cursor_x, cursor_y);
		}
	}

	if(frame >= 0 && frame<= 100){
		render_text("micro", &font_render2, buffer, 50, 100, y, 255, 255, 255);
	}
	if(frame >= 0 && frame<= 100){
		render_text("BYTE", &font_render2, buffer, 130, 100, y, 255, 255, 255);
	}
}

static void randomize_dither_table(){
	uint16_t *dither_table = (uint16_t *)initial_dither_table;

	for (size_t i = 0; i < sizeof(initial_dither_table) / 2; ++i){
		dither_table[i] = rand() & 0xffff;
	}
}

static void render_text(const char *text, font_render_t *render, uint16_t *buffer, int src_x, int src_y, int y, uint8_t color_r, uint8_t color_g, uint8_t color_b){

	if(src_y - y >= ST7789_BUFFER_SIZE || src_y + (int)render->max_pixel_height - y < 0){
		return;
	}

	while(*text){
		uint32_t glyph;
		text += u8_decode(&glyph, text);
		font_render_glyph(render, glyph);
		draw_gray2_bitmap(render->bitmap, buffer, color_r, color_g, color_b, src_x + render->bitmap_left, render->max_pixel_height - render->origin - render->bitmap_top + src_y - y, render->bitmap_width, render->bitmap_height, 240, ST7789_BUFFER_SIZE);
		src_x += render->advance;
	}
}

static void draw_gray2_bitmap(uint8_t *src_buf, uint16_t *target_buf, uint8_t r, uint8_t g, uint8_t b, int x, int y, int src_w, int src_h, int target_w, int target_h) {
	if (x >= target_w || y >= target_h || x + src_w <= 0 || y + src_h <= 0){
		return;
	}
	int min = target_w;
	if(src_w + x < target_w) min = src_w + x;

	int max = x;
	if(0>x) max = 0;

	const size_t src_size = src_w * src_h;
	const size_t target_size = target_w * target_h;
	const size_t line_w = min - max;
	const size_t src_skip = src_w - line_w;
	const size_t target_skip = target_w - line_w;
	size_t src_pos = 0;
	size_t target_pos = 0;
	size_t x_pos = 0;
	size_t y_pos = 0;

	if (y < 0){
		src_pos = (-y) * src_w;
	}
	if (x < 0){
		src_pos -= x;
	}
	if (y > 0){
		target_pos = y * target_w;
	}
	if (x > 0){
		target_pos += x;
	}

	while(src_pos < src_size && target_pos < target_size){
		uint8_t src_r, src_g, src_b;
		uint8_t target_r, target_g, target_b;
		color_to_rgb(target_buf[target_pos], &src_r, &src_g, &src_b);
		uint8_t gray2_color = (src_buf[src_pos >> 2] >> ((src_pos & 0x03) << 1)) & 0x03;
		switch(gray2_color){
			case 1:
				target_r = r >> 1;
				target_g = g >> 1;
				target_b = b >> 1;
				src_r = (src_r >> 1) + target_r;
				src_g = (src_g >> 1) + target_g;
				src_b = (src_b >> 1) + target_b;
				target_buf[target_pos] = rgb_to_color_dither(src_r, src_g, src_b, x_pos, y_pos);
				break;
			case 2:
				target_r = r >> 2;
				target_g = g >> 2;
				target_b = b >> 2;
				src_r = (src_r >> 2) + target_r + target_r + target_r;
				src_g = (src_g >> 2) + target_g + target_g + target_g;
				src_b = (src_b >> 2) + target_b + target_b + target_b;
				target_buf[target_pos] = rgb_to_color_dither(src_r, src_g, src_b, x_pos, y_pos);
				break;
			case 3:
				target_buf[target_pos] = rgb_to_color_dither(r, g, b, x_pos, y_pos);
				break;
			default:
				break;
		}

		x_pos++;

		if (x_pos == line_w){
			x_pos = 0;
			y_pos++;
			src_pos += src_skip;
			target_pos += target_skip;
		}
		src_pos++;
		target_pos++;
	}
}

static inline void __attribute__((always_inline)) color_to_rgb(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
	*b = (color << 3);
	color >>= 5;
	color <<= 2;
	*g = color;
	color >>= 8;
	*r = color << 3;
}

static inline uint16_t __attribute__((always_inline)) rgb_to_color_dither(uint8_t r, uint8_t g, uint8_t b, uint16_t x, uint16_t y) {
	const uint8_t pos = ((y << 8) + (y << 3) + x) & 0xff;
	uint8_t rand_b = initial_dither_table[pos];
	const uint8_t rand_r = rand_b & 0x07;
	rand_b >>= 3;
	const uint8_t rand_g = rand_b & 0x03;
	rand_b >>= 2;

	if (r < 249){
		r = r + rand_r;
	}
	if (g < 253){
		g = g + rand_g;
	}
	if (b < 249){
		b = b + rand_b;
	}
	return rgb_to_color(r, g, b);
}

static uint16_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b){

	return ((((uint16_t)(r) >> 3) << 11) | (((uint16_t)(g) >> 2) << 5) | ((uint16_t)(b) >> 3));
	
}

static inline uint8_t __attribute__((always_inline)) fast_sin(int value) {
	return sin_table[value & 0x3ff];
}