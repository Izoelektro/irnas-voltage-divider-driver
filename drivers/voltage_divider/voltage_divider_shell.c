/**
 * If CONFIG_VOLTAGE_DIVIDER_SHELL is set, this file will add
 * voltage divider related commands to the shell
 *
 */

#include <zephyr.h>

#include <device.h>
#include <shell/shell.h>

#include <voltage_divider.h>

#include <stdlib.h>

// this function is called when the `voltage_divider get` subcommand is  invoked
static int cmd_vd_get(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	// get device binding
	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(shell, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	// take a sample
	err = voltage_divider_sample(dev);
	if (err < 0) {
		shell_error(shell, "voltage_divider_sample, err: %d", err);
		return -err;
	}

	shell_print(shell, "%s voltage: %d", dev->name, err);

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

// create a dinamic set of subcommands. This is called every time
// "voltage_divider get" is invoked in a shell and will create a list of devices
// with @device_name_get
SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

// the help string for the get subcommand
#define VD_GET_HELP "Take a voltage divider sample. Syntax:\n<device_name>"

// all subcommands of command "voltage_divider"
SHELL_STATIC_SUBCMD_SET_CREATE(sub_vd,
			       SHELL_CMD_ARG(get, &dsub_device_name, VD_GET_HELP, cmd_vd_get, 2, 0),
			       SHELL_SUBCMD_SET_END);

// this is the root level "voltage_divider" command,
// it has the array `sub_vd` as subcommands
SHELL_CMD_REGISTER(voltage_divider, &sub_vd, "Voltage divider commands", NULL);
