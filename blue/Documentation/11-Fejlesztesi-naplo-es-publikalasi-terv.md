# Silver-K Blue — fejlesztési terv és munkanapló

Utolsó frissítés: **2026-11-05**

## Aktuális összesítés — 2026-11-02

Ellenőrzött main: `538de73` (#28 beolvasztása után). Az alábbi összesítés az aktuális státusz; a későbbi dátumozott munkabejegyzések a fejlesztés közbeni állapotot őrzik, a bennük szereplő „fejlesztési ág”, „ellenőrizendő” vagy „nem futott” nem írja felül ezt a táblát.

| Egység | Integráció | Ellenőrzés és határ |
|---|---|---|
| #24 audit, #25 dokumentáció | Mainben | Import-, webes billentyűzet-, forráseredet-javítások és GUI-megőrzési elvek |
| [#26](https://github.com/RobCZart82/Silver-K-Blue/pull/26) bemeneti védelem, SMF-kompatibilitás, tesztbővítés | Main `8e145188799bca5a9cb9b48429bc541bfaa0743c` | Javított PR-fej `23a5d40`: Windows/macOS/web PASS; 11 ROM nélküli teszt. Privát Windows persistence/latencia referencia- és natív KS-vezérlővel PASS |
| [#27](https://github.com/RobCZart82/Silver-K-Blue/pull/27) K-Station bemeneti diagnosztika | Main `72531ed0750cb25c51c2ef355ad2e7b9397f0df3` | Összevezetett PR-fej `db8672716ff5f0c8faa611ae98c7fbb58b64c7cd`: [Windows](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41436917239), [macOS](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41436917263), [web](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41436917230) PASS. A workflow már 12 ROM nélküli tesztet választ ki; privát K-Station számláló-/session-próba helyben PASS |
| [#28](https://github.com/RobCZart82/Silver-K-Blue/pull/28) KS indítási hangállapot-teszt | Mainben, #28 beolvasztva | Összevezetett fej `e075d353757053bfc4d0e4d5f3af872f03f97857`: [Windows](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41444798083), [macOS](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41444797963), [web](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41444798082) PASS. A firmware-es ág CI-ben SKIP; helyi Windows referencia/natív próba PASS |

A #27 utáni main `72531ed` Windows (41443754456), macOS (41443754232) és web (41443754303) futása PASS. A #28 összevezetett fejének mindhárom ellenőrzése PASS; ezért a PR beolvadt (`538de73`). Az új main külön futásait ez nem minősíti automatikusan sikeresnek.

### Következő lépések és megmaradt korlátok

- GP-01 részben kész: a felsorolt tesztek már CI-ben vannak; a teljes lefedettségi felmérés nyitott.
- GP-02/GP-03 részben igazolt: izolált Windows firmware-es mentés/indulás/latencia és natív KS-ismétlés sikeres. A teljes többpéldányos, DAW-os, fizikai MIDI- és macOS firmware-es elfogadás nincs lezárva.
- #28 a #27 utáni mainnel összevezetve és zöld CI után beolvasztva. A teszt a jelenlegi szerkesztési csatornát fedi le, nem minden Performance-partot/sustain-állapotot. A beragadt induló hang a vizsgált esetben nem reprodukálható; upstream CC64/123/120 tisztítás nem került át, motorhiba-javítás nem állítható.
- GP-04: ez az egyeztetés frissíti az aktív státuszokat; a történeti kiadási és grafikai feljegyzéseket megőrzi.
- GP-05 méréssel igazolt szűk keresztmetszetre vár; GP-06 automatikus Patches-figyelés továbbra is opcionális terv. KS audio-bemenet/sztereó fogadás külön képességvizsgálatot igényel.

A privát próbák K-Station 2.101 és KS 2.102 firmware-t, KS All Data fájlt és generált 129024 bájtos nullás hullámállományt használtak. Ez nem igazolja a gyári minták hanghűségét. A nyilvános firmware-es teszt SKIP-je nem PASS. GUI/skin-változtatás, teljes upstream-merge, új tag vagy kiadás nem része ennek a munkának.

## Történeti munkabejegyzés: upstream átvétel — első fejlesztési egység, 2026-11-02

Alap: Blue main `5c9556fcaa7072b9f6eeb9652e598ab84962ea47`; célzott upstream forrás: temaniak/Silver-K `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6` (v0.5a). A felhasználóval elfogadott, változatlan GUI-t megkövetelő terv alapján ez az első, korlátozott implementációs lépés; nem a teljes 0.5 Alpha átvétele.

- **Megvalósítva a fejlesztési ágon:** a K-Station meglévő mono bemenete legfeljebb 256 mintás, stacken tárolt blokkba másolódik a resampler meghívása előtt. Az upstream bemeneti védelméből átvett elv megőrzi az inputot közös host input/output puffer esetén, a NaN/Inf mintákat nullára cseréli. A buszelrendezés, controller, paraméterek, skinek és GUI változatlanok.
- **Új ROM nélküli regresszió:** `silver_audio_input_safety`, külön és bal/jobb kimenettel közös bemeneti pufferrel, leválasztott bemenettel, NaN/Inf értékekkel, kimeneti őrmintákkal és 0/1/32/63/64/255/256/257/511/1024 mintás blokkokkal. Ez a bemeneti adapter tesztje, nem teljes firmware-es audio-elfogadás.
- **GP-01 részleges megvalósítás:** a Windows/macOS CI a korábbi hat végrehajtott ROM nélküli teszt mellé bevonja az új bemeneti tesztet és négy meglévő tesztet: `silver_patch_workflow`, `silver_persistence_and_rescan`, `silver_skin_packages`, `silver_resampler_realtime`. Így 11 ROM nélküli teszt szerepel a kiválasztásban. A teljes tesztlefedettségi felmérés ettől még nem lezárt.
- **Külön firmware-es eredmény:** a `silver_latency_reentry` önálló, kifejezetten firmware-t igénylő CI-lépésben szerepel, magán-ROM nélkül várható SKIP eredménnyel. A preset-/persistence-tesztek CI-ben csak a ROM nélküli ágaikat igazolják; privát firmware-es részeik nem válnak ettől elfogadottá.
- **Upstream tesztparaméterezés:** a latenciateszt átveszi a `SILVER_TEST_KS_DATA` és `SILVER_TEST_KS_WAVES` környezeti változók kezelését. A saját firmware és adatok nem kerülnek a repository-ba vagy a publikus CI-be.

Helyi bizonyíték: az új bemeneti regresszió MSVC C++17 `/W4 /WX` fordítással és futtatással **PASS**. Whitespace-ellenőrzés **PASS**. A teljes platformfordítás és a kibővített tesztlista eredménye a fejlesztési PR CI-jében ellenőrizendő; e bejegyzés még nem állít platform-PASS-t. GUI-/hangteszt saját firmware-rel: **NEM FUTOTT**.

Következő külön lépések maradnak: KS indítási hang/sustain tisztítás célzott reprodukciója és regressziója; háttérdiagnosztika bővítése; KS audio-bemenet és sztereó fogadás külön tervezése. Natív vezérlőcsere nincs ebben a fejlesztési egységben. Új grafikai elrendezés, kezelőszerv, tag vagy Release nem készül.

### A kibővített CI első futásának eredménye és javítása

A `77721f1` első Windows/macOS futása sikeresen fordult, de mindkét platformon 9/11 ROM nélküli teszt volt sikeres. Az új `silver_audio_input_safety` átment. Két korábban nem bevont regresszió feltárt eltérést:

- `silver_skin_packages`: a teszt a Custom méretválasztást még három preset után kereste. A jelenlegi 50/75/100/125% menühöz négy lépés szükséges. Csak a teszt navigációja javítva; a GUI és a menü változatlan.
- `silver_persistence_and_rescan`: a szigorúbb SMF-parser megszüntette a V-Station ismert, 21 bájtos nevet kihagyó hosszmezőjének kompatibilitását. A parser most kizárólag a pontos Novation/K-Station fejléccel, 141-es deklarált hossz mellett 162 bájtos, teljes, hétbites payloaddal, érvényes névvel és záró F7-tel rendelkező esetet fogadja el; a track határán soha nem olvas túl. Külön natív regresszió ellenőrzi az elfogadást, idegen gyártó hibás hosszának és csonkolt névnek az elutasítását.

A friss SMF-regresszió helyi MSVC-fordítása és futása **PASS**. A javított teljes platformtesztek eredménye az új PR-futásokban ellenőrizendő; az első futások **FAIL** eredményét nem minősítjük át sikeresnek, és a hibázó tesztek a CI-ben maradnak.

## Aktuális fejlesztési döntés — 2026-11-02: a GUI megőrzése

**Felhasználói követelmény:** a jelenlegi skinek és grafikai felület megőrzendők. A további terv nem tartalmazhat grafikai átrendezést, új panelt vagy kezelőszervet, illetve a meglévő elemek helyének, méretének, megjelenésének és az oldalak elrendezésének módosítását. A követelmény a Standard, Retina és Legacy nézetre egyaránt vonatkozik. A jelenlegi méretválasztás és működő kezelési módok megmaradnak.

A dokumentációfrissítés alapja: main `5c9556fcaa7072b9f6eeb9652e598ab84962ea47`. A [PR #24](https://github.com/RobCZart82/Silver-K-Blue/pull/24) már beolvadt; az alábbi auditbejegyzés nyitott PR-ra vonatkozó mondata a korabeli állapotot őrzi. Ez a szakasz felülírja a korábbi fejlesztési javaslatokat, ahol azok a GUI megváltoztatását feltételezték. A korábbi grafikai munkák történetét megőrizzük.

### Aktív terv és lezárási feltételek

Az alábbi táblázat a feladatok tartalmát és lezárási feltételeit rögzíti; megvalósítási állapotuk a fenti aktuális összesítésben található. A sorrend előbb a meglévő működés igazolását, utána a célzott háttérfejlesztést követi.

| ID / sorrend | Feladat | Lezárási feltétel | GUI-korlát |
|---|---|---|---|
| GP-01 / 1 | A már meglévő regressziós tesztek és a CI szűrőinek összevetése; a firmware nélkül futtatható preset-, állapotmentési/Rescan-, külsőskin- és resamplertesztek bevonása | Windows/macOS futási lista és pontos PASS/FAIL/SKIP eredmény; a firmware-es tesztrészek külön felsorolva | Kizárólag teszt/build/workflow; nincs felületi változtatás |
| GP-02 / 2 | Mentés/visszatöltés és életciklus ellenőrzése: K/KS projekt, több példány, hiányzó/áthelyezett adatok, firmware-váltás | Izolált adatmappás automatikus eredmények, majd a szükséges saját firmware-rel és DAW/Standalone környezetben rögzített elfogadás | Meglévő vezérlők és állapotformátum kompatibilitásának megőrzése |
| GP-03 / 3 | Kis/változó hangblokkok és frekvenciaváltás: előbb a meglévő lefedettség vizsgálata, majd a hiányzó 1/32/63/64/255/256/257 mintás esetek | Célzott memóriabiztonsági, végeskimeneti és állapotmegőrzési ellenőrzés; az alkalmazhatatlan kombinációk indoklása | Audio-/MIDI-belső javítás; nincs új beállítási felület |
| GP-04 / 4 | Dokumentáció és eredetadatok egyeztetése a tényleges kóddal, kiadási folyamatokkal és tesztekkel | Egyértelmű aktuális állapottábla; történeti szakaszok jelölése; Blue/upstream revíziók és tesztbizonyíték külön | Csak dokumentáció; nincs grafikai erőforráscsere |
| GP-05 / 5 | Teljesítményjavítás csak kimért szűk keresztmetszet esetén | Azonos bemenettel előtte/utána mérés, helyességi és hangkimeneti regresszió; host callback blokkolásának/allokációjának vizsgálata | Meglévő diagnosztika használata, új panel/gomb nélkül |
| GP-06 / 6 | Opcionális Patches-változásfigyelés a meglévő Rescan mellett | Háttérben összevont feldolgozás; kiválasztás, szándékosan leválasztott források és nagy könyvtár kezelése; nincs audio-/UI-megakadás | A meglévő böngésző elrendezése és vezérlői változatlanok; ha ez nem tartható, nem része a tervnek |

### Ellenőrzési hiányok és fejlesztések külön kezelése

A `silverLatency_test.cpp` firmware hiányában 77-es SKIP kóddal tér vissza. Például a [426e59c Windows-futásában](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41395891478) hét kiválasztott tesztből hat futott sikeresen, a `silver_latency_reentry` kimaradt. A sikeres workflow ezért nem zárja le a firmware-es latencia- és életciklus-elfogadást. Már létező teszt esetén előbb annak futtatását/lefedettségét kell rendezni, nem azonos célú új tesztet írni.

Minden aktív feladatnál külön rögzítendő: megvalósítás (tervezett/PR/main), automatizált ellenőrzés (commit és futáslink, PASS/FAIL/SKIP), valamint használati elfogadás (platform/host/firmware és eredmény, vagy NEM FUTOTT). E három állapot egyike sem helyettesíti a másikat.

### Kivett és feltételes javaslatok

- A natív Undo/Redo, A/B összehasonlítás, új oldalak, panelek, gombok, dekorációk és skinátrendezések nem részei az aktív tervnek.
- Külső audio-bemenet bővítése vagy upstream átvétel csak a jelenlegi GUI megtartásával kerülhet külön tervbe. Előbb képességenkénti eltéréslista szükséges: a kódban már van KS natív vezérlő és mono külső bemeneti út; a „0.5 nincs integrálva” történeti mondat nem jelenti minden ilyen képesség hiányát. Ez nem igazolja a teljes upstream kiadás átvételét.
- Nem kerül be automatikus, teljes upstream-merge. Új paramétert, módosult kezelőszervet vagy elrendezést igénylő upstream változás nem vehető át ezen terv részeként.
- Az X-Station webes alkalmazás külön munkafolyamat. További protokoll-/hardverellenőrzése nem jelent felhatalmazást a natív Silver-K Blue GUI megváltoztatására.

A GP-01 első implementációs egysége a #26/#27 részeként mainbe került; GP-02 és GP-03 további lefedettségi felmérése erre épülhet. Ez a dokumentációs frissítés nem indít beolvasztást vagy kiadást.

## Korábbi kiegészítés — 2026-11-02: repository-audit javításai

A [PR #24](https://github.com/RobCZart82/Silver-K-Blue/pull/24) a 2026-11-02-i célzott audit négy megállapítását javítja. Alapcommit: `3ae4505aaa1de7bb9ee2f26ee66e238c98a4d016`; javításokat tartalmazó commit: `426e59c898c795bccc0af03ac6f981cf33eef8c3`. A PR e bejegyzés készítésekor nyitott, nincs beolvasztva. A lentebbi „aktuális” szakaszok korábbi, dátumozott pillanatfelvételek; ez a kiegészítés az újabb állapot.

### Megvalósított javítások

| Auditpont | Korábbi hiba | Javítás és lefedettség |
|---|---|---|
| 1 — X-Station build | Két nem követett `demo-*.syx` importja miatt a tiszta checkout Vite- és portable-buildje is leállt. | A szerkesztő üres könyvtárral indul, saját SYX/ZIP importálható. A kötelező demóimportok és beolvasó pluginok eltávolítva, a követett hordozható HTML újragenerálva. A külön webes CI buildet, teszteket és a HTML-forrás egyezését ellenőrzi. |
| 2 — natív SMF/SysEx import | A patch-böngésző az F7 continuation esemény státuszát lezárásnak tekintette, így a csomagokra bontott dump csonkán jutott az ellenőrzéshez. | Határellenőrzött SMF track-/eseményfeldolgozás, F0/F7 csomagok összefűzése, metaesemények és running status kezelése. Hibás/csonkolt fájl nem publikál részleges patchlistát; a nyers SYX út megmarad. A fájlalapú API is ezt a feldolgozót használja. Regressziók a `silver_framework_safety` tesztben. |
| 3 — webes tesztbillentyűzet | A `z` lenyomása után Shift mellett érkező `Z` felengedés nem küldött note-offot. | A fizikai billentyűhöz a ténylegesen indított hang tartozik; Shift vagy kiosztásváltás után is azt állítja le. Külön tesztek a módosult felengedésre, ismétlésre, szövegbevitelre és visszaállításra. |
| 4 — forráseredet | A régebbi csomagoló a Blue HEAD SHA-ját upstream repository-URL-be illesztette. | A `package_release.py` a Blue snapshotot a Blue repository-hoz linkeli; az upstream baseline külön, a `blue/upstream-state.json` repository-/commitadataival szerepel. Ez a metadata a forrássnapshot része; külön regressziós teszt ellenőrzi a két revízió szétválasztását. |

Az X-Station webes szerkesztő külön alkalmazás; az üres induló könyvtár nem a natív plugin presetkönyvtárának módosítása. E munkában új upstream-verzió integrációja és kiadáspublikálás nem történt.

### Ellenőrzési bizonyíték — `426e59c`

- **PASS, helyi:** X-Station 25 teszt, Vite build és portable build. **SKIP:** egy külső hivatalos fixture-adatot igénylő teszt.
- **PASS, helyi:** a Python-eszközök mind a 16 unittest tesztje; repository-audit és whitespace-ellenőrzés.
- **PASS, helyi MSVC C++17:** a tényleges `midiToSysex.cpp` és az új SMF-regressziók elkülönített harnessben. A fájlolvasás helyettesített volt; ez a memóriabeli parser ellenőrzése, nem teljes natív alkalmazásteszt.
- **PASS, GitHub CI:** [X-Station web editor — 41395891667](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41395891667), beleértve a tiszta buildet és a követett HTML egyezését.
- **PASS, GitHub CI:** [Windows x64 — 41395891478](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41395891478), natív build, ROM nélküli regressziók és csomagolás.
- **PASS, GitHub CI:** [macOS Universal 2 — 41395891680](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41395891680), natív build, ROM nélküli regressziók és csomagolás.

A fenti futások a javításokat tartalmazó `426e59c` commithoz tartoznak, nem a későbbi dokumentációs commithoz. A fizikai MIDI-eszközös próba, DAW-/hangteszt és minden firmware-rel végzett elfogadás **NEM FUTOTT** e javítási körben. A CI sikere nem kiadási vagy beolvasztási engedély. Új tag vagy Release nem készült.

Újrafuttatási parancsok és a csomagolók közti különbség: [Fordítás és forrásátadás](06-Forditas-es-atadas.md). Rövid változáslista: [CHANGELOG-HU](../CHANGELOG-HU.md).

## Korábbi kiegészítés — 2026-11-01

Ez a kiegészítés felülírja a lentebbi történeti állapotjelzéseket. PR #1–#21 beolvadt; main: `144bf20235a124dfa01dc5665e2347e5083ea20b`. A hozzá tartozó Windows (#62, run 41343709287) és macOS (#61, run 41343709273) build sikeres. A #19 Blue terméknevet és külön pluginazonosítókat, valamint használati javításokat hozott; #20 a README-logót igazította; #21 a Setup, ikon, LFO és képernyőképek javításait tartalmazza. A korábbi „nyitott PR / nincs automatikus merge” megjegyzések az akkori munkafázist írják le. A 0.5 Alpha nincs integrálva. A CI sikere nem teljes hanghűségi vagy minden DAW-ra kiterjedő elfogadás.

### Külső hibajelentés értékelése

A felhasználótól kapott `silver-k-blue-bug-list.txt` statikus jelentését célzott forrásolvasással ellenőriztük. A súlyossági címkék nem jelentenek bizonyított hibát.

| Pont | Forrás alapján megállapított állapot |
|---|---|
| macOS include | A MacOSDefaults.cmake belső CMAKE_HOST_APPLE feltétele védi a beállításokat. Nem igazolt platformhiba. |
| Pufferek / 256 minta | A min(256,count-offset) kezeli a rövid utolsó blokkot. Nyers mutatóból nem állapítható meg a tömb kapacitása; konkrét hibás hívást nem mutat a jelentés. |
| Mintavételi frekvencia | A prepareAudio elutasítja a nem véges/nem pozitív host-frekvenciát; a készülék frekvenciája pozitív konstans. A feltételezett nullával osztás nem igazolt. |
| Minimum 64 | Belső előkészítési méret, nem a host pufferhosszának átírása. Kis blokkokkal ellenőrizendő. |
| Status szálkezelése | Az olvasás zárolt; a részlet önmagában nem bizonyít adatversenyt. |
| Konstruktor kivétele | A rebuildK elkapja és státuszba írja; engine nélkül a processBlock némít. |
| Offline előkészítés | Publikálás előtt történik; stopEngines megvárja az aktív audiohívásokat. Konkrét párhuzamos hibafolyamat nincs igazolva. |
| Függőségek | Konkrét hibás CMake-függőség vagy linkerhiba nincs megnevezve. Platform-CI megtartandó. |

### Felvett tesztelési feladatok — még nem végrehajtva

- [ ] Előbb a meglévő határérték-, életciklus- és latenciatesztek lefedettségének ellenőrzése; csak a hiányzó esetek hozzáadása.
- [ ] 1, 32, 63, 64, 255, 256 és 257 mintás blokkok: rövid utolsó blokk, kimeneti tartományok és memóriabiztonság.
- [ ] Host-frekvencia és pufferméret váltása: állapotmegőrzés, engine újraépítése, véges és ésszerű latencia; hibás frekvencia elutasítása.
- [ ] K-Station/KS firmware-váltás lejátszás közben: nincs összeomlás, beragadt hang vagy tartós Loading állapot. Hiányzó/hibás firmware esetén értelmes hiba és néma kimenet.

Ezek tervezett regressziós ellenőrzések, nem bizonyított HIGH hibák. A jelentés alapján audio- vagy GUI-kódot nem írtunk át. Javítás reprodukálható eltérés vagy konkrét forrásbizonyíték alapján indokolt.

### Upstream 0.5 Alpha — megvizsgált jelölt

A [v0.5a](https://github.com/temaniak/Silver-K/releases/tag/v0.5a), forráscommit `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`, új natív K/KS C++ vezérlőket, külső mono/sztereó audio-bemenetet és kapcsolódó MSVC/tesztfüggőségi javításokat tartalmaz. Az upstream main a vizsgálatkor `722f65c`, amely a korábbi alaphoz képest csak .gitignore-változást hozott. A kiadási tageket is figyelni kell.

Állapot: összehasonlítva, átvételre érdemes jelölt, de nincs integrálva és Blue-kompatibilitásra elfogadva. Külön integrációs lépcső, platform-, firmware-, hostállapot- és audio-bemeneti ellenőrzés szükséges. A Linux PR #3 külön nyitott upstream javaslat.

Felhasználói korlát: a Blue GUI meglévő lapjai és elrendezése, saját termékneve és pluginazonosítói megőrzendők. Esetleg szükséges GUI-módosítás előtt külön egyeztetés kell. Ez a naplófrissítés nem valósít meg új funkciót, upstream-integrációt vagy Release-t.

## Aktuális összefoglaló — 2026-10-31

Ez a szakasz az aktuális állapot; a lentebbi dátumozott bejegyzések történeti bizonyítékok. A repository nyilvános, PR #1–#17 beolvadt. Legutóbb ellenőrzött PR #17: `bae54f9007232017ecea0463e30f0b6f1f3441fb`, Windows és macOS ellenőrzése sikeres; main: `23a5ea2`. A GitHub Release-lista a frissítéskor üres; új közös kiadás még nincs publikálva. A sikeres CI nem teljes körű hibamentességi vagy hanghűségi igazolás.

| Tétel | Aktuális állapot |
|---|---|
| H01–H09 és A01–A04 javítások | Main-ben, kötelező platformellenőrzések sikeresek; PR #1/#3/#5 |
| Upstream-frissítés | Nyilvános repository-val működik; nincs automatikus main-merge |
| Közös Release-folyamat | PR #6 beolvadt: Windows/macOS/forrás és SHA256SUMS egy kiadásban; publikálás hátravan |
| Mappadokumentáció | PR #7 beolvadt; ROMs/Patches/Skins tényleges keresési szabályai dokumentálva |
| Auto Blue és Legacy | PR #8 beolvadt; Auto → Blue Retina, külön Standard/Retina/Legacy választás; régi projektek migrációja tesztelve |
| Setup új elrendezése | PR #11/#12/#14/#15 mainben: útvonalak, Haladó KS-rész, három panel, közvetlen méretmenü; KS-adatválasztók és demógomb elérhetősége PR #16/#17-ben javítva |
| Automatikus Patches-figyelés | Javasolt/tervezett; jelenleg kézi Rescan, nem új automatikus működés |
| Grafikai eredet és teljes történeti leltár | Részleges; FinaleOnOff upstream forrásmegjelölése rögzítve |
| Felhasználói elfogadás | Korábbi macOS REAPER AU/VST3 beszámoló megvan; az új kiadás külön használati tesztet igényel |

### Következő logikus feladatok

1. Az elkészült Setup használati ellenőrzése valódi K-Station/KS firmware-rel és a célrendszereken; automatikus ROM nélküli ellenőrzései sikeresek.
2. Az új állapotból sikeres, azonos commitú Windows/macOS csomagok közös kiadása forrás-ZIP-pel és ellenőrzőösszegekkel; letöltési linkek és release-manifeszt aktualizálása.
3. Az új csomagok DAW/Standalone használati próbái, külön Windows és macOS eredményekkel.
4. Eredetleltár és történeti tartalomellenőrzés lezárása. Patches-változásfigyelés csak külön célzott fejlesztésként.

### A01–A04 javítások

- A01: hibás/nem létező tárolóba írás elutasítása a StreamBufferben; hibás alfolyam-írás regresszió.
- A02: sikertelen JobGroup enqueue visszaállítja a számlálót és értesíti a várakozókat; leállított munkasor utáni wait regresszió.
- A03: forEach általános callbackkel működik, az elemet és callbacket érték szerint őrzi; késleltetett végrehajtás a helyi konténer megszűnése után is tesztelve. A callback saját referenciafogásainak élettartama továbbra is a hívó felelőssége.
- A04 / SK-002: PNG/JPEG/GIF fejlécméret ellenőrzése dekódolás előtt, legfeljebb 64 Mi pixel/kép és összesen 128 Mi pixel/skin (RGBA névleg 256/512 MiB, dekóder és cache többlet nélkül). A 256×32768 Retina strip támogatott. Ismeretlen/hibás képfejléc elutasítva.
- Helyi ellenőrzés: 13 Python-teszt sikeres; natív framework regresszió sikeres. A platformellenőrzések és a skin-integráció eredményét a lezáró bejegyzés rögzíti.

A lentebbi korábbi eseményeket megőrizzük történeti feljegyzésként; a „nincs main-en” kijelentések nem a mai összefoglaló állapotát jelentik.


> **Publikálási előkészítés folyamatban:** a dokumentációs ág angol/magyar főoldalt, friss telepítési útmutatókat, nyilvános működésre alkalmas upstream-frissítőt és angol csomagútmutatót tartalmaz. Ez még nem main-állapot és nem új bináris kiadás. Részletes nyitott tételek: [PUBLICATION](../../doc/PUBLICATION.md).
Repository: [RobCZart82/Silver-K-Blue](https://github.com/RobCZart82/Silver-K-Blue)
Dokumentumállapot: tervezési és követési alap; nem kiadási megfelelőségi igazolás.

## 1. Cél és kiinduló állapot

A cél a feltárt kódhibák javítása, a platformellenőrzések befejezése és a Silver-K Blue nyilvános, érthetően dokumentált terjesztésének előkészítése. A Blue skinek és a meglévő projektállapotok megőrzése minden lépésben követelmény.

| Elem | Rögzített állapot |
| --- | --- |
| Auditált main | `7f4e9a8398799128da81dac80a49f4d385b19898` |
| Dokumentált upstream alap | temaniak/Silver-K v0.4.4a; `4baaead06e23e2358729cca32b6b511c72022318` |
| Hibajavítási PR | [#1 — framework, MIDI és szálleállítás](https://github.com/RobCZart82/Silver-K-Blue/pull/1) |
| PR #1 ellenőrzött fejcommit | `5303193820a14b3d8c36c39553c857e2ac85dd64` |
| PR #1 állapota a napló létrehozásakor | Nyitott; nincs beolvasztva |
| Megtartott Windows-kiadás | [Windows x64 build 3](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/windows-blue-3-1) |
| Megtartott macOS-kiadás | [macOS Universal 2 build 2](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/macos-blue-2-1) |
| A két kiadott csomag forrása | `40b68eeebd8e2c7d4a92229aa25fcb1217a652cc`; a PR #1 javításait még nem tartalmazza |
| Láthatóság az előkészítési áttekintéskor | Privát; a nyilvánossá tétel külön művelet |

A korábbi audit 9 találatát az alábbi H01–H09 azonosítók rögzítik. Több hiba az örökölt közös keretrendszerben található; nem mindegyikről igazolt, hogy a plugin szokásos használati útvonalán is előfordul. A bizonyítékok a korábbi audit eredményei, nem e dokumentum készítésekor újrafuttatott tesztek.

**Státuszok:** „PR-ben javítva” = a megoldás elkészült a javítási ágon, de még nem elfogadott main-változás. „Tervezett” = még végrehajtandó. „Ellenőrizendő” = az elfogadáshoz további bizonyíték szükséges. „Lezárt” csak sikeres ellenőrzés és szükség esetén main-beolvasztás után használható.

## 2. Logikus fejlesztési sorrend

| Szakasz | Feladat | Függőség / lezárási feltétel | Állapot |
| --- | --- | --- | --- |
| M0 | Kiinduló commitok, kiadások és feladatok rögzítése | Követhető napló és hivatkozások | E dokumentummal rögzítve |
| M1 | Memória- és bemenetkezelés: H01–H04, H08 | Érvényes és hibás bemenetek célzott tesztjei | PR #1-ben javítva; platformellenőrzés kell |
| M2 | Munkaszálak és MIDI: H05–H07 | Leállítási, MIDI-parser és exporttesztek | PR #1-ben javítva; platformellenőrzés kell |
| M3 | Forráscsomag és integráció: H09 | Hiánytalan forráscsomag, Windows/macOS CI, main-beolvasztás | PR #1-ben előkészítve; beolvasztás függőben |
| M4 | Nyilvános működésre alkalmas automatizálás | Publikus repository-eset tesztelve; main-merge továbbra is felügyelt | Dokumentációs PR-ben megvalósítva; 9 helyi upstream-teszt sikeres |
| M5 | Angol főoldal és naprakész útmutatók | Helyes linkek, telepítés, formátumok és korlátok | Dokumentációs PR-ben előkészítve; hiteles Standard/Retina képernyőképek még hiányoznak |
| M6 | Közzéteendő tartalom és eredet ellenőrzése | Fájlok, előzmények és csomagok átnézve; grafikai eredet tisztázva | Ellenőrizendő |
| M7 | Nyilvánossá tétel és működésellenőrzés | M4–M6 kész; szabályok és CI működnek | Tervezett |
| M8 | Ellenőrzött, friss bináris kiadás | M1–M3 elfogadva; platformbuild és használati ellenőrzés dokumentálva | Tervezett |

A dokumentáció és az upstream-frissítő előkészítése nem igényli a blokkolt platformbuild megvárását. Ha a privát Actions-keret akadálya miatt a nyilvánossá tétel előbb szükséges, M7 megelőzheti M3 lezárását, de csak az előkészítés és a tartalomellenőrzés után. Ebben az esetben a nyitott hibajavításokat és a régebbi kiadások állapotát világosan fel kell tüntetni; a sikertelen vagy el sem indult ellenőrzés nem számít sikeresnek.

## 3. Hibajegyzék és elfogadási feltételek

### M1 — memória, fájlbeolvasás és fordíthatóság

| ID | Hiba és helye | Következmény / korábbi bizonyíték | Elfogadási feltétel |
| --- | --- | --- | --- |
| H01 | Bináris olvasó: `source/framework/baseLib/binarystream.h`; a `size() - tellg()` ellenőrzés túlcsordulhat, ha a pozíció a vég után van | Pufferhatáron túli olvasás. ASan-reprodukció: 4 bájtos puffer, 100-as olvasási pozíció | A vég előtti, végi és vég utáni olvasás, valamint érvénytelen méret kezelése; nincs határon túli hozzáférés |
| H02 | WAV-olvasó: `source/framework/synthLib/wavReader.cpp`; a fejléc és chunkok méretellenőrzése hiányos | Csonkolt vagy hibás méretű WAV pufferen kívüli olvasást válthat ki; kód alapján azonosított találat | Rövid fejléc, túlméretes/csonkolt chunk, RIFF-határok és szükséges padding ellenőrzése; érvényes WAV továbbra is beolvasható |
| H03 | WAV-olvasó: ugyanott; ellenőrizetlen `bits_per_sample` osztó | Nulla értéknél nullával osztás; UBSan-reprodukció | Nulla és nem támogatott formátumértékek elutasítása az osztás előtt; támogatott minták helyes kezelése |
| H04 | FourCC-sablon: `source/framework/baseLib/binarystream.h`; `void` függvény logikai értéket ad vissza | A `read4CC("ABCD")` példányosítása fordítási hibát ad; nem feltétlenül érinti a jelenlegi pluginbuildet | Megfelelő visszatérési típus; egyező és eltérő FourCC-val forduló és futó teszt |
| H08 | Opaque FXB-bank: `source/framework/synthLib/vstpreset.cpp`; az `FBCh` közös chunkját programonként próbálja olvasni | Szabályos, kétprogramos egychunkos tesztbank elutasítása reprodukálva | Közös bankchunk egyszeri beolvasása; érvényes többprogramos bank elfogadása, csonkolt bank elutasítása |

**Állapot:** mind az öt tételhez készült javítás a PR #1-ben. H02/H03 az általános keretrendszer WAV-olvasóját érinti; a külön Wave ROM Builder másik olvasási útvonalat használ.

### M2 — szálkezelés és MIDI

| ID | Hiba és helye | Következmény / korábbi bizonyíték | Elfogadási feltétel |
| --- | --- | --- | --- |
| H05 | Több munkaszálas leállítás: `source/framework/juce/jucePluginLib/patchdb/jobqueue.cpp`, kapcsolódó fejléc | Egyetlen ébresztőfeladat mellett más szálak várakozva maradhatnak, a `join()` megakadhat. Két munkaszállal reprodukálva | Minden várakozó szál felébred és leáll; üres sor, függő munkák és ismételt indítás/leállítás ellenőrizve |
| H06 | Valós idejű MIDI-bájtok: `source/framework/synthLib/midiBufferParser.cpp` | A `90 3C F8 64` bemenetben az F8 a Note On adatának számíthat; hibás események reprodukálva | MIDI Clock külön esemény; a Note On változatlanul összeáll; running status és SysEx közbeni realtime-kezelés is ellenőrizve |
| H07 | SysEx → MIDI-export: `source/framework/synthLib/sysexToMidi.cpp` | Hiányzó End of Track deltaidő és hibás trackhossz. A korábbi reprodukcióban 5 deklarált és 8 tényleges bájt | Szabályos deltaidő és a teljes tracket tartalmazó hossz; az export visszaolvasható MIDI-olvasóval |

**Állapot:** a javítások és regressziós tesztek a PR #1-ben vannak. H05 több munkaszálra igazolt; ebből önmagában nem következik az egyszálas alapbeállítás lefagyása.

### M3 — csomagolás és platformintegráció

| ID | Hiba és helye | Következmény / korábbi bizonyíték | Elfogadási feltétel |
| --- | --- | --- | --- |
| H09 | Forráscsomag: `tools/package_release.py`; hiányzik a `blue/` a fájlkiválasztásból | A forrás-ZIP nem tartalmazza a CMake által várt beépített skineket és ikont | Minden szükséges Blue build-erőforrás bekerül; a kibontott csomagból történő konfigurálás/fordítás ellenőrizve |

H09 a generikus `package_release.py` útvonal hibája; a Blue Actions külön csomagolószkriptet használ. A PR #1 leírása további VST3-presetolvasási módosítást is említ; ennek diffjét és tesztlefedettségét a PR felülvizsgálatakor külön ellenőrizni kell, nem újabb igazolt auditmegállapításként kezelni.

A PR #1-ben rögzített helyi ellenőrzések:

- C++ regressziós futás AddressSanitizer és UndefinedBehaviorSanitizer mellett sikeres.
- A helyi futás naplózási/szálprioritási adapterei helyettesítve voltak; leak-ellenőrzés a környezet korlátja miatt nem futott.
- Forráscsomagtesztek: 2 sikeres; upstream-automatizálási tesztek: 4 sikeres.
- Python-szintaxis és workflow YAML-ellenőrzés sikeres.

A korábbi teljes audit ezenfelül 31 sikeres webes/MIDI-tesztet és egy kihagyott fixture-tesztet rögzített. Ez külön ellenőrzési kör, nem a teljes natív pluginbuild igazolása.

**Még szükséges:**

- [ ] A PR aktuális fejcommitján a `windows` és `macos` job ténylegesen elindul és sikeresen lefut.
- [ ] A natív regressziós és forráscsomagtesztek a platformfutásokban is sikeresek.
- [ ] A kihagyott, firmware-függő tesztek külön meg vannak nevezve.
- [ ] A PR felülvizsgálata után, zöld kötelező ellenőrzésekkel történik a main-beolvasztás.

A felhasználó által bemutatott GitHub-értesítés a privát projektek havi Actions-keretének kimerülését jelezte. Ez magyarázta a buildlépések előtt leálló futásokat; nem fordítási hibának minősítendő. A keret és a futások aktuális állapotát újraindítás előtt ismét ellenőrizni kell.

## 4. Nyilvánossá tételi feladatjegyzék

| ID | Feladat / érintett fájlok | Kívánt eredmény |
| --- | --- | --- |
| P01 | `tools/sync_blue_upstream.py`, `tools/test_sync_blue_upstream.py` | A „no longer private” tiltás megszüntetése az elvárt repository ellenőrzésének megtartásával. Publikus eset tesztelése; nincs automatikus main-merge. A frissítés nélküli, tiszta merge, ütközéses és release-only esetek továbbra is működnek |
| P02 | `.github/workflows/windows-blue.yml`, `macos-blue.yml`, `upstream-sync.yml` | Privát megnevezések javítása; a PR #1 CI-változásainak megőrzése. Idegen forkból érkező PR ne tudjon Release-t publikálni; kiadás továbbra is szándékos, kézi művelet |
| P03 | `tools/package_blue_windows.py` | Angol README a ZIP-ben, opcionális magyar útmutatóval; helyes telepítési helyek, firmware-követelmény, forráscommit és licenchivatkozások |
| P04 | `README.md`, új `README.hu.md` | Angol főoldal: nem hivatalos Silver-K-változat, Blue-képernyőképek, Windows/macOS letöltések, Standard/Retina különbség, telepítés, saját firmware, eredeti projekt és licencek |
| P05 | `blue/Documentation/01-Telepites-es-hasznalat.md`, `doc/INSTALL.md`, `doc/USER_GUIDE.md` | Az elavult „csak külső skin / nincs Windows vagy AU” állítások javítása; beépített skinek az alapút. A Blue és a Silver-K azonos pluginazonosítójának, a teljes bundle cseréjének és a mentés módjának leírása |
| P06 | `doc/BUILD.md` | Blue repository klónozása, működő Windows/macOS parancsok, tényleges célokhoz illő tesztlista. Firmware-függő és ROM nélküli ellenőrzések elkülönítése |
| P07 | `THIRD_PARTY.md`, `blue/Artwork/README.md`, `04-Verziozas-es-eredet.md` | A teljes Blue grafikai készlet eredetének és terjesztési feltételeinek dokumentálása. A régi „két knob PNG” leírás frissítése; eredeti szerzői megjelölések megtartása |
| P08 | `blue/Documentation/02–10`, `blue/CHANGELOG-HU.md`, új angol változásjegyzék | Régi archívumútvonalak és privát hivatkozások javítása; történeti anyagok megjelölése. Az aktuális útmutató és a régi munkanapló ne mondjon egymásnak ellent |
| P09 | Release-leírások és `blue/EMBEDDED-RELEASE.json` | A két megtartott kiadás angol leírása; pontos forráscommit, formátumok és ellenőrzési korlátok. A régi binárisokhoz nem szabad utólag az új hibajavítások meglétét állítani |
| P10 | Git-előzmény, tagek, jelenlegi fájlok és kiadási ZIP-ek | Közzéteendő tartalom ellenőrzése. A jelenlegi fa átnézése önmagában nem igazolja a teljes történet vagy a letölthető csomagok tartalmát |
| P11 | Repository láthatóság és main-ruleset | Nyilvánossá tétel után a már létrehozott védelem és a tényleges `windows`/`macos` ellenőrzésnevek ellenőrzése; PR-követelmény, naprakész ág, lezárt beszélgetések, törlés- és force-push-tiltás |
| P12 | Új build, csomagolás és kiadás | Javított main-ből készült, ellenőrzött csomagok; friss letöltési linkek, ellenőrzőösszegek és angol kiadásjegyzet |

P07 és P10 eredményét tényleges ellenőrzéssel kell rögzíteni. Ez a napló nem állítja, hogy a teljes Git-történet, minden grafika vagy a kiadási ZIP-ek tartalma már ellenőrzött.

A jelenlegi `doc/INSTALL.md` szerint a macOS build ad hoc aláírt és nem notarizált. Ezt és a Windows-csomag aláírási állapotát a kiadott csomag tényei szerint kell leírni. A beállított minimum macOS-verzió nem jelent minden fizikai gépen elvégzett tesztet.

## 5. Új kiadás elfogadási pontjai

- [ ] A kiadási forráscommit main-en van, és a szükséges platformellenőrzések sikeresek.
- [ ] Windows x64: VST3 és Standalone; macOS Universal 2: AU, VST3 és Standalone elkészült.
- [ ] Mindkét Blue skin külön telepítés nélkül betölthető; a mentett skinválasztás és hostállapot visszaáll.
- [ ] Saját firmware-rel a szokásos DAW-os működés és hangpróba eredménye dokumentált; a nem tesztelt platform/gép külön megnevezve.
- [ ] A csomaghoz tartozó README, build-info és ellenőrzőösszegek helyesek.
- [ ] A kiadás forrása elérhető, és a szükséges build-erőforrások megtalálhatók.
- [ ] A README és a Release-leírás ugyanarra a ténylegesen kiadott verzióra mutat.
- [ ] A régebbi kiadások sorsa az új csomagok ellenőrzése után rendezett; a felhasználó kívánsága szerint platformonként a legfrissebb szükséges build marad elérhető.

## 6. Munkanapló

| Dátum | Esemény | Eredmény / következő lépés |
| --- | --- | --- |
| 2026-10-30 | A korábbi audit 9 kód- és csomagolási találatának rögzítése | H01–H09; a reprodukált és a kód alapján azonosított problémák megkülönböztetve |
| 2026-10-30 | A korábban elkészített PR #1 állapotának ellenőrzése | Nyitott; fejcommit `5303193820a1`; main-beolvasztás még nincs |
| 2026-10-30 | A platformellenőrzések akadályának rögzítése | A felhasználó értesítése alapján privát Actions-keretkimerülés; sikeres újrafutás szükséges |
| 2026-10-30 | Release-lista áttekintése | Windows build 3 és macOS build 2 maradt; a régi kiadásokat a tulajdonos rendezte |
| 2026-10-30 | Publikálási előkészítés áttekintése | Privát repositoryt megkövetelő frissítő, elavult telepítési állítások, eredeti repo klónozása, magyar ZIP-README és pontatlan eredetleírás azonosítva |
| 2026-10-30 | Fejlesztési terv és napló elkészítése | M0–M8 sorrend, H01–H09 hibajegyzék és P01–P12 publikálási feladatlista létrehozva |

| Dátum | Esemény | Eredmény / következő lépés |
| --- | --- | --- |
| 2026-10-30 | P01–P06, P08: nyilvános működés és dokumentáció előkészítése | Angol/magyar főoldal, friss telepítés és build-repocím, angol ZIP-útmutató és kézi kiadási feltétel. 9 upstream-teszt és 2 szintetikus Windows-csomagteszt sikeres; Python-szintaxis és workflow-guard ellenőrizve. Natív build nem futott |
| 2026-10-30 | P07/P10 publikálási bizonyítékok állapota | Grafikai eredetleltár készült; FinaleOnOff és további beküldött/referenciából készült anyagok terjesztési bizonyítékai még tisztázandók. Teljes Git-előzmény/Release/Actions-tartalom ellenőrzése nincs lezárva |
| 2026-10-30 | Következő lépések | Hiteles képernyőképek; eredet- és tartalomellenőrzés; Release-leírások frissítése; PR #1 és a dokumentációs ág workflow-módosításainak egyesítése a platformtesztek megőrzésével |

### Következő naplóbejegyzés sablonja

Minden érdemi változtatásnál az alábbi adatokat kell hozzáadni:

| Dátum | Feladatazonosító | Módosítás | Bizonyíték | Állapot / következő lépés |
| --- | --- | --- | --- | --- |
| ÉÉÉÉ-HH-NN | Hxx / Pxx | Mi változott és miért | PR, commit, Actions-futás vagy dokumentált helyi teszt | Tervezett / PR-ben / ellenőrizendő / lezárt |

Ne írjuk át utólag a történeti bejegyzést úgy, mintha egy későbbi javítás már a korábbi kiadásban is benne lett volna. Az aktuális státuszt és a lezáró bizonyítékot új bejegyzésben kell rögzíteni.

## 7. Kapcsolódó dokumentumok

- [Hibajavítási PR #1](https://github.com/RobCZart82/Silver-K-Blue/pull/1)
- [Korábbi fejlesztési terv](02-Aktualis-fejlesztesi-terv.md) — e napló sorrendjével összehangolandó.
- [Upstream-frissítési folyamat](08-Upstream-frissites.md)
- [Első beépített skines kiadás](10-Elso-beepitett-kiadas.md)
- [Fordítás](../../doc/BUILD.md)
- [Telepítés](../../doc/INSTALL.md)
- [Forráseredet és függőségek](../../THIRD_PARTY.md)

## Integrációs ellenőrzés — 2026-10-30

- Az `integration/blue-release-readiness` ág egyesíti a PR #1 és PR #2 változásait; ez még nem main és nem új kiadás.
- A Windows/macOS workflow-konfliktusok feloldva: forráskiválasztás, upstream-biztonság, Blue-csomagteszt és natív framework-regresszió egyaránt megmarad. A Release publikálása továbbra is kézi indításhoz kötött.
- Robert 2026-10-29-i felhasználói beszámolója szerint macOS-en REAPER-ben AU és VST3 működik. A beküldött képeken AU és K-Station/KS felület látható. Ez nem Windows- vagy fizikai Intel-gépes teszt.
- A grafikai eredet és az új javításokat tartalmazó platformbuild továbbra is nyitott. A régi kiadások tartalma nem változott.

- Integrációs helyi ellenőrzés: 13 Python unittest sikeres; beépített skin erőforrás-egyezés sikeres; CMake konfigurálás és `silverFramework_test` natív fordítás sikeres; CTest `silver_framework_safety` 1/1 sikeres; repository-fájlellenőrzés 755 fájlon sikeres. Teljes platformtermék-fordítás és új kiadás nem történt.

- 2026-10-30, P07: Robert közvetlen forrásmegjelölése szerint a FinaleOnOff az eredeti Silver-K repository gombgrafikáiból újrarenderelt anyag. Az eredetleltárban rögzítve, a megőrzött upstream on/off képek és generálási szövegek hivatkozásával. A konkrét végleges fájlok pontos leszármazási lánca még nem bizonyított; ez nem minden grafika eredetének lezárása.

## Helyi Universal 2 integráció — 2026-10-30

- Forráscommit: `9004549`; helyi AU, VST3 és Standalone Release fordítás sikeres. A csomagoló mindhárom termék arm64/x86_64 architektúráját és ad hoc aláírását ellenőrizte; a ZIP és SHA-256 elkészült. Ez helyi tesztcsomag, nem új GitHub Release.
- A Standalone CFBundleIconFile értéke Icon.icns, az erőforrás jelen van.
- CTest: framework safety, conversion, native editor, performance editor, embedded Blue és host clock sikeres (6 teszt). A firmware-függő `silver_latency_reentry` teszt kihagyva (saját firmware nincs a tesztadatkönyvtárban); nem sikeresként számolva.
- Valódi 2× Standard/Retina render készült a beépített skinek tesztjével; firmware nélkül, ezért az inaktív elemek halványak. README-ben megjelenítve, grafikai áttervezés nélkül.
- Összefogó draft PR: #3. Windows platformellenőrzés nem indult: a GitHub számlázási/keretkorlátot jelzett (run 41074962800). Teljes helyi Windows/DAW/hangteszt nem történt; a main és a korábbi kiadások változatlanok.

## Release-ismétlések — 2026-10-30

A régi kiadási workflow a GITHUB_RUN_ATTEMPT értékét is a tagbe tette; újrafuttatáskor azonos csomagból új Release keletkezett. Windows build 3 (-1/-4) és macOS build 2 (-1/-2) ZIP-je platformonként azonos GitHub SHA-256 digestű. Javítás: futásonként állandó -1 tag, a már létező kiadás forráscommitjának ellenőrzése és megtartása. Eltérő commitnál hiba, automatikus törlés/felülírás nincs. A történeti duplikátumok eltávolítása külön jóváhagyásra vár.

### A01–A04 helyi ellenőrzés lezárása

2026-10-30: a módosított `silverFramework_test` natív fordítása és CTest futása sikeres. A `silverSkin_test` csomag-, betöltés-, megjelenítés-, visszaállítás- és DPI-próbája grafikus asztali hozzáféréssel sikeres; az első korlátozott futtatás csak az asztali menühozzáférés hiánya miatt nem zárult sikerrel. 13 Python unittest sikeres. Az eredeti alfolyam-írási összeomláspróba ASan/UBSan mellett többé nem omlik össze; az új regresszió az elutasítást is ellenőrzi. Új teljes Windows/macOS termékbuild még a PR ellenőrzésére vár; a kiadott binárisok e javításokat még nem tartalmazzák.

### CI-környezet frissítése — 2026-10-30

A PR #5 macOS-futása a macos-14 gépkép kivezetése miatti GitHub brownout alatt, fordítás előtt megszakadt (run 41131868233). Átállítva macos-15-re a GitHub hivatalos migrációs ajánlása szerint: https://github.com/actions/runner-images/issues/13518 . A Universal 2 architektúrák és a 12.0 deployment target változatlan; az új környezeten a CI-t ismét ellenőrizni kell.

## Közös kiadás előkészítése — 2026-10-30

PR #5 zöld Windows/macOS ellenőrzés után beolvadt; A01–A04 a main `f0a6eb9` commitjában szerepel. A main új platformfutásai külön ellenőrzendők.

Új kézi `blue-release.yml` workflow: két sikeres, azonos SHA-jú, saját repository main ágán futott Windows/macOS build artifactját ellenőrzi. A ZIP-ek build-info és teljes SHA256SUMS leltára ellenőrzött. Ugyanazon commit git archive forrás-ZIP-je tartalmazza a Blue build-erőforrásokat; a függőségek továbbra is rögzített revíziókból töltődnek le. Egyetlen Release kapja a két platform ZIP-jét, a forrás-ZIP-et és SHA256SUMS.txt-t. Részleges feltöltés draft marad; meglévő verziót nem ír felül. A platformworkflow-k többé nem publikálnak külön Release-t.

A közös kiadás ebben a változtatásban csak előkészített; tényleges publikálás és a README letöltési linkjeinek átvezetése még hátravan. A skinek v1.0 száma különbözik a program kiadási verziójától.

## Adatmappák dokumentációja — 2026-10-31

A `doc/DATA_FOLDERS.hu.md` rögzíti a ROMs/Patches/Skins tartalmát és tényleges keresési mélységét. KS All Data.MID a ROMs alá tartozik; automatikus név- és tartalomellenőrzése csak a közvetlen fájlokat nézi. Firmware- és Patches-keresés rekurzív, skin-katalógus közvetlen csomagalmappás. Folyamatos Patches-változásfigyelés tervezett, nem megvalósult. Magyar útmutatók és angol felhasználói mappatáblázat frissítve; futásidejű működés nem változott.

## Auto Blue és egyszerűsített skinválasztó — 2026-10-31

Új fejlesztési ágon: Auto, Blue Standard v1.0, Blue Retina 4K v1.0, Silver-K Legacy. Auto → beépített Blue Retina, firmware-követő kezelőszervekkel; nincs külön K/KS skin. Új beállítás alapja Auto; korábban kifejezetten megjegyzett más skin továbbra is választható. A régi globális Auto beállítás Blue-ra vált, a régi DAW-projekt Auto értéke blueAutoVersion hiányában Legacyként áll helyre. Új Auto projektállapot verziójelzővel Blue marad. A régi Silver-K/KS Grey/Classic projektértékek háttérben olvashatók; a választó Legacyként jelzi őket. A betöltés sorszám helyett névalapú felismerést használ. Eredeti grafikai tartalék és külső skin támogatás megmarad. Célzott teszt: Auto feloldás, új állapot visszatöltése és régi Auto migráció; platformellenőrzés és beolvasztás még szükséges.

Helyi ellenőrzés: az átfordított `silverEmbeddedSkin_test` sikeres; mindkét Blue skin négy oldala, állapotmegőrzés, Auto → Retina feloldás és régi Auto → Legacy migráció ellenőrizve. Diffellenőrzés sikeres. Új kiadási bináris nem készült.

2026-10-31: az Auto Blue PR platformtesztjei a régi, álló knob-grip megjelenést ellenőrző Legacy regresszión álltak meg. A silverEditor_test immár kifejezetten Silver-K Legacy-t választ; a Blue/Auto viselkedését a silverEmbeddedSkin_test ellenőrzi. A termék forgási viselkedése nem változott, tesztet nem távolítottunk el. Új platformfutás szükséges.

## Elfogadott Setup-koncepció — 2026-10-31

A felhasználó elfogadta a három szekciós, ezüst–kék elrendezési tervet. A képi koncepció nem működő felület és nem implementációs bizonyíték. A meglévő fő fejléc és Blue tipográfia megtartandó; a generált kép eltérő díszítése nem követelmény.

### 1. Fájlok és mappák

- Három másolható, csak olvasható útvonalsor: Patches, ROMs, Skins, mindegyik mellett Megnyitás. Külön „Silver-K main” útvonalsor nem kell.
- Adatmappa módosítása… külön kisebb gombként megmarad; a három almappa követi a kiválasztott főmappát. Önálló almappa-áthelyezést nem vezetünk be.
- Firmware, Wave ROM és KS-adatok külön kiválasztási sorok; a többfájlos KS-adatkészletet is jelezni kell. A firmware állapot-/hibaüzenetei megmaradnak.
- ROM-lista frissítése megmarad. K-Station módban a nem használható KS-elemek egyértelműen inaktívak, magyarázattal.
- A KS All Data.MID célhelye ROMs marad az eredeti működéssel összhangban; a többi presetbank a Patches alá tartozik.

### 2. Megjelenés

- Auto, Blue Standard v1.0, Blue Retina 4K v1.0 (256 px/kocka), Silver-K Legacy; külső skin betöltése megmarad.
- Felületméret, Skin betöltése…, Újratöltés és Legacy sablon exportálása.
- **Rögzített felhasználói döntés: Export template változatlanul mindig az eredeti Silver-K/Legacy sablont exportálja, függetlenül az aktív skintől. Aktív Blue-export nem fejlesztési feladat.**
- A Skins folder funkcióját a fentebbi útvonalsor Megnyitás gombja adja.

### 3. Hang és működés

- MIDI-csatorna, audio buffer, késleltetés és teljesítménydiagnosztika megmarad.
- Lenyitható Haladó KS-beállítások: KS controller, kísérleti két CPU-magos feldolgozás, KS gyári demó. Semelyik funkciót nem töröljük az átrendezéskor.
- A Demo Song a teljes kompatibilis KS Flash-be ágyazott demót indítja, nem általános MIDI-lejátszó; a KS All Data.MID önmagában nem biztosít demózenét. Az elérhetőséghez igazított gombállapot és magyarázó gyorstipp PR #17-ben megvalósítva; a teljes Flash demó használati próbája hátravan.

### Elfogadási feltételek

- Minden eddigi Setup művelet elérhető és azonos jelentésű marad; a firmware-/adatkezelési működés nem változik.
- K/KS, Standard/Retina/Legacy és a támogatott GUI-méretek alatt nincs takarás vagy levágás.
- Útvonalak frissülnek adatmappaváltáskor, teljes tartalmuk másolható; nem szerkeszthetők véletlenül.
- Átrendezés a külső skin-layout felülírásokkal és a mentett állapotokkal együtt is ellenőrizendő.

### Patches-figyelés: még nem implementált ötlet

Rekurzív háttérfigyelés, másolás befejezésének megvárása, csak változott fájlok újraindexelése, címkék/kedvencek és szándékos kizárások megőrzése, kikapcsolhatóság. Nem változtatja meg automatikusan az aktív hangszínt. ROM/firmware vagy skin cseréjét önálló engedélyezett műveletként kell kezelni; automatikus hangmotor-újraindítás nem elfogadott követelmény.

### Naplóbejegyzés

2026-10-31: a PR #5–#8 beolvasztási állapota, a sikeres dd76cfa main-platformellenőrzések és az üres Release-lista GitHubon ellenőrizve. Setup-koncepció és Export template döntés rögzítve. Ez a változtatás dokumentációs, futásidejű kódot nem módosít.

## 2026-10-31 — Vektoros fejléc-logó

A felhasználó KS és K-Station látványterve alapján a Blue Standard és Retina fejlécében csak a SILVER-K szójel jelenik meg, BLUE sor és aláhúzás nélkül. Függőleges szélei a firmware-választóhoz igazodnak; a vektor függőlegesen nyújtva tölti ki a kijelölt helyet. Legacy változatlan. A README-k tetején a teljes Blue logó szerepel. Forrás: a felhasználó Silver-K-Blue.svg állománya; a fejléc-változat ebből származik. Ellenőrzés és kiadás állapota a kapcsolódó PR-ben.

## 2026-10-31 — Setup első lépcső: mappaútvonalak

A #10 logó-PR Windows és macOS ellenőrzése sikeres, beolvadt. A következő külön lépcső a Setup lap három másolható, csak olvasható Patches/ROMs/Skins útvonalsora és Megnyitás gombja. Az útvonalak követik az adatmappa megváltozását; a periodikus frissítés nem törli a kijelölést. A főmappa kiválasztása megmarad, egyértelműbb Change data folder felirattal. Nincs külön almappa-áthelyezés, nincs automatikus presetbetöltés. A három szekciós teljes átrendezés és a Haladó panel további lépcső; a régi Skins folder gomb átmenetileg még megmarad. Az embedded UI teszt ellenőrzi a csak olvashatóságot és az adatmappaváltás követését.

## 2026-10-31 — Setup második lépcső: Haladó KS-rész

A #11 mappaútvonal-PR mindkét kötelező platformon sikeres és beolvadt. A KS controller, a kísérleti kétmagos feldolgozás és a KS Demo Song most alapértelmezetten összecsukott, külön gombbal nyitható részben marad elérhető. A nyitás/csukás nem módosít hangparamétert; az oldalak közötti váltás megőrzi a nyitott állapotot az editor élettartama alatt. Másik oldalon a Haladó kezelőszervek nem látszanak. K-Station esetén a firmware szerinti inaktív állapot megmarad, a Demo Song is inaktívként látható a nyitott részben. A teljes háromszekciós átrendezés továbbra is hátralévő lépcső.

### PR #12 platformellenőrzés javítása

Az első Windows/macOS körben a natív editor teszt még az összecsukható rész előtti láthatóságot várta (Demo visible in wrong model). A teszt most előbb ellenőrzi az alapértelmezett csukott állapotot, majd megnyitja a részt és külön ellenőrzi a Demo firmware szerinti használhatóságát. A hiba ugyanabban a PR-ben javítva; új fejlesztési lépcső nem indult.

## 2026-10-31 — README képernyőképek frissítése

A felhasználó új, REAPER-ben készített K-Station és KS Main képernyőképei váltják a korábbi firmware nélküli Standard/Retina renderpárt az angol README-ben. Mindkét új kép Retina 4K, 256 px-es forgógomb-kockákat használ; a képfeliratok és helyettesítő szövegek ezt és a firmware-verziót jelölik. A teljes Blue logó megmarad, az ismétlődő cím elhagyva. A képek az eredeti PNG-k változatlan másolatai. A meglévő #13 PR bővítése, új PR nélkül.

## 2026-10-31 — Setup harmadik lépcső: három szekció

A #12 és #13 PR sikeres Windows/macOS ellenőrzés után beolvadt. A Blue Setup lap három egységes panelre rendezve: Files and folders, Appearance, Audio and operation. A mappaútvonalak felülre kerültek; a firmware/Wave ROM/KS adatválasztók, állapotüzenet és adatfájl-összefoglaló együtt láthatók. A Skins folder duplikált gombját a Skins útvonalsor Open művelete váltja ki. Megjelenésnél megmarad a skinválasztás, betöltés, újratöltés és változatlan Legacy-export. A hangszekcióban MIDI-csatorna, buffer, késleltetés, diagnosztika és lenyitható KS-rész található. Legacy saját elrendezése megmarad; a külső skin komponens-felülírások az alapelrendezés után érvényesülnek. A GUI-méret meglévő jobb egérgombos menüje változatlan.

## 2026-10-31 — Setup felületméret-gomb

A #14 háromszekciós Setup PR mindkét kötelező platformon sikeres és beolvadt. A Blue Megjelenés részben közvetlen Interface size gomb nyitja a meglévő méretválasztó menüt: 50/75/100% és egyéni 50–150%. A jobb egérgombos menü, méretmentés és aránytartás változatlan; a gomb nem vezet be második méretezési logikát. Az átfedés-ellenőrzés a gombra is kiterjed.

## 2026-10-31 — Setup KS-adatválasztók állapota

A Wave ROM és KS factory data választók K-Station módban most inaktívak; a gyorstipp elmagyarázza, hogy ezek KS-adatok, a K-Station presetbankok a Patches alá tartoznak. KS módban a betöltésen kívül használhatók, a meglévő választási funkció változatlan. Az állapot oldalváltáskor és firmware-váltáskor frissül. A natív editor teszt mindkét modellnél ellenőrzi az elérhetőséget, amikor ROM-paraméterekkel fut; firmware nélkül a KS alapállapotot ellenőrzi.

## 2026-10-31 — KS Demo Song tényleges elérhetősége

A #16 PR Windows/macOS ellenőrzése sikeres és beolvadt. A Demo Song gomb engedélyezése most az engine meglévő demoAvailable jelzését követi, nem csupán a KS firmware betöltését. Teljes, kompatibilis demót tartalmazó KS Flash szükséges; KS All Data.MID/preset MIDI esetén a gomb inaktív és a gyorstipp elmagyarázza a feltételt. A demólejátszás és adatellenőrzés meglévő működése változatlan. A firmware nélküli és hiányzó demó-adatos tesztek az új lekérdezést is ellenőrzik.

## 2026-10-31 — Átadási állapot a Setup-fejlesztés után

A jóváhagyott Setup-fejlesztési lépcsők mainben vannak (#11–#17). Az automatikus, ROM nélküli platformellenőrzések sikeresek; nincs nyitott fejlesztési PR az átadási dokumentáció megkezdésekor. A README-k törölt régi kiadásokra mutató linkjeit eltávolítottuk. Új Release nem készült.

Hátralévő ellenőrzések és külön döntések:

- Valódi K-Station és KS firmware-rel a fájlválasztás, adatmappaváltás, méretmenü, inaktív gombok és az összes Setup-művelet interaktív próbája.
- Hosszú elérési utak másolása, 50–150%-os méretek és saját külső skin-layoutok ellenőrzése; a helyi geometriai teszt nem helyettesíti ezt.
- Teljes demót tartalmazó kompatibilis KS Flash-sel a Demo Song indítás/leállítás használati próbája. A KS All Data.MID nem ilyen adat.
- Azonos main-commitból készült Windows/macOS artifactok kiválasztása és közös kiadás külön indítása, forráscsomaggal és ellenőrzőösszegekkel. Nem történik automatikus kiadáspublikálás.
- Grafikai eredetleltár nyitott pontjai és az upstreamhez ajánlható változtatások külön áttekintése. Automatikus Patches-figyelés továbbra is javaslat, nem beépített funkció.

Ez fejlesztési átadás, nem általános hibamentességi, hanghűségi vagy minden platformra kiterjedő használati elfogadás. A következő új funkció előtt ezek eredményét és a felhasználó prioritását kell figyelembe venni.

## 2026-11-01 — Használati teszt észrevételek, közös nyitott PR

1. Terméknév Silver-K Blue, külön Standalone/VST3/AU bundle és belső azonosítók a párhuzamos upstream telepítéshez. Bundle: com.robczart.silverkblue; AU subtype/plugin code: SkBl, gyártókód TusP megőrizve. A korábbi Blue az eredeti identitását használta: régi host-projektekhez a régi build megőrzendő, automatikus hostazonosító-migráció nincs. A megosztott adatkönyvtár és a felhasználó által jóváhagyott SILVER-K fejlécgrafika változatlan. Csomagolás és bináris metadata-ellenőrzés az új nevet/azonosítót követi.

A PR több egymást követő felhasználói észrevételt gyűjt; nem olvasztható be a felhasználó lezáró jelzéséig. Automatikus merge nincs.

### 2. észrevétel — fejléc igazítása

A négy partfül közös közepe a Mixer x=639 középvonalára került mindkét Blue felületen. A firmware mező a változatlan méretű logó és partgombsor közé került. A Performance LED/felirat kompaktabb, közösen a partgombsor és navigáció közti rés közepén. Dobtérkép esetén a ritkán megjelenő Drum Edit a Performance csoport alá kerül, hogy ne fedje a navigációt. Legacy és a hangparaméterek változatlanok. Ugyanazon #19 PR további tétele; automatikus merge nincs.

### 3. észrevétel — Setup információs gyorstipp

A Files and folders információs összefoglaló fölötti, a tartalmat eltakaró ismétlődő tooltip eltávolítva. A látható állapot- és adatfájl-feliratok, mappaútvonalak és a kezelőgombok magyarázó gyorstippjei megmaradnak. Ugyanazon #19 PR része, beolvasztás nélkül.

### 4. észrevétel — Setup alumínium anyag

A Blue Setup három szekciója a Main lap meglévő aluminiumGrain1 anyagát használja, az eredeti 2× pixelsűrűséggel csempézve, a lekerekített panelhatáron belül. A finom szemcsézet nem nyúlik meg a széles panelen; a Haladó rész nyitásakor is követi a panel méretét. Standard/Retina és K-Station/KS közös megjelenítés; Legacy nem változik. Meglévő grafika újrafelhasználása, nincs új képfájl. Ugyanazon #19 PR-ben marad.

### 5. észrevétel — LED-feliratok balra igazítása

A Blue felületek ovális LED-es kapcsolóinak feliratai a LED utáni szövegterületen balra igazítottak. Az oszcillátorok PW Position / LFO 2 PWM / Env PWM sorai közös kezdővonalat kapnak a középre igazítás helyett; azonos szabály Standard/Retina és K-Station/KS nézetben. A normál nyomógombok, partfülek és Legacy szövegigazítása változatlan. Ugyanazon #19 PR része.

### 6. észrevétel — közvetlen méretlista és 125%

A Setup Interface size gomb közvetlenül a százaléklistát nyitja, köztes Size almenü nélkül. 125% új sor hozzáadva; 50/75/100%, elválasztó és Custom megmarad. A jobb kattintásos menü továbbra is Size almenüt használ, ugyanazzal a kibővített listával. Az egyéni 50–150% tartomány, aránytartás és mentés változatlan; a meglévő méretezési teszt 125%-ot is ellenőriz. Ugyanazon #19 PR része.

### 7. észrevétel — Setup gombok egységes szélessége

A fájlműveletek öt és a Megjelenés négy kijelölt gombja egységes 210 logikai px szélességet és 34 px magasságot kapott, 12 px közökkel. A skinválasztó mező a fennmaradó helyet használja, így minden művelet egy sorban marad. Standard/Retina, K-Station/KS közös elrendezés; a két skin JSON-komponenspozíciója is frissítve. A leghosszabb Change data folder felirat számára is megmarad az eddigi szélesség. Ugyanazon #19 PR része.

### 8. észrevétel — egységes Setup mezők és Haladó kapcsoló

A skinválasztó, Editor MIDI channel és Audio buffer mezők, valamint a Performance diagnostics gomb egyaránt 314 logikai px szélességű. A Haladó KS-gomb világos alapú fekete kerettel/felirattal, két szélén lefelé mutató nyíllal jelzi a kinyitást; nyitva a nyilak felfelé mutatnak. A funkció és a beállítások változatlanok, Standard/Retina és mindkét firmware nézet közös módosítása. Ugyanazon #19 PR, beolvasztás nélkül.

### 9. észrevétel — Patches állapotszöveg kontrasztja

A lista alatti betöltési és egyéb állapotüzenetek explicit sötét szövegszínt kaptak a világosszürke háttéren, a többi böngészőfelirattal egyezően. A közös böngésző miatt K-Station és KS módban, Standard és Retina felületen is érvényes; a skin színfelülírása továbbra is támogatott.

### PR #19 beolvasztás előtti ellenőrzés

A Windows CI feltárta, hogy a konverziós ellenőrzés még a korábbi terméknevet várta. Az elvárt név Silver-K Blue lett; a teszt többi ellenőrzése változatlan. Beolvasztás csak a legfrissebb commit kötelező Windows/macOS ellenőrzéseinek sikerével engedélyezett.

### README-logó térközének finomítása

A BLUE felirat és a hozzá tartozó kék vonal együtt 24 SVG-egységgel feljebb került a SILVER-K alá. A két sor közti hézag 44-ről 20 egységre csökkent, a rajzvászon magassága ehhez igazodik. A plugin fejlécének logója változatlan. Ellenőrzés: érvényes SVG/XML és tiszta diff.

A #20 PR a #19 beolvasztása után friss mainnel összefésülve; mindkét fejlesztési naplóbejegyzés megőrizve.

## 2026-11-01 — További Setup-finomítások

Az Audio and operation részben a Haladó KS kapcsoló, a KS controller lista és a Demo Song gomb is a felső mezők 314 logikai px szélességét követi, két közös oszlopban. A Setup három paneljének alapja egyszínű ezüst, színátmenet nélkül; a meglévő finom alumíniumszemcsézet megmarad. Standard/Retina és K-Station/KS közös módosítása.

A fő Data folder útvonala negyedik, legfelső sorba került a Blue Setupban, a meglévő mappaválasztó műveletet végző Open gombbal. Patches/ROMs/Skins továbbra is a saját mappáját nyitja meg. Az alsó ismétlődő főútvonal eltávolítva; a fájlműveletek és információk lejjebb igazítva a panelen belül. Legacy elrendezése megmarad.

Az alkalmazás ikonforrása 1024×1024 méretű, átlátszó margóval és lekerekített sarkokkal készült az eredeti képből. A meglévő JUCE ICON_BIG beállítás ezt használja a platformikonok előállításához. Vizuális előnézet ellenőrizve; a megjelenés az új build telepítése után frissül.

A K-Station Main LFO négy tekerőjének pozíciója a KS nézet már megfelelő rácsához igazítva: bal oszlop x=1027, jobb x=1152, sorok y=502/608. Korábban a bal oszlop 3 px-lel balra, a jobb 3 px-lel jobbra, mindkettő 4 px-lel lejjebb volt. A Rate 1/2 és Delay 1/2 funkciók és feliratok változatlanok. Mindkét Blue JSON elrendezésben a négy téglalap egyezése ellenőrizve.

A README Main-előnézetei a felhasználó új K-Station.png és KS-Rack.png képeire cserélve, eredeti PNG-felbontásban. A képleírások a két firmware-nézetet nevezik meg, hostra vonatkozó feltételezés nélkül.

## 2026-11-02 — KS indítási hangállapot célzott ellenőrzése

Az upstream `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6` indítás utáni sustain/All Notes Off/All Sound Off tisztításának átvétele előtt a `silver_latency_reentry` új firmware-es próbát kapott. Panic vagy új programdump nélkül ellenőrzi a kezdeti csendet, a vocoder és oszcillátormixer módosítása utáni csendet, majd egy MIDI-hang megszólalását és elengedését. A pozitív kontroll kizárja, hogy egy végig néma hangszer téves sikernek számítson. A produkciós motor és a GUI változatlan.

Helyi Windows/MSVC: KS 2.102, KS All Data és generált 129024 bájtos nullás hullámállomány mellett a referencia- és a natív KS-vezérlővel is PASS. Mindkét futásban dry/vocoder/mixer/released csúcs 0, a megszólaltatott hang csúcsa 0.0260137. A teljes KS és K-Station latencia/host-visszahívás/állapot-visszaállítás teszt is PASS (111.55 s, illetve 79.39 s). Elkülönített adatkönyvtár, privát firmware; nincs feltöltött gyártói állomány. Ez nem gyári Wave ROM-hanghűségi, macOS-, DAW- vagy fizikai MIDI-elfogadás.

A feltételezett indítási beragadt hang ebben a tesztesetben nem reprodukálható, ezért az upstream tisztítás nem került át. A teszt egy aktuális szerkesztési csatornát fed le; nem igazolja az összes Performance-part és sustain-állapot hibamentességét. További motorváltoztatás előtt ilyen célzott reprodukció szükséges. A nyilvános CI firmware nélkül továbbra is SKIP-ként jelenti ezt a tesztet.

## 2026-11-01 — Setup információs sorok rendezése

A Blue Files and folders információs része az útvonalak sorrendjét követi: Patches, ROMs, Skins. A három sor bal oldali neve félkövér, közös szélességű címkeoszlopban; a részletek normál betűvel jelennek meg. A ROMs sor a ténylegesen kiválasztott firmware-, Wave ROM- és KS-adatfájlneveket mutatja, nem rögzített példaneveket. Hosszú soronkénti szövegre a meglévő szélességhez igazítás érvényes. Standard/Retina közös rajzolás, Legacy szövege változatlan. Ez megjelenítési változás; importálási és keresési szabály nem módosult. Ellenőrzés: tiszta diff; platformfordítás és interaktív hosszúfájlnév-próba a CI/használati ellenőrzés feladata.


## 2026-11-02 — Ellenőrzött upstream első kör és K-Station bemeneti diagnosztika

A #26 PR javított fejcommitja `23a5d401f7a79e443e36fda987c8fff58a1b40b3`: [Windows](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41416475564), [macOS Universal 2](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41416475465) és [web](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41416475462) ellenőrzése **PASS**. Mindkét natív platformon mind a 11 ROM nélküli teszt átment; a külön firmware-es latenciateszt a publikus CI-ben **SKIP**. A korábbi `77721f1` hibás futásait ez nem minősíti át.

Ugyanezen kód helyi Windows-ellenőrzése saját K-Station 2.101 és KS 2.102 firmware-rel, KS All Data fájllal és kifejezetten **szintetikus nulla hullámbankkal** is megtörtént. A persistence/automatikus indulás/hostállapot/hiányzó adatok és a latencia/host-visszahívás/frekvencia-/pufferváltás/állapot-visszatöltés/offline–realtime átmenet vizsgálata **PASS**; a persistence és latencia kör `SILVER_KS_CONTROLLER=native` ismétlése is **PASS**. A felhasználó archívumai csak olvasva voltak, a kinyert privát adatok és runtime profilok elkülönítve maradtak, nem kerültek Gitbe. Ez nem eredeti mintahullám-hűségi, DAW-, fizikai MIDI- vagy macOS firmware-es elfogadás.

A következő kis fejlesztési egység a Blue main `2e933fb48c4f17132d6e708515991d9ef3656a07` alapján, a #26-tól függetlenül készül. Upstream forrás: temaniak/Silver-K `ab91ba112f2dd7be5ad21c1a6567290fe7ef20e6`, K-Station input drop accounting és controller diagnosztika. Megvalósítás a fejlesztési ágon:

- A K-Station eszköz számlálja a host–worker bemeneti sor és a DSP bemeneti puffer elutasított mintáit. A meglévő eldobási/routing működés változatlan; csak a visszautasított minták száma válik megfigyelhetővé. Az atomikus számláló az engine élettartamához tartozik, nem nullázódik diagnosztikai Stop/Start hatására.
- A meglévő JSON-riport K-Station esetén tényleges `original-h8` controller-backendet ad a korábbi `not-applicable` helyett. Ez nem új vezérlőmotor és nem K-Station natív C++ migráció.
- Új JSON-mezők: `input_overflows`, `input_overflows_supported`, `input_overflows_scope`; a sessionben `input_overflows_at_start` és támogatottság. KS/engine nélküli állapotban a támogatottság hamis: a nulla érték ott nem a KS bemeneti sor veszteségmentességének bizonyítéka.
- A `silver_input_diagnostics` ROM nélküli része a séma/elérhetőség viselkedését ellenőrzi. Egy saját K-Station firmware argumentummal a valódi mono bemeneti út, backend, session-kezdőérték és Stop/Start alatti monoton számláló is vizsgálható. A ROM nélküli PASS nem a privát rész elfogadása.

A fejlesztési egység nem módosít skint, editorgeometriát, kezelőszervet, buszelrendezést vagy pluginazonosítót. A célzott új teszt és a platform-CI eredményét a fejlesztési PR rögzíti; a fenti #26 eredmények nem az új diagnosztikai kódra vonatkoznak.


Az új diagnosztikai egység helyi MSVC C++17 buildje és ROM nélküli tesztje **PASS**. Saját K-Station 2.101 firmware-rel a valódi offline mono bemenet veszteségmentessége, majd szándékosan túlméretezett realtime hostblokkal a túlcsordulási számláló növekedése, valamint a Stop/Start alatti értékmegőrzés is ellenőrzött. A túlterhelési fixture nem realtime benchmark és nem megfigyelt hétköznapi hanghiba. A teljes Windows/macOS CI az új PR feladata; a helyi eredmény nem platform-PASS állítás.


### K-Station diagnosztikai PR első CI-köre és macOS timer-teszt

A `f7e5ba9` Windows- és webes CI-je **PASS**; a macOS build sikerült, az új `silver_input_diagnostics` teszt átment, de a meglévő `silver_native_editor` teszt a forgatógomb átmeneti értékfeliratának visszaállítására várva hibázott. A teszt 1450 ms után feltétel nélkül ellenőrzött, miközben a produkciós timer 1200 ms és a futtató üzenetsorában az aszinkron értesítések késleltethetik vagy újraindíthatják az időzítést. Ez a napló nem állít bizonyított grafikai regressziót.

A macOS teszt most az elvárt Semitone feliratot figyeli az üzenetsor pumpálása közben, legfeljebb 5 másodpercig. Továbbra is hibázik, ha a felirat nem áll vissza; hiba esetén az aktuális szöveg/szerkesztési állapotot is kiírja, és ellenőrzi, hogy a paraméterérték megmarad. A produkciós 1200 ms timer, rajzolás és GUI-elrendezés változatlan. A javított fejcommit platformeredménye külön ellenőrizendő; az első macOS FAIL megmarad történeti eredményként.


### Diagnosztika: javított CI és main összevezetése

A #27 javított `0c1cbf16bcba817e554bcdacffa9b2c00b26d583` commitján a [Windows](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41428949293), [macOS](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41428949502) és [web](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/41428949315) futás **PASS**. A produkciós GUI változatlan maradt.

A #26 közben mainbe olvadt (`8e14518`). A #27 fejlesztési ágának összevezetése megőrzi annak teljes tesztlistáját és a külön firmware-es latencialépést, és hozzáadja a diagnosztikai tesztet: 12 ROM nélküli teszt, továbbra is külön firmware-es eredménnyel. Az összevezetett fejcommit új CI-eredménye külön ellenőrizendő; a korábbi PASS nem automatikus igazolása az új revíziónak.


## 2026-11-02 — Áttekintés utáni dokumentációs egyeztetés

A repository áttekintésekor 16 helyi Python-teszt PASS; a mono bemeneti regresszió Clang C++17, AddressSanitizer és UndefinedBehaviorSanitizer mellett PASS. Teljes helyi pluginfordítás és privát firmware-es próba ebben a körben nem futott. A webes helyi teszt Node/npm hiánya miatt nem futott; a main web CI-je PASS. A #28 zöld, friss mainre épülő fejét beolvasztottuk; a #29 a dokumentációt ehhez igazítja. A RELEASE.json korábbi közös identitásra utaló értéke javítva; a régi csomagkiadási dátum történeti jelölést kapott. Nem készült új Release vagy GUI-módosítás.

## 2026-11-02 — Tételes upstream v0.5a leltár

A `338c703` Blue main és az upstream `ab91ba1` v0.5a összevetése alapján nem teljes az átvétel. A [részletes leltár](12-Upstream-v05-atveteli-leltar.md) külön rögzíti a már átvett mono bemeneti/diagnosztikai javításokat, a még hiányzó sztereó/KS hangutat, host- és mikrofonengedély-teszteket, natív K/KS migrációt és kutatóeszközöket, a drum-flash fixture-t és a macOS fejlesztői indítás adatútvonal-továbbítását. A #28 teszt mainben van, a runtime CC-tisztítás nincs átvéve. A GUI és a baseline nem változik; ez dokumentációs leltár, új runtime kód vagy kiadás nélkül.

## 2026-11-02 — Izolált macOS fejlesztői indítás

A #30 leltár PR Windows/macOS/web ellenőrzése sikeres, beolvasztva (`5f5778a`). A következő kis fejlesztési lépcső a `script/build_and_run.sh`: az upstream megoldás alapján a LaunchServices explicit megkapja a nem üres `SILVER_K_DATA` értéket, így a fejlesztői app ugyanazt a tesztadatmappát használhatja, mint a parancssor. A korábban bent maradt eredeti app/binary/log nevek Silver-K Blue-ra javítva mindkét build-elrendezésben.

Ellenőrzés: `bash -n`, `git diff --check` PASS; ideiglenes helyettesített open/cmake/pgrep parancsokkal a két elrendezés, hiányzó/üres/egyedi adatútvonal és szóközös útvonal PASS. Ez az argumentumátadást ellenőrzi, valódi LaunchServices-/appindítást nem helyettesít. GUI, motor, termékazonosítók változatlanok; nincs kiadás. PR CI eredménye még függő.

## 2026-11-02 — KS indulási regresszió: sustain és minden hangkimenet

A #31 Windows/macOS/web ellenőrzése sikeres, beolvasztva (`88e1a2a`). Következő tesztlépcső: a meglévő KS indulási próba immár mind a négy hangkimeneten ellenőrzi a véges mintákat és a csendet. Az érintetlen indulási/mixer/vocoder ellenőrzések után külön pozitív kontroll vizsgálja a CC64 sustain bekapcsolását, a Note Off utáni kitartást és a pedál elengedése utáni csendet, Panic/CC120/CC123 nélkül.

Ez a lépcső továbbra is az aktuális szerkesztői MIDI-csatornát vizsgálja; nem állít teljes 16 csatornás vagy négypartos indulási lefedettséget. A következő bővítésnek a partok aktiválását és csatornakiosztását is kontrollált fixture-rel kell biztosítania. A motor CC-tisztítása továbbra sincs átvéve, GUI/runtime változás nincs.

Helyi ellenőrzés: Universal 2 két architektúrájának C++ szintaxisvizsgálata és diff-ellenőrzés; a firmware-es sustain próba tényleges futtatása még hátravan. A publikus CI firmware nélkül ezt az ágat SKIP-pel zárja, ami nem bizonyítja a hangviselkedést. Az új teszteset eredménye ezért nem kezelhető privát firmware-es PASS-ként.

## 2026-11-02 — Elkülönített KS part/csatorna sustain-mátrix (fejlesztés)

A #32 Windows/macOS/web ellenőrzése sikeres, beolvasztva (`15a6a97`). Új, opcionális firmware-es teszt készült: `SILVER_TEST_KS_CHANNEL_MATRIX=1` esetén a latency teszt az érintetlen indulási ellenőrzés UTÁN lép Performance módba. Négy egymást követő kiosztásban a négy part együtt lefedi mind a 16 MIDI-csatornát. A firmware-ből mentett Performance dumpban ellenőrzi az aktív maszkot és a kiosztást, majd minden part/csatorna esetén hallható Note On → sustain alatti Note Off → pedálfelengedés utáni csend pozitív kontrollt végez, mind a négy kimenetet figyelve.

Ez visszatöltés UTÁNI MIDI/sustain teszt, nem hidegindítási bizonyíték: a Performance mód váltása állapotot tölt vissza. Nem helyettesíti az induláskori 16 csatornás tulajdonosvizsgálatot. A GUI és a motor változatlan, a CC64/123/120 indulási takarítás nincs átvéve.

Validáció első körben: arm64/x86_64 szintaxis és diff ellenőrzés PASS. A PR kezdetben draftként készült a még hiányzó privát firmware-es próba miatt; ennek későbbi eredményét az alábbi bejegyzés rögzíti. Firmware nélkül a meglévő teszt SKIP, az opcionális kapcsoló nélkül külön NOT RUN sor jelzi a mátrix kimaradását. Privát futtatáskor a `silverLatency_test` programot közvetlenül kell indítani a szokásos K/KS firmware/data/wave környezeti változókkal és az új kapcsolóval; a hosszabb mátrixot nem kötöttük automatikusan a meglévő 180 másodperces CTest-keretbe.

### #33 privát firmware-es elfogadás és sorrendjavítás

Helyi macOS arm64 Release fordítás a #33 aktuális forrásából sikeres. A Performance módváltás után külön feldolgozási/szinkronizálási lépés került a tesztbe a partparaméterek írása ELŐTT, mert a módváltás csak sorba állítja a state dumpot. A teszt naplózza a tényleges KS vezérlőmódot is.

Az új forrást újrafordítva, explicit H8 (`controller_mode=0`) és native (`controller_mode=1`) választással a teljes `silverLatency_test` kétszer sikeres, exit 0. Mindkét futás mind a 16 part/csatorna-sustain esetet, az indulási csendet, a pedálpróbát és a teljes K/KS latency/buffer/rate/state/async életciklust teljesítette. A mátrixban a hallható/kitartott csúcsok kb. 0,026, minden pedáleengedés utáni csúcs 0. Firmware: K-Station 2.1.01 és KS 2.1.02, helyi `KS All Data.MID`, zero Wave ROM. Ez nem igazol valódi sampled-wave/drum-hanghűséget vagy teljes hidegindítási 16 csatornás lefedettséget. Privát adat és nyers log nem került a repositoryba.

A privát tesztfeltétel teljesült, a PR készre jelölhető; beolvasztás csak az új javított commit összes kötelező CI-ellenőrzésének sikere után. A korábbi macOS újrafutások közül egy a csomagfeltöltés GitHub-időtúllépésén, mások szabad hosted ARM gép hiányán álltak meg; ezek nem firmware-es teszthibák.

## 2026-11-02 — Sztereó bemeneti adapter előkészítése

A #33 javított fejének Windows/macOS/web ellenőrzése sikeres, beolvasztva (`db11f24`). A következő kis lépcső a meglévő mono bemeneti pufferadapter opcionális sztereó változata, az upstream `ab91ba1` KEngine szabályával: a két csatornán külön NaN/Inf → 0, majd 0,5 L + 0,5 R. A teljes 256 mintás részblokk másolása a kimenetre írás előtt történik. A meglévő mono hívások kompatibilis wrapperen át ugyanazt a viselkedést tartják meg.

Ez még nem sztereó busz vagy KS ADC integráció: a host/engine sztereó bekötése külön lépcső. GUI, vezérlő és felhasználói routing nem változik. A helper előkészítése lehetővé teszi, hogy a sztereó bekötés később már ellenőrzött keverést használjon.

Helyi ellenőrzés: önálló C++17 teszt `-Wall -Wextra -Werror`, AddressSanitizer és UndefinedBehaviorSanitizer mellett PASS. Régi mono esetek megmaradtak; új sztereó esetek mindkét csatorna kimenettel közös puffereit (fordított bekötéssel is), nem véges mintákat, 0/1/255/256/257/511/1024 mintás határokat, őrzőértékeket, maximális véges amplitúdót és hiányzó elsődleges bemenetet vizsgálnak. `git diff --check` PASS. CI és integráció függő.

## 2026-11-03 — K-Station belső sztereó bemeneti interfész

A #34 Windows/macOS/web ellenőrzése sikeres, beolvasztva (`5e9da90`). A következő kis lépcső a `KEngine::process` opcionális jobb bemeneti mutatója és a már tesztelt közös sztereó pufferadapter használata, az upstream v0.5a interfészével összhangban. A régi hívások változatlanul fordulnak, a hiányzó jobb bemenet a korábbi mono működést tartja meg.

A host busz továbbra is mono; a Processor sztereó bekötését nem engedélyeztük egyoldalúan. A KS-ben még külön ADC-konverzió, időzített bemeneti sor és mindkét DSP RX0 táplálása szükséges. A globális busz sztereóra bővítését ezért csak a KS út és firmware-váltási teszt elkészülte után szabad engedélyezni, különben a KS mód látszólag elfogadhatna olyan bemenetet, amelyet nem dolgoz fel. A K natív vezérlő migrációja ettől külön feladat, nem történt meg.

Validáció: a mono/sztereó pufferadapter összes regressziós esete ASan/UBSan és szigorú figyelmeztetések mellett PASS; az aktuális forrás `silverKEngine.cpp` és `silverProcessor.cpp` arm64 és x86_64 szintaxisellenőrzése PASS; diff-ellenőrzés PASS. Ez nem hoston át végzett sztereó hangpróba. GUI, meglévő routing, terméknév/azonosítók változatlanok. CI és integráció függő.

## 2026-11-03 — KS ADC-konverter célzott átvétele

A #35 Windows/macOS/web ellenőrzése sikeres, beolvasztva (`5ecce5b`). A következő lépcső az upstream `ab91ba1` két változatlan fájlja: `ksAudioInput.h` és `ksAudioInput_test.cpp`. A konverter a host mintáit szűrt, 24 bites KS ADC mintákká alakítja; 64 tap/256 fázis, 31 hostmintás bemeneti késleltetés. Az együtthatók előkészítése konstrukciókor történik; a majdani hangfeldolgozási út nem ott készíti őket.

Az önálló `ks_audio_input` CTest bekerült a Windows/macOS Blue ROM nélküli céljai közé (a kiválasztás 12-ről 13-ra bővült). A tisztán konverteres teszthez nem kapcsolunk H8 referenciaobjektumokat. Helyi C++17 szigorú warning + ASan/UBSan próba PASS: csend, DC, NaN/Inf, 24 bites telítés, sztereó kioltás, 44,1/48/96/192 kHz átvitel és magas frekvenciás alias-elnyomás. Diff-ellenőrzés PASS.

A konverter még nincs bekötve a KS workerbe vagy DSP-kbe; önmagában nem engedélyez külső KS hangbemenetet. Következő függő lépcső a DSP RX0 bemenet és sorainak tesztelése, majd a Device időzítés és hostbekötés. GUI, jelenlegi hangút és vezérlők változatlanok. CI és integráció függő.

## 2026-11-03 — KS Device ADC-út (draft fejlesztés)

A #36 Windows/macOS/web ellenőrzése sikeres, beolvasztva (`d05564c`). Az újabb forrásellenőrzés pontosítása: a DSP `pushAudioInput`/RX0 sora már a Blue alapjában jelen van és megegyezik a vizsgált upstream fájlokkal. Ezt nem kell újra átvenni. A tényleges hiány a Device host→worker időzített sora és a konverter használata volt.

A célzott Device-átvétel az upstream `ab91ba1` alapján: opcionális mono/sztereó bemeneti mutatók, véges minták keverése a kimeneti felülírás előtt, host mintaszámmal és lead-del időzített SPSC sor, worker oldali mintakivétel, ADC konverzió és mindkét DSP táplálása. Bemeneti túlcsordulásszámláló hozzáadva, a Processor diagnosztikájába még nincs bekötve. A korábbi `controllerReferenceAvailable` API és a Blue vezérlőmódok megmaradnak; az upstream indulási CC-takarítást nem másoltuk át.

Helyi ellenőrzés: Device C++ szintaxis arm64/x86_64 és diff PASS. A teljes arm64 Release fordítás és a privát firmware-es regresszió H8 és natív vezérlővel is sikeres: négykimenetes indulási csend, 16 csatornás sustain mátrix, mintavétel/blokkméret váltás, állapot-visszatöltés és offline→realtime átmenet. A külön `ksDeviceInput_test` natív Device-próbája is PASS: 48 kHz/256 minta, privát KS OS és zero-wave, repository INIT flash. Mono és azonos L/R sztereó peak 0.0227428; ellenfázisú sztereó, kezdeti és utólagos csend 0; mindkét bemenet/kimenet közös puffere mellett ugyanaz a peak; input overflow 0. Ez a Device offline pozitív kontrollja, nem valódi DAW/realtime vagy mintahang-hűségi bizonyíték. A teszt privát fixture nélkül 77/SKIP; nem számít firmware-es CI PASS-nak. A korábbi null-bemenetes regresszió ettől külön ellenőrzés. Processor/host bekötés, sztereó busz, mikrofonengedély és felhasználói működés még nincs engedélyezve. GUI változatlan. Ez az állapot nem kész külső hangbemeneti funkció és nem beolvasztható pusztán zöld, ROM nélküli CI alapján.

## 2026-11-03 — KS mono hostbemenet és diagnosztika

A #37 minden Windows/macOS/web ellenőrzése sikeres, beolvasztva (`3b298ea`). A következő körülhatárolt lépcső a már elfogadott mono hostbemenet továbbítása a KS Device felé. A bemenet a JUCE közös bemeneti/kimeneti pufferéből érkezik; a Device az olvasást a kimenet felülírása előtt végzi. Kikapcsolt bemenetnél nullptr kerül átadásra. A busz továbbra is mono; sztereó busz és standalone alapértelmezés nem változik.

A diagnosztika KS esetén most a tényleges bemeneti sor túlcsordulását mutatja, `ks-host-input-queue` hatókörrel; a K-Station korábbi host/DSP számlálója változatlan. A `SILVER_TEST_HOST_INPUT=1` privát latency-próba engedélyezi a mono buszt, néma belső oszcillátorok mellett 440 Hz külső jelet ad a Processor bemenetére, majd csendet; ellenőrzi a véges kimenetet, a pozitív hangkontrollt és a diagnosztika támogatottságát/hatókörét/nulla túlcsordulását. A teszt után visszaállítja a belső hangforrást a további regressziókhoz.

Helyi arm64 Release fordítás és natív KS firmware-es teljes latency/regressziós próba PASS, K-Station átmenetekkel együtt. Külső jel peak 0.0227428; előtte/utána csend 0. A privát ROM és zero-wave nincs a repository-ban. Ez processzorszintű offline próba, nem valódi AU/VST3 host vagy mikrofonengedély teszt. GUI és Blue azonosítók változatlanok.

## 2026-11-03 — Sztereó hostbemenet mindkét modellhez

A #38 Windows/macOS/web ellenőrzése sikeres, beolvasztva (`96545bb`). A következő lépcső engedélyezi a sztereó bemeneti buszt a mono és kikapcsolt állapot mellett, és továbbítja a jobb csatornát mindkét motor meglévő, ellenőrzött keverő útjára. A standalone bemenet továbbra is alapból kikapcsolt; mikrofonengedély és valódi hostpróba külön feladat. GUI és termékazonosítók változatlanok.

Az opcionális privát `SILVER_TEST_HOST_INPUT=1` teszt mindkét firmware-modellnél ellenőrzi a mono, sztereó, ellenfázisú és jel nélküli működést a Processor közös bemeneti/kimeneti pufferén át; a diagnosztikai hatókört modell szerint vizsgálja. Helyi arm64 Release fordítás és teljes natív KS/K-Station regresszió PASS. KS mono/sztereó peak 0.0227428; K-Station mono 0.0931342, sztereó 0.0931351; ellenfázis és elő-/utócsend mindkettőnél 0, input overflow 0. A sztereó peakre 1% tolerancia van, a firmware belső állapota miatt nem bitazonosságot követelünk. A zero-wave fixture nem bizonyít mintahang-hűséget.

## 2026-11-03 — Standalone bemeneti alapbeállítás és macOS leírás

A #39 minden Windows/macOS/web ellenőrzése sikeres, beolvasztva (`d1ddc3e`). Az upstream v0.5a következő kis lépcsője a mono bemeneti busz engedélyezése csak standalone példánynál; AU/VST3 alapértelmezés továbbra is kikapcsolt. A CMake mikrofonhasználati leírása a Blue terméknevet használja, a bundle/plugin azonosítók változatlanok.

Helyi CMake konfiguráció PASS; a generált standalone Info.plist tartalmazza az `NSMicrophoneUsageDescription` kulcsot és a Blue nevű magyarázatot. A használt JUCE standalone kód a bemenetigény alapján kéri a recordAudio engedélyt, és a bemenet némítása alapból aktív (`muteInput=true`); az alkalmazás nem kezd automatikusan hallható mikrofon-visszacsatolásba. Ez forrásellenőrzés, nem helyettesíti a valódi macOS engedélyezési és visszautasítási próbát. A fizikai bemenet kiválasztása, némítás ki/be, engedély elfogadása/elutasítása és újraindítás utáni viselkedés kézi kiadás előtti teszt marad. GUI változatlan.

## 2026-11-03 — Firmware nélküli bemeneti buszregresszió

A #40 kötelező Windows/macOS/web ellenőrzései sikeresek, beolvasztva (`b29e308`). A már mindkét platform CI-jében futó `silver_input_diagnostics` teszt most a tényleges Processor buszszerződését is vizsgálja: kikapcsolt/mono/sztereó bemenet elfogadása és beállítása; háromcsatornás bemenet elutasítása; eredeti kiosztás visszaállítása. Firmware nélkül nem nulla mintákkal feltöltött összes kimeneti csatorna a feldolgozás után néma kell legyen. Ez különösen a közös bemeneti/kimeneti puffer miatti közvetlen bemeneti átszivárgást védi.

Helyi arm64 Release fordítás és ROM nélküli futás PASS. A teszt meglévő privát K-Station ága változatlan; annak kihagyása nem minősül firmware-es PASS-nak. Nincs új felhasználói funkció, GUI vagy kiadás. A valódi AU/VST3/Standalone input-/engedély-/realtime ellenőrzések még hátravannak.

## 2026-11-03 — Bemeneti mintavételváltási regresszió

A #41 Windows/macOS/web ellenőrzése sikeres, beolvasztva (`020fbd3`). A privát latency-próba új `SILVER_TEST_INPUT_RATES=1` kapcsolója a meglévő 48 kHz/256 mintás pozitív kontroll után 44,1 és 96 kHz/512 mintás motor-újraépítéssel ismétli a mono/sztereó/ellenfázis/csend és input-diagnosztikai próbát, majd visszatér a teljes belső hangforrás-/latencia-/állapotregresszióhoz. A kapcsoló a `SILVER_TEST_HOST_INPUT=1` mellett értelmes.

Az első tesztbővítésnél a visszaállított mixerüzenetek feldolgozása előtt történt újraépítés, ezért az utána következő belső hangpróba néma lett. A fixture most külön feldolgozási szakasszal megvárja a visszaállított MIDI vezérlők érvényesülését. Ez tesztsorrendi javítás; nem bizonyítja, hogy felhasználói vezérlőváltoztatással pontosan egy időben végzett host-újraépítés minden versenyhelyzetét lefedtük. A kezdeti hibát nem pusztán a teszt áthelyezésével kerültük meg: az eredeti sorrendben is sikeres a teljes futás.

Helyi arm64 Release és natív KS/K-Station teljes futás PASS. Mindkét modell három mintavételen hallható külső jelet adott, sztereó/mono eltérés 1% alatt; ellenfázisú kimenet és jel nélküli szakasz 0, input overflow 0. A további belső hangszerhang, state restore, offline/realtime átmenet is PASS. Privát firmware nem került be a repository-ba; ez nem valódi DAW vagy fizikai mikrofon teszt. GUI és runtime kód változatlan.

## 2026-11-03 — Valódi AU/VST3 wrapper bemeneti próba

A #42 kötelező Windows/macOS/web ellenőrzései sikeresek, beolvasztva (`16d0d96`). Átvettük az upstream `ab91ba1` opcionális `silver_external_host_check` eszközét; a dinamikusan keresett AU factory a Blue exportnevét használja. A folyamaton belüli tesztregisztráció külön `BlXi` subtype, ez nem módosítja a kiadott plugin `SkBl` azonosítóját. A teszt közvetlenül a megadott build bundle-jét tölti, nem telepít plugint.

A privát Processor állapot előkészítése a meglévő firmware-es teszt `SILVER_TEST_EXPORT_INPUT_STATE` opcionális könyvtárával történik; csak a konfigurált, MIDI-ben is érvényesített külső bemeneti állapotot írja ki. Az állomány privát adatokat és eszközútvonalakat tartalmazhat, nem commitolható/publikálható. Futtatás: `silver_external_host_check vst3|au <bundle abszolút útja> <privát processor.state>`. A külön buildcél EXCLUDE_FROM_ALL; a hosteszköz nem része a felhasználói csomagnak.

Helyi macOS arm64 Release AU/VST3 és hosteszköz fordítás PASS. Tényleges wrapperbetöltés és 48 kHz/256 mintás mono offline feldolgozás mindkét modellen PASS: KS AU/VST3 signal RMS 0.0160814, K-Station AU/VST3 0.0658167; jel nélküli RMS mindegyiknél 0. Az AU input callback mindkét esetben 1126 hívást kapott. Az első AU regisztrációs próbát a korlátozott végrehajtási környezet blokkolta; rendszerhang-szolgáltatás hozzáféréssel sikeres, nem plugin-runtime javítás történt.

Ez valódi AU/VST3 wrapperbizonyíték, de nem REAPER projektpróba, nem Windows runtime, nem Intel/Rosetta futtatás, nem mikrofonengedély vagy fizikai input, nem realtime terhelésmérés, és nem vocoder/filter feldolgozás teljes tesztje. Ezek kiadás előtti illetve későbbi célzott próbák maradnak. GUI és termékazonosítók változatlanok.

## 2026-11-03 — Nyitott KS szűrő/burkoló bemeneti vizsgálat

A #43 beolvasztva (`993b55f`), összes kötelező ellenőrzése sikeres. Az új `SILVER_TEST_INPUT_FILTER=1` célzott próba (a `SILVER_TEST_HOST_INPUT=1` mellett) a közvetlen effektút után kikapcsolja a direct-to-FX routingot, lenémított belső hangforrásokkal teljesen nyitott szűrőt és azonnali, teljes sustain burkolót állít, host-MIDI Note On/Off segítségével vizsgálja a külső jel hangindítását és elengedését. Ez jelenleg vizsgálati teszt, nem kész regressziós PASS.

K-Station eredmény PASS: filtered peak 0.0107882, released 0, további teljes belső hang-/latenciaregresszió PASS. KS eredmény FAIL: filtered/released 0, miközben ugyanabban a futásban a direct-to-FX mono/sztereó bemenet peak 0.0227428. Az eredeti H8 és a Blue natív vezérlő egyaránt reprodukálja a KS némaságát. UI MIDI helyett host-MIDI, external-trigger bit bekapcsolása és silent-wave hangkiosztási kontroll sem oldotta meg. A firmware-snapshotban external level 127, cutoff 127, keytrack 0, attack/decay/release 0, sustain 127, direct-to-FX bit 0, volume 127, output/channel 0 igazolt. A vizsgálat nem állapította meg az okot; nem minősíthető még biztosan runtime hibának vagy natív migrációval javulónak.

A GUI és a production kód nem módosult. A vizsgálati lépcső nem beolvasztható pusztán ROM nélküli zöld CI alapján. Következő: a KS belső hangforrás/note pozitív kontroll és a vizsgált upstream v0.5a azonos fixture-rel történő ellenpróbája; csak igazolt ok után célzott javítás. A külső szűrő/burkolóút teljes működését a kiadásban egyelőre nem szabad igazoltnak állítani.

### Azonos napi ellenpróbák

A Blue KS fixture belső oszcillátoros Note On pozitív kontrollja hallható (peak 0.0260717), miközben a külső szűrőút néma. Az eredeti upstream v0.5a (`ab91ba1`) külön, módosítatlan forrásból fordított `silverExternalInput_test` próbája ugyanazon helyi firmware/KS All Data/zero-wave fájlokkal, 48 kHz/256 mintán sikeres: direct RMS 0.0160808, filtered RMS 0.00189148, released 0, vocoder RMS 0.00656004, muted 3.63324e-07. Az RMS és peak értékek nem közvetlenül összehasonlíthatók. Ez szűkíti a vizsgálatot, de a tesztek eltérő inicializálása és a vezérlőimplementáció különbsége miatt még nem bizonyítja a konkrét hiba okát. Következő lépés a fixture és a KS paraméter-/routing-beállítások összevetése. Privát fájl vagy nyers log nem került a repositoryba. A #44 továbbra is draft, nem beolvasztható.

További helyi kizáró próba: a szűrőút tesztje előtt mind a 16 csatornán CC64/123/120 küldése és feldolgozása sem szünteti meg a némaságot (filtered/released 0, belső oszcillátor peak 0.0260249). Ez nem azonos az upstream konstruktoron belüli hidegindítási takarítással, ezért annak hatását nem zárja ki. Az upstream normál célja nem engedi a H8 módot, így abból H8 ellenpróba nem készült.

Az upstream 16 csatornás CC64/123/120 hidegindítási takarítását a Device konstruktorában, a patch-visszaállítás után és a meglévő preroll előtt helyben kipróbálva a KS szűrőút továbbra is néma (filtered/released 0; belső kontroll peak 0.0260717). A kísérleti runtime változtatást visszavontuk. A bemeneti fixture mentett programjaiban csak az attack/release beállítás tért el, amelyeket a szűrőpróba később eleve nulláz; ez önmagában nem magyarázza az eltérést.

Közvetlen tesztazonosítás: az upstream `silverExternalInput_test` teszttörzsét a Blue könyvtárakkal is helyben lefordítottuk (csak az eltérő backend-név ellenőrzését hagytuk ki). Ugyanazokkal a privát adatokkal a direct/mono/stereo/cancel/disconnect kontrollok PASS, de filtered/released RMS 0; a vocoder és muted RMS 0.012764/0.0127634, tehát az input nélkül sem némul. Az upstream eredeti könyvtáraival ugyanez a szűrő/vocoder próba PASS. Így az eltérés nem pusztán a két teszt eltérő sorrendjéből származik. A vizsgálati fájlcserét visszaállítottuk, production változtatás nincs; a vezérlő/DSP programozási különbség további elkülönítést igényel.

### Igazolt ok és célzott javítás

A KS DSP B soros olvasási callbackje az ADC által kitöltött RX0 bemenetet felülírta a DSP A TX1 segéd-DAC jelével. Az upstream megfelelő javítását célzottan átvettük: az A→B ütemezett keretet továbbra is ürítjük és számoljuk, de B RX0 értékét az ADC callback számára megőrizzük. Ezzel a helyi natív firmware-es teljes silverLatency_test PASS (exit 0): KS filtered peak 0.00267446, released 0; K-Station filtered peak 0.0107882, released 0. A teljes további latency/rate/buffer/state regresszió is sikeres. A GUI és Blue azonosítók nem módosultak. H8 ellenpróba és a javított head platformellenőrzése még szükséges; a PR egyelőre draft marad.

## 2026-11-04 — KS ADC javítás ellenőrzése

A #44 javított `a5d3b22` commitjának Windows/macOS/web ellenőrzései sikeresek. A privát arm64 H8 ellenpróba is teljes PASS (controller_mode=0, KS filtered peak 0.00267446, released 0, teljes latency/rate/buffer/state/offline→realtime regresszió exit 0), egyezően a korábbi natív eredménnyel. A javítás így nem függ a nagy v0.5a natív migrációtól. A nyitott vizsgálat konkrét szűrőút-okát ez lezárja; a külön vocoder és tényleges realtime/DAW validáció nem tekinthető ettől késznek. A leltár frissítve; beolvasztás csak a végső head kötelező ellenőrzései után.

## 2026-11-04 — Külső vocoder pozitív és némítási kontroll

A #44 minden végső head ellenőrzése sikeres, beolvasztva: `b91c3cc`. Friss mainből a meglévő opcionális SILVER_TEST_INPUT_FILTER próba a vocoder hordozóját is ellenőrzi: külső modulátorral hallható jel, majd a modulátor eltávolítása után legalább tízszeres csillapítás. Privát arm64 natív teljes futás PASS: K-Station peak 0.0598646 / muted 3.14492e-06; KS peak 0.0119195 / muted 8.49364e-07. A további teljes latency regresszió is PASS. Runtime és GUI változtatás nincs. H8 ellenpróba, platform CI és tényleges realtime/DAW teszt külön hátravan.

## 2026-11-04 — Vocoder tesztlépcső lezárása

A #45 `231ea54` headjén Windows/macOS/web PASS és beolvasztható állapot igazolva; a privát arm64 H8 futás is teljes PASS (controller_mode=0, KS vocoder/muted peak 0.0119195 / 8.49364e-07). Beolvasztva: `8c71d94`. A leltár a #43 hosteszköz, #44 ADC javítás és #45 vocoder teszt main státuszát követi. További kézi validáció: valódi DAW/realtime bemenet, Windows/Intel futtatás, fizikai bemenet és macOS engedély elfogadás/elutasítás. Ezekre a helyi offline arm64 és a ROM nélküli CI nem szolgál bizonyítékkal. Új kiadás nem készült.

## 2026-11-04 - Paced realtime input test

Optional SILVER_TEST_INPUT_REALTIME=1 (with SILVER_TEST_HOST_INPUT=1) tests wall-clock paced 48 kHz/256-sample realtime Processor input on Balanced profile. Checks finite four-channel output, signal and silence; reports missing sample frames. SILVER_TEST_INPUT_REALTIME_STRICT=1 additionally requires zero missing frames. Local arm64 KS native peak 0.0227428 and K-Station peak 0.0931327, silence and missing frames both zero; full regression PASS. Preliminary Low latency K-Station run had 916 missing frames, so no universal capacity claim. Physical input, actual DAW, H8 and platform validation remain separate. No runtime or GUI change.

## 2026-11-04 - Realtime rate coverage and diagnostic context

#47 merged as afd9ca4 after Windows/macOS/web checks and a strict arm64 H8 run passed. Combined SILVER_TEST_INPUT_RATES=1 and SILVER_TEST_INPUT_REALTIME=1 now reports the sample rate and block size on every paced measurement. Local native KS and K-Station signal/mute measurements completed at 48 kHz/256, 44.1 kHz/512 and 96 kHz/512: signal peaks approximately 0.022743 (KS) and 0.09313 (K), quiet 0, missing sample frames 0 in the completed reporting run; full regression PASS.

An earlier strict run stopped on 1222 missing K-Station frames at 48 kHz/256, despite passing signal/mute. The subsequent zero-frame result does not erase that failure or establish guaranteed realtime capacity. Treat this as machine/scheduling-sensitive evidence; physical input, DAW and Windows/Intel runtime checks remain open. The only test code change adds rate/block labels. No runtime or GUI change.

## 2026-11-04 — Natív K-Station elővizsgálat és CPU-korlát

A #48 minden kötelező ellenőrzés után mainbe került (`0079460`). A következő migráció előtt izolált upstream clean/observer és original/native mérések készültek; részletes eljárás, eredmény és korlát az [átvételi leltárban](12-Upstream-v05-atveteli-leltar.md#2026-11-04--k-station-natív-migráció-elővizsgálata).

Hat alap referencia-forgatókönyv pontos egyezést adott. Az opcionális program-forgatókönyv riport- és nyers egyezése mellett a tesztscript közös beállítási kapuja hibát jelzett, ezt megőriztük; teljes suite PASS nincs. A külön 24 futásos CPU-sorozat funkcionális riportkapuja sikeres, de a hosszabb megerősítő mérés nem mutat általános natív CPU-előnyt. Nem történik automatikus backendváltás. Következő munka a Blue eredeti H8 elleni szélesebb paritás, majd célzott migrációs döntés a bizonyítékok alapján. A termék forrása, GUI és Blue azonosítók nem módosultak; kiadás nem készült. Privát firmware, PCM, RAM/flash és nyers riport nem kerül a repository-ba.

## 2026-11-04 — Blue H8 / upstream H8 / natív paritás futtató

A #49 mainben (`27b6df3`). Új, csak fejlesztői `tools/kstationParity/compare.py` futtató rögzített helyi probe binárisokat hasonlít össze; nem tölti le vagy váltja át a termék vezérlőjét. A közös upstream harness Blue-kompatibilitási típusguardját és az izolált fordítás feltételeit a mellékelt README rögzíti.

Privát macOS arm64 Release ellenőrzés: mind a 12 forgatókönyv PASS a Blue eredeti H8, upstream H8 és upstream native között (idle, held1, held8, notes, sweep, arp, program, sustain, clockburst, expression, sysexpatch, programstress). Mind a 36 futás exit 0, dropped_frames 0; PCM/flash/MIDI teljes SHA-256 egyezés, patch/program/clock riportegyezés; native értelmezett H8 lépésszám 0. Két másodperc warmup + két másodperc offline capture, 256 minta. Bináris- és firmware-hash ellenőrzött. A korábbi observer script program-beállítási kapuhibája ettől külön, megőrzött eredmény.

Ez nem teljes RAM-paritás, Processor/device state-save/restore, GUI-, külső bemeneti, DAW-, Windows/Intel- vagy CPU-bizonyíték. A natív migráció és az alapértelmezett backend nem változott. A nyers firmware-eredetű állományok privát kimeneti könyvtárban maradnak. Helyi help/szintaxis és diff ellenőrzés PASS; termék-CI a PR végső commitján szükséges.

## 2026-11-04 — K-Station Device állapot ABI körpróba

A #50 mainben (`926be8f`). Az explicit, alapértelmezett buildből és CTestből kihagyott `kstationDeviceState_test` privát firmware-rel exportál/importál Device-állapotot. CC74=93 és CC71=27 pozitív kontrollt ellenőriz patch- és controller-snapshotban, továbbá véges sztereó kimenetet és megszólaló hangot követel. Meglévő kimeneti fájlt nem ír felül; backendjét kiírja. A használat és az upstream natív fordítás feltétele a tools/kstationParity/STATE.md dokumentumban szerepel.

Helyi arm64 Release: Blue original-h8 export → upstream explicit native-cpp-semantic import/export → Blue original-h8 import/export mindhárom futása PASS. A három teljes 134280 bájtos állapotfájl byte-azonos; a két kontroll megmaradt. Ez egy program és két CC Device ABI-próbája, nem minden bank/arp, Processor/DAW állapot vagy mintapontos hangfázis bizonyítása. A hang pozitív kontroll, nem bitazonossági feltétel. Privát állományok és nyers logok nem kerülnek repository-ba. Blue runtime, GUI és backendválasztás változatlan. Helyi fordítás és diff ellenőrzés PASS; platform-CI a végső PR-headen szükséges.

## 2026-11-04 — K-Station natív vezérlő választható fordítási integrációja

A #51 mainben (`08edfc3`). Az upstream `ab91ba1` natív K-Station vezérlő és összefüggő fejlécei izolált Blue munkapéldányba kerültek, a H8 facade/Hardware/Device függőségekkel együtt. A `SILVER_K_NATIVE_CONTROLLER` CMake opció alapértéke OFF; a normál termék továbbra is original-h8. ON kizárólag explicit fejlesztői fordításnál választja a natív motort. GUI választó, oldalátrendezés, Blue azonosító vagy verzióváltozás nincs. Az eredeti Blue input-overflow számlálás és a privát állapotpróba megmaradt.

Helyi arm64 Release ON: teljes Processor latency/reentry/buffer/rate/state/offline→realtime regresszió, mindkét modell mono/sztereó bemenete, 44,1/48/96 kHz, filter/envelope és vocoder pozitív/némítási kontroll PASS. K natív input diagnosztika PASS: támogatottság/backend/session, offline nominal drops 0, mesterséges túlterhelésre növekvő számláló. A ROM nélküli controller-device-state guard 30000 műveletet és 9838 buszeseményt ellenőrzött, PASS. A Windows/macOS workflow ugyanennek a guardnak a fordítását és futtatását is kéri; a végső head CI még szükséges. Forráskiválasztási Python tesztek (3) és diff ellenőrzés PASS.

A normál OFF konfiguráció lefordult; privát K input diagnosztika PASS, nominal drops 0. A teljes privát OFF latency/input/filter/vocoder/rate/state regresszió is PASS (exit 0). Natív CPU-előny továbbra sincs általánosan igazolva. A teljes KS v0.5a migráció, valódi DAW/fizikai bemenet és Windows/Intel firmware-es futtatás hátravan. Eredet és használati korlát: tools/kstationParity/INTEGRATION.md. Privát ROM, állapot, hang és nyers riport nincs a változásban.

## 2026-11-04 — Review: állapot-visszatöltési backend javítása

A #52 review feltárt egy upstream eredetű hibát: setState a replacement Hardware példányt explicit mód nélkül készítette, így native kiválasztás mellett H8-ra váltott, miközben a diagnosztikai név native maradt. A korábbi state-roundtrip byte-egyezés ezért nem bizonyította a natív backend megőrzését. A replacement most megkapja a tárolt controllerMode értéket, és a tényleges isNative állapotot is ellenőrzi a cached címke helyett. A korábbi állapotpróbák backend-megőrzési következtetése érvénytelen; javított firmware-es újrafuttatás szükséges. Beolvasztás nem történt.

Javított arm64 ON újrafuttatás: teljes privát latency/input/filter/vocoder/rate/state regresszió PASS (exit 0). A Blue H8 állapot → javított natív Device → Blue H8 körpróba teljes 134280 bájtja ismét azonos, kontrollok és hang PASS; a setState tényleges replacement isNative ellenőrzése most aktív. A privát Device eszköz ON konfigurációban explicit Native konstrukciót kap, nem hagyatkozik a Device Original alapértékére. A régi mérés hiányosságát ez a külön javított futás pótolja; a PR új head platformellenőrzései még szükségesek.

## 2026-11-05 — KS natív átvétel elővizsgálata

A javított #52 minden Windows/macOS/web ellenőrzése után mainbe került (`114f8aa`); a normál K-Station build továbbra is H8. KS-ben még nincs v0.5a motorcsere.

A fagyasztott upstream `ab91ba1` valódi natív executor/H8 összehasonlítása privát OS-sel PASS: 248 ciklus, 248442 fázis, 0 allokáció; a ROM nélküli admission/immutable védelmek és a privát natív cold-start → első Main próba is PASS. Ez nem a régi ksDspEquivalence/ksCpuCost legacy eszközök eredménye, és nem Blue termékintegráció.

Új explicit `ksBoardParity_probe` fejlesztői cél: ugyanaz a forrás vizsgálja a jelenlegi Blue hybrid Boardot és az upstream szemantikus natív Boardot. A natív példány valódi ownerét és nulla értelmezett H8 lépését ellenőrzi. A 40960 képkockás egyprogramos note/sustain/release szakaszban a teljes PCM (3932160 bájt), aktuális state SysEx (270 bájt) és user Flash (263168 bájt) egyezett; MIDI output mindkét oldalon üres, ezért nem pozitív MIDI-kimeneti bizonyíték. Peak mindkettőnél 1568304. Privát MIDI-ből a production vetítési szabályaival képzett 600 program/100 performance Flash és zero-wave fixture; nincs mintahang-hűségi állítás.

A probe alapértelmezett buildből, CTestből és kiadási csomagból ki van hagyva; meglévő kimenetet nem ír felül. Használat és korlát: tools/ksParity/README.md. Ez nem eredeti H8 Board-ellenpróba, nem négypartos/arp/bank/state-restore/DAW/CPU teljes bizonyítás. További vizsgálat szükséges a KS integráció előtt. GUI, termékazonosító, verzió és alapértelmezett motor változatlan; privát fájlok nem kerülnek repository-ba.

## 2026-11-05 — KS négypartos performance paritás

A #53 mainben (`77ad9c2`). A privát Board probe opcionális fourpart szakasza a valódi performance bank/program és SysEx flag/mask bájtokkal állítja be a négy részt, 1–4 MIDI-csatornán küld note/all-sounds-off eseményeket, és ellenőrzi a performance módot/15 aktív maszkot. Az első, hiányos fixture-beállítás maszkellenőrzése megbukott; a javítás a teszt helyes MIDI/SysEx előkészítése, nem runtime módosítás.

Blue hybrid és upstream szemantikus natív arm64 Release: 40960 frame, peak 883066 mindkettőnél; teljes PCM 3932160 bájt, performance-state 1038 bájt és user Flash 263168 bájt byte-azonos. MIDI output mindkettőnél üres. Ez összesített négy-részes kimeneti/állapot-paritás, nem minden part külön hallható pozitív kontrollja vagy minden performance/bank/arp/restore bizonyítéka. GUI és motorválasztás változatlan. A single alapértelmezett szakasz külön újrafuttatása a korábbi teljes PCM/state/flash/MIDI állományokkal egyezik (PASS); privát eredmények nem kerülnek a repository-ba.

## 2026-11-05 — KS szemantikus natív motor elkülönített fordítási opciója

A #54 mainben (`b48cb03`). A fagyasztott upstream `ab91ba1` összefüggő KS forrásegység külön ksNative/ksLib útvonalra került; az eredeti Blue ks/ksLib fájljai változatlanok. A SILVER_KS_SEMANTIC_CONTROLLER opció alapértéke OFF, ezért a normál fordítás továbbra is a meglévő Blue hybrid motort használja. ON egy koherens natív include/source készletet és az upstream production/comparison forrásszétválasztást választja. A Blue availability API adaptere megőrzi a meglévő GUI hívásait; új kezelőszerv vagy elrendezésváltozás nincs.

Helyi arm64 ON teljes privát latency/input/filter/vocoder/rate/state/offline→realtime regresszió PASS. A ROM nélküli ksSemanticController guard PASS: CPU nélküli fizikai állapot, nincs comparison factory, natív alapérték és immutable/admission védelem. Három forráskiválasztási Python teszt PASS. Külön Windows/macOS native CI job ellenőrzi a natív guardot, kísérleti csomagot nem publikál; a szokásos platformjobok OFF konfigurációjúak. A normál OFF konfiguráció végső fordítása és teljes privát latency/input/filter/vocoder/rate/state/offline→realtime regressziója is PASS (exit 0).

A bevitt upstream proof/comparison segédek jelenléte nem jelenti minden upstream teszt sikeres lefuttatását. ON buildben a régi H8/hybrid preference nem tesz elérhetővé H8 motort; ez explicit validációs fordítás, nem alapértelmezett termékváltás. CPU-előny, teljes bank/arp, mintahang-hűség, DAW/fizikai bemenet és Windows/Intel privát runtime bizonyíték nincs. Eredet/korlát: tools/ksParity/INTEGRATION.md. Blue azonosítók és verzió változatlan; kiadás nincs.

## 2026-11-05 — #55 review: natív KS nyilvános fejlécútvonal

A review ABI/ODR hibát talált: a szemantikus KS forrásokat fordító ksLib még a hybrid könyvtár include gyökerét exportálta. Emiatt a külső használó, például a ksBoardParity_probe, eltérő Board osztálydefiníciót láthatott. ON konfigurációban az exportált gyökér most source/novation/ksNative; OFF esetben továbbra is source/novation/ks. A natív produkciós guard külön, a ksNative könyvtáron kívül fordított fogyasztói regressziót kapott, amely a publikus ksLib/ksBoard.h felől követeli a natív Board API-t. A korábbi, helyi idézőjeles fejléceket használó guard ezt az útvonalhibát nem tudta kimutatni. A korábbi zöld CI nem igazolta a fogyasztói ABI helyességét; a javított fejre friss platformellenőrzés és külön firmware-es elfogadás szükséges. GUI, alapértelmezett motor és kiadás változatlan.

Célzott helyi MSVC ellenőrzés: a ksSemanticHeaders_test.cpp objektum a tényleges ON konfigurációban lefordult (PASS). Ugyanezt a fordítóparancsot a korábbi hybrid include gyökérrel futtatva a natív API hiánya miatt megbukott, ahogy a negatív kontroll elvárja. A generált fordítóparancsok szerint a Processor és a ksBoardParity_probe is a natív include gyökeret örökli. A három forráscsomag-választási teszt és a whitespace-ellenőrzés PASS. Ez nem teljes új helyi pluginbuild vagy firmware-es újrafuttatás; a javított PR-fej platformeredménye külön szükséges.

## 2026-11-05 — Forrásaudit: KS tesztforrások és helyi fejlécek

Vizsgálati alap: main 8bb92b1d83ebfaecf705feeceb7fce7df0817451 (#55 beolvasztása után). A nyilvános natív include gyökér javítása mellett a CMake több KS tesztet továbbra is a hybrid könyvtárból fordított. Az idézőjeles ksBoard.h/ksDevice.h include előbb a teszt saját könyvtárát választja, ezért a teszt és a linkelt natív könyvtár eltérő osztálydefiníciót használhatott (például ksDrumEdit_test és ksDeviceInput_test). Ez fejlesztői tesztbuild-/ABI-hiba; a normál OFF termékben nem igazolt új hanghiba.

A tesztforrások most a kiválasztott implementáció könyvtárából származnak, beleértve a H8 összehasonlító adaptert és a Board próbákat. ON konfigurációban külön CMake-kapu tiltja a hybrid könyvtárból származó tesztforrásokat; OFF továbbra is az eredeti könyvtárat választja. A Windows/macOS natív job a Device input és Drum edit tesztbinárist is lefordítja a meglévő guard mellett. A fordítás nem firmware-es futtatás.

Helyi bizonyíték: tényleges ON generált fordítóparancsok szerint Device/Board/H8-reference tesztforrások a natív könyvtárból, OFF esetben a hybrid könyvtárból származnak (PASS). A natív Device input és Drum edit fogyasztói objektum MSVC-vel lefordult (PASS). Három forráscsomag-választási teszt és whitespace-ellenőrzés PASS. Teljes linkelt platformbuild/firmware-es újrafuttatás helyben nem történt; a végleges PR CI-je külön szükséges. GUI, pluginazonosítók, verzió, alapértelmezett motor és kiadás változatlan.

A CMake-védelmi blokk külön, minimális generálási próbában a hybrid helyi fejlécet használó fogyasztót elutasítja, a natív forrást elfogadja (negatív és pozitív kontroll PASS).

## 2026-11-05 — Javított KS natív konfiguráció firmware-es elfogadási lépcsője

Vizsgálati alap a #55–56 javításokat tartalmazó main 6ed41b5, friss arm64 Release ON fordításból. A teljes privát Processor latency/reentry/buffer/rate/state/offline→realtime regresszió exit 0 PASS, mindkét modell mono/sztereó bemeneti, filter/envelope, vocoder és 44,1/48/96 kHz kontrolljaival. A ROM nélküli szemantikus produkciós guard PASS. Ez a javított include/source páros külön újrafuttatása, felülírja a korábbi, ABI-javítás előtti ON eredmény koherencia-következtetését.

Az ON ksBoardParity_probe cél most explicit KS_SEMANTIC_PROBE definíciót kap, így tényleges native owner és nulla értelmezett H8 lépés a futás feltétele. A generált függőségi fájl natív ksBoard.h útvonalat igazol. A single és fourpart próba exit 0 PASS, rendre peak 1568304 és 883066. A teljes PCM/state/Flash a korábbi Blue hybrid referenciaállományokkal byte-azonos, mindkét esetben 3932160 PCM és 263168 Flash bájt, 270 illetve 1038 state bájt. MIDI kimenet mindkét oldalon üres. A privát zero-wave fixture a mintahang-hűség vizsgálatát nem fedi le. Teljes bank/arp, CPU, fizikai DAW és Windows/Intel firmware-es futtatás továbbra is nyitott. A privát fixture-ek és nyers eredmények helyben maradnak.

Az átvételi leltár követi a #55–56 beolvasztását és ezt a javított futást. Az új probe guard végső PR-commitjához platform-CI szükséges.


## 2026-11-05 — KS hidegindítás tárolt Performance-ból

Az 57-es PR beolvadt (4e875f3). A privát Board probe új cold-channels próbája négy friss példányt indít a Flash-ben előre beállított Performance-ból. A négy példány együtt lefedi mind a 16 MIDI-csatornát. Mindegyik indulás ellenőrzi az aktív részek maszkját és a tárolt csatornakiosztást, majd az első host MIDI előtt mind a négy DAC-kimeneten pontos nullát követel. A részeket egyenként ellenőrzi hallható note-on és kitartott note-off kontrollal, majd pedálfelengedés utáni pontos nullával.

Helyi macOS arm64 Release, privát KS firmware és zero-wave fixture. A Blue hybrid és a szemantikus natív build is teljesítette mind a 16 próbát. A natív próba a tényleges natív tulajdonost és a nulla értelmezett H8 lépést is ellenőrizte. A próba Board szinten indul tárolt Performance-ból. A Processor vagy a DAW indulását és az upstream külön CC-tisztításának hatását nem méri közvetlenül.

## 2026-11-05 — A KS Board próba platformfordítása

Az 58-as PR mind az öt ellenőrzés után beolvadt (1db005c). A Windows és a macOS CI most a hibrid és a szemantikus natív konfigurációban is lefordítja a privát KS Board próbát. A helyi arm64 Release fordítás mindkét konfigurációban sikeres. A firmware-es futtatás továbbra is a helyi, privát teszt része.

## 2026-11-05 — KS CPU-mérés macOS-en

Az 59-es PR beolvadt (3be2723). A privát Processor mérő macOS-en is kiolvassa a teljes folyamat és a hívó szál CPU-idejét, a riportban pedig külön rögzíti a vezérlőmotor nevét. A Windows mérés a meglévő rendszerhívásokat használja.

Helyi macOS arm64 Release fordítás mindkét konfigurációban sikeres. Azonos 48 kHz-es, 128 mintás offline terhelést futtattam motoronként háromszor, váltott sorrendben. A 2 másodperces bemelegítés után minden futás 576000 mintakeretet dolgozott fel. Mindegyik adott hangot, a hiányzó keretek száma nulla volt. A teljes folyamat CPU-idejének mediánja 9,719 másodperc a Blue hibrid és 15,655 másodperc a szemantikus natív motorral, azaz ezen a terhelésen körülbelül 61%-kal több. A falióra szerinti medián 4,999 és 7,984 másodperc.

A hanghash ismétlések között is eltért. A mérés azonos host terhelést ad a két motornak, de nem igazolja a teljes belső munkamennyiség vagy a kimenet azonosságát. Az eredmény erre az offline futásra, a használt firmware-re és zero-wave fixture-re korlátozódik. A nyers riportok helyben maradnak.


## 2026-11-06 — KS CPU-mérés ütemezett host terheléssel

A 60-as PR beolvadt (18d8184). A Windows és macOS CI mindkét KS konfigurációban fordítja a privát Processor CPU-mérőt. A firmware-es mérés helyben futott az előző körben lefordított macOS arm64 Release binárisokkal.

Motoronként három, váltott sorrendű futás készült 48 kHz-en, 512 mintás blokkokkal. Mind a hat futás 576000 mintakeretet adott hiány nélkül. A teljes folyamat CPU-idejének mediánja 4,914 másodperc volt a hibrid és 7,843 másodperc a szemantikus natív motorral. Ezen a 12 másodperces szintetikus terhelésen a natív motor körülbelül 60%-kal több CPU-időt használt. A natív hanghash mindhárom ismétlésben egyezett, a hibrid hanghash változott. A két motor csúcsszintje eltért. A mérés a zero-wave fixture és az adott szintetikus host eredménye. A natív alapértelmezés elfogadásához ebből nem következik teljesítményelőny.

A 61-es PR natív Windows buildjén az MSVC duplikált szimbólumot jelzett. A CPU-mérő örökölte a tesztek H8 referenciaobjektumát és kompatibilitási könyvtárát, amely ugyanazt a vezérlő-elérhetőségi függvényt definiálta, mint a szemantikus fogyasztó. A natív mérő most a termék könyvtáraival linkel. A generált macOS linkerparancs igazolja, hogy a referenciaobjektum és a kompatibilitási könyvtár kikerült a natív mérőből. A korábbi mérést a javított binárissal külön ellenőrizni kell.

A linkjavítás utáni újramérésen két hibrid futás szélsőséges, véges csúcsértéket adott, miközben a régi teszt sikeresnek jelölte őket. A mérő ugyanazt a puffert adta vissza bemenetként a következő blokknak, törlés nélkül. Most minden blokk néma bemenettel indul, és a mérő a névleges teljes skála tizenhatszorosát meghaladó csúcsot hibának tekinti. Az eddigi CPU-összevetések előzetesek. A korrigált bemenettel meg kell ismételni a hibrid és natív mérést.

A 61-es PR mind az öt ellenőrzés után beolvadt (9205c0e). A javított mérő 12 helyi arm64 futása sikeres, szélsőséges hangcsúcs és hiányzó mintakeret nélkül. A medián CPU-idő offline 9,796 és 15,537 másodperc, ütemezve 4,901 és 7,804 másodperc a hibrid, illetve a szemantikus natív motorral. Mindkét esetben körülbelül 59%-kal több CPU-időt mértünk a natív motorral. A hanghash motoronként és ütemezési módonként mindhárom ismétlésben egyezik, a két motor között eltér. A következő vizsgálat célja az eltérés első reprodukálható pontjának elkülönítése.

## 2026-11-06 — A KS hangeltérés helye a Processor próbában

A privát mérőben a SILVER_CPU_TRACE_BLOCKS=1 az összesített hanghash értékét minden blokk után eltárolja, majd a futás végén a JSON-riportba írja. A követés diagnosztikára szolgál. A CPU-összehasonlításhoz a kapcsoló kikapcsolva marad.

Mindkét motorral két offline és két ütemezett macOS arm64 futás készült. Mind a nyolc próba sikeres volt, hiányzó mintakeret nélkül. Motoron belül az összes blokkhash egyezett az ismétlések között. A motorok között offline először az 52992–53119. mintakeretet tartalmazó blokk tért el. Ütemezve az első eltérő blokk az 55808–56319. mintakeretet tartalmazta. Az indexek nullától indulnak. Mindkét eltérés az első note-off (52800) után jelent meg. Az eltérő blokkméret és pufferprofil miatt a két eltolás külön értelmezendő. A blokkhash a különbség helyét szűkíti, az amplitúdót, a hallhatóságot és a kiváltó okot további méréssel lehet megállapítani.

## 2026-11-06 — Mintaszintű KS hangkülönbség az első hangváltásnál

A 62-es PR beolvadt (e2924e9). A SILVER_CPU_TRACE_RELEASE=1 diagnosztikai opció a privát JSON-riportban megőrzi mind a négy kimenet mintáit az 52000–57999. keret között. A mintákat renderelés közben memóriába gyűjti, és a futás végén írja ki.

Az arm64 Release hibrid és szemantikus natív build offline és ütemezett próbája hiányzó mintakeret nélkül zárult. Offline az első eltérés az 53113. keretben volt. Az első note-off (52800) és az új hangok (53760) közötti szakaszban a maximális abszolút mintakülönbség 0,01127 volt, az új hangok után a vizsgált ablakban 0,08134. Ütemezve az események közötti szakasz egyezett, az első eltérés az 56144. keretben jelent meg, és a vizsgált ablakban a maximális különbség 0,04335 volt. Az indexek nullától indulnak. A két teszt eltérő pufferprofilja miatt az időpontok nem azonos késleltetést képviselnek. A mérés a hangeltérést számszerűsíti, a kiváltó okot külön kell ellenőrizni. A nyers riportok helyben maradnak.

## 2026-11-06 — Egyetlen KS hang elengedése

A 63-as PR mind az öt ellenőrzés után beolvadt (8bbc91b). A SILVER_CPU_RELEASE_ONLY=1 ugyanazt az első hangot és note-off eseményt játssza le, a későbbi hangokat kihagyja. A riport a lejátszott szakasz nevét is megadja.

A négy helyi macOS arm64 Release futás sikeres volt, hiányzó mintakeret nélkül. Offline az első eltérő mintakeret ismét 53113, a rögzített ablak maximális eltérése 0,01127. Ütemezve a teljes 576000 keretes hanghash és a mintaszintű ablak egyezett. A későbbi hangok tehát az offline eltéréshez nem szükségesek. Az ütemezett egyhangos egyezés erre a terhelésre és pufferprofilra vonatkozik. A kiváltó ok még nyitott.

## 2026-11-06 — KS egyhangos próba azonos pufferbeállításokkal

A 64-es PR beolvadt (ee59f41). A privát CPU-mérőben a SILVER_CPU_BLOCK_FRAMES (128 vagy 512) és SILVER_CPU_BUFFER_PROFILE (0 vagy 3) egymástól és a host ütemezésétől függetlenül állítható. A meglévő alapértékek továbbra is 128/0 offline és 512/3 ütemezett módban.

A helyi macOS arm64 Release mátrix mind a 16 futása sikeres volt, hiányzó mintakeret nélkül. Az egyhangos szakasz teljes hanghash-e 512 mintás blokkokkal mindkét profilban egyezett a két motor között, offline és ütemezve is. 128 mintás blokkokkal mindkét ütemezésnél eltért. A 0-s profil első eltérő blokkja 52992-nél kezdődött, a 3-as profilé már 7040-nél, az első note-off előtt. A jelentett késleltetés minden azonos beállítású motorpárban egyezett. Ennél a terhelésnél az ütemezés önmagában nem választotta szét az egyező és eltérő eseteket. Az eltérés oka további vizsgálatot igényel.

## 2026-11-06 — KS egyhangos próba blokkhatáron

A 65-ös PR mind az öt ellenőrzés után beolvadt (b108de0). A SILVER_CPU_ALIGN_RELEASE=1 az egyhangos próbában 4608-ra és 52736-ra állítja a note-on és note-off keretét. Mindkét esemény a 128 és az 512 mintás blokk elejére esik. Ez az eredeti próbához képest a hang kezdetét és hosszát is módosítja.

Mind a nyolc macOS arm64 Release offline próba hiányzó mintakeret nélkül futott. A két motor első eltérő blokkja 128/0 esetén 4992, 128/3 esetén 54784, 512/0 esetén 5120 volt. 512/3 mellett egyezett a teljes hanghash. A különbség tehát blokkhatárra eső MIDI-vel is reprodukálható, két beállításnál már az első note-off előtt. A beállításfüggő eltérés okát ez a próba nem állapítja meg.

## 2026-11-06 — KS hangelengedés közvetlen Board próbában

A 66-os PR mind az öt ellenőrzés után beolvadt (9495482). A privát Board próba note-release módban 8192 keret várakozás után 48000 Board-keretig tart egy hangot, majd további 8192 keretet rögzít a note-off után. A kezdő programot a Flash-ből tölti be.

Helyi macOS arm64 Release módban mindkét motor sikeresen lefutott. A 64384 keret összes, 24 csatornás PCM mintája (6180864 bájt), a 270 bájtos állapot és a 263168 bájtos Flash egyezett. A MIDI-kimenet üres volt, a csúcs mindkét esetben 836483. A natív próba ellenőrizte a tényleges natív tulajdonost és a nulla értelmezett H8 lépést. Ez a közvetlen Board próba eltérő programbeállítással, saját mintavételi órán futott. A Processorban mért eltérés okát ebből még nem lehet megállapítani.

## 2026-11-06 — KS hangelengedés közvetlen Device próbában

A 67-es PR mind az öt ellenőrzés után beolvadt (17a54d1). A privát Device próba 48 kHz-en, azonos MIDI-időzítéssel 65536 keretet rögzít 128 vagy 512 mintás blokkokban. A Flash kezdőprogramját és a Device saját pufferelését használja. Mindkét motor mindkét blokkmérettel hallható, véges kimenetet adott, hiányzó keret nélkül.

128 mintás blokkoknál az első eltérés az 53105. keretben jelent meg, a teljes felvétel legnagyobb abszolút eltérése 0,000535. 512 mintás blokkoknál mind a négy kimenet teljes felvétele bájtról bájtra egyezett. A blokkméretfüggő eltérés tehát ezen a próbán már a Device szintjén előállt, Processor nélkül. A Processor-próba más kezdőállapota és nagyobb eltérése miatt a két mérés nem igazol azonos kiváltó okot.

## 2026-11-06 — KS Device összehasonlítás azonos előrefutással

A 68-as PR beolvadt (9d71059). A privát Device próba új módja 128 mintás blokkokat használ, de az 512 mintás blokkokhoz tartozó 640 keretes előrefutással. Mindkét macOS arm64 Release futás hallható, véges kimenetet adott hiányzó keret nélkül. A két motor mind a négy csatornája bájtról bájtra egyezett. A 128 mintás alapbeállítás korábbi próbáján eltérés volt. Az előrefutás a MIDI-események esedékességét is módosítja, ezért az egyezésből nem következik, hogy önmagában a puffer mérete okozza az eltérést.

## 2026-11-06 — KS Device kontroll azonos MIDI-esedékességgel

A 69-es PR mind az öt ellenőrzés után beolvadt (aa48273). A 128 mintás próba új módja megtartja a 256 keretes alapértelmezett Device-előrefutást, és 384 keretet ad mindkét MIDI-esemény időzítéséhez. Így ugyanazokra a keretekre esnek az események, mint a 640 keretes előrefutású próbában.

A két helyi macOS arm64 Release futás véges, hallható kimenetet adott hiányzó keret nélkül. A négycsatornás PCM a két motor között egyezett. Mindkét motor eredménye a saját korábbi 640 keretes próbájával is bájtról bájtra egyezett. A megfigyelt eltérés eltüntetéséhez ezen a teszten elég volt a MIDI-időzítést módosítani. A vezérlőn belüli eltérés pontos oka továbbra is nyitott.

## 2026-11-06 — KS Device kontroll külön note-on és note-off eltolással

A 70-es PR mind az öt ellenőrzés után beolvadt (b70cf87). A 128 mintás Device próbában külön is eltolható a note-on vagy a note-off 384 kerettel, az alapértelmezett audio-előrefutás mellett. Mind a négy macOS arm64 Release futás véges, hallható kimenetet adott hiányzó keret nélkül.

Csak a note-on eltolásával a négycsatornás felvétel egyezett a két motor között. Csak a note-off eltolásával az eltérés megmaradt. Az első különböző bájt a nullától számozott 53495. keretbe esett. A kontrollok a hang hosszát is módosítják. Az eredmény a kezdő esemény és a hang előtörténetének szerepét jelzi ebben a próbában, a belső okot további vizsgálatnak kell azonosítania.

## 2026-11-06 — KS Device összehasonlítás a DSP mintavételi frekvenciáján

A 71-es PR beolvadt (c28068a). A privát Device próba új módja 128 mintás blokkok mellett a DSP pontos mintavételi frekvenciáját használja (kb. 45340,4 Hz). Azonos keretindexen küldi a MIDI-eseményeket, mint a 48 kHz-es eset, ezért az eltelt idő különbözik.

Mindkét macOS arm64 Release futás véges, hallható kimenetet adott hiányzó keret nélkül. Az első eltérés az 53108. keretben jelent meg, a legnagyobb abszolút mintakülönbség 0,000595. Az eltérés tehát egyező host és DSP mintavételi frekvenciával is előállt. A Device interpolációja és az eseményütemezés továbbra is része a próbának.

## 2026-11-06 — KS Device MIDI-időzítés azonos hanghosszal

A 72-es PR mind az öt ellenőrzés után beolvadt (5d92b45). A natív mintavételi frekvenciájú Device próba külön note-on és note-off eltolást fogad. A helyi macOS arm64 Release sorozatban mindkét eseményt azonos mértékben toltam el, így a hang hossza állandó maradt. Mind a 24 futás véges, hallható kimenetet adott hiányzó keret nélkül.

A két motor teljes felvétele 1, 16 és 64 keretes eltolással egyezett. A vizsgált 0, 2, 3, 4, 8, 32, 128, 256 és 384 keretes eltolásoknál eltért. Egyes esetekben az első eltérés már a note-off előtt megjelent. Az egyezés így a hanghossz módosítása nélkül is változik a MIDI időpontjával. Ez a vizsgált program és induló állapot eredménye, a vezérlőn belüli okot nem azonosítja.

## 2026-11-07 — az első közös kiadás publikálásának előkészítése

A felhasználó sikeres macOS- és Windows-működési próbáról számolt be, majd kérte a publikálási fejlesztési munkák megkezdését. A visszajelzés nem sorolta fel a hostokat és az egyedi teszteseteket.

A közös publikálófolyamat külön, áttekinthető UTF-8 Markdown kiadási jegyzetet fogad. A workflow alapértelmezett jegyzete a blue/Release-notes.md. A program a tényleges csomagok forráscommitját és platformfutásait automatikusan hozzáfűzi. Üres jegyzetet az első GitHub-művelet előtt elutasít. A közvetlen parancssori használat jegyzet nélkül továbbra is a korábbi generált leírást használja.

Helyi ellenőrzés a publikáló segéd meglévő tesztjeivel, Python szintaktikai ellenőrzéssel és üres jegyzet negatív próbával. A publikáláshoz két sikeres, azonos commitból származó főági platformfutást kell kiválasztani. A javasolt első címke blue-v1.0.0. A publikáló workflow indítása külön kiadási művelet.

## 2026-11-07 — repository-audit javításai

A Blue alapértelmezett beállításfájlja a The Usual Suspects/Silver-K Blue/settings.json. Első indításkor a korábbi Silver-K/settings.json érvényes objektumát egyszer, folyamatközi zárral és atomikus fájlcserével másolja át. Meglévő Blue beállítást nem ír felül. A SILVER_K_DATA felülbírálás továbbra is a megadott mappa settings.json fájlját használja. A korábbi adatmappa és felhasználói fájlok elérési útjai megmaradnak.

A kiadásfeltöltő az azonos forráscommitú, című és leírású tervezet megszakadt feltöltését folytatja. A már kész állományokat SHA-256 és méret alapján ellenőrzi, csak a hiányzókat tölti fel. A GitHub által hátrahagyott starter állományokat eltávolítja, majd újratölti. Eltérő vagy publikált kiadást elutasít. A teljes feltöltött fájllistát publikálás előtt ismét ellenőrzi.

Az upstream-frissítő a javaslati ág összes, főágon és upstreamen kívüli commitjának szerzőjét és committerét vizsgálja. Kézi commitot egy későbbi botcommit sem fedhet el.

Célzott regressziók készültek a beállításmigrációra, a megszakadt kiadásfeltöltésre és a botcommit alatti kézi változásra. A 18 Python-teszt sikeres.

A helyi macOS arm64 Release silverPersistence_test fordítása és a silver_persistence_and_rescan CTest sikeres. A migrációs próba ellenőrzi a skin és az adatmappa megőrzését, a régi fájl érintetlenségét, a meglévő Blue beállítás megőrzését és a hibás forrásfájl elutasítását. A publikáló regresszió részleges feltöltést és starter állományt is szimulál. A Windows/macOS/web PR-ellenőrzések külön kapuk.

A #75 felülvizsgálati kiegészítése közvetlenül a publikálás előtt újraellenőrzi a tervezet draft állapotát, címkéjét, célcommitját, címét és leírását. Feltöltés közbeni eltérésnél megáll. Az öt módosítási esetet regressziós teszt fedi le. A teljes Python-sorozat 19 sikeres teszt. A GitHub ellenőrzés és publikálás közötti műveletsor továbbra sem atomikus.

A #75 automatizált review további hibát jelzett a sikertelen migráció útvonalán. A konstruktor most migrációs hiba esetén is végig inicializálja az adatmappát, az eszközválasztásokat és a preferenciák alapállapotát. Ilyenkor az adott példány beállításírása tiltott, a destruktor sem ment, így a későbbi újranyitás megismételheti a migrációt. A migráció eredeti hibajelzése megmarad. A helyi Release fordítás és a beállításkezelési CTest ismét sikeres.
