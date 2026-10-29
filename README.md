# Silver-K Blue

Privát, önálló fejlesztési repository a Silver-K Blue Standard és Retina skinekhez, illetve a hozzájuk készített helyi felületi javításokhoz.

Upstream: [temaniak/Silver-K](https://github.com/temaniak/Silver-K), v0.4.4a, commit `4baaead06e23e2358729cca32b6b511c72022318`. Az eredeti projekt és szerzőinek munkájára épül; nem hivatalos kiadás. Eredeti ismertető: [UPSTREAM-README](doc/UPSTREAM-README.md). A Gearmulator eredet és a függőségek megjelölései a [THIRD_PARTY.md](THIRD_PARTY.md) fájlban maradtak meg.

## Első beépített skines tesztkiadás

**[Windows x64: VST3 + Standalone](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/windows-blue-3-1)** · **[macOS Universal 2: AU + VST3 + Standalone](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/macos-blue-2-1)**

Mind az öt termék elkészült, mindkét Blue skin külön telepítés nélkül benne van. [Ellenőrzések, használat és a GitHub Actions fiókkorlátja](blue/Documentation/10-Elso-beepitett-kiadas.md). A csomagok forrása a `40b68eeebd8e` commit; a későbbi dokumentációs commitok nem változtatják a kiadott binárisokat.

## Aktuális állapot

- Silver-K Blue Standard v1.0: gyári v0.4.4a-val is használható 64 px / 128 kockás skin.
- Silver-K Blue Retina 4K v1.0: helyi javított buildet igénylő 256 px / 128 kockás skin.
- A produkciós forrás tartalmazza a legutóbbi elfogadott helyi felületi javításokat: Retina képbetöltés, faderskála, négysoros fő pontmátrix-LCD, natív lenyílóbetűk és navy hátterek, waveform-hover, inaktív Performance képek halványítása, kerekített gyorstippek.
- A két skin a közös felületi kódba van csomagolva, ezért Windows VST3/Standalone és macOS Universal 2 AU/VST3/Standalone alatt külön skintelepítés nélkül választható. Új beállításoknál a Retina az alapértelmezett; mentett választások és külső skinek továbbra is támogatottak. A grafikai forráscsomagok a `blue/Skins` mappában megmaradtak.
- Ez a helyi 2026-10-25-i végleges csomag forrásellenőrzési alapja; újabb upstream fejlesztési ágakat nem emel be.

## Használat és fordítás

A beépített skinek a Setup választójában közvetlenül elérhetők. A `blue/Skins` két almappája opcionálisan külső, szerkeszthető skinnek is használható.

Fordítás: [doc/BUILD.md](doc/BUILD.md). Példa macOS Universal 2 buildhez:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build --target silverK_VST3 -j 6
```

Az első konfigurálás hálózatról tölti a rögzített függőségeket. A repository nem tartalmaz kész VST3-at, nagy ZIP-archívumot, letöltött függőséget, személyes beállítást, Novation firmware-t vagy hangbankot. Az upstream `assets/ks-init-flash.bin` generált szintetikus teszt/indító adat, nem gyártói ROM; az eredeti asset-README és manifest írja le az eredetét.

## Fejlesztési anyagok

- [Aktuális fejlesztési terv](blue/Documentation/02-Aktualis-fejlesztesi-terv.md)
- [Későbbi upstream együttműködés](blue/Documentation/03-GitHub-egyuttmukodes.md)
- [Verziózás és eredet](blue/Documentation/04-Verziozas-es-eredet.md)
- `blue/Design/Masters`: grafikai mesterek és generálási leírások.
- `blue/Tests`: megőrzött felületi regressziótesztek.
- `blue/upstream-v0.4.4a.patch`: az eredeti tiszta alaphoz tartozó teljes helyi produkciós javítás.

A `blue/Documentation` lezáró csomagra hivatkozó leírásai a teljes helyi archívum szerkezetét követik. A 32 történeti csomag, a bináris és az ellenőrzési archívum helyben maradt; azok nem részei ennek a repositorynak. A forráskód licence [GPLv3](LICENSE.md); az eredeti szerzői megjelölések megmaradtak. A grafikai anyagok eredete külön dokumentált, publikus terjesztésük előtt áttekintendő.

A kézzel indítható **Silver-K Blue Windows x64** GitHub Actions VST3-at és önálló Windows-alkalmazást készít a privát repositoryban. [Indítás és letöltés](blue/Documentation/07-Windows-GitHub-Actions.md). Az eredeti workflow-k a `blue/Upstream-Workflow-Examples/workflows` mappában mintaként maradnak meg.

## Upstream követés

Naponta ellenőrizzük az eredeti Silver-K main ágát és stabil kiadását. Új változásból privát ellenőrzési ág készül; ütközés esetén jelentés, automatikus main-merge nélkül. Az összehasonlító linkről kézzel nyitható PR. [Frissítési folyamat](blue/Documentation/08-Upstream-frissites.md).

## Beépített skinek és desktop build

[Integráció, tesztelés és mind az öt célváltozat](blue/Documentation/09-Beepitett-skinek.md). A Windows és macOS Actions munkafolyamat kézzel indítható; sikeres buildből külön privát Release készülhet.
