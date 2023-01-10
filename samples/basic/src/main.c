
#include <device.h>
#include <devicetree.h>
#include <logging/log.h>
#include <voltage_divider.h>
#include <zephyr.h>

LOG_MODULE_REGISTER(main);

#define VOLTAGE_DIVIDER_DEV "VBAT"

void main(void)
{
	LOG_INF("Voltage divider test running on: %s", CONFIG_BOARD);

	const struct device *dev;
	int err;

	// get device binding
	dev = device_get_binding(VOLTAGE_DIVIDER_DEV);
	if (dev == NULL) {
		LOG_ERR("Device unknown (%s)", VOLTAGE_DIVIDER_DEV);
		return;
	}

	// take a sample
	while (1) {

		err = voltage_divider_sample(dev);
		if (err < 0) {
			LOG_ERR("voltage_divider_sample, err: %d", err);
		} else {
			LOG_INF("Voltage divider sample valeue: %d", err);
		}

		k_sleep(K_SECONDS(3));
	}
}
