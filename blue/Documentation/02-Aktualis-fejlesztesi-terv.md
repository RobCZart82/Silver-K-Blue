# Aktuális fejlesztési terv és megvalósítási állapot

**2026-11-02 — aktuális követelmény:** a jelenlegi Standard, Retina és Legacy GUI/skinek megőrzendők. Nincs grafikai átrendezés, új panel vagy kezelőszerv, elemek átméretezése/áthelyezése vagy megjelenésváltás. A követelmény az upstream átvételekre is érvényes.

## 1.0.1 hibajavító kiadás — elfogadott terv, 2026-11-08

2026-11-14-i egyeztetési alap: main `3a2dc034` (#84 beolvadt), nyitott fejlesztési PR nincs. A Windows/macOS normál és natív, valamint web ellenőrzések PASS ugyanazon a main commiton; az EXE és PKG összeállítása PASS. A [verify-only futás](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/43574545945) a Windows (`43411611683`) és macOS (`43411611544`) artifactokat is sikeresen ellenőrizte azonos `3a2dc034` forrásról: nyolc kiadási fájl, metaadat és ellenőrzőösszeg egyezik. A jelentés `published: false`; a release, címke és kiadási assetek változatlanok. A Standalone a felhasználó választotta `{app}` mappát követi; a frissítés nem törli rekurzívan az alkalmazásmappát, és csak kiválasztott VST3-nál takarítja a VST3-csomagot. A meglévő telepítő/metaadat felülírását elutasítja. A kézi tiszta telepítés/frissítés/eltávolítás és DAW-elfogadás továbbra is nyitott.

Az eredeti 1.0.1 terv main-alapja `1ecd7cb` (#80) volt; azon a #75/#78/#79 audit- és buildjavításai, valamint a Windows telepítő első CI-lépése szerepelt. A #79 mind az öt Windows/macOS/web kötelező ellenőrzése PASS. A Windows Prefetch/Offline hangpróba még privát firmware-állapottal végzendő. Az [1.0.0 kiadás](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/blue-v1.0.0) 2026-11-07-én megjelent, forrása `5adc19e`. A #75 és #79 javításai az 1.0.1-be kerülnek. Az alábbi tételek a még nyitott vagy kézi elfogadást igénylő munkát mutatják.

| Azonosító | Prioritás | Javítás | Elfogadási feltétel |
|---|---|---|---|
| FIX-101-01 | P2 | Mainben, #79 (`dd99c3a`): a VST3 Prefetch teljes sebességű, nem realtime renderútja Windowsra is kiterjed; a JUCE forrás buildhelyi másolatát és a két modellt vizsgáló hostpróbát Windows CI is fordítja. | ROM nélküli átalakítási teszt és Windows/macOS build/CI PASS. Még kell külön Windows Prefetch/Offline PCM-próba privát K és KS állapottal, véges hallható kimenettel és nulla hiányzó mintával; paced Realtime kontroll mindkét modellen. A CI nem futtatta ezeket a privát állapotokat. |
| FIX-101-02 | P2 | Mainben, #77: a publikáló ellenőrzi a meglévő Git-címke tényleges commitját; a release target_commitish önmagában nem elegendő. | Könnyű és annotált címke feloldása, eltérő commit elutasítása létrehozás/feltöltés előtt, újbóli ellenőrzés publikálás előtt; regressziós tesztek. |
| FIX-101-03 | P2 | Mainben, #78 (`f5bb587`): a doc/BUILD.md Windows/macOS buildjei a dokumentált tesztprogramokat is lefordítják. | Helyi és kötelező CI-ellenőrzések sikeresek; firmware nélkül a SKIP külön jelölt. |
| FIX-101-04 | P2 | Mainben, #78: README.md és README.hu.md a valódi 1.0.0 letöltésekre mutat, az 1.0.1-et fejlesztés alatt jelöli. | 1.0.1 publikálásakor mindkét letöltési hivatkozás frissítendő. |
| FIX-101-05 | P3 | Mainben, #78: a CHANGELOG külön választja a kiadott 1.0.0 tartalmat és az 1.0.1 terveit. | Publikálás előtt az 1.0.1 tényleges változáslistájának és címkéjének végső egyeztetése. |
| FIX-101-06 | P3 | Mainben, #78: a doc/PUBLICATION.md az aktuális ellenőrző és helyreállítási folyamatot írja le. | Publikálás előtt a sikeres futások, csomagok és tesztkorlátok végső bizonyítékainak megadása. |
| INS-101-WIN | Kiadási követelmény | Mainben, #80 (`1ecd7cb`): Windows x64 EXE Inno Setup 6-tal; VST3 és Standalone külön választható, 64 bites, rendszerszintű célmappákba kerül. A build metaadata 1.0.1-es verziót és forráscommitot rögzít. | Windows CI összeállítás PASS; valós gépen komponensenkénti tiszta telepítés, frissítés, eltávolítás; VST3 hostfelderítés és Standalone indítás; user beállítások/presetek/firmware-adatok megőrzése. Authenticode aláírás jelenleg nincs igazolva. |
| INS-101-MAC | Kiadási követelmény | Mainben, #81 (`9aeaf1d`): Apple `pkgbuild`/`productbuild` komponenscsomagok; egy Universal 2 PKG-ben külön AU, VST3 és Standalone választással. | A main macOS CI PKG-összeállítása PASS. Kézi komponensválasztás, helyes célmappák, tiszta telepítés/frissítés és dokumentált eltávolítás; AU/VST3 hostfelderítés; Apple Silicon és x86_64 ellenőrzés. Aláírás/notarizálás állapota kiadáskor pontosan közlendő. |
| INS-101-CI | Kiadási követelmény | Mainben, #80–84: a Windows EXE és macOS PKG a platformbuild ZIP mellett CI-artifactként készül; a publikáló ellenőrzi a telepítőket, sidecarokat és leltárt. | A [verify-only workflow](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/43574545945) PASS: az azonos `3a2dc034` main commit Windows (`43411611683`) és macOS (`43411611544`) artifactjaiból mind a nyolc fájl, verzió, platform, forráscommit, fájlnév és SHA-256 ellenőrizve. `published: false`. A tényleges telepítés, frissítés/eltávolítás és DAW-felismerés kézi elfogadása nyitott. |

A telepítők minden kiválasztott termékébe beépítve kerül a Standard és Retina skin, a Legacy elérhetőségével. Külön Blue termékazonosítók, meglévő GUI és projektkompatibilitás megőrzendő. Saját firmware és gyári hangadat nem kerül a telepítőkbe. A Windows telepítő nem törölheti a felhasználói app-adat mappát. A macOS PKG jelenleg aláíratlan; a benne levő appok helyi teszteléshez ad hoc aláírásúak, a csomag nem notarizált. Az aláírási tanúsítványok és a macOS notarizálás lehetősége felmérendő; a tényleges aláírási állapot szerepeljen a kiadási leírásban. A Windows technológia Inno Setup 6; a macOS választás a natív Installer `pkgbuild`/`productbuild` eszközök.

Egyszerre egy fejlesztési PR. A beolvasztást a legfrissebb commit sikeres Windows/macOS/web ellenőrzései és a feloldott review-szálak kapuzzák. Az 1.0.1 elfogadásához a telepítőkből végzett kézi próba és a privát firmware-t igénylő Prefetch/Offline PCM-próba is szükséges. A jelenlegi publikus kiadás 1.0.0; a 1.0.1 publikálása nem történt meg.

## 2026-11-26 — Elfogadott upstream 0.8.5b átvételi ütemterv

Forrás: [Silver-K 0.8b](https://github.com/temaniak/Silver-K/releases/tag/v0.8b) (november 24.), [0.8.5b](https://github.com/temaniak/Silver-K/releases/tag/v0.8.5b) (november 26.), upstream main `1b28443`, karbantartási commit [affc0f0](https://github.com/temaniak/Silver-K/commit/affc0f0). Az előző upstream vizsgálathoz képest 530 fájl változott. A verziók tervezett célverziók, az átvétel és az elfogadás külön igazolandó.

| Célverzió / azonosító | Prioritás | Feladat | Elfogadás |
|---|---|---|---|
| 1.0.1 | Kiadási kapu | Meglévő hibajavítások és EXE/PKG lezárása | A 83e547d main Windows/macOS/web CI és [csomagellenőrzése](https://github.com/RobCZart82/Silver-K-Blue/actions/runs/45217794115) PASS. A kézi telepítő- és pluginpróba pozitív; a fennmaradó részletes elfogadás és publikálás még nyitott. |
| 1.0.2 — UP085-01 | P1 | Arpeggiátor Host mód: élő MIDI DAW Stop mellett, BPM fallback, Play rácshoz igazítás, Key Sync első lépés | K és KS, Stop/Play, rácspont és Key Sync, eltérő blokkméretek; meglévő motorokkal reprodukált regresszió |
| 1.0.2 — UP085-02 | P1 | MIDI-csatornák és preset paraméter-visszajelzés | Bejövő csatorna megőrzése, KS Performance aktív part, eltérő csatornás preset, host automation, Standalone; meglévő Setup és projektkompatibilitás megtartása |
| 1.0.2 — UP085-03 | P1 | Preset Browser Rescan, zárolt cache, kézzel csatolt könyvtárak | Hozzáadott/törölt forrás és bank, manuális könyvtár, zárolt cache; saját tagok és kedvencek megőrzése |
| 1.0.2 — UP085-04 | P2 | Transport, MIDI loop, seek és clock source váltás | K/KS MIDI és audio teszt ciklus-, klip- és transporthatárokon, kitartott hangokkal; belső és host clock |
| 1.0.2 — UP085-05 | P2 | Knob/slider bemeneti kezelés, macOS képernyőszél | A vezérlők logikai viselkedésének javítása a Standard/Retina/Legacy geometriájának és grafikájának megtartásával |
| 1.1.0 — UP085-06 | Magas | Új natív K/KS command vezérlők és DSP integráció elővizsgálata | Elkülönített backend, referenciahang/állapot/MIDI/arp/CPU összevetés, meglévő motorok és projektek kompatibilitása; alapértelmezett váltás külön elfogadással |
| 1.1.0 — UP085-07 | Közepes | Fejlettebb presetkezelés, mentés és KS Performance Part | Mentés/visszatöltés sound/part/performance szinten, bank és firmware modellváltás; meglévő Blue felületen vizsgálandó |
| 1.1.0 — UP085-08 | Közepes | Wave ROM és Drum Map automatikus előkészítés | Saját fájlok, hiányzó/hibás adatok, indulás és felhasználói adatmegőrzés; gyári tartalom nem csomagolható |
| 1.1.0 — UP085-09 | Vizsgálat | Windows Software/OpenGL renderer beállítás megőrzése | Előbb meglévő Blue renderút és kompatibilitás felmérése; editor újranyitás, plugin reload |
| Későbbi — LINUX-01 | Halasztott | Linux x64 VST3/Standalone elővizsgálata | A lentebb rögzített platformterv szerint; LV2 külön döntés |
| Későbbi — UP085-10 | Halasztott | KS Drum Sequencer és Drum Edit bővítés | Új funkció és felületi illesztés külön egyeztetése; KS+ preset és időzítési kompatibilitás |

**Megvalósítási sorrend:** 1.0.1 lezárása; UP085-01–05 kódszintű eltéréseinek feltárása, reprodukció és célzott javítása egyenként; UP085-06–09 elővizsgálata és integrációja; Linux és Drum Sequencer később. A 0.8b új architektúrájához kötött javításokat a Blue meglévő vezérlőihez kell adaptálni. A teljes upstream merge és SoulTrain grafikai átvétele nem része ennek a tervnek. Standard/Retina/Legacy elrendezés, Blue azonosítók, saját adatútvonalak és a telepítők megőrzendők. Egyidejűleg egy fejlesztési PR; minden lépcsőn helyi célzott ellenőrzés, Windows/macOS/web kötelező CI, napló és forráscommit, majd zöld állapotban main integráció.

## Halasztott terv — Linux támogatás

**LINUX-01, elővizsgálat, célverzió nélkül.** Az 1.0.1 után felmérendő a Linux x86_64 VST3 és Standalone támogatás. Kezdeti javaslat egy rögzített Ubuntu LTS buildalap; az architektúra, minimális rendszerkövetelmény és csomagformátum az elővizsgálat eredménye alapján véglegesítendő.

Saját fizikai Linux gép nélkül is megkezdhető a munka. GitHub Actions Linux futtatóval fordítás és ROM nélküli teszt végezhető, virtuális gépen a felület és hostbetöltés próbálható. A későbbi támogatott kiadáshoz Linux-felhasználó kézi DAW/audio/MIDI próbája szükséges, mert a CI és a virtuális hangút nem igazolja a fizikai hangkártyás működést.

1. Függőségek, DSP/JIT és fájlútvonalak Linux-kompatibilitásának felmérése a repository rögzített JUCE-verziójával.
2. Izolált Linux build és ROM nélküli CI, a meglévő Windows/macOS kapuk megőrzésével.
3. VST3 és Standalone betöltés, Standard/Retina/Legacy megjelenítés, projektmentés/visszatöltés, saját firmware-es hangpróba virtuális vagy tesztelői környezetben.
4. Valós Linux hostban MIDI, audio, puffer/frekvenciaváltás és külső bemenet; támogatott disztribúció/architektúra rögzítése.
5. Kiadási csomagolás és telepítési útmutató csak a mérések alapján.

Források az elővizsgálathoz: [JUCE Linux-függőségek](https://github.com/juce-framework/JUCE/blob/master/docs/Linux%20Dependencies.md), [GitHub Linux futtatók](https://docs.github.com/en/actions/how-tos/manage-runners/github-hosted-runners/use-github-hosted-runners). Ezek a platform lehetőségeit írják le; a Silver-K Blue Linux-működését a fenti próbák igazolhatják.

## Korábbi GP-01–GP-06 munkafolyamok

Az alábbi összesítés a korábbi terv folytatható témaköreit őrzi. A részletes, dátumozott bizonyítékot a [központi fejlesztési napló](11-Fejlesztesi-naplo-es-publikalasi-terv.md) tartalmazza.

| Prioritás | Tervezett munka | Jelenlegi állapot |
|---|---|---|
| 1 — GP-01 | Meglévő tesztek CI-lefedettsége; ROM nélküli és firmware-es eredmények különválasztása | Részben kész: 14 ROM nélküli CI-teszt mainben; teljes lefedettségi felmérés nyitott |
| 2 — GP-02 | Projektmentés, visszatöltés, adatfájlok és firmware-életciklus | Izolált Windows firmware-es persistence/indulás PASS referencia/natív KS-sel; DAW/macOS/többpéldányos elfogadás nyitott |
| 3 — GP-03 | Kis/változó hangblokkok, frekvenciaváltás és valós idejű működés | Bemeneti adapter és Windows firmware-es latencia/rate/buffer próbák PASS; további lefedettségi felmérés szükséges |
| 4 — GP-04 | Aktuális dokumentáció, eredetadat és kiadási bizonyíték egyeztetése | Main/PR/CI/helyi bizonyíték egyeztetve a központi naplóban; nem teljes kiadási elfogadás |
| 5 — GP-05 | Méréssel indokolt belső teljesítményjavítás | Csak bizonyított szűk keresztmetszetre, helyességi ellenőrzéssel |
| 6 — GP-06 | Opcionális automatikus Patches-figyelés | Későbbi lehetőség; jelenleg kézi Rescan működik |

Undo/Redo, A/B összehasonlítás és új felületi vezérlők nem szerepelnek az aktív tervben. A külső audio/upstream bővítések előbb képességenkénti összevetést igényelnek; GUI-változással járó részeik kizártak. A részletes elfogadási és változatlansági szabályokat a [fordítási/átadási útmutató](06-Forditas-es-atadas.md) rögzíti.

## Történeti alap — v1.0, 2026-10-25

Az alábbi archívumútvonalak, grafikai tervek, készültségi állítások és binárisleírások korabeli pillanatfelvételek, nem aktív grafikai fejlesztési megbízások. A jelenlegi telepítés a [friss útmutatóban](01-Telepites-es-hasznalat.md), a grafikai eredet nyitott pontjai a [PROVENANCE](../Artwork/PROVENANCE.md) fájlban találhatók.

Állapot a történeti feljegyzéskor: Silver-K Blue v1.0, 2026-10-25. Alap: upstream v0.4.4a, commit `4baaead06e23e2358729cca32b6b511c72022318`. A „kész” itt helyben megvalósított és célzottan ellenőrzött fejlesztést jelentett; nem eredeti repositoryba beolvasztott változtatást.

## Elkészült grafikai munka

- Forgatógomb: teljes kupak és egybeépített értékjelző forgása, flat kék betét, világosabb szürke LED-körsáv, a template-hez illeszkedő 21 pont. A világító pontok közepén natív 1 logikai pixel fehér mag, Retina exportban ennek 4×4 pixeles megfelelője. Az új glow-kísérlet nincs a végleges skinben.
- 128 kockás függőleges strip unipoláris, bipoláris, nyers 64-es középpontú és volume használathoz. Unipoláris/volume minimumtól nő, bipoláris középről két irányba. A paraméterértékeket továbbra is a plugin normalizálja; a kép nem alakítja át a hangparaméter tartományát vagy görbéjét.
- Ezüstös, finoman szemcsés alumíniumpanelek; üvegkék faderkupakok; a felhasználó FinaleOnOff anyagából véglegesített nyomógombképek.
- Performance LCD-hátterek és ezüst panelek; Patches lista hajszálvékony kerete; Setup késleltetésfelirat és Load firmware gomb távolságának skinjavítása.
- Standard és Retina végleges név, állandó azonosító és v1.0 kiadási alap.

## Újrafordítást igénylő fejlesztések

| Azonosító | Téma | v1.0 helyi állapot | Következő lépés az upstream felé |
|---|---|---|---|
| SK-001 | Pontmátrixos LCD | Fő LCD 4 sor × 24 oszlop, szó- és hosszúság szerinti törés; élő szöveg; ékezetleképezés; nem támogatott karaktereknél natív visszaesés | Általános, opcionális skin API és karakterkészlet egyeztetése |
| SK-002 | Retina filmstrip | 256×32768 px/128 kocka engedése célzott képbetöltési módosítással, nagyobb faderkép logikai méretet tartó skálával | Betöltés előtti PNG-méretvizsgálat, összesített memóriakeret és hibás fájlok további tesztjei |
| SK-003 | Egységes navy LCD mezők | R2 sötétsége és rácsa megmaradt; firmware mező és popuphátterek egységesítve | R2 opcionális konfiguráció és régi skinek változatlan alapértékei |
| SK-004 | Waveform kijelölés/hover | K-Station hullámforma gombok állandó háttere eltűnik, saját grafikánál egér fölött újra van kiemelés; tiltott gomb nem kap hoverfényt | Kis, célzott, visszafelé kompatibilis javításként előkészíteni |
| SK-005 | Gyorstipp sarkai | Közös kerekített háttér- és keretgeometria, ténylegesen nem opaque TooltipWindow; sarokkilógás megszűnt | Külön kis javításként javasolható, Standard skinre is érvényes a javított pluginnal |
| SK-006 | Performance inaktív képek | Nem használható rotary/fader csoportok is 0,35-ös alphára halványulnak; az aktív KS vezérlők teljes fényerejűek maradnak | Tényleges enabled állapothoz kötött megjelenés és regressziók bemutatása |

**A lenyílók pontmátrixos betűi vissza lettek vonva.** A végleges irány minden lenyíló és megnyitott lista eredeti, jól olvasható betűje, Retina esetén R2 tengerészkék rácsos háttérrel. A Performance presetmezők és Preset feliratok is natív karakteresek; a fő LCD pontmátrixos marad. A világosabb R3 háló és átlátszóságkísérlet történeti anyag, nem v1.0 követelmény.

## Korlátok és nyitott feladatok

1. Az eredeti v0.4.4a képoldalankénti 8192 px korlátja miatt a 128 képkockás függőleges strip legfeljebb 64 px/kockás lehet. A Retina 256 px strip helyi binárisfüggő; tiszta skinmódosítással ez nem oldható meg.
2. Egy kibontott 256×32768 RGBA kép körülbelül 32 MiB, négy strip névleges képtárolása 128 MiB, gyorsítótárak és további képek nélkül. A ZIP-méret nem futásidejű memóriaigény.
3. A fő LCD megjelenítése 4 sorra korlátozott. A több szöveg a kereten túl nem jelenhet meg; a teljes szöveg a natív felirat adataiban megmarad. A jelenlegi 5×7 karakterkészlet nem teljes nemzetközi Unicode font.
4. A hajszálvékony skin-dekorációs listakeret a rögzített layoutot követi. Dinamikus átrendezéshez célszerűbb natív listakeret API-t egyeztetni.
5. macOS Universal 2 célzott felületi tesztek vannak. Más operációs rendszer, DAW, minden skálázási mód és teljes hangmotor-regresszió nincs ezzel a kiadással igazolva. A korábbi KS felületi próbák zero wave tesztállapotot is használtak, nem igazolják a hiányzó mintahullámok hangját.
6. A Windows x64 VST3/Standalone és macOS Universal 2 AU/VST3/Standalone tesztbuild 2026-10-29 óta elérhető (forrás: `40b68eee`). A későbbi keretrendszer-javítások és alkalmazásikon még nem szerepelnek ezekben. Upstream-beolvasztás, Linux build, notarizált/aláírt telepítő és automatikus alkalmazásfrissítés nincs igazolva.

## Javasolt következő kiadás

Először visszajelzés a napi használatból: K és KS firmware, Main/Perf/Patches/Setup, inaktív állapotok, hosszú patchnevek, több GUI-méret. Új vizuális változtatás előtt a v1.0 érintetlenül megmarad. A javításokhoz kis, áttekinthető eltérések és tesztek készüljenek; a grafikai ízlésváltoztatást válasszuk külön a programhibák javításától.
