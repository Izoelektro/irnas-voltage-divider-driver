#include "voltage_divider.h"

#include <drivers/adc.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <hal/nrf_saadc.h>
#include <init.h>
#include <zephyr.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(voltage_divider, CONFIG_VOLTAGE_DIVIDER_LOG_LEVEL);

struct io_channel_config {
	uint8_t channel;
};

struct gpio_channel_config {
	const char *label;
	uint8_t pin;
	uint8_t flags;
};

struct divider_config {
	struct io_channel_config io_channel;
	bool has_power_gpio;
	struct gpio_channel_config power_gpios;
	/* output_ohm is used as a flag value: if it is nonzero then
	 * the voltage is measured through a voltage divider;
	 * otherwise it is assumed to be directly connected to Vdd.
	 */
	uint32_t output_ohm;
	uint32_t full_ohm;
};

struct divider_data {
	const struct device *adc;
	const struct device *gpio;
	struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;
	int16_t raw;
};

#define DEV_CFG(dev) ((const struct divider_config *)((dev)->config))
#define DEV_DATA(dev) ((struct divider_data *)((dev)->data))

/**
 * @brief Enable/Disable the pin that enables power to flow into the voltage
 * divider
 *
 * @param state The state to set. 1 to enable, 0 to disable
 * @return int 0 on success, or negative error code
 */
static int voltage_divider_measure_enable_set(const struct device *dev,
					      int state)
{
	const struct divider_config *cfg = DEV_CFG(dev);
	const struct gpio_channel_config *gpio_chn_cfg = &cfg->power_gpios;
	struct divider_data *data = DEV_DATA(dev);

	int err = 0;

	if (cfg->has_power_gpio) {
		LOG_DBG("Seting gcp->pin (%d) to %d", gpio_chn_cfg->pin, state);
		err = gpio_pin_set(data->gpio, gpio_chn_cfg->pin, state);
		// if enabling, then wait a bit
		if (state) {
			k_sleep(K_MSEC(10));
		}
	}
	return err;
}

int voltage_divider_sample(const struct device *dev)
{
	const struct divider_config *cfg = DEV_CFG(dev);
	struct divider_data *data = DEV_DATA(dev);
	struct adc_sequence *adc_seq = &data->adc_seq;

	int err = 0;

	voltage_divider_measure_enable_set(dev, 1);

	err = adc_read(data->adc, adc_seq);

	adc_seq->calibrate = false;

	if (err == 0) {
		int32_t val = data->raw;
		LOG_DBG("REF INTERNAL IS: %d", adc_ref_internal(data->adc));
		adc_raw_to_millivolts(adc_ref_internal(data->adc),
				      data->adc_cfg.gain, adc_seq->resolution,
				      &val);

		if (cfg->output_ohm != 0) {
			LOG_DBG("val: %d", val);
			LOG_DBG("full_ohm: %d", cfg->full_ohm);
			LOG_DBG("output_ohm: %d", cfg->output_ohm);
			err = val * (uint64_t)cfg->full_ohm / cfg->output_ohm;
			LOG_DBG("voltage_divider: raw %u ~ %u mV => %d mV",
				data->raw, val, err);
		} else {
			err = val;
			LOG_DBG("raw %u ~ %u mV\n", data->raw, val);
		}
	}

	voltage_divider_measure_enable_set(dev, 0);

	return err;
}

/**
 * @brief Set up GPIO and ADC for the voltage divider
 *
 * @return int 0 on success, or negative error code
 */
static int divider_setup(const struct device *dev)
{
	const struct divider_config *cfg = DEV_CFG(dev);
	struct divider_data *data = DEV_DATA(dev);

	const struct io_channel_config *io_ch_cfg = &cfg->io_channel;
	const struct gpio_channel_config *gpio_chn_cfg = &cfg->power_gpios;
	struct adc_sequence *adc_seq = &data->adc_seq;
	struct adc_channel_cfg *adc_cfg = &data->adc_cfg;

	int rc;

	if (!device_is_ready(data->adc)) {
		LOG_ERR("ADC device is not ready %s", data->adc->name);
		return -ENOENT;
	}

	// set up GPIO pin that enables the voltage divider
	if (cfg->has_power_gpio) {
		data->gpio = device_get_binding(gpio_chn_cfg->label);
		if (data->gpio == NULL) {
			LOG_ERR("Failed to get GPIO %s", gpio_chn_cfg->label);
			return -ENOENT;
		}
		rc = gpio_pin_configure(data->gpio, gpio_chn_cfg->pin,
					GPIO_OUTPUT_INACTIVE |
						gpio_chn_cfg->flags);
		if (rc != 0) {
			LOG_ERR("Failed to control feed %s.%u: %d",
				gpio_chn_cfg->label, gpio_chn_cfg->pin, rc);
			return rc;
		}
	}

	*adc_seq = (struct adc_sequence){
		.channels = BIT(io_ch_cfg->channel),
		.buffer = &data->raw,
		.buffer_size = sizeof(data->raw),
		.oversampling = 4,
		.calibrate = true,
	};

	*adc_cfg = (struct adc_channel_cfg){
		.channel_id = io_ch_cfg->channel,
		.gain = ADC_GAIN_1_6,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	};

	if (cfg->output_ohm != 0) {
		adc_cfg->input_positive =
			SAADC_CH_PSELP_PSELP_AnalogInput0 + io_ch_cfg->channel;
	} else {
		adc_cfg->input_positive = SAADC_CH_PSELP_PSELP_VDD;
	}

	adc_seq->resolution = 14;

	rc = adc_channel_setup(data->adc, adc_cfg);
	LOG_INF("Setup AIN%u got %d", io_ch_cfg->channel, rc);

	return rc;
}

/**
 * @brief Initializes a voltage divider device
 *
 * @return int 0 on success, or negative error code
 */
static int voltage_divider_init(const struct device *dev)
{
	// const struct divider_config *cfg = DEV_CFG(dev);
	// struct divider_data *data        = DEV_DATA(dev);

	int err = divider_setup(dev);
	if (err) {
		LOG_ERR("Divider setup, err: %d", err);
	}
	return err;
}

#define DT_DRV_COMPAT irnas_voltage_divider

#define VOLTAGE_DIVIDER_INIT(N)                                                \
	static struct divider_data divider_data_##N = {                        \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(N))};            \
                                                                               \
	static const struct divider_config divider_config_##N = {              \
		.io_channel =                                                  \
			{                                                      \
				DT_INST_IO_CHANNELS_INPUT(N),                  \
			},                                                     \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(N, power_gpios),              \
			   (.has_power_gpio = true, ))                         \
			IF_ENABLED(DT_INST_NODE_HAS_PROP(N, power_gpios),      \
				   (.power_gpios =                             \
					    {                                  \
						    DT_INST_GPIO_LABEL(        \
							    N, power_gpios),   \
						    DT_INST_GPIO_PIN(          \
							    N, power_gpios),   \
						    DT_INST_GPIO_FLAGS(        \
							    N, power_gpios),   \
					    }, ))                              \
				.output_ohm = DT_INST_PROP(N, output_ohms),    \
		.full_ohm = DT_INST_PROP(N, full_ohms),                        \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(N, voltage_divider_init, NULL,                   \
			      &divider_data_##N, &divider_config_##N,          \
			      APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(VOLTAGE_DIVIDER_INIT)
