# Silver-K – forráskódot érintő fejlesztési ötletek

Létrehozva: 2026. október 24.

Ez egy bővíthető ötletlista és döntési alap. Ide azok a kívánságok kerülnek, amelyek a jelenleg vizsgált Silver-K 0.4.4a skinrendszerével önmagukban nem oldhatók meg, vagy általános támogatásukhoz forráskód-módosítás kell. A lista összeállítása nem jelent döntést saját pluginváltozat készítéséről, újrafordításról vagy GitHub-közzétételről. Ezekről később, a felgyűlt igények alapján döntünk.

A műszaki megállapítások alapja a v0.4.4a kiadás, commit: `4baaead06e23e2358729cca32b6b511c72022318`. Megvalósítás előtt az akkori hivatalos forrásban ismét ellenőrizni kell őket.

## Nyilvántartás

| Azonosító | Ötlet | Miért kellene forrásmódosítás? | Jelenlegi állapot | Döntés |
|---|---|---|---|---|
| SK-001 | Választható, valódi pontmátrixos LCD-karakterek | A kijelző jelenleg natív szövegkomponens; a skinben készített háttér nem alakítja át az élő betűket pixelmátrixszá. | Grafikai előnézet és alkalmazhatóságában ellenőrzött forrásjavítás-prototípus készült. Nincs lefordítva vagy DAW-ban tesztelve. | Később eldöntendő |
| SK-002 | Normál méretű Retina forgógombok 128 képkockával | A gyári képbetöltő 8192 px képszélkorlátja miatt az egyenes 128 képkockás filmstrip képkockája legfeljebb 64 px lehet. | 192/256 pixeles grafikák és alkalmazhatóságában ellenőrzött betöltőjavítás-prototípus készült. Nincs lefordítva vagy DAW-ban tesztelve. | Később eldöntendő |
| SK-003 | LCD-textúrás háttér a lenyíló listák felcsukott mezőiben | A drawComboBox a 0.4.4a-ban színátmenetet rajzol, nem támogat általános háttérképet vagy rácstextúrát. | A tengerészkék színváltozat skinből elkészült; textúrarajzoló még nincs. | Később eldöntendő |
| SK-004 | Waveform-gombokon csak hover esetén látható háttér | A natív ág on vagy hover állapotban is kiemel, az egyedi képes ág pedig nem rajzol hover-hátteret. Skinből nincs külön hover-állapot. | Javasolt kapcsoló és alkalmazhatóságában ellenőrizendő forrásprototípus; nincs lefordítva. | Később eldöntendő |

## SK-001 – Valódi pontmátrixos LCD

**Cél:** hardveres hatású kijelző, amelyen a változó presetnév, paraméternév és érték is tényleges pixelcellákból rajzolódik ki. A világító karakterpixelek illeszkedjenek a háttér halvány, inaktív celláihoz.

**A hivatalos fejlesztésbe javasolható megoldás:** a skin választhasson natív és pontmátrixos szövegrajzolás között egy dokumentált LCD-stílusbeállítással. Az alapértelmezett viselkedés maradjon kompatibilis a meglévő skinekkel. A funkció más skinek számára is legyen használható, ne egy konkrét képfájl vagy erőforrás neve kapcsolja be. A JSON-mező nevét és szerkezetét még egyeztetni kell a fejlesztővel.

**A prototípus korlátai:** jelenleg 24 oszlop × 2 sor, 5×7-es nagybetűs karakterek; a hosszú sorokat rövidíti, a magyar ékezetes betűket alapbetűre cseréli, egyes további karaktereket nem támogat. Ezek kísérleti döntések, nem végleges követelmények. A bekapcsolás jelenleg egy konkrét erőforrás nevéhez kötött, amit a közösségi változatban általánosítani kell.

**Megvalósítás előtt tisztázandó:**

- A kívánt sor- és oszlopszám, betűméret és karakterkészlet.
- Kisbetűk, ékezetek, mértékegységek és különleges karakterek kezelése.
- Hosszú presetnevek és üzenetek megjelenítése: tördelés, görgetés vagy más megoldás.
- Mely megjelenési tulajdonságok állíthatók a skinből: színek, cellaköz, fényerő, inaktív pixelek.
- A háttér és a betűk pontos illesztése különböző kijelzőméreteknél és GUI/DPI-nagyításoknál.

**Elfogadási feltételek:** a kijelző szövege továbbra is élő adat; minden fontos üzenet és érték érthetően megjelenik; a régi skinek kompatibilisek; a funkció fordítási és futtatási ellenőrzése sikeres; Reaperben presetváltás, paraméterállítás és projektújranyitás során is megfelelően működik. Más feliratok betűtípusa nem változik meg mellékhatásként.

## SK-002 – Retina filmstrip-támogatás

**Cél:** az elfogadott, teljes egészében forgó kupak és a LED-gyűrű normál kezelőméretnél is éles legyen nagy pixelsűrűségű kijelzőn. A jelenlegi 72 logikai pixeles főgombhoz 2× mintavételezésnél legalább 144 px/képkocka szükséges; a 192 pixeles változat ehhez tartalékot ad.

**A hivatalos fejlesztésbe javasolható megoldás:** a képbetöltő és a filmstrip-formátum kapjon dokumentált, memóriában is korlátozott nagyfelbontású támogatást. Lehetséges irány a hosszú filmstripek célzott engedélyezése, vagy egy több sorból és oszlopból álló képkockarács támogatása. A konkrét megoldás még nincs eldöntve; nem feltétlenül kell mindkettőt megvalósítani.

**A prototípus korlátai:** a jelenlegi javítás a pontosan 128-szoros képarányú sávokat legfeljebb 256 px/képkockáig engedi. Ez a saját csomagunkhoz készült célzott kísérlet. Egy 256×32768-as RGBA kép 32 MiB nyers képadat; több erőforrás betöltése jelentős memóriát használhat. A tömörített PNG mérete nem azonos a futás közbeni memóriaigénnyel.

**Megvalósítás előtt tisztázandó:**

- Támogatott képkockaszámok, elrendezések és maximális felbontások.
- Dekódolt képméretre és összes betöltött erőforrásra vonatkozó korlátok.
- Memóriahasználat, betöltési idő és GUI-nagyítás minősége.
- Hibás, hiányos vagy túl nagy képek ellenőrzése.
- Kompatibilitás a jelenlegi függőleges/vízszintes és komponensenként választott filmstripekkel.

**Elfogadási feltételek:** a nagyfelbontású gombok megbízhatóan betöltődnek és megfelelően skálázódnak; a memóriahasználat mérhető és ésszerűen korlátozott; a korábbi skinek tovább működnek; az összes képkocka és a paraméterpozíció helyesen illeszkedik; a javítás fordítási és DAW-próbán is átmegy.

## SK-003 – LCD-textúrás lenyíló mezők

Felvéve: 2026. október 25.

**Cél:** minden felcsukott lenyíló listának nagyon sötét tengerészkék, finoman LCD-rácsos háttere legyen, fehér betűkkel és számokkal. A lenyitott menük olvasható sötétkék témát kapjanak.

**Ami már skinből működik:** a standard mezők háttérszínátmenete, kerete, szövegszíne és a megnyitott menü színei. A Performance presetmező külön szülőrajzolásának színezése is lehetséges. A színváltozat nem valósít meg valódi textúrát.

**A hiányzó támogatás:** a Silver-K 0.4.4a drawComboBox rajzolója nem használja a backgroundImage beállítást. Általános, a méretváltozást, oldalváltást és K/KS vezérlőtípusokat követő háttérkép- vagy procedurális LCD-rács támogatás kellene. A nyíl, keret, szöveg, fókusz és letiltott állapot megfelelő rétegezését meg kell őrizni. A nagy képek torzítása helyett célszerű ismételhető textúrát vagy mérettől független rácsozást támogatni. A Performance presetmezők egyedi rajzolását is kezelni kell.

**Elfogadási feltételek:** Main, Extra, Performance, Patches és Setup oldalakon, mindkét modellel, több GUI-méretben és változó szöveghosszal működő megjelenés; jól olvasható fehér szöveg; a régi skinek változatlan működése; fordítási és DAW-próba.

**Állapot:** gyűjtött fejlesztési igény; a színváltozat készen áll kipróbálásra, textúrás megoldás nincs lefordítva vagy tesztelve. A végleges API és a megvalósítás később eldöntendő.

## SK-004 – Waveform hover és kijelölés szétválasztása

**Elvárt működés:** a hullámforma-választó körüli halvány háttér kizárólag az egér rámutatásakor látható, az aktív hullámformát a piros lámpa jelzi. Működjön natív és saját On/Off grafikákkal is, a Sync gombéhoz hasonló visszajelzéssel.

**Jelenlegi korlát:** a gyári waveform ág az on || hover feltételt használja. A saját kép elsőbbségi rajzolóága a hover előtt visszatér. Ezért a csak skinből készített háttérmentes változatban hover-jelzés sincs. Nincs külön hover-kép a vizsgált API-ban.

**Javaslat:** opcionális, komponensenkénti waveformHoverOnly beállítás, a régi skinek változatlan alapértelmezésével. A hover rajzolása az egyedi kép mögött is megtörténne. Az API-nevet a fejlesztővel kell egyeztetni.

**Állapot:** helyi forrásprototípus és JSON-beállítási minta készült. Fordítás és DAW-próba nem történt; a telepített pluginhoz nem nyúltunk. Ellenőrizendő mind a 12 waveform-gomb, kiválasztott/nem kiválasztott, hover/nem hover, letiltott állapot és a KS menüs változat változatlansága.

## Ami jelenleg skinből is megoldható

Ezeket már külön kezeljük a forráskódot érintő listától:

- A teljes kupak és a kék középrésszel egybefüggő mutató forgása filmstripből.
- A 21 LED-es körív, a LED-színek és a szélesebb, világosabb szürke körsáv.
- LCD-háttérkép, finom rácsozás, peremárnyék és statikus üvegfény.
- Az exportált template megtartása, a skin által támogatott elrendezési és színmódosítások.

Ezekhez önmagukban nem szükséges saját VST3-at fordítani. A pontmátrixos élő betűrajzolás és a normál méretű Retina-filmstripek támogatása a fenti külön igény.

## Későbbi döntési folyamat

1. Gyűjtjük az új, tényleges használat során felmerülő igényeket.
2. Minden ötletnél ellenőrizzük, hogy valóban forrásmódosítás kell-e, illetve nem került-e már be a hivatalos fejlesztésbe.
3. A lista alapján eldöntjük, mely tételek érik meg a megvalósítást és karbantartást.
4. A közösségi hozzájárulásra kiválasztott ötleteket egyeztetésre alkalmas, képekkel bemutatott javaslattá formáljuk.
5. Ha a fejlesztő támogatja az irányt, elkészítjük az aktuális forráshoz igazított, általános megoldást, lefordítjuk és teszteljük, majd külön döntés alapján pull requestben felajánljuk.

A jelenlegi prototípusok nem jelentenek bizonyítékot a kész plugin működésére. A két javítás együttes alkalmazása, fordítása és futtatása még nincs ellenőrizve. A befogadásról és a hivatalos kiadásba kerülésről az eredeti projekt karbantartója dönt.

## Új ötlet felvételi sablon

- **Azonosító és cím:** SK-005 – …
- **Tapasztalt probléma vagy kívánt viselkedés:** …
- **Miért nem elég a jelenlegi skinrendszer?** …
- **Elvárt felhasználói eredmény:** …
- **Forrásmódosítás lehetséges terjedelme:** …
- **Kompatibilitás és karbantartási szempontok:** …
- **Szükséges ellenőrzés és elfogadási feltételek:** …
- **Állapot:** ötlet / egyeztetendő / prototípus / tesztelt / felajánlott / befogadott / elvetett.
- **Döntés:** később eldöntendő / kiválasztott / elhalasztott / elvetett.
