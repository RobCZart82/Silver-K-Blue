> **Történeti kiadási jelentés — 2026-10-29.** A privát feltöltésre és az akkori Actions-korlátra utaló részek az eredeti eseményeket rögzítik. Aktuális fejlesztési állapot: [napló](11-Fejlesztesi-naplo-es-publikalasi-terv.md). A PR #1 későbbi javításai nem részei e binárisoknak.

# Első beépített skines kiadás — 2026-10-29

A közös, ellenőrzött forráscommit: `40b68eeebd8e2c7d4a92229aa25fcb1217a652cc`.

| Platform | Elkészült termékek | Privát letöltés |
|---|---|---|
| Windows x64 | VST3, Standalone EXE | [Windows build 3](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/windows-blue-3-1) |
| macOS Universal 2 | AU, VST3, Standalone APP; arm64 és x86_64 | [macOS build 2](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/macos-blue-2-1) |

Mindegyik tartalmazza a **Silver-K Blue Standard v1.0** és **Silver-K Blue Retina 4K v1.0** skint. A beépített skinek használatához nem kell külön Skins mappát telepíteni. A mellékelt külső skinek opcionális, szerkeszthető másolatok.

Új beállításoknál a Retina az alapértelmezett; a meglévő választásokat megtartjuk. A Setup listában a négy korábbi beépített kinézet után következik a két beépített Blue opció. Ha a külső Blue csomagokat már korábban telepítetted, azok a listában később külön is megjelenhetnek; a beépített opciók a lista felső részén vannak. Meglévő projektben válaszd ki a beépített verziót és mentsd el a projektet, ha annak használatára szeretnél átállni.

## Ellenőrzési eredmény

- A két GitHub-platformbuild sikeres; az öt termék elkészült és csomagolva lett.
- Mindkét CI-körben öt teszt sikeres, egy firmware-függő latency-reentry teszt SKIP státuszú, nulla teszthiba. A SKIP nem igazolja a firmware-es késleltetés- vagy hangtesztet.
- A beépített skinek fájl nélküli betöltése, négy oldala, Reload és hostállapot-visszatöltése ellenőrzött; a hangparaméterek változatlanok.
- Helyi macOS-on az editor, Performance editor, külső skin API és a teljes natív menü/export/reload regresszió sikeres.
- Az Intel-kódrész beépített skinjei Rosetta alatt sikeresek. Fizikai Intel Mac és Windows DAW hangpróba külön feladat.
- A 4 MiB-os erőforrásegységek összerakásakor minden eredeti skinfájl bájtra egyezik, rejtett segédfájlok nélkül; azonos GUI-méretnél a két teljes Main-előnézet pixelre egyezik a korábbi beépített változattal.
- A letöltött CI-csomagok ZIP-épsége, külső SHA-256 összege, beépített-skin metaadata és forráscommitja ellenőrzött.

Futások: [Windows](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/40900906956), [macOS](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/40900911467).

## GitHub Actions korlátozás

A fordítási feladatok sikeresek, de a külön Release-mentési feladatok nem indultak el. A macOS futás GitHub-hibaüzenete számlázási vagy költési keretkorlátozást jelzett; nem különítette el, hogy melyik ok áll fenn. A fiók Billing & plans beállításait a tulajdonosnak érdemes áttekintenie a további felhőfutásokhoz.

A kész csomagokat külön API-feltöltéssel megőriztük a fenti privát Release-ekben. Számlázási, fizetési vagy költési beállítást nem módosítottunk. A futási oldal piros összjelzése ezért a későbbi mentési feladat hibáját is mutathatja, miközben a build/teszt/csomagolás zöld.

A kiadás tesztkiadás: a következő átvételi lépés a szokásos DAW-ban, saját firmware-rel végzett működési és hangpróba. A csomagok nem települnek automatikusan, a gyári példány biztonsági másolatát és a host bezárását telepítés előtt őrizd meg.
