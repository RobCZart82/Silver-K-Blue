# Silver-K Blue — változásjegyzék

## Fejlesztés alatt — 2026-11-02, PR #24

- Az X-Station webes szerkesztő tiszta checkoutból is fordítható: a hiányzó külső demók helyett üres könyvtárral indul. A hordozható HTML újragenerálva; külön webes build-/teszt-CI ellenőrzi a forrásával való egyezést.
- A natív patch-import összefűzi az SMF F0/F7 SysEx-folytatásokat, figyelembe veszi a track- és eseményhatárokat, és elutasítja a hibás/csonkolt bemenetet részleges patchlista nélkül. A nyers SYX import megmarad.
- A webes tesztbillentyűzet Shift vagy kiosztásváltás után is leállítja a ténylegesen indított hangot; új billentyűkezelési regressziós tesztek.
- A régebbi `package_release.py` külön rögzíti a Blue snapshot és az upstream baseline repository-ját/revízióját. Az upstream metadata bekerül a forrássnapshotba; új proveniencia-teszt.
- A pontos commitok, platformeredmények és a még el nem végzett hardveres/DAW-ellenőrzések a [fejlesztési naplóban](Documentation/11-Fejlesztesi-naplo-es-publikalasi-terv.md) szerepelnek. Ez a bejegyzés nyitott PR-t dokumentál, nem új kiadást.

## Fejlesztés alatt — 2026-10-30

- Bináris olvasás és alfolyamok határellenőrzése; javított FourCC sablon.
- Csonkolt és hibás WAV-fájlok biztonságos elutasítása; nullával osztás megszüntetése.
- MIDI realtime üzenetek és running status helyes feldolgozása; szabályos End of Track és trackhossz az exportban.
- Opaque FXB-bankok és VST3 preseteltolások javított beolvasása.
- Több munkaszálas feladatsor leállítása a függő feladatok befejezésével.
- A forráscsomag tartalmazza a fordításhoz szükséges Blue skineket és ikont.
- Új natív regressziós teszt és forráscsomag-teszt; Windows/macOS ellenőrzések pull requesteknél is.

## v1.0 — 2026-10-25

- Végleges skinnevek: Silver-K Blue Standard v1.0; Silver-K Blue Retina 4K v1.0.
- Állandó skinazonosítók és külön kiadási verzió-metaadat.
- Elfogadott Standard grafika és legutóbbi R2 Navy Native Fonts Retina 256 grafika változatlanul megőrizve.
- Legutolsó helyi VST3: natív betűs lenyílók, R2 navy háttéregységesítés, 4 soros fő LCD sortöréssel, inaktív Performance-képek halványítása és javított gyorstipp-sarkok.
- Aktuális használati és fordítási útmutató, teljes forrásjavítás, tesztek, grafikai mesterek és jövőbeli upstream együttműködési terv.
- 32 régi kimeneti csomag visszaállítható archívuma; aktuális kiadási ellenőrzés és ellenőrzőösszegek.

Ez a skin első végleges néven kiadott változata. A VST3 termékazonosítója és alapverziója továbbra is Silver-K / 0.4.4; a v1.0 a Blue skinek kiadási száma. A névátvezetés nem változtatta meg a legutóbbi bináris programkódját.
