# irnas-voltage-divider-driver

This repository contains driver and basic test for voltage divider driver, based on Zephyr Battery Voltage Measurement sample. Driver uses Zephyr ADC infrastructure to measure the voltage of the device power supply.

## Setup

Before using the driver, you will need to install nrF SDK. Driver is compatible with NCS 2.0.

If you already have a NCS setup you can follow these steps:

1. To get the driver you need to update `<path to ncs>/ncs/nrf/west.yml`. First in the `remotes` section add:

   ```yaml
    - name: irnas
      url-base: https://github.com/irnas
   ```

2. Then in the `projects` section add at the bottom:

    ```yaml
    - name: irnas-voltage-divider-driver
        repo-path: irnas-voltage-divider-driver
        path: irnas/irnas-voltage-divider-driver
        remote: irnas
        revision: v1.0.0
    ```

3. Then run `west update` in your freshly created bash/command prompt session.
4. Above command will clone `irnas-voltage-divider-driver` repository inside of `ncs/irnas/`. You can now run samples inside it and use its voltage-divider driver code in your application projects.
5. To use driver in your project, add DTS entry .dts or overlay file. For example:

```dts
vbatt {
  label = "VBAT";
  compatible = "irnas,voltage-divider";
  io-channels = <&adc 0>;
  output-ohms = <10000>;
  full-ohms = <(10000 + 10000)>;
  power-gpios = <&gpio0 7 GPIO_ACTIVE_HIGH>;
  status = "okay";
};

```
