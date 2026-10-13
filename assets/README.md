# Synthetic KS user flash

`ks-init-flash.bin` is a reproducible 524288-byte **user flash substitute** created by Silver-K. It is not a dump of a physical instrument. It contains our own simple `Silver Init` saw programs and four-part `Silver Multi` initial performances, plus startup settings.

No OS code, DSP firmware, sampled wave data or factory patches are included. Reserved/OS regions remain erased (`0xff`). The emulator inserts the user's separately supplied OS into its in-memory flash when booting.

Select this file in Setup's KS data field when using KS firmware. Sample-dependent oscillators remain silent without the user's external wave bank.

Release archives also contain a separate `ks-wave-zero.bin`, generated as 129024 zero bytes by `tools/package_release.py`. Select that file in **Wave ROM**, and select this Init Flash in **KS factory data**. The zero Wave ROM and test KS Data serve different purposes; neither is the original OS. See [English installation instructions](../README.md#ks-data-and-wave-rom).

Regenerate without any ROM or factory MIDI:

```sh
c++ -std=c++17 tools/create_init_flash.cpp -o /tmp/silver-create-init-flash
/tmp/silver-create-init-flash /tmp/ks-init-flash.bin
cmp assets/ks-init-flash.bin /tmp/ks-init-flash.bin
```

The generator uses the control catalog's field layout and numeric defaults, then sets our own sound and part settings. Its only input is the output pathname.
