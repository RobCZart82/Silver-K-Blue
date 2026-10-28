# X-Station controller template format

Static reconstruction from original XStationEditor-i386 SHA256 3dc8851aafc50fe424015eb8f4570491985ea68e702c384dde0419c2feb29da5. No hardware observations. REA/Ghidra evidence exported evidence.json. Root packet fixtures: 8208 bytes, 13-byte header +392 common bytes +150 records of52 +12 34 F7.

## Control records (offset405+52*n)
- 0..15: ASCII name, max16 (FUN_b5d04)
- 16: type; enums from FUN_4b058:0Disabled,1CC,2NRPN,3RPN,4SysEx,5MMC,6/7NoteOnOff,8BankSelect,9ProgramChange,10PitchBend,11DrumNote,12Aftertouch. Wire7 maps internal6 on read. Original serializer needs follow-up to distinguish6/7.
- 17: low;18:high. Reverse ranges allowed (native setters flag high<low).
- 19,20,21: flags, preserve
- 22: MSB;23: LSB/CC number, confirmed NRPNGUI FUN_134676
- 24: for type!=5: low nibble=channel-1 (1..16), high nibble preserved separately (channel group flags). For type5MMC: whole byte is MMC device ID, not channel.
- 25: for type!=5: default/snapshot value; type5MMC: command.
- 26..46: SysEx control data21 (first3 metadata then18 content); preserve until full decoder.
- 47: buttons: step=byte+1; pots ignore47..51. Final4 bytes ignored.

Do not reconstruct whole packet from decoded data: original parser ignores reserved bytes; serializer explicitly combines edited values with original raw bytes using XOR masks. Keep byte-for-byte unknown regions.

## Common fields
- 13..28: template name16
- 29: original program byte
- 30..42: manufacturer13
- 43: locals+10
- 44: keyboardchannel low nibble; bits4,5,6 routing flags mappedlocals+e,+c,+d (exact port semantic not pursued)
- 45,46,47,48: locals+6,+7,+8,+9
- 49: bit0->locals+11; invertedbit1->locals+12; invertedbit2->locals+f
- 50: override MIDI channel,0disabled1..16 override; GUI confirmed
- 51,52: other values, preserve

Synth_Template checkbox binds locals+12. Thus synth_template=(packet[49]&2)==0. Enable:byte49&~2;disable:byte49|2. This setting changes controller template Mod/Env3 behavior (help.html), not physical standalone synth mode.

## Key evidence
- Templateparser FUN_4cf16:ev_ba6d234de6301d1269afb29ec79f0a706319212e0aeecbb638228cc419dc26be
- RecordparserFUN_b5db0:ev_326093c175e0df1320d69766d92df70153b6424ae24486cbfaf9d5f112a70164
- CommoncontrolfieldsFUN_b5aae:ev_36d9853a779f405df90a38a7e64f90184eeb73026a59d7d298d1c75e47a7cc2b
- TypenamesFUN_4b058:ev_26de42aecbcc9f128d6d24018cac63ad8b4c0c09ff1f222587604cbd34ba130a
- CommonparserFUN_4c87a:ev_81521949f856bca739cfc45921b71d7e952cf010a7cd7ae9b13d01d64cd245d9
- SettingsrefreshFUN_1278e4:ev_992bc6697724ad3911575a3704802add506a15b6120bda0befbe982a36fcb993
- NRPNrefreshFUN_134676:ev_59abc04dc5ed30868321a438f2d4efbe470c6e246f24e1156afeb487e7e81e49
- Raw preserving serializerFUN_5ddf6:ev_d30e94a4d215382c00382385f816b467f0873d857f55eb798fa8c5807913937f

Remaining: type-specific constraints/physicalpotvsbutton index list, port flags, exactSysEx metadata, globalSynthByControl/MIDI fields and physicalmode switch not explored. Rendering reserved bytes as safe raw data should remain possible.

## Follow-up: SysEx and button fields (static disassembly)

Further evidence from `/private/tmp/xstation-analysis/disassembly.txt`, original SHA256 unchanged; REA was not reopened. See FUN_000954da ASCII parser, FUN_00095432 validator, FUN_000957cc formatter, FUN_000b5944 record serializer.

SysEx control metadata:
- +26: untouched by ASCII editor, preserve (meaning unresolved)
- +27: encoded length = body token count +1; count0..18 (valid editor input at least1)
- +28: DV position =1-based token index,0meansnoDV
- +29..46:18bodybytes, excludes F0/F7 framing; valid data bytes00..7F
- Text: space-separated two-digit hex and optional single DV token; storesDV byte7F. Formatter uses +28 to distinguishDV from literal7F.
- Preserve unused payload tail. Factory MS2000 example length9/position7 displays `42 30 58 41 35 00 DV 00`. Its following77byte is unused, not a required terminator.

Exact record kinds extracted from template parser into `control-kinds.json`:1meanspot/slider/touch,0meansbutton.150values. Button indices0based:
1,9,12,20,23,31,34,42,45,53,56,63,64,65,72,73,74,77,78,81,82,85,86,95,96,97,98,99,100,101,102,103,112,113,114,115,116,117,118,119,120,122,123,125,126,128,129,131,132,134,136,144,145,146,147,148,149.

Button action state0x50:0Normal,1Momentary,2Toggle,3Step. Verified Common.xrc names and native callback assignments:NormalFUN_13bde8 passes0,MomentaryFUN_13bd60passes1,ToggleFUN_13bcd8passes2,StepFUN_13bc50passes3 to CSetActionCommand.

Wire action flags:clearbyte19bits2and3andbyte20bit5; Normalsetsnone;Momentarysetsbyte19bit2;Togglesetsbyte19bit3;Stepsetsbyte20bit5. Preserveotherbits. Parserb53f6/b54d2,serializerb58ac/b577e. Step size byte47+1; UIrange1..64 (Common.xrc Button_Step_Size).

Note wire6/7: both named NoteOnOff and parserconverts7to6. Original serializerwritesinternal6. For buttons,type6serializerforceshighvaluebyte18to0; potbranch retainsmin/max. Exact purposeoflegacywire7 remainsunresolved, so preserveexistingtypeunlessexplicitlychanged.
