#include <voltage_divider.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

const struct device *dev = DEVICE_DT_GET(DT_PATH(vbatt));

void main(void)
{
	LOG_INF("Voltage divider test running on: %s", CONFIG_BOARD);

	int err;

	while (1) {
		err = voltage_divider_sample(dev);
		if (err < 0) {
			LOG_ERR("voltage_divider_sample, err: %d", err);
		} else {
			LOG_INF("Voltage divider sample value: %d", err);
		}

		k_sleep(K_SECONDS(1));
	}
}
