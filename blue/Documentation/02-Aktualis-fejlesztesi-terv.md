# Aktuális fejlesztési terv és megvalósítási állapot

**2026-11-02 — aktuális követelmény:** a jelenlegi Standard, Retina és Legacy GUI/skinek megőrzendők. Nincs grafikai átrendezés, új panel vagy kezelőszerv, elemek átméretezése/áthelyezése vagy megjelenésváltás. A követelmény az upstream átvételekre is érvényes.

## 1.0.1 hibajavító kiadás — elfogadott terv, 2026-11-08

Tervezési alap: main `86f44ba`, a #75 auditjavításai beolvadtak. Az [1.0.0 kiadás](https://github.com/RobCZart82/Silver-K-Blue/releases/tag/blue-v1.0.0) 2026-11-07-én megjelent, forrása `5adc19e`. A #75 javításai az 1.0.1-be kerülnek. Az alábbi tételek nyitott feladatok; a dokumentálásuk nem megvalósítási vagy teszteredmény.

| Azonosító | Prioritás | Javítás | Elfogadási feltétel |
|---|---|---|---|
| FIX-101-01 | P2 | A VST3 Prefetch ütemezési javítása jelenleg csak APPLE buildben érvényes a cmake/JuceCompatibility.cmake fájlban. Windows útvonal vizsgálata és javítása. | Windows Prefetch/Offline reprodukció mindkét modellen, véges hallható kimenet és hiányzó minták ellenőrzése; realtime regresszió. A Windows hanghiba egyelőre kódból következő kockázat, nem lefuttatott reprodukció. |
| FIX-101-02 | P2 | Fejlesztési PR-ben: a publikáló ellenőrizze a meglévő Git-címke tényleges commitját; a release target_commitish önmagában nem elegendő. | Könnyű és annotált címke feloldása, eltérő commit elutasítása létrehozás/feltöltés előtt, újbóli ellenőrzés publikálás előtt; regressziós tesztek. |
| FIX-101-03 | P2 | A doc/BUILD.md macOS/Windows parancsai építsék meg az összes utána kiválasztott tesztprogramot. | Tiszta buildből a dokumentált parancsokkal nincs hiányzó teszt-executable; firmware nélkül a SKIP külön jelölt. |
| FIX-101-04 | P2 | README.md és README.hu.md letöltési táblázata és „még nincs kiadás” állítása elavult. | Az elérhető 1.0.0 csomagokra mutató valós linkek, majd 1.0.1 publikálása után frissítés; kiadott és fejlesztési változat külön jelölve. |
| FIX-101-05 | P3 | CHANGELOG.md függőben lévőként jelöl már beolvasztott munkát és a #1 PR-t. | A tényleges 1.0.0 tartalom és az 1.0.1 javításai külön verziózott szakaszban; a #75 státusza pontos. |
| FIX-101-06 | P3 | A doc/PUBLICATION.md aktuális ellenőrzőlista helyett korábbi nyilvánossá tételi és #1 PR-feladatokat sorol. | Aktuális publikálási lépések és dátumozott történeti részek; valódi forráscommit, CI és tesztbizonyíték. |
| INS-101-WIN | Kiadási követelmény | Egy Windows x64 EXE telepítő, külön választható VST3 és Standalone összetevőkkel. | Komponensenkénti tiszta telepítés, frissítés és eltávolítás; VST3 hostfelderítés és Standalone indítás; felhasználói beállítások/presetek/firmware megőrzése. |
| INS-101-MAC | Kiadási követelmény | Egy macOS Universal 2 PKG telepítő, külön választható AU, VST3 és Standalone összetevőkkel. | Komponensválasztás, helyes célmappák, tiszta telepítés/frissítés és dokumentált eltávolítás; AU/VST3 hostfelderítés; Apple Silicon és x86_64 ellenőrzés. |
| INS-101-CI | Kiadási követelmény | Telepítők CI-előállítása és közös kiadásba csomagolása. | Mindkét telepítő azonos forráscommitból, fájllista és ellenőrzőösszeg; a publikáló elfogadja és ellenőrzi az új állományokat. |

A telepítők minden kiválasztott termékébe beépítve kerül a Standard és Retina skin, a Legacy elérhetőségével. Külön Blue termékazonosítók, meglévő GUI és projektkompatibilitás megőrzendő. Saját firmware és gyári hangadat nem kerül a telepítőkbe. Az aláírási tanúsítványok és a macOS notarizálás lehetősége felmérendő; a tényleges aláírási állapot szerepeljen a kiadási leírásban. Telepítőtechnológia és a telepítés felhasználói/rendszerszintű hatóköre az elővizsgálatban rögzítendő.

Egyszerre egy fejlesztési PR. A beolvasztást a legfrissebb commit sikeres Windows/macOS/web ellenőrzései és a feloldott review-szálak kapuzzák. Az 1.0.1 elfogadásához a telepítőkből végzett kézi próba is szükséges.

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
| 1 — GP-01 | Meglévő tesztek CI-lefedettsége; ROM nélküli és firmware-es eredmények különválasztása | Részben kész: 12 ROM nélküli CI-teszt mainben; teljes lefedettségi felmérés nyitott |
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
