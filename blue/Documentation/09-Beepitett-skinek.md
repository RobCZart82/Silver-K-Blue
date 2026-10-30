# Beépített Blue skinek — desktop fejlesztési cél

Az első teljes célmátrix:

| Platform | Termék |
|---|---|
| Windows x64 | VST3, Standalone EXE |
| macOS Universal 2 | AU, VST3, Standalone APP |

## Működés

A Standard v1.0 és Retina 4K v1.0 képei és JSON-beállításai a közös programkönyvtárba fordított erőforrások. A build determinisztikus ZIP-eket állít elő, amelyeket legfeljebb 4 MiB-os fordítási egységekben JUCE BinaryData tárol. Ez korlátozza egy-egy generált C++ adatforrás méretét a Windows-fordító számára. A rejtett rendszerfájlok kimaradnak. A futó program ezeket memóriából olvassa; nem bontja ki őket és nem telepít másolatot a Skins mappába.

Mindkét név megjelenik a Setup skinválasztójában. Új beállításoknál a Retina az alapértelmezett. Korábban mentett Auto, Classic, külső Package/Custom és más választások megmaradnak; régi, skinválasztás nélküli hostállapot továbbra is Auto módra áll vissza. A beépített választás hostprojektben visszatölthető és F5/Reload útján újratölthető fájlútvonal nélkül. A külső skinbetöltés és a régi beépített kinézetek továbbra is elérhetők.

A grafikai skinverziók v1.0-k maradtak, mert a végleges képi anyag nem változott. A build pontos forrását a csomag build-info.json fájlja és a Release commitja rögzíti. A plugin azonosítója továbbra is az eredeti Silver-K-é; a csomag nem telepít automatikusan a meglévő példányra.

## Tesztelés

Az új silverEmbeddedSkin_test a két memóriából betöltött skin nevét, képeit, mind a négy oldal renderelését, Reload működését, host-state visszaállítását és a hangparaméterek változatlanságát ellenőrzi izolált adatkönyvtárban. Nem igényel firmware-t vagy külső skinmappát. A régi silverSkin_test tovább ellenőrzi a külső skin API-t.

A sikeres fordítás és a ROM nélküli felületi teszt nem egyenértékű teljes DAW-/hangminőségi teszttel. A végleges átvételhez külön Windows és macOS hostpróba, saját firmware és hangbank, valamint mentés/visszatöltés ellenőrzése kell. A Universal 2 csomag mindkét architektúrát tartalmazza; fizikai Intel gépen végzett próba külön feladat.

## Fordítás

Windows: a Silver-K Blue Windows x64 Actions-workflow VST3-at és Standalone-t épít. macOS: a Silver-K Blue macOS Universal 2 workflow AU-t, VST3-at és Standalone-t épít. Mindkettő kézi indítású, és sikeres futás után opcionális Release-mentést kínál.

Helyi macOS:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build --target silverK_AU silverK_VST3 silverK_Standalone silverEmbeddedSkin_test silverSkin_test --parallel 4
SILVER_K_DATA=/teljes/izolalt/tesztmappa ctest --test-dir build --output-on-failure -R '^(silver_embedded_blue|silver_skin_packages)$'
```

A beépített erőforrások miatt a binárisok nagyobbak. A képek ettől nem kapnak új vizuális effektet, és a hangmotor működése nem változik.
