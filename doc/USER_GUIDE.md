# User guide

This guide covers file loading, the editor and the patch library.

## First startup

1. Open Silver-K in Standalone or on an instrument track in your DAW.
2. In Setup, choose Change data folder.... Use a permanent location. The default is Documents/Silver-K.
3. For KS, use KS factory data to select one MIDI file containing the complete program and performance data, or select several compatible MIDI files together. Conflicting records are rejected.
4. KS also requires Wave ROM. A zero bank is created and selected automatically. It supports generated oscillator waves; sounds that need sampled waves may be silent or incomplete. Use Wave ROM to select a compatible bank if needed.
5. Use Load firmware to select a compatible firmware BIN. The top selector switches between available firmware images. Wait for loading to finish.
6. In Standalone, select an audio output and MIDI input. In a DAW, enable track monitoring and send MIDI to the plugin.

KS cannot start without firmware, Wave ROM and program data. K-Station uses its own firmware and does not require KS data.

Required files are copied into ROMs when selected from another location. Their originals remain unchanged. Keep the selected assets available when reopening projects. Rescan ROMs updates the firmware list after files change.

## Main folder

Blue Setup groups controls into Files and folders, Appearance, and Audio and operation. Appearance → Interface size opens the existing size menu (50%, 75%, 100%, or Custom 50–150%); the right-click menu remains available. The Skins path row replaces the duplicate Skins folder button. Legacy keeps its original arrangement.

Setup shows the full Patches, ROMs and Skins paths in read-only fields. Select and copy a path, or use its Open button to reveal the folder. Change data folder changes their shared parent; it does not relocate the three subfolders independently.

| Folder | Purpose |
| --- | --- |
| ROMs | K-Station/KS firmware BIN, compatible KS Wave ROM, and KS All Data.MID (program/performance startup data) |
| Patches | Additional patch banks and individual presets |
| Skins | External skin packages, one package per immediate subfolder |

Wave ROM and KS factory data are KS-only selectors. In K-Station mode they are disabled, with tooltips explaining why; K-Station preset banks still belong in Patches.

Firmware discovery scans ROMs recursively (Setup → Rescan ROMs). Automatic KS All Data discovery checks only files directly in ROMs: .mid/.midi with “all data” in the name, validates their content, and only selects one when no KS data is already selected. Other names or multi-file sets can be selected with Setup → KS factory data. Patches folder scans include subfolders; continuous change watching is not yet implemented. See the [detailed folder map (Hungarian)](DATA_FOLDERS.hu.md).

Blue Standard and Blue Retina 4K are built in and selectable in Setup without installing external files. New settings default to Retina; saved choices are preserved. External copies are optional and may appear separately in the list.

Put each skin package in its own subfolder of Skins, then click Setup → Reload and select it in the skin list. Load skin also opens a package's skin.json or an older Silver-K layout JSON directly. Export template creates an editable copy of the built-in skin. Reload (F5) applies file changes; an invalid reload keeps the working interface. See [Skin packages](SKINS.md) for the authoring format.

## Patch library

Open Patches. Add folder scans a folder and its subfolders; Add file connects a bank or preset. Supported formats include SYX, MIDI, compatible FXB/FXP and ZIP archives.

Click a row or use the arrow keys to load a patch. Use the source, model, bank and tag filters to narrow the list. Click a column heading to sort. Previous and Next on Main follow the current browser selection.

Rescan refreshes connected sources. Remove source clears the selected source and its cached rows without deleting files from disk. Removed sources stay absent after reopening. An explicit Rescan restores removed paths still inside Patches; reconnect external sources with Add file or Add folder.

Identical patches from a ZIP and its extracted files appear once when viewed together. Different sounds with the same name remain separate.

Save current saves the current sound. Export .syx exports the selected library entry. Save tags stores tag and favourite edits. Allow pending edits to reach the engine before saving.

## Controls

Drag a knob to change its value. Click it without moving to show the value in place of its caption. Click the caption to enter a number. The caption returns after a short delay. Mod Env and Amp Env retain their visible values.

Blue LED arcs start at the centre for bipolar parameters: negative values extend left, positive values right. The PW selector changes the parameter and caption of the PW knob.

O opens additional controls for its section. Program uses Options. LFO headings identify which LFO is being edited.

## Parts and drums

K-Station has one part. KS supports four parts. On Perf, enable Performance and use tabs 1–4 to select the editing destination. Check each part's MIDI channel, active state, keyboard range and output routing.

A Program loads into the selected part. A Performance restores the four-part setup. Main 1+2 is the stereo output; Aux 3+4 requires a separate host output.

With a KS Drum Map loaded, enable Drum Edit and play a key to select its sound for editing. Save current exports the edited map, including its shared effects.

## Clock and buffer

Program → Options → Clock source applies to the whole instance:

- Host follows DAW tempo and transport. Without host timing, incoming MIDI Clock is accepted.
- Internal uses the Tempo control and ignores host and incoming MIDI clock timing.
- MIDI Clock follows incoming MIDI clock and transport.

LFO and effect Sync settings remain separate from the clock source.

Setup → Audio buffer sets the plugin's scheduling reserve. Balanced is the default. A larger reserve can reduce dropouts and adds latency. The audio-device buffer is set in the DAW or Standalone audio settings.

Setup → **Show advanced KS settings** reveals the KS controller, **KS: use two CPU cores (experimental)** and Demo Song. This section starts collapsed; opening or closing it does not change the audio settings. The controls retain their normal availability for the loaded firmware. The two-core option applies to KS only and can be changed while the firmware is loaded. Performance improvement is not guaranteed; total CPU usage may increase. It is off by default. The choice is saved for new instances and in DAW/Standalone state.

## Saving a session

Save the DAW project or Standalone state to retain the current sound, parts and clock choice. Firmware and Wave ROM remain external files. Use Save current for a separate reusable patch.

## Common issues

- Engine does not load: check the required files in Setup and the loading message.
- No sound: check MIDI channel, track monitoring, part state and output routing. A zero Wave ROM does not contain samples.
- Missing patches: clear browser filters and check the selected source. Use Rescan when files have changed.
- Updated AU still shows the previous interface: fully restart the DAW and rescan the plugin.

## Blue skin selection

Auto uses the embedded Blue Retina 4K skin. Blue Standard and Blue Retina 4K can also be selected explicitly; Silver-K Legacy retains the original interface. All modes adapt available controls to the loaded firmware. Auto does not select a different skin for each firmware. New settings default to Auto; explicitly saved preferences can still select another skin. Old DAW projects whose Auto state predates the Blue Auto version marker restore Legacy; new Auto project states restore Blue. Historical Silver-K/KS Grey/Classic project values remain readable and appear under the Legacy selector entry. External skin loading remains available.
