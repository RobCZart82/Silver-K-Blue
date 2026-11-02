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

## Első célzott upstream átvétel ellenőrzése

Az új ROM nélküli bemeneti adapterteszt külön is fordítható/futtatható a konfigurált natív buildben:

```powershell
cmake --build build --config Release --target silverAudioInput_test --parallel 4
ctest --test-dir build -C Release --output-on-failure --no-tests=error -R '^silver_audio_input_safety$'
```

A teljes Windows/macOS CI 11 ROM nélküli tesztet választ ki, köztük a meglévő patch-, persistence/Rescan-, külsőskin- és hosszú resamplertesztet. A `silver_latency_reentry` külön lépésben fut; magán-ROM nélkül SKIP, nem sikeres firmware-es teszt. A ROM nélküli preset-/persistence-futás sem igazolja a firmware-es ágaikat.

Saját firmware-es latenciaellenőrzéshez a meglévő `SILVER_TEST_K_OS` és `SILVER_TEST_KS_OS` mellett `SILVER_TEST_KS_DATA` és `SILVER_TEST_KS_WAVES` is megadható. Ezek privát, helyi fájlútvonalak; elkülönített `SILVER_K_DATA` könyvtárat használj. A környezeti változók támogatása nem jelenti, hogy az adott firmware-/hangpróba már lefutott.

### KS indítási csend regressziós próba

A `silver_latency_reentry` KS-ága firmware-betöltés után, Panic vagy új programdump előtt ellenőrzi a mixer/vocoder módosítása melletti csendet, egy megszólaltatott MIDI-hangot és annak elengedését. Ugyanazokkal a privát környezeti változókkal futtatható; `SILVER_KS_CONTROLLER=native` mellett ismételd meg a natív KS-vezérlő ellenőrzéséhez. A helyi Windows referencia/natív futás sikeres, a tesztcsatornán beragadt hang nem reprodukálható. A nullás teszthullámállomány nem igazolja a gyári minták hanghűségét. Firmware nélkül a teljes latenciateszt SKIP marad.

## Forráseredet a csomagolókban

A jelenlegi Blue platform-workflow-k a `tools/package_blue_windows.py` eszközt használják Windows és macOS csomagolására; a `build-info.json` a Blue repository-t és a pontos forráscommitot rögzíti. A PR #24 proveniencia-javítása a külön, régebbi `tools/package_release.py` eszköz `SOURCES.txt` kimenetét érinti.

Ebben a Blue snapshot linkje a `RobCZart82/Silver-K-Blue` repository-ra mutat. Az upstream baseline repository-ja és commitja a `blue/upstream-state.json` külön adataiból származik, nem a Blue HEAD SHA-jából. A metadata a generikus forrássnapshot része. A snapshot hash-e, alapcommitja és a helyi módosítások jelölése továbbra is külön szerepel. A forráslinkek javítása önmagában nem új bináris kiadás vagy upstream-integráció.


## K-Station bemeneti diagnosztika

A fejlesztési ág `silver_input_diagnostics` tesztje ROM nélkül a JSON-riport elérhetőségét/sémáját vizsgálja. A `silverInputDiagnostics_test` bináris egy saját K-Station firmware útvonalával a valódi mono bemeneti sor és a diagnosztikai Stop/Start viselkedését is ellenőrzi. Mindkét esethez elkülönített `SILVER_K_DATA` mappa szükséges; firmware-t ne tölts fel a publikus CI-be.

A JSON `input_overflows` értéke az adott K-Station engine létrehozása óta eldobott minták száma a host–worker és DSP input sor határán. A diagnosztikai session nem nullázza; a session `input_overflows_at_start` mezője a kezdőérték. Új engine új számlálót jelent. A `controller_backend` K-Station esetén `original-h8`; ez a Blue jelenlegi implementációját írja le.

Az érték csak `input_overflows_supported=true` mellett értelmezhető. KS vagy engine nélküli állapotban a támogatottság hamis, a scope `not-applicable`; a nulla ilyenkor nem sikeres KS bemeneti mérés. A riport kiegészítése nem hoz új GUI-elemet, és a meglévő audio-routingot nem változtatja.


A privát teszt előbb normál offline mono adatúton ellenőrzi, hogy a számláló nem nő, majd szándékosan túlméretezett realtime hostblokkal eldobást idéz elő. Ez a számlálás és session-élettartam regressziója, nem a valós audio-driver realtime teljesítményének mérése.
