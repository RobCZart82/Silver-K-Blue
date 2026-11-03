# Upstream v0.5a átvételi leltár

Ellenőrzés: 2026-11-02. Blue alap: `338c7033d23a5c29000167eb11676622eb8b562f`. Összehasonlítás: eredeti v0.4.4a `4baaead06e23e2358729cca32b6b511c72022318` → [v0.5a](https://github.com/temaniak/Silver-K/tree/ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6) `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`. Az ellenőrzéskor az upstream main is erre a commitra mutatott.

**Nem teljes a v0.5a-integráció.** A #26–28 célzott változásokat és teszteket hozott át; a nagy natív vezérlőváltás nincs integrálva. A korábbi terv fő témákat tartalmazott, de nem minden alábbi részfeladatot nevezett meg külön. Ez a leltár ezeket is nyilvántartásba veszi; a „tervezett” nem jelent kész implementációt vagy engedélyezett automatikus teljes merge-et.

## Tételes állapot és következő ellenőrzés

| Upstream változás / forrás | Blue állapota | Teendő / átvétel feltétele |
|---|---|---|
| K-Station bemenet kimenettel közös pufferének védelme, nem véges minták kezelése (`silverKEngine.cpp`) | Részben átvéve #26: teljes mono részblokk másolása írás előtt, NaN/Inf → 0, hiányzó bemenet → 0 | Meglévő mono regressziós teszt megőrzése; sztereó külön feladat |
| K-Station bemeneti sorok túlcsordulásának számlálása és backend megnevezése (`silverDiagnostics.cpp`, K device) | #27 mainben, Blue-specifikus támogatottság-/hatókörjelzéssel | K továbbra is original-h8; ez nem natív migráció |
| Latenciateszt privát KS data/wave útvonalai (`silverLatency_test.cpp`) | #26 tesztparaméterezés mainben | Privát macOS és tényleges host teszt még szükséges; CI SKIP nem firmware-es PASS |
| KS indulás utáni sustain/key tulajdonosok törlése mind a 16 csatornán, CC64/123/120 (`ksDevice.cpp`) | Javítás nincs átvéve; #28 regressziós teszt mainben | Jelenlegi teszt nem reprodukálta a hibát. Sustain pozitív kontroll és mind a négy hangkimenet ellenőrzése a következő teszt-PR-ben; privát futtatás még hátravan. Opcionális post-restore négypartos/16 csatornás sustain-mátrix a #33-ban: helyi macOS arm64 H8/native firmware-es próbája PASS; CI és integráció függő. Hidegindításkori minden part/csatorna lefedettsége továbbra is feladat; csak igazolt szükség esetén célzott runtime javítás |
| Sztereó bemeneti busz és L/R mono keverés (`silverProcessor.cpp`, `silverKEngine.cpp`) | A #34 pufferadapter mainben; opcionális KEngine jobb bemenet a következő PR-ben; Processor/host busz és KS bekötés még nincs átvéve | Adapter mono/sztereó/alias/NaN/Inf/blokkhatár teszt ASan/UBSan PASS; tényleges busz, samplerate és mindkét modell hosttesztje továbbra is szükséges |
| KS teljes külső ADC hangút, időzített bemeneti sor, konverzió és két DSP táplálása (`ksDevice.cpp`, `ksAudioInput*`, DSP) | ADC konverter és önálló teszt #36 mainben; DSP RX0 sor már az alapban jelen van. Device sor/konverter bekötése #37-ben: helyi natív mono/sztereó/ellenfázis/alias/csend próba PASS; Processor/host még nincs bekötve | Konverter ASan/UBSan PASS; további realtime sorviselkedés, overflow és offline render vizsgálata szükséges |
| KS `input_overflows` diagnosztika | Nincs átvéve; Blue korrektül nem támogatottként jelzi | A KS hangbemenettel együtt; ne jelenjen meg hamis nulla mint működő mérés |
| Standalone alapértelmezett bemeneti busz és macOS mikrofonhasználati leírás (`silverProcessor.cpp`, plugin CMake) | Nem tekinthető teljesen átvett funkciónak | Külső hangbemenet integrációjával együtt ellenőrizni; macOS engedélykérés, mute/input settings és visszautasítás teszt |
| Külső bemeneti layout-/Standalone-/hostpróbák (`silverExternalInput_test.cpp`, `external_input_host_check.cpp`, `ksAudioInput_test.cpp`) | Upstream teljes tesztcsomag nincs átvéve | Új hangút mellé ROM nélküli layout/konverzió és valódi VST3/AU/Standalone próba; vocoder/filter/effect firmware-rel is |
| K-Station natív C++ vezérlő és alapértelmezett Native választás (`kstationLib`, `silverKEngine.cpp`) | Nincs átvéve | Külön migráció eredeti H8 összehasonlítással: MIDI, arp, patch/state, hang és CPU. Teljesítményelőnyt mérni kell, nem feltételezni |
| KS natív vezérlő/service-gráf, H8 összehasonlító források leválasztása (`ksLib`, `ksControllerSourcePartition.cmake`) | v0.5a változat nincs átvéve; a korábbi Blue KS native lehetőség nem azonos ezzel | Összefüggő függőségi egységként kezelni: boot/DSP, voice/velocity/glide, MIDI/SysEx, arp/timer, panel/state/store/restore. Eredeti H8 ellenpróba és privát firmware-es regresszió szükséges |
| `controllerReferenceAvailable` → `boardComparisonAvailable`, production selector natívra korlátozása (`silverEditor.cpp`, tesztek) | Nincs átvéve | Backend-migráció függősége, nem külön UI-javítás. Meglévő Blue oldalak és elrendezés változatlan marad |
| Workflow teszt összehasonlító board factory/CPU használata és opcionális `SILVER_TEST_DRUM_FLASH` | Nincs teljesen átvéve | Natív migrációhoz igazítani; drum flash fixture hiányát külön SKIP/korlátként jelezni; zero-wave nem mintahang-hűségi bizonyíték |
| Natív property/call/oracle tesztek és bővített CMake/CI célok | Nincs teljesen átvéve | Csak az integrált alrendszerek releváns tesztjeit bekötni; referencia és production függőségek elkülönítése, Windows + Universal 2 fordítás |
| K-Station kutató-/CPU-/paritásmérő eszközök (`tools/kstationResearch`) | Nincs átvéve | Hasznos GP-05 és natív migráció bizonyítékához; külön fejlesztői eszköz, privát firmware és mérési eredmény ne kerüljön kiadásba |
| macOS fejlesztői indítás `SILVER_K_DATA` továbbadása LaunchServices számára (`script/build_and_run.sh`) | Következő PR fejlesztési ágán megvalósítva | LaunchServices explicit környezeti továbbítás; Blue app/binary/log név javítva. Mindkét build-elrendezés és szóközös útvonal parancshelyettesítős próbája PASS; valódi macOS appindítás még kézi ellenőrzés |
| Kiadási verzió 0.5.0/0.5a, eredeti termékazonosítók és csomagolási módosítások | Nem másolandó automatikusan | Blue név, külön AU/VST3/bundle azonosítók és közös platformcsomag megőrzése. Verzióemelés/kiadás csak külön döntéssel |
| Persistence teszt Windows megjegyzésének pontosítása | Nincs önálló új runtime javítás | Ne számítsuk külön hiányzó funkciónak; szemantikai változás nélkül dokumentációs eltérés |

## Határok és ajánlott sorrend

1. Kis, független fejlesztői indítási javítás vizsgálata; privát tesztek reprodukálhatósága.
2. KS indulási teszt kiterjesztése; külön igazolni, kell-e a CC-tisztítás a jelenlegi motorhoz.
3. Külső hangbemenet képességvizsgálata, majd mono/sztereó + KS + host/engedély/diagnosztika összefüggő terv szerinti integrációja.
4. K és KS natív vezérlők külön migrációs lépcsői, referencia-összehasonlítás és CPU-mérés után.

A Standard/Retina/Legacy grafikák, oldalak és kezelőszervek elrendezése változatlan marad. A leltár nem engedélyez új GUI-t vagy kiadást. Egyszerre egy fejlesztési PR, minden integráció után releváns Windows/macOS ellenőrzések.

## Az audit bizonyítékának határa

A teljes változott fájllistát és az alkalmazás-/build-/tesztintegráció változásait összevetettük. A 2560 változott fájl döntő része a két vezérlőkönyvtárban van; ezek függő alrendszerként szerepelnek a leltárban. Ez nem minden natív eljárás soronkénti helyességbizonyítása, és nem teljes v0.5a firmware-/DAW-teszt. A meglévő Blue teszteredmények és korlátaik a [fejlesztési naplóban](11-Fejlesztesi-naplo-es-publikalasi-terv.md) találhatók. Az upstream baseline marad v0.4.4a, amíg a teljes új állapotot nem fogadtuk el.
