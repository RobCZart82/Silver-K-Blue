# Fordítás és forrásátadás

A teljes repository az aktuális build kiindulópontja. Nem kell rá újból alkalmazni a blue/upstream-v0.4.4a.patch történeti pillanatfelvételt vagy régi helyi tooltip-patcheket.

A pontos Windows x64 és macOS Universal 2 parancsokat a [doc/BUILD.md](../../doc/BUILD.md) tartalmazza. Az első konfigurálás hálózatról tölti a rögzített függőségek forrásait; ez nem teljes offline környezet.

Mindkét Blue skin és az alkalmazásikon a build erőforrásai közé tartozik. A kész csomag forráscommitját a build-info.json rögzíti, nem a skin v1.0 verziója. A képek módosítása nélkül készült új kód is új forráscommitot és új binárist jelent.

Teszteléshez külön SILVER_K_DATA könyvtárat használj. A ROM nélküli regressziók és a saját firmware-t igénylő tesztek eredményét külön rögzítsd; a kihagyott teszt nem sikeres firmware-es teszt.

A main-beolvasztás feltételeit, a PR #1 nyitott ellenőrzéseit és a publikálási sorrendet a [fejlesztési napló](11-Fejlesztesi-naplo-es-publikalasi-terv.md) rögzíti. A letölthető csomagok tartalmazzák a forrásra mutató commitlinket és licenceket; a generikus forráscsomag Blue-erőforrásainak javítása a PR #1 része.
