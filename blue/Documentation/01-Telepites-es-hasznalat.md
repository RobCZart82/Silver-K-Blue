# Silver-K Blue — telepítés és használat

Ez a jelenlegi Windows és macOS beépített skines csomagok útmutatója. A korábbi helyi archívumok mappanevei nem telepítési követelmények.

## Frissítés előtt

Zárd be a DAW-ot. A Blue és az eredeti Silver-K pluginazonosítója azonos: a korábbi teljes példányt mentsd a host keresési útvonalain kívülre, majd a teljes bundle-t cseréld. Ne maradjon több azonos példány beolvasott mappákban. Az automatikus telepítés nem része a ZIP-nek.

## Platformok

| Platform | Csomagbeli termék | Hely |
| --- | --- | --- |
| Windows x64 | Plugins/VST3/Silver-K.vst3 | C:\\Program Files\\Common Files\\VST3\\ |
| Windows x64 | Standalone/Silver-K.exe | Állandó mappából indítható |
| macOS | Plugins/VST3/Silver-K.vst3 | ~/Library/Audio/Plug-Ins/VST3/ |
| macOS | Plugins/AU/Silver-K.component | ~/Library/Audio/Plug-Ins/Components/ |
| macOS | Standalone/Silver-K.app | Applications vagy más állandó hely |

64 bites host kell. A macOS Universal 2 arm64 és x86_64 kódot tartalmaz, minimumcélja macOS 12.0; ez nem minden fizikai Intel gépen végzett próba. A macOS build ad hoc aláírt és nem notarizált; a Windows build nincs tanúsítvánnyal aláírva. Hiányzó Windows Visual C++ futtatókörnyezet külön telepítendő.

Indítsd újra a hostot, és frissítsd a pluginlistát.

## Beépített és külső skinek

A Standard v1.0 és Retina 4K v1.0 a Setup listában külön telepítés nélkül választható. Új beállításnál a Retina az alapértelmezett, mentett projektek választása megmarad.

A mellékelt Skins mappa szerkeszthető külső másolatokat tartalmaz. Ezek opcionálisak; telepítésük után külön bejegyzésként jelenhetnek meg. Külső csomagonként külön almappát használj a Setup által megnyitott Skins könyvtárban, majd Reload. A Standard upstream v0.4.4a-val is használható; a Retina 256 px képeihez kompatibilis Blue bináris kell.

## Első indítás

A Setup oldalon saját kompatibilis firmware-t válassz. K-Station saját firmware-t használ; KS módban program/performance MIDI-adat és Wave ROM is kell. Az automatikusan létrehozott nulla Wave ROM a generált oszcillátorhullámokhoz használható; mintahullámot igénylő hangszínek hiányosak vagy némák lehetnek. Gyártói firmware és gyári hangadat nincs a csomagban.

Standalone esetén válassz hangkimenetet és MIDI-bemenetet; DAW-ban engedélyezd a sávmonitorozást. Bővebben: [User guide](../../doc/USER_GUIDE.md).

A jelenlegi kiadások és ellenőrzési korlátok a [főoldalon](../../README.hu.md) és a [kiadási jelentésben](10-Elso-beepitett-kiadas.md) találhatók. A ROM nélküli teszt nem helyettesíti a saját firmware-rel végzett DAW-os hangpróbát.
