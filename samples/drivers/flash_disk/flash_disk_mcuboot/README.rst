.. _disk_flash-sample:

Disk Flash Sample
##############

Overview
********

This sample demonstrates how to use the disk flash driver in order
to emulate a flash on a disk like an SD-Card or a ramdisk. This flash
can then e.g. be used as a secondary partition for mcuboot.

Building and Running
********************

RAM-disk Example
================

The application will build for all targets which are compatible with ramdisk
and have at least 1kb of free RAM.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/flash_disk
   :board: native_sim
   :goals: build flash
   :compact:

Sample Output
-------------

.. code-block:: console

    *** Booting Zephyr OS build 4d678207b778 ***
    [00:00:00.000,000] <inf> main: Read/Write test successfull

west build -b nrf52dk_nrf52832 samples/drivers/flash_disk/flash_disk_mcuboot/ -DIMAGE_SECONDARY=ON
rm -r build
west build -b nrf52dk_nrf52832 samples/drivers/flash_disk/flash_disk_mcuboot/ --sysbuild -Dmcuboot_DTC_OVERLAY_FILE=${ZEPHYR_BASE}/samples/drivers/flash_disk/flash_disk_mcuboot/nrf52dk_nrf52832.overlay
