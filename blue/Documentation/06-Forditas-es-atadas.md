# Fordítás és forrásátadás

A teljes repository az aktuális build kiindulópontja. Nem kell rá újból alkalmazni a blue/upstream-v0.4.4a.patch történeti pillanatfelvételt vagy régi helyi tooltip-patcheket.

## Fejlesztési és átvételi követelmény — változatlan GUI

A 2026-11-02-i felhasználói döntés alapján a további háttérfejlesztések megőrzik a Standard, Retina és Legacy skinek jelenlegi grafikáját, komponenspozícióit/méreteit, panel- és oldalelrendezését, valamint a meglévő kezelőszerveket. Új panel, gomb, beállítási mező vagy skinmódosítás nem része az aktív fejlesztési tervnek. A követelményt upstream változásokra is alkalmazni kell.

GP-01–GP-06 munkák átvételekor:

- A változáslistán ellenőrizni kell a skin-layout/theme/manifest, képfájlok és a natív editor geometriai/rajzolási módosításait. E fájlok vagy kódrészek módosítása nem vélelmezhetően szükséges egy háttérjavításhoz; a grafikai megjelenést/elrendezést módosító rész nem tartozik az engedélyezett körbe.
- Az érintett natív editor-, Performance-, beépítettskin- és külsőskin-regressziókat azonos tesztkörnyezettel kell ellenőrizni. Ha a változás közvetve érintheti a GUI-t, összehasonlítható render vagy használati ellenőrzés szükséges a releváns skinekkel és méretekkel. Ez ellenőrzés, nem grafikai újratervezés.
- Meg kell őrizni a hostállapot, pluginazonosítók, skinválasztás és meglévő méretezés kompatibilitását. Teljesítményjavításnál ugyanazon bemenethez kell helyességi/hangkimeneti és időzítési bizonyítékot rögzíteni.
- A PASS/FAIL/SKIP eredményekhez pontos commit és futáslink vagy helyi parancs tartozzon. A firmware hiányában kihagyott tesztrész nem firmware-es elfogadás; a mockolt és izolált teszt nem DAW-/hardverpróba.

A [napló aktív terve](11-Fejlesztesi-naplo-es-publikalasi-terv.md) külön kezeli a megvalósítást, az automatizált ellenőrzést és a használati elfogadást. Új kezelőszervet igénylő Undo/A–B vagy audio-bővítés nem kezdhető meg ezen terv alapján. A jelen dokumentációfrissítés nem változtat futó kódot, grafikát vagy tesztelési workflow-t.

A pontos Windows x64 és macOS Universal 2 parancsokat a [doc/BUILD.md](../../doc/BUILD.md) tartalmazza. Az első konfigurálás hálózatról tölti a rögzített függőségek forrásait; ez nem teljes offline környezet.

Mindkét Blue skin és az alkalmazásikon a build erőforrásai közé tartozik. A kész csomag forráscommitját a build-info.json rögzíti, nem a skin v1.0 verziója. A képek módosítása nélkül készült új kód is új forráscommitot és új binárist jelent.

Teszteléshez külön SILVER_K_DATA könyvtárat használj. A ROM nélküli regressziók és a saját firmware-t igénylő tesztek eredményét külön rögzítsd; a kihagyott teszt nem sikeres firmware-es teszt.

A main-beolvasztás feltételeit, az aktuális PR-ok ellenőrzéseit és a publikálási sorrendet a [fejlesztési napló](11-Fejlesztesi-naplo-es-publikalasi-terv.md) rögzíti. A korábbi PR #1-re vonatkozó nyitott státusz történeti volt; a 2026-11-02-i auditjavításokat a PR #24 tartalmazza.

## Az auditjavítások célzott ellenőrzése — PR #24

A natív importáló SMF track-/eseményhosszakat ellenőriz, és összefűzi az F0/F7 SysEx-folytatásokat. Csonkolt vagy hibás SMF-ből nem ad át részleges patchlistát; a nyers SYX-beolvasás megmarad. A regressziók az egy- és többeseményes üzenetet, hosszú metaadatot, running statust, F7 escape-et, csonkolást és hibás VLQ-t is ellenőrzik. A szigorú SMF-feldolgozás nem ígéri a korábban heurisztikusan beolvasott, nem szabályos MIDI-fájlok támogatását.

Konfigurált natív buildkönyvtárból, Windows PowerShellben:

```powershell
cmake --build build --config Release --target silverFramework_test --parallel 4
$env:SILVER_K_DATA = "$env:TEMP/silver-blue-audit-tests"
ctest --test-dir build -C Release --output-on-failure --no-tests=error -R '^silver_framework_safety$'
python -m unittest discover -s tools -p "test_*.py"
python tools/check_repository.py
```

macOS-on a már konfigurált Release buildhez ugyanez a target és CTest-szűrő használható; az elkülönített adatmappát shell-környezeti változóként add meg. A teljes platformellenőrzéseket továbbra is a Windows/macOS workflow-k futtatják.

Az X-Station szerkesztő külön webalkalmazás. Node.js 24 mellett, a repository gyökeréből:

```sh
cd x-station-editor/web
npm ci
npm test
npm run build
```

A könyvtár kezdetben üres; saját kompatibilis SYX/ZIP fájlokat kell importálni. A build nem igényel és nem csomagol gyártói demódumpot. A `build/` a webes kimenet, a követett `x-station-editor/dist/XStationEditor.html` a hordozható változat. Webes forrásmódosítás után ezt is újra kell generálni; a külön `xstation-web.yml` workflow eltérés esetén hibázik. A hardveres MIDI-próbát a mockolt tesztek nem helyettesítik.

## Forráscommit és upstream eredet

A jelenlegi Blue platform-workflow-k a `tools/package_blue_windows.py` eszközt használják Windows és macOS csomagolására; a `build-info.json` a Blue repository-t és a pontos forráscommitot rögzíti. A PR #24 proveniencia-javítása a külön, régebbi `tools/package_release.py` eszköz `SOURCES.txt` kimenetét érinti.

Ebben a Blue snapshot linkje a `RobCZart82/Silver-K-Blue` repository-ra mutat. Az upstream baseline repository-ja és commitja a `blue/upstream-state.json` külön adataiból származik, nem a Blue HEAD SHA-jából. A metadata a generikus forrássnapshot része. A snapshot hash-e, alapcommitja és a helyi módosítások jelölése továbbra is külön szerepel. A forráslinkek javítása önmagában nem új bináris kiadás vagy upstream-integráció.
