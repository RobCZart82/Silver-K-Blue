# X-Station synth protocol findings

Sources:
- Official MIDI chart: https://fael-downloads-prod.focusrite.com/customer/prod/downloads/X-Station%20MIDI%20Implementation%20Chart.pdf (downloaded as midi.pdf)
- Official support: https://support.novationmusic.com/hc/en-gb/articles/206690209-Novation-X-Station-Troubleshooting-and-FAQs
- User-provided official 100 New Sounds archive, extracted under /private/tmp/xstation-analysis/100-new-sound2. Readme explains dump and audition behavior.
- KS candidate offset source: /Users/evgeniystrukov/Silver-K/source/novation/ks/ksJucePlugin/ksControls.h. These offsets are KS reverse-engineered values, NOT independently verified X-Station parameter offsets.

Observed individual synth frame (100 fixtures):
- 270 bytes total: 13-byte header, 256-byte seven-bit payload, F7.
- Exact common header: F0 00 20 29 01 42 7F 00 00 10 00 00 00.
- Name: payload151..166 (file164..179), ASCII, space-padded to16. Filenames can abbreviate/rename, so use internal field.
- Local supplied README explicitly confirms individual dumps load the temporary program buffer, can be auditioned, and require pressing hardware WRITE three times to persist.

Observed bank frame (100 frames in xstationbank.syx):
- Header F0 00 20 29 01 42 7F 01 00 10 00 02 pp, pp=0..99.
- README states bank upload overwrites100 programs in bank selected on hardware.
- Every bank frame matched its individual counterpart by embedded name. Payload0..220 matches exactly. Only221..255 differs; all100 differ237..255. Preserve unknown tail bytes. Do not use KS mirror packing that zeroes209..255.
- Header conversion to edit-buffer form (b7=0,b11=0,b12=0) is supported by corresponding individual fixtures but has not been tested on a connected device. Set exact observed header; retain payload.
- No request-current-patch packet has been verified. Support manual user-initiated dump receive.

parameters.json:
- 133 entries, document-backed CC/NRPN wiring, candidate shared-KS offline offsets, explicit evidence per entry.
- Every candidate range validated against100 supplied individual payloads; this is corroboration, not semantic/hardware proof.
- Null offset: live modwheel/breath only.
- Packed controls are changed through atomic alternative NRPN messages to avoid overwriting other live fields. NRPN value=base+logical value. For normal NRPNs, base=0. Send CC99=0, CC98=number, CC6=value.
- Original packed byte and candidate masks retained for offline file mutation; use masked update to preserve other fields.
- Arp pattern limited0..6 (drum7 is printed in red as KS-only); observed values are0,1,2,3,4,6.
- Added enum labels from KS catalog where available; labels are candidate shared-format terminology.

MIDI chart red text explicitly means KS-only. Removed CC7,14,15,33..37,77,95 and NRPN23..26,61,68,79,82,85..91,100 plus effects group. Official support says X-Station effects section parameters cannot be controlled via MIDI, although the chart leaves many effects messages black. Effects should remain unadvertised pending hardware verification.

Hardware setup needed later:
- Enter synth mode.
- Synth global menu -> Synth MIDI Src -> USB or MIDI IN.
- Device absent: no USB driver, patch delivery, audible behavior or hardware response verified.

Additional decoded material:
- xstation-os.bin: controller OS H8 image decoded from official XS3003 OS.syx. Header family02 01 differs from synth family01 42. Novation update stream has512 seven-bit bytes per block ->448 unpacked bytes, followed by16-bit byte swap. Result95872 bytes. Contains template-controller strings, no synth labels found. This decode is analytic, not intended for upload.
