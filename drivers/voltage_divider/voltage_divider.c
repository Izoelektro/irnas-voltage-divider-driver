#include "voltage_divider.h"

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/sys/util_macro.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(voltage_divider, CONFIG_VOLTAGE_DIVIDER_LOG_LEVEL);

struct divider_config {
	/* Analog channel id. Directly connected with the analog pin number (eg. 3 is AIN3). */
	uint8_t analog_channel;

	/* If true, the internal vdd is measured, instead of a external pin. */
	bool measure_internal_vdd;

	/* If true, the power_gpio struct is filled and it should be enabled during adc sampling. */
	bool has_power_gpio;
	struct gpio_dt_spec power_gpio;
	int32_t power_startup_delay_ms;

	/* Resistor values */
	uint32_t upper_resistor_ohm;
	uint32_t lower_resistor_ohm;
};

struct divider_data {
	/* Adc structs required for the measurement. */
	const struct device *adc;
	struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;

	/* Raw buffer, passed to adc_seq for storage. */
	int16_t raw;
};

/**
 * @brief Set the state of the pin that enables power to the voltage divider
 *
 * @param[in] state	The logical state to set.
 *
 * @return int 0 on success, or negative error code
 */
static void voltage_divider_measure_enable_set(const struct device *dev, int state)
{
	const struct divider_config *cfg = dev->config;

	LOG_DBG("Setting power_gpio (%s.%d) to %d", cfg->power_gpio.port->name, cfg->power_gpio.pin,
		state);
	gpio_pin_set_dt(&cfg->power_gpio, state);

	/* if enabling, then wait a bit */
	if (state) {
		k_sleep(K_MSEC(cfg->power_startup_delay_ms));
	}
}

int voltage_divider_sample(const struct device *dev)
{
	const struct divider_config *cfg = dev->config;
	struct divider_data *data = dev->data;
	struct adc_sequence *adc_seq = &data->adc_seq;

	int rc = 0;

	if (cfg->has_power_gpio) {
		voltage_divider_measure_enable_set(dev, 1);
	}

	rc = adc_read(data->adc, adc_seq);
	if (rc) {
		goto cleanup;
	}

	/* Calibration should be perfrom for the first sample only, after that it can be switched
	 * off. No idea if this is completly valid, it was like this before this commit, however I
	 * did not find sufficient evidence to support that claim. (I did find some github issue
	 * about that: https://github.com/zephyrproject-rtos/zephyr/issues/11922.)    */
	adc_seq->calibrate = false;

	/* Get result of sampling */
	int32_t val = abs(data->raw);
	LOG_DBG("internal ref: %d", adc_ref_internal(data->adc));
	rc = adc_raw_to_millivolts(adc_ref_internal(data->adc), data->adc_cfg.gain,
				   adc_seq->resolution, &val);
	if (rc) {
		goto cleanup;
	}

	int result;
	/* Apply voltage divider conversion if not measuring directly */
	if (cfg->upper_resistor_ohm && cfg->lower_resistor_ohm) {
		result = ((uint64_t)val * (cfg->upper_resistor_ohm + cfg->lower_resistor_ohm)) /
			 cfg->lower_resistor_ohm;
	} else {
		result = val;
	}
	LOG_DBG("val: %d", val);
	LOG_DBG("raw %u ~ %u mV\n", data->raw, val);

	rc = result;
cleanup:
	if (cfg->has_power_gpio) {
		voltage_divider_measure_enable_set(dev, 0);
	}
	return rc;
}

/**
 * @brief Initializes a voltage divider device
 *
 * @return int 0 on success, or negative error code.
 */
static int voltage_divider_init(const struct device *dev)
{
	const struct divider_config *cfg = dev->config;
	struct divider_data *data = dev->data;

	int rc;

	if (!device_is_ready(data->adc)) {
		LOG_ERR("ADC device (%s) is not ready", data->adc->name);
		return -ENOENT;
	}

	if ((cfg->upper_resistor_ohm && cfg->measure_internal_vdd) ||
	    (cfg->lower_resistor_ohm && cfg->measure_internal_vdd)) {
		LOG_ERR("Invalid configuration, one of resistors is set to non-zero, but "
			"measure-internal-vdd is set to true");
		return -EINVAL;
	}

	if ((cfg->upper_resistor_ohm && !cfg->lower_resistor_ohm) ||
	    (!cfg->upper_resistor_ohm && cfg->lower_resistor_ohm)) {
		LOG_ERR("Invalid configuration, one of resistors is set to while other isn't");
		return -EINVAL;
	}

	if (cfg->power_startup_delay_ms < 0) {
		LOG_ERR("Invalid configuration, power_startup_delay_ms is negative");
		return -EINVAL;
	}

	if (cfg->has_power_gpio) {
		rc = gpio_pin_configure_dt(&cfg->power_gpio, GPIO_OUTPUT_INACTIVE);
		if (rc) {
			LOG_ERR("Failed to configure power_enable (%s.%u): %d",
				cfg->power_gpio.port->name, cfg->power_gpio.pin, rc);
			return rc;
		}
	}

	data->adc_seq = (struct adc_sequence){
		.channels = BIT(cfg->analog_channel),
		.buffer = &data->raw,
		.buffer_size = sizeof(data->raw),
		.oversampling = 4,
		.calibrate = true,
		.resolution = 14,
	};

	data->adc_cfg = (struct adc_channel_cfg){
		.channel_id = cfg->analog_channel,
		.gain = ADC_GAIN_1_6,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	};

	if (cfg->measure_internal_vdd) {
		data->adc_cfg.input_positive = SAADC_CH_PSELP_PSELP_VDD;
	} else {
		data->adc_cfg.input_positive =
			SAADC_CH_PSELP_PSELP_AnalogInput0 + cfg->analog_channel;
	}

	rc = adc_channel_setup(data->adc, &data->adc_cfg);

	if (rc) {
		LOG_ERR("Adc channel could not be setup");
		return rc;
	}

	return 0;
}

#define DT_DRV_COMPAT irnas_voltage_divider

#define VOLTAGE_DIVIDER_INIT(N)                                                                    \
	static struct divider_data divider_data_##N = {                                            \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(N))};                                \
                                                                                                   \
	static const struct divider_config divider_config_##N = {                                  \
		.analog_channel = DT_INST_IO_CHANNELS_INPUT(N),                                    \
		.measure_internal_vdd = DT_INST_PROP(N, measure_internal_vdd),                     \
		.power_startup_delay_ms = DT_INST_PROP(N, power_startup_delay_ms),                 \
                                                                                                   \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(N, upper_resistor_ohms),                          \
			   (.upper_resistor_ohm = DT_INST_PROP(N, upper_resistor_ohms), ))         \
                                                                                                   \
			IF_ENABLED(DT_INST_NODE_HAS_PROP(N, lower_resistor_ohms),                  \
				   (.lower_resistor_ohm = DT_INST_PROP(N, lower_resistor_ohms), )) \
                                                                                                   \
				IF_ENABLED(                                                        \
					DT_INST_NODE_HAS_PROP(N, power_gpios),                     \
					(.has_power_gpio = true,                                   \
					 .power_gpio = GPIO_DT_SPEC_INST_GET(N, power_gpios), ))   \
                                                                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(N, voltage_divider_init, NULL, &divider_data_##N,                    \
			      &divider_config_##N, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,  \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(VOLTAGE_DIVIDER_INIT)
