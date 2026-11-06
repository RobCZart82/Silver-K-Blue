# Private KS Board parity probe

Build the explicit `ksBoardParity_probe` target. It is excluded from default builds, public CTest and release packages. Arguments: KS OS, **decoded 524288-byte user Flash**, 129024-byte Wave ROM, and a fresh private output prefix. A KS All Data MIDI file is not raw Flash: use the production importer to obtain compatible data. Never publish these fixtures or output files.

The probe boots the Board, settles 8192 frames, sends a note, sustain/note-off/second-note, then sustain-off/all-sounds-off. It captures all 24 output streams over 40960 native frames, requires audible output, and saves canonical current-state SysEx, user Flash and MIDI output. Existing output files are rejected. Compare complete `.pcm`, `.state`, `.flash` and `.midi` files between frozen builds.

The current Blue Board reports its existing hybrid backend. It is **not an original-H8 oracle**. To compare upstream semantic native at commit `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`, compile the same source against that frozen ksLib with `KS_SEMANTIC_PROBE=1`. This requires the actual native owner and zero interpreted H8 steps before and after the passage; a backend label alone is insufficient. Keep dependencies, compiler configuration, stimuli and fixtures identical.

Local arm64 Release comparison matched all complete outputs: 3932160 PCM bytes, 270 state bytes, 263168 user-Flash bytes; MIDI output was empty on both sides, so it is not a positive MIDI-output test. Both peaks were 1568304. The private input Flash was projected from 600 programs/100 performances in the user's MIDI data using the production bank/address/global rules. A zero Wave ROM was used; this does not establish sampled-wave fidelity.

This bounded single-program Board passage does not prove all banks, four-part behavior, arp, Device/Processor state restore, DAW behavior, realtime capacity or CPU improvement. Native KS is not integrated into Blue by this tool. Wider reference and product integration checks remain prerequisites. Keep raw logs, PCM, state and Flash outside the repository and public Actions artifacts.

Optional final argument `fourpart` selects a separate bounded performance passage (`single` is the unchanged default). It selects performance bank/program, supplies the performance flags/mask in the actual SysEx program bytes, clones the known single program into four MIDI-channel parts, renders notes and all-sounds-off on channels 1–4, then verifies performance mode and active mask 15. Merely setting the C++ State.activeMask field is insufficient: stateMessage serializes the program bytes.

Local fourpart comparison: all 24 PCM streams (3932160 bytes), performance state (1038 bytes) and user Flash (263168 bytes) match exactly; both peaks 883066. Both MIDI outputs remain empty. This proves bounded aggregate output/state parity, not an independent audible-positive control for every part, all channel mappings or arbitrary performance programs. The initial fixture setup that omitted performance-selection/mask bytes failed the active-mask assertion; it was corrected without changing the runtime.

Use the final argument `cold-channels` to boot four fresh Boards from stored
Performance 0, with channel groups 1–4, 5–8, 9–12 and 13–16. Supply a placeholder
for PREFIX because this mode writes no captures. Each boot checks the stored
part mask and channel mapping and requires exact silence on the four DAC
outputs before host MIDI. Each part then needs audible note-on and sustained
note-off output, followed by exact silence after pedal release. The test sends
no host program changes, state restores, or all-sounds-off messages. A semantic
build also checks native ownership and zero interpreted H8 steps.

The optional `note-release` passage settles for 8192 Board frames, holds
note 60 for 48000 Board frames and captures 8192 frames after note-off. It
uses the program loaded from Flash. Compare the PCM and state files from
the two builds using the same private fixtures. This passage uses the
Board sample clock directly.

Use `device-release-128` or `device-release-512` for a direct Device comparison
at 48 kHz. The probe renders 65536 frames offline, sends note-on at frame
4800 and note-off at frame 52800, and checks for finite audible output and
zero underruns. It writes only PREFIX.pcm, with four interleaved native-endian
float samples per frame. Compare runs with identical private fixtures and
block sizes. These modes use the starting Flash program and Device defaults.

The `device-release-128-lead640` case keeps 128-frame host blocks and raises
the Device lead to the default for 512-frame blocks. Compare each backend
at the same setting. The lead also affects MIDI event scheduling.

The `device-release-128-delay384` case keeps the default 128-frame Device
lead and adds 384 frames to both MIDI offsets. This produces the same
scheduled event frames as the 640-frame lead case while keeping the
default audio buffer lead.
