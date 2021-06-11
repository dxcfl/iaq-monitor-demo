#include <zephyr.h>
#include <device.h>
#include <drivers/display.h>
#include <drivers/sensor.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "gui.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gui);

const struct device *display_dev;

/* GUI objects ... 
*/
lv_obj_t *headline;
lv_obj_t *qualitiy_meter;
lv_obj_t *qualitiy_label;
lv_obj_t *temp_label;
lv_obj_t *temp_value_label;
lv_obj_t *pressure_label;
lv_obj_t *press_value_label;
lv_obj_t *humidity_label;
lv_obj_t *humid_value_label;
lv_obj_t *co2_label;
lv_obj_t *co2_value_label;
lv_obj_t *tvoc_label;
lv_obj_t *tvoc_value_label;

/* GUI setup ... 
*/
void gui_setup(void)
{
	display_dev = device_get_binding(CONFIG_LVGL_DISPLAY_DEV_NAME);

	if (display_dev == NULL)
	{
		LOG_ERR("Display device not found!");
		return;
	}

	display_blanking_off(display_dev);

	lv_theme_t *theme = lv_theme_material_init(LV_COLOR_GREEN, LV_COLOR_WHITE, LV_THEME_MATERIAL_FLAG_DARK, &lv_font_montserrat_14, &lv_font_montserrat_16, &lv_font_montserrat_18, &lv_font_montserrat_22);
	lv_theme_set_act(theme);

	headline = lv_label_create(lv_scr_act(), NULL);
	qualitiy_meter = lv_linemeter_create(lv_scr_act(), NULL);
	qualitiy_label = lv_label_create(lv_scr_act(), NULL);
	temp_label = lv_label_create(lv_scr_act(), NULL);
	temp_value_label = lv_label_create(lv_scr_act(), NULL);
	pressure_label = lv_label_create(lv_scr_act(), NULL);
	press_value_label = lv_label_create(lv_scr_act(), NULL);
	humidity_label = lv_label_create(lv_scr_act(), NULL);
	humid_value_label = lv_label_create(lv_scr_act(), NULL);
	co2_label = lv_label_create(lv_scr_act(), NULL);
	co2_value_label = lv_label_create(lv_scr_act(), NULL);
	tvoc_label = lv_label_create(lv_scr_act(), NULL);
	tvoc_value_label = lv_label_create(lv_scr_act(), NULL);

	static lv_style_t large_style;
	lv_style_init(&large_style);
	lv_style_set_text_font(&large_style, LV_STATE_DEFAULT, lv_theme_get_font_title());

	lv_obj_add_style(headline, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(headline, 115);
	lv_obj_set_y(headline, 10);
	lv_obj_set_height(headline, 20);
	lv_obj_set_width(headline, 50);
	lv_label_set_text(headline, "Air Quality");

	lv_obj_set_x(qualitiy_meter, 10);
	lv_obj_set_y(qualitiy_meter, 75);
	lv_obj_set_width(qualitiy_meter, 90);
	lv_obj_set_height(qualitiy_meter, 90);
	lv_linemeter_set_range(qualitiy_meter, 0, 100);

	lv_obj_set_x(qualitiy_label, 30);
	lv_obj_set_y(qualitiy_label, 170);
	lv_obj_set_width(qualitiy_label, 60);
	lv_obj_set_height(qualitiy_label, 40);
	lv_label_set_text(qualitiy_label, "Quality");

	unsigned int line0_y = 50;
	unsigned int line_space = 40;
	unsigned int line = 0;

	lv_obj_set_x(temp_label, 115);
	lv_obj_set_y(temp_label, line0_y + line_space * line);
	lv_label_set_text(temp_label, "Temp (C°)");
	lv_obj_add_style(temp_value_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(temp_value_label, 245);
	lv_obj_set_y(temp_value_label, line0_y + line_space * line);
	lv_label_set_text(temp_value_label, "...");

	line++;
	lv_obj_set_x(pressure_label, 115);
	lv_obj_set_y(pressure_label, line0_y + line_space * line);
	lv_label_set_text(pressure_label, "Pressure (hPa)");
	lv_obj_add_style(press_value_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(press_value_label, 245);
	lv_obj_set_y(press_value_label, line0_y + line_space * line);
	lv_label_set_text(press_value_label, "...");

	line++;
	lv_obj_set_x(humidity_label, 115);
	lv_obj_set_y(humidity_label, line0_y + line_space * line);
	lv_label_set_text(humidity_label, "Humidity (%)");
	lv_obj_add_style(humid_value_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(humid_value_label, 245);
	lv_obj_set_y(humid_value_label, line0_y + line_space * line);
	lv_label_set_text(humid_value_label, "...");

	line++;
	lv_obj_set_x(co2_label, 115);
	lv_obj_set_y(co2_label, line0_y + line_space * line);
	lv_label_set_text(co2_label, "eCO2 (ppm)");
	lv_obj_add_style(co2_value_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(co2_value_label, 245);
	lv_obj_set_y(co2_value_label, line0_y + line_space * line);
	lv_label_set_text(co2_value_label, "...");

	line++;
	lv_obj_set_x(tvoc_label, 115);
	lv_obj_set_y(tvoc_label, line0_y + line_space * line);
	lv_label_set_text(tvoc_label, "TVOC (ppb)");
	lv_obj_add_style(tvoc_value_label, LV_LABEL_PART_MAIN, &large_style);
	lv_obj_set_x(tvoc_value_label, 245);
	lv_obj_set_y(tvoc_value_label, line0_y + line_space * line);
	lv_label_set_text(tvoc_value_label, "...");
}

/* Updates the value label for the refered sensor channel / value ... 
*/
void gui_update_sensor_value(enum sensor_channel channel, struct sensor_value value)
{
	value.val2 = value.val2 / 10000;
	switch (channel)
	{
	case SENSOR_CHAN_AMBIENT_TEMP:
		lv_label_set_text_fmt(temp_value_label, "%d.%02d", value.val1, value.val2);
		break;
	case SENSOR_CHAN_PRESS:
		lv_label_set_text_fmt(press_value_label, "%d.%02d", value.val1, value.val2);
		break;
	case SENSOR_CHAN_HUMIDITY:
		lv_label_set_text_fmt(humid_value_label, "%d.%02d", value.val1, value.val2);
		break;
	case SENSOR_CHAN_CO2:
		lv_label_set_text_fmt(co2_value_label, "%u", value.val1);
		break;
	case SENSOR_CHAN_VOC:
		lv_label_set_text_fmt(tvoc_value_label, "%u", value.val1);
		break;
	}
}

/* Updates the line meter and it's label with the 'relative IQAI' and the rating ... 
*/
void gui_update_qmeter(int8_t quality, const char *rating)
{
	lv_linemeter_set_value(qualitiy_meter, quality);
	lv_label_set_text(qualitiy_label, rating);
}

/* Thread for activating the LVGL taskhandler periodicly ... 
*/
void gui_run(void)
{
	while (1)
	{
		lv_task_handler();
		k_sleep(K_MSEC(20));
	}
}

// Define our GUI thread, using a stack size of 4096 and a priority of 7
K_THREAD_DEFINE(gui_thread, 4096, gui_run, NULL, NULL, NULL, 7, 0, 0);
