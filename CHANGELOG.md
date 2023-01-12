# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.0.0] - 2023-01-12

### Added

-   west.yml, so samples can be built as stand alone.
-   GitHub Action files for basic release process.

### Changed

-   The way how driver is configured in the device tree files, see 
    `dts/bindings/irnas,voltage-divider.yaml` for more info. Additional propery 
    for measuring internal vdd was added.
-   No need to specify to set `CONFIG_VOLTAGE_DIVIDER` manually, this is now 
    done automatically, if proper node is added.
-   This driver can now be used in NCS `v2.2.0`.
-   Formatting in source files.

## [v1.0.1]

### Fixed

-   Reading of power-gpio property from DTS

## [v1.0.0]

### Added

-   Voltage divider driver

[Unreleased]: https://github.com/IRNAS/irnas-voltage-divider-driver/compare/v2.0.0...HEAD

[2.0.0]: https://github.com/IRNAS/irnas-voltage-divider-driver/compare/v1.0.1...v2.0.0

[v1.0.1]: https://github.com/IRNAS/irnas-voltage-divider-driver/compare/v1.0.0...v1.0.1

[v1.0.0]: https://github.com/IRNAS/irnas-voltage-divider-driver/compare/828e69b754eea559f6c2ee2ce877975840dc17ae...v1.0.0
