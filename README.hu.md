![Silver-K Blue](blue/Artwork/Silver-K-Blue-logo.svg)

# Silver-K Blue

A [temaniak/Silver-K](https://github.com/temaniak/Silver-K) nem hivatalos, saját Blue Standard és Blue Retina 4K felülettel továbbfejlesztett változata. Az eredeti szerzők munkájára épül; dokumentált alapja v0.4.4a.

## Letöltések

| Platform | Formátumok | Letöltés |
| --- | --- | --- |
| Windows x64 | VST3, Standalone | [1.0.0 letöltése](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/blue-v1.0.0) |
| macOS Universal 2 | AU, VST3, Standalone; arm64 + x86_64 | [1.0.0 letöltése](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/blue-v1.0.0) |

Az első közös kiadás, [1.0.0](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/blue-v1.0.0), 2026-11-07-én jelent meg, forrása `5adc19eccddc38335b7652be599766823a6d684c`. Windows/macOS ZIP-csomagokat, forrást és SHA-256 ellenőrzőösszegeket tartalmaz. Az 1.0.1 fejlesztés alatt áll, hibajavításokkal és választható összetevős telepítőkkel. A main és a CI-csomagok a kiadottnál újabb módosításokat is tartalmazhatnak.

## Telepítés és használat

Mindkét skin be van építve, a Setup listából választható. Új beállításnál a Retina az alapértelmezett; a mentett választás megmarad. A külső Skins mappa opcionális.

A Silver-K Blue külön pluginazonosítót kapott, ezért az eredeti Silver-K mellé telepíthető. A régi Blue-val mentett projektek a korábbi, közös azonosítóra hivatkoznak; nem váltanak automatikusan az új Blue-ra. Ezekhez őrizd meg a korábbi buildet.

Saját kompatibilis firmware szükséges; KS módban Wave ROM és program/performance MIDI-adat is kell. A nulla Wave ROM nem tartalmaz mintahullámokat. Gyártói firmware-t és gyári hangadatot nem mellékelünk. A macOS minimumcél 12.0; a build ad hoc aláírt és nem notarizált. A Windows build nincs tanúsítvánnyal aláírva.

- [Magyar telepítési útmutató](blue/Documentation/01-Telepites-es-hasznalat.md)
- [Angol telepítés](doc/INSTALL.md), [használat](doc/USER_GUIDE.md), [fordítás](doc/BUILD.md)
- [Fejlesztési napló és publikálási terv](blue/Documentation/11-Fejlesztesi-naplo-es-publikalasi-terv.md)
- [Változásjegyzék](CHANGELOG.md)

A Standard külső skin upstream v0.4.4a-val is használható. A Retina nagyobb képeihez Blue-kompatibilis bináris kell. A napi upstream-ellenőrzés külön javaslati ágat készít; nem olvaszt automatikusan main-be és nem publikál kiadást.

Forráslicenc: [GPLv3](LICENSE.md); [eredet és függőségek](THIRD_PARTY.md); [grafikai eredet és nyitott ellenőrzések](blue/Artwork/PROVENANCE.md). A történeti dokumentumok korábbi helyi csomagokra utalhatnak.

## Mappák és fájlok

- **ROMs:** firmware, Wave ROM és **KS All Data.MID** (KS program/performance indítóadat).
- **Patches és almappái:** további presetek, programok és bankok.
- **Skins:** külső skin-csomagok külön közvetlen almappákban; a beépített Blue skinekhez nem szükséges.

[Részletes keresési és betöltési szabályok](doc/DATA_FOLDERS.hu.md).

## Képernyőképek

A Main nézet Silver-K Blue Retina 4K v1.0 felülettel, 256 px-es forgópotméter-képkockákkal.

**K-Station — Main nézet**

![Silver-K Blue Main nézet K-Station OS 2.1.01 firmware-rel](blue/Artwork/Previews/k-station-retina-4k-main.png)

**KS-Rack — Main nézet**

![Silver-K Blue Main nézet KS-Rack OS 2.1.02 firmware-rel](blue/Artwork/Previews/ks-retina-4k-main.png)
