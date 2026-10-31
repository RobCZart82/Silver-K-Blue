# Adatmappák és fájlkeresés

A Setup oldalon kiválasztott Silver-K adatmappa például macOS-en `/Users/robert/Documents/Silver-K/`. Ez felhasználó által választható hely, nem minden gépen kötelezően azonos útvonal.

| Almappa | Mit tárol / keres itt a program? | Keresés és betöltés |
| --- | --- | --- |
| **ROMs/** | K-Station vagy KS **firmware BIN**, kompatibilis **Wave ROM**, **KS All Data.MID** vagy más kompatibilis KS program/performance MIDI-adat | A firmware-vizsgálat az almappákra is kiterjed; a listát a Setup → Rescan ROMs frissíti. A firmware és a Wave ROM kiválasztása tudatos művelet. |
| **Patches/** | További presetek, programok és bankok: `.syx`, `.mid`, `.midi`, kompatibilis `.fxb`, `.fxp`, `.zip` | A kezelt Patches könyvtár csatlakoztatva van a könyvtárhoz. Add folder és Rescan az almappákat is feldolgozza. A felismert hangszínek a Patches listájába kerülnek; a megszólaló hangot a felhasználó választja ki. |
| **Skins/** | Külső skin-csomagok, csomagonként külön közvetlen almappában: `skin.json` és a hozzá tartozó layout/theme/képfájlok | Setup skinlista, Load skin és Reload/F5. A katalógus a Skins közvetlen almappáit nézi, nem tetszőleges mélységű könyvtárfát. A két beépített Blue skinhez ez a mappa nem szükséges. |

## ROMs — fontos különbségek

- **Firmware:** a K-Station és a KS saját rendszerprogramja; például `K-Station_OS_2.1.01.bin`, illetve `KS4_KS5_KSRack_OS_2.1.02.bin`. A `.bin` kiterjesztés önmagában nem bizonyít kompatibilitást.
- **Wave ROM:** a KS mintahullámforma-bankja; kompatibilis nyers bankméret 129 024 bájt. A `ks-wave-zero.bin` a program által létrehozott üres helyettesítő, nem gyári mintakészlet. Saját bankot Setup → Wave ROM alatt lehet kiválasztani.
- **KS All Data.MID:** KS indításhoz szükséges program/performance-adat. **Ezt is a ROMs mappában kell tartani**, nem csupán a Patches könyvtárban. Automatikus felderítése jelenleg a ROMs közvetlen fájljai között történik: `.mid`/`.midi`, a névben `all data` (kis-/nagybetűtől függetlenül), majd tartalmi ellenőrzés. Ez csak akkor választ adatot, ha még nincs kijelölt KS adatfájl. Más nevű kompatibilis fájl vagy több fájlos készlet a Setup → KS factory data gombbal választható ki.
- A másik helyről kiválasztott szükséges fájlokat a program a kezelt ROMs mappába másolja; az eredetiket megtartja. A projekt újranyitásakor a kijelölt fájlokra továbbra is szükség van.

A `.mid` fájl szerepét a tartalma és a felhasználási helye dönti el: egy további presetbank a Patches alá kerülhet, de az indítási KS All Data fájl a ROMs alá tartozik. K-Station módnak nincs szüksége KS All Data vagy KS Wave ROM adatra.

## Jelenlegi és tervezett frissítés

A folyamatos, fájlbedobást érzékelő Patches-figyelés **még tervezett funkció**, nem jelenlegi ígéret. Másolás után használd a Patches → Rescan gombot. Az explicit Rescan a kezelt Patches alatti korábban eltávolított forrásokat is újra felfedezheti; külső leválasztott forrást Add file / Add folder kapcsol vissza.

Tervezett: háttérben történő változásészlelés, a másolás befejezésének megvárása, változott presetek újraindexelése. A ROM és skin felismerése külön kezelendő az aktív firmware, hang vagy felület automatikus lecserélésétől.
