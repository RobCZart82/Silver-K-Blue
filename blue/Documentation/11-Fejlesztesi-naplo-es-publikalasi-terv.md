# Silver-K Blue — fejlesztési terv és munkanapló

Utolsó frissítés: **2026-10-30**

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
