# Upstream v0.5a átvételi leltár

Leltár frissítése: 2026-11-05 (upstream összehasonlítás: 2026-11-02). Blue alap: `338c7033d23a5c29000167eb11676622eb8b562f`. Összehasonlítás: eredeti v0.4.4a `4baaead06e23e2358729cca32b6b511c72022318` → [v0.5a](https://github.com/temaniak/Silver-K/tree/ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6) `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`. Az ellenőrzéskor az upstream main is erre a commitra mutatott.

**Nem teljes a v0.5a-integráció.** A #26–28 célzott változásokat és teszteket hozott át; a K és KS natív motor külön, alapból kikapcsolt fordítási opcióval integrálva, teljes elfogadása még folyamatban. A korábbi terv fő témákat tartalmazott, de nem minden alábbi részfeladatot nevezett meg külön. Ez a leltár ezeket is nyilvántartásba veszi; a „tervezett” nem jelent kész implementációt vagy engedélyezett automatikus teljes merge-et.

## Tételes állapot és következő ellenőrzés

| Upstream változás / forrás | Blue állapota | Teendő / átvétel feltétele |
|---|---|---|
| K-Station bemenet kimenettel közös pufferének védelme, nem véges minták kezelése (`silverKEngine.cpp`) | Részben átvéve #26: teljes mono részblokk másolása írás előtt, NaN/Inf → 0, hiányzó bemenet → 0 | Meglévő mono regressziós teszt megőrzése; sztereó külön feladat |
| K-Station bemeneti sorok túlcsordulásának számlálása és backend megnevezése (`silverDiagnostics.cpp`, K device) | #27 mainben, Blue-specifikus támogatottság-/hatókörjelzéssel | K továbbra is original-h8; ez nem natív migráció |
| Latenciateszt privát KS data/wave útvonalai (`silverLatency_test.cpp`) | #26 tesztparaméterezés mainben | Privát macOS és tényleges host teszt még szükséges; CI SKIP nem firmware-es PASS |
| KS indulás utáni sustain/key tulajdonosok törlése mind a 16 csatornán, CC64/123/120 (`ksDevice.cpp`) | Javítás nincs átvéve; #28 regressziós teszt mainben | Jelenlegi teszt nem reprodukálta a hibát. Sustain pozitív kontroll és mind a négy hangkimenet ellenőrzése a következő teszt-PR-ben; privát futtatás még hátravan. Opcionális post-restore négypartos/16 csatornás sustain-mátrix a #33-ban: helyi macOS arm64 H8/native firmware-es próbája PASS; #33 mainben. Hidegindításkori minden part/csatorna lefedettsége továbbra is feladat; csak igazolt szükség esetén célzott runtime javítás |
| Sztereó bemeneti busz és L/R mono keverés (`silverProcessor.cpp`, `silverKEngine.cpp`) | A #34 adapter, #35 KEngine jobb bemenet, #37 KS Device és #38 mono hostbekötés mainben. Sztereó Processor/busz #39 mainben, mindkét modell helyi pozitív kontrollja PASS | Adapter mono/sztereó/alias/NaN/Inf/blokkhatár teszt ASan/UBSan PASS; tényleges busz, samplerate és mindkét modell hosttesztje továbbra is szükséges |
| KS teljes külső ADC hangút, időzített bemeneti sor, konverzió és két DSP táplálása (`ksDevice.cpp`, `ksAudioInput*`, DSP) | ADC konverter és önálló teszt #36 mainben; DSP RX0 sor már az alapban jelen van. Device sor/konverter bekötése #37-ben: helyi natív mono/sztereó/ellenfázis/alias/csend próba PASS; Device #37 mainben; mono Processor #38 és sztereó #39 mainben, helyi natív firmware-es próba PASS | Konverter ASan/UBSan PASS; további realtime sorviselkedés, overflow és offline render vizsgálata szükséges |
| KS `input_overflows` diagnosztika | KS Device számláló #37 mainben; Processor riport #38 mainben, helyi támogatottság/hatókör/érték próba PASS | A KS hangbemenettel együtt; ne jelenjen meg hamis nulla mint működő mérés |
| Standalone alapértelmezett bemeneti busz és macOS mikrofonhasználati leírás (`silverProcessor.cpp`, plugin CMake) | Standalone alapértelmezett mono bemenet és Blue macOS mikrofonleírás #40 mainben; generált plist ellenőrizve, valódi engedélykérés kézi teszt marad | Külső hangbemenet integrációjával együtt ellenőrizni; macOS engedélykérés, mute/input settings és visszautasítás teszt |
| Külső bemeneti layout-/Standalone-/hostpróbák (`silverExternalInput_test.cpp`, `external_input_host_check.cpp`, `ksAudioInput_test.cpp`) | Célzott Blue Device/Processor hangpróbák #37–39 mainben; ROM nélküli buszregresszió #41 mainben; 44,1/48/96 kHz-es privát bemeneti kontroll #42 mainben PASS. Az opcionális AU/VST3 hosteszköz #43 mainben, helyi arm64 direct-input wrapperpróba mindkét modellen PASS. A teljes hosttesztcsomag nincs átvéve | Új hangút mellé ROM nélküli layout/konverzió és valódi VST3/AU/Standalone próba; vocoder/filter/effect firmware-rel is; a #44 célzott javítása megőrzi a DSP B ADC RX0 értékét az A TX1 segéd-DAC helyett; K-Station és KS H8/native filter/envelope privát arm64 próba PASS, teljes latency regresszió PASS; #44 mainben (`b91c3cc`). A #45 vocoder signal/mute kontroll mainben (`8c71d94`), privát arm64 H8/native PASS. #47–48 mainben: helyi arm64, faliórához ütemezett Processor signal/mute próbák; korábbi hiányzó mintakeretek miatt nincs általános realtime kapacitásgarancia. Valódi DAW/fizikai bemenet és Windows/Intel futtatás továbbra is hátravan |
| K-Station natív C++ vezérlő és alapértelmezett Native választás (`kstationLib`, `silverKEngine.cpp`) | #52 mainben (114f8aa), explicit fordítási opt-in; OFF az alapérték, normál build H8. Állapot-visszatöltés megőrzi és ténylegesen ellenőrzi a backendjét | Külön migráció eredeti H8 összehasonlítással: MIDI, arp, patch/state, hang és CPU. 2026-11-04: izolált upstream H8/native előmérés készült, általános CPU-előny nem igazolt; részletek lent. Blue H8/upstream H8/native 12 forgatókönyves helyi hang/flash/MIDI/patch/clock paritás PASS (tools/kstationParity); Device állapot H8 → native → H8 helyi körpróba byte-azonos egy program/két CC esetén (kstationDeviceState_test); teljes Processor/DAW state és integrációs teszt továbbra is szükséges |
| KS natív vezérlő/service-gráf, H8 összehasonlító források leválasztása (`ksLib`, `ksControllerSourcePartition.cmake`) | A fagyasztott ab91ba1 szemantikus forrásegység elkülönített ksNative/ksLib átvétele #55 mainben (8bb92b1), SILVER_KS_SEMANTIC_CONTROLLER=OFF alapértékkel; nyilvános fejlécútvonal és #56 tesztforrás-illesztés javítva (6ed41b5). A korábbi Blue KS native lehetőség nem azonos ezzel | Összefüggő függőségi egységként kezelni: boot/DSP, voice/velocity/glide, MIDI/SysEx, arp/timer, panel/state/store/restore. Blue hybrid ↔ upstream szemantikus Board single/fourpart PCM/state/Flash paritás PASS; a #55–56 ABI-javítások utáni opt-in arm64 Processor regresszió PASS (2026-11-05, 6ed41b5). Eredeti H8 Board ellenpróba, teljes bank/arp és CPU-mérés továbbra is szükséges; GUI-változás és alapértelmezett motorváltás nincs |
| `controllerReferenceAvailable` → `boardComparisonAvailable`, production selector natívra korlátozása (`silverEditor.cpp`, tesztek) | Nincs átvéve | Backend-migráció függősége, nem külön UI-javítás. Meglévő Blue oldalak és elrendezés változatlan marad |
| Workflow teszt összehasonlító board factory/CPU használata és opcionális `SILVER_TEST_DRUM_FLASH` | Nincs teljesen átvéve | Natív migrációhoz igazítani; drum flash fixture hiányát külön SKIP/korlátként jelezni; zero-wave nem mintahang-hűségi bizonyíték |
| Natív property/call/oracle tesztek és bővített CMake/CI célok | Nincs teljesen átvéve | Csak az integrált alrendszerek releváns tesztjeit bekötni; referencia és production függőségek elkülönítése, Windows + Universal 2 fordítás |
| K-Station kutató-/CPU-/paritásmérő eszközök (`tools/kstationResearch`) | Nincs átvéve | Hasznos GP-05 és natív migráció bizonyítékához; külön fejlesztői eszköz, privát firmware és mérési eredmény ne kerüljön kiadásba |
| macOS fejlesztői indítás `SILVER_K_DATA` továbbadása LaunchServices számára (`script/build_and_run.sh`) | #31 mainben | LaunchServices explicit környezeti továbbítás; Blue app/binary/log név javítva. Mindkét build-elrendezés és szóközös útvonal parancshelyettesítős próbája PASS; valódi macOS appindítás még kézi ellenőrzés |
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

## 2026-11-04 — K-Station natív migráció elővizsgálata

Blue vizsgálati alap: `0079460621c44b46dcad205cb7e1a490296217fb`; upstream rögzített alap: `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`. A kutatóeszközök izolált helyi példányban futottak; sem a natív vezérlő, sem az eszközcsomag nincs még átvéve a Blue termékbe.

A clean/observer/detailed referenciaellenőrzés idle, held1, held8, notes, sweep és arp esetén pontos PCM/RAM/flash és riportegyezést adott. Az opcionális program esetben szintén pontos az egyezés és mindhárom futás exit 0, de a közös tesztscript beállítási kapuja hibát jelez: a programváltás a warmup alatt visszaállítja az envelope 62/65 bájtjait 2/40 értékre, a script viszont 0/0 értéket vár. A teljes sorozat ezért nem minősíthető PASS-nak; a hibás kaput külön kell tisztázni. Ez önmagában nem Blue runtime hibabizonyíték.

A külön CPU-sorozat ugyanazon upstream Release arm64 bináris original és native módját váltott sorrendben mérte: 8 másodperc mért szakasz, 2 másodperc warmup, három ismétlés, 256 mintás faliórához ütemezett szintetikus host. Teljes processz user+system CPU / leadott hangidő, egy CPU-mag százalékában:

| Forgatókönyv | H8 medián | Native medián |
|---|---:|---:|
| idle | 52,46% | 52,34% |
| held8 | 53,15% | 53,11% |
| notes | 52,99% | 52,63% |
| clockburst | 49,65% | 50,35% |

Mind a 24 futás teljesítette az eszköz funkcionális riportkapuját, a native ismétlések teljes RAM-checksumja egyezett, a native értelmezett H8 lépésszáma nulla; dropped_frames és paced-duration hibák száma nulla. A CPU-sorozat a H8/native teljes RAM-egyezést nem követeli meg, és nem byte-onkénti hang-/állapotparitási bizonyítás. A blokkonkénti deadline-mérés ebben a sorozatban ki van kapcsolva.

A rövid első mérésben látott nagyobb held8/notes CPU-különbséget a hosszabb megerősítő sorozat nem igazolta. Nincs kimutatott általános gyorsulás; emiatt nem állítunk teljesítményelőnyt és nem váltunk automatikusan natív alapértelmezésre. Következő feltétel: Blue eredeti H8 elleni szélesebb, reprodukálható MIDI/arp/patch/state/hang összehasonlítás, majd külön integrációs és platformellenőrzés. Ez nem DAW-, fizikai hangkártya-, Windows- vagy Intel-bizonyíték. A privát firmware és nyers mérési állományok helyben maradnak; GUI, Blue azonosítók és kiadási verzió változatlan.
