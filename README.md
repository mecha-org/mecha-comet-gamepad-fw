# Mecha Comet Gamepad Extension - Zephyr Firmware

[Zephyr](https://zephyrproject.org) firmware for the **Mecha Comet Gamepad Extension**, the gamepad
of the Mecha Comet. It attaches to the Comet with magnetic pogo pins and also works on its own over
USB-C as a gamepad on any USB host. It has buttons, a D-pad, a left analog stick and a Mecha Pivot
Pip pointing stick as the right stick.

Full documentation is coming. Until then, [`app`](app) has the firmware and
[`boards`](boards/mecha/comet_gamepad) has the board definition.

To flash, put the chip in its ROM bootloader (hold BOOT_PTA4 low while powering up) and use
[`blhost`](https://pypi.org/project/spsdk) (`pip install spsdk`) with the image from the build:

```
blhost -u 0x15a2:0x0073 flash-erase-all
blhost -u 0x15a2:0x0073 write-memory 0x0 build/zephyr/zephyr.bin
blhost -u 0x15a2:0x0073 reset
```

Licensed under the [BSD 3-Clause license](LICENSE).
