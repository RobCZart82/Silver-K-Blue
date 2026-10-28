# Silver-K Blue

Privát, önálló fejlesztési repository a Silver-K Blue Standard és Retina skinekhez, illetve a hozzájuk készített helyi felületi javításokhoz.

Upstream: [temaniak/Silver-K](https://github.com/temaniak/Silver-K), v0.4.4a, commit `4baaead06e23e2358729cca32b6b511c72022318`. Az eredeti projekt és szerzőinek munkájára épül; nem hivatalos kiadás. Eredeti ismertető: [UPSTREAM-README](doc/UPSTREAM-README.md). A Gearmulator eredet és a függőségek megjelölései a [THIRD_PARTY.md](THIRD_PARTY.md) fájlban maradtak meg.

## Aktuális állapot

- Silver-K Blue Standard v1.0: gyári v0.4.4a-val is használható 64 px / 128 kockás skin.
- Silver-K Blue Retina 4K v1.0: helyi javított buildet igénylő 256 px / 128 kockás skin.
- A produkciós forrás tartalmazza a legutóbbi elfogadott helyi felületi javításokat: Retina képbetöltés, faderskála, négysoros fő pontmátrix-LCD, natív lenyílóbetűk és navy hátterek, waveform-hover, inaktív Performance képek halványítása, kerekített gyorstippek.
- A skinek jelenleg a `blue/Skins` mappában, külső csomagként vannak. **A VST3-ba építés még következő fejlesztés**, ebben az első feltöltésben nincs megvalósítva.
- Ez a helyi 2026-10-25-i végleges csomag forrásellenőrzési alapja; újabb upstream fejlesztési ágakat nem emel be.

## Használat és fordítás

A `blue/Skins` két almappája a plugin Setup → Skins folder helyére másolható. A skinválasztóban a fenti végleges nevek jelennek meg.

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

Az eredeti GitHub Actions munkafolyamatok a `blue/Upstream-Workflow-Examples/workflows` mappában megőrzött példák. Az első feltöltés nem indít automatikus felhőfordítást; a saját CI beállítása későbbi feladat.
