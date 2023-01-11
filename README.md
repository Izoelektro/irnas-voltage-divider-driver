# irnas-voltage-divider-driver

This repository contains driver and basic test for voltage divider driver, based
on Zephyr Battery Voltage Measurement sample. Driver uses Zephyr ADC
infrastructure to measure the voltage of the device power supply.

**IMPORTANT:** This driver is only suitable for Nordic chips, like `nrf52832`,
`nrf52840` and so on. This is due to the chip specific way how adc channels are
configured.

**IMPORTANT:** This driver hard codes adc gain, reference, `acquisition_time`,
adc resolution and some other adc settings. This is done in
`driver/voltage_divider/voltage_divider.c` in `voltage_divider_sample` function,
see assignment of `data->adc_seq` and `data->adc_cfg` structs.

## Usage

To use add below two snippets into your project's `west.yml` file and run `west
update``:

1. In `remotes` section, if not already added:

```yaml
- name: irnas
  url-base: https://github.com/irnas
```

2. In the `projects` section, select revision you need:

```yaml
- name: irnas-voltage-divider-driver
   repo-path: irnas-voltage-divider-driver
   path: irnas/irnas-voltage-divider-driver
   remote: irnas
   revision: v2.0.0
```

### Device tree

To enable this driver you need to add below snippet into your DTS or overlay
file. For information on how to properly configure this driver see
`dts/bindings/irnas,voltage-divider.yaml`

```dts
vbatt {
    compatible = "irnas,voltage-divider";
    io-channels = <&adc 1>;
    upper-resistor-ohms = <1000>;
    lower-resistor-ohms = <1000>;
    status = "okay";
};
```

### Shell

This driver also implements a shell interface for interacting with the voltage
divider device instance. To add it to your existing shell add below line into
your Kconfig fragment file:

```
CONFIG_VOLTAGE_DIVIDER_SHELL=y
```
