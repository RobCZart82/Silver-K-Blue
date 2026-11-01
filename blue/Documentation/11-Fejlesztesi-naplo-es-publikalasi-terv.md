# Silver-K Blue — fejlesztési terv és munkanapló

Utolsó frissítés: **2026-10-31**

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
