#ifndef _VOLTAGE_DIVIDER_H_
#define _VOLTAGE_DIVIDER_H_

#include <zephyr/device.h>

/**
 * @brief Measure the voltage_divider voltage
 *
 * @param dev The voltage divider to measure
 * @return int The voltage in millivolts, or a negative error code
 */
int voltage_divider_sample(const struct device *dev);

#endif /* _VOLTAGE_DIVIDER_H_ */
