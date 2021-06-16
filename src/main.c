/*
SPDX-License-Identifier: GPL-3.0-or-later

iaq-monitor-demo
Copyright (C) 2021  dxcfl

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>
#include <drivers/sensor/ccs811.h>
#include <drivers/display.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <lvgl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "gui.h"
#include "iaq.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BME280 DT_INST(0, bosch_bme280)
#if DT_NODE_HAS_STATUS(BME280, okay)
#define BME280_LABEL DT_LABEL(BME280)
#else
#error The devicetree has no enabled nodes with compatible "bosch,bme280"
#define BME280_LABEL "<none>"
#endif

#define CCS811 DT_INST(0, ams_ccs811)
#if DT_NODE_HAS_STATUS(CCS811, okay)
#define CCS811_LABEL DT_LABEL(CCS811)
#else
#error The devicetree has no enabled nodes with compatible "ams_ccs811"
#define CCS811_LABEL "<none>"
#endif

#define CALIBRATION_TIME_SECONDS 20 // should be 20 minutes! ;)

/* Auxiliary function: Format time string.
*/
static const char *time_str(uint32_t time, bool with_millis)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	unsigned int ms = time % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	time /= MSEC_PER_SEC;
	s = time % 60U;
	time /= 60U;
	min = time % 60U;
	time /= 60U;
	h = time;
	if (with_millis)
		snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
				 h, min, s, ms);
	else
		snprintf(buf, sizeof(buf), "%u:%02u:%02u",
				 h, min, s);

	return buf;
}

static const char *now_str()
{
	return time_str(k_uptime_get_32(), true);
}

/* Bluetooth beacon setup ...
 * "stolen" from the Zephyr bluetooth beacon example 
 * (zephyr/samples/bluetooth/beacon/main.c).
 * Setup a non-connectable Eddystone beacon.
 * Later we will "abuse" the name data in the scan
 * resonse to transport our IAQ rating.
*/

/*
	 * Set Advertisement data. Based on the Eddystone specification:
 	 * https://github.com/google/eddystone/blob/master/protocol-specification.md
 	 * https://github.com/google/eddystone/tree/master/eddystone-url
 	*/
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				  0xaa, 0xfe, /* Eddystone UUID */
				  0x10,		  /* Eddystone-URL frame type */
				  0x00,		  /* Calibrated Tx power at 0m */
				  0x00,		  /* URL Scheme Prefix http://www. */
				  'e', 'x', 'a', 'm', 'p', 'l', 'e',
				  0x08) /* .org */
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA(BT_DATA_NAME_SHORTENED, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready(int err)
{

	char addr_s[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addr = {0};
	size_t count = 1;

	if (err)
	{
		printk("\n[%s]: Bluetooth init failed (err %d)\n", now_str(), err);
		return;
	}
	printk("\n[%s]: Bluetooth initialized\n", now_str());

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
						  sd, ARRAY_SIZE(sd));
	if (err)
	{
		printk("\n[%s]: Advertising failed to start (err %d)\n", now_str(), err);
		return;
	}

	/* For connectable advertising you would use
	 * bt_le_oob_get_local().  For non-connectable non-identity
	 * advertising an non-resolvable private address is used;
	 * there is no API to retrieve that.
	 */

	bt_id_get(&addr, &count);
	bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));

	printk("\n[%s]: Beacon started, advertising as %s\n", now_str(), addr_s);
}

/* Auxiliary function to handle timing issues when fetching a
 * sample from the CCS811 sensor: Repeat sensor_sample_fetch
 * until valid data has been received.
*/
int ccs811_sample_fetch(struct device *dev)
{
	static bool first = true;
	static bool ccs811_fw_app_v2 = false;
	int rc;

	if (first)
	{
		struct ccs811_configver_type cfgver;
		rc = ccs811_configver_fetch(dev, &cfgver);
		if (rc == 0)
		{
			printk("\n[%s]: CCS811: HW %02x; FW Boot %04x App %04x ; mode %02x\n",
				   now_str(),
				   cfgver.hw_version, cfgver.fw_boot_version,
				   cfgver.fw_app_version, cfgver.mode);
			ccs811_fw_app_v2 = (cfgver.fw_app_version >> 8) > 0x11;
		}
		first = false;
	}

	rc = sensor_sample_fetch(dev);
	while (rc != 0)
	{
		const struct ccs811_result_type *rp = ccs811_result(dev);

		if (ccs811_fw_app_v2 && !(rp->status & CCS811_STATUS_DATA_READY))
		{
			printk("\n[%s]: CCS811: Stale data!\n", now_str());
			continue;
		}

		if (rp->status & CCS811_STATUS_ERROR)
		{
			printk("\n[%s]: CCS811: ERROR: %02x\n", now_str(), rp->error);
			break;
		}

		k_sleep(K_MSEC(10));
		rc = sensor_sample_fetch(dev);
	}

	return rc;
}

/*
 * Main application logic ...
*/
void main(void)
{
	/* General setup and initialization
	*/

	/* Setup sensor: BME280
	*/
	const struct device *bme280 = device_get_binding(BME280_LABEL);
	if (bme280 == NULL)
	{
		printk("No device \"%s\" found; Initialization failed?\n",
			   BME280_LABEL);
		return;
	}
	else
	{
		printk("Found device \"%s\"\n", BME280_LABEL);
		printk("Device is %p, name is %s\n", bme280, bme280->name);
	}

	/* Setup sensor: CCS811
	*/
	const struct device *ccs811 = device_get_binding(CCS811_LABEL);
	if (ccs811 == NULL)
	{
		printk("No device \"%s\" found; Initialization failed?\n",
			   CCS811_LABEL);
		return;
	}
	else
	{
		printk("Found device \"%s\"\n", CCS811_LABEL);
		printk("Device is %p, name is %s\n", ccs811, ccs811->name);
	}

	/* Setup and start Bluetooth beacon
	*/
	int bt_err;
	printk("\n[%s]: BT: Starting beacon ...\n", now_str());
	bt_err = bt_enable(bt_ready);
	if (bt_err)
	{
		printk("\n[%s]: BT: Initiialization failed (err %d)\n", now_str(), bt_err);
	}

	/* Setup GUI
	*/
	gui_setup();

	/* Forever ...
	*/
	while (1)
	{
		int rc = -1;
		bool valid_env_data_bme280 = false;
		bool valid_env_data_ccs811 = false;
		uint32_t now = k_uptime_get_32();
		int32_t calibration_time_remaining = CALIBRATION_TIME_SECONDS * MSEC_PER_SEC - now;

		struct sensor_value temp, press, humidity, co2, tvoc;

		/* Read sensor: BME280
		*/
		rc = sensor_sample_fetch(bme280);
		if (rc == 0)
		{
			valid_env_data_bme280 = true;

			/* Get sensor values for temperature, pressure and humidity
			*/
			sensor_channel_get(bme280, SENSOR_CHAN_AMBIENT_TEMP, &temp);
			sensor_channel_get(bme280, SENSOR_CHAN_PRESS, &press);
			sensor_channel_get(bme280, SENSOR_CHAN_HUMIDITY, &humidity);

			printk("\n[%s]: BME280: temp: %d.%06d; press: %d.%06d; humidity: %d.%06d\n",
				   now_str(),
				   temp.val1, temp.val2,
				   press.val1, press.val2,
				   humidity.val1, humidity.val2);

			/* Update the appropriate GUI elements
			*/
			gui_update_sensor_value(SENSOR_CHAN_AMBIENT_TEMP, temp);
			gui_update_sensor_value(SENSOR_CHAN_PRESS, press);
			gui_update_sensor_value(SENSOR_CHAN_HUMIDITY, humidity);

			/* Accurate calculation of gas levels requires accurate environment data. 
			 * Measurements are only accurate to 0.5 Cel and 0.5 RH.
			 * The CCS811 features an ENV_DATA register, which can be 'fed' with
			 * with the actual environmental data to improve the accuracy of the
			 * provided values for gas levels.
			*/
			rc = ccs811_envdata_update(ccs811, &temp, &humidity);
			if (rc == 0)
			{
				printk("\n[%s]: CCS811: Env data updated!\n", now_str());
			}
			else
			{
				printk("\n[%s]: CCS811: Failed to update env data!\n", now_str());
			}
		}
		else
		{
			valid_env_data_bme280 = false;
			printk("\n[%s]: BME280: Failed to fetch sensor data!\n", now_str());
		}

		/* Read sensor: CCS811
		*/
		rc = ccs811_sample_fetch(ccs811);
		if (rc == 0)
		{
			valid_env_data_ccs811 = true;

			/* Get sensor values for CO2 and VOC concentration
			*/
			sensor_channel_get(ccs811, SENSOR_CHAN_CO2, &co2);
			sensor_channel_get(ccs811, SENSOR_CHAN_VOC, &tvoc);

			printk("\n[%s]: CCS811: %u ppm eCO2; %u ppb eTVOC\n",
				   now_str(),
				   co2.val1,
				   tvoc.val1);

			/* Update the appropriate GUI elements
			*/
			gui_update_sensor_value(SENSOR_CHAN_CO2, co2);
			gui_update_sensor_value(SENSOR_CHAN_VOC, tvoc);
		}
		else
		{
			valid_env_data_ccs811 = false;
			printk("\n[%s]: CCS811: Failed to fetch sensor data!\n", now_str());
		}

		/* Calculate and display the IAQI rating
		*/

		/* If calibration time elapased and valid sensor readings are available ...
		*/
		if (calibration_time_remaining <= 0 && valid_env_data_bme280 && valid_env_data_ccs811)
		{
			/* Calculate the IAQI and update the GUI's meter component with the 'relative qualitity'
			 * and the IAQI rating.
			*/
			uint8_t iaq_index = get_iaq_index(temp.val1, humidity.val1, co2.val1, tvoc.val1);
			uint16_t quality = iaq_index * 100 / get_max_iaq_index();
			printk("\n[%s]: APP: IAQ index: %d (%d %%)\n", now_str(), iaq_index, quality);
			gui_update_qmeter(quality, get_iaq_rating(iaq_index));

			/* Update the scan reponse data for the Bluetooth beacon: 'Misuse' the name data for
			*  transporting the IAQI rating.
			*/
			struct bt_data new_sd[] = {
				BT_DATA(BT_DATA_NAME_COMPLETE, get_iaq_rating(iaq_index), strlen(get_iaq_rating(iaq_index))),
				BT_DATA(BT_DATA_NAME_SHORTENED, DEVICE_NAME, DEVICE_NAME_LEN),
			};
			bt_err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad),
										   new_sd, ARRAY_SIZE(new_sd));
			if (bt_err)
			{
				printk("\n[%s]: BT: Advertising update failed (err %d)\n", now_str(), bt_err);
			}
		}
		/* If we are still calibrating ...
		*/
		else if (calibration_time_remaining > 0)
		{
			printk("\n[%s]: APP: Calibration time remaining: %s\n", now_str(), time_str(calibration_time_remaining, true));
			/* Show remaining time for calibration
			*/
			gui_update_qmeter(0, time_str(calibration_time_remaining, false));
		}

		k_sleep(K_MSEC(1000));
	}
}
