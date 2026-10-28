# Aktuális fejlesztési terv és megvalósítási állapot

Állapot: Silver-K Blue v1.0, 2026-10-25. Ez váltja fel a történeti tervek aktuális státuszát. Alap: upstream v0.4.4a, commit `4baaead06e23e2358729cca32b6b511c72022318`. A „kész” itt helyben megvalósított és célzottan ellenőrzött fejlesztést jelent; nem eredeti repositoryba beolvasztott változtatást.

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
6. Upstream merge, hivatalos terjesztés, Windows/Linux build, aláírt/notarizált telepítő és automatikus frissítés nincs kész.

## Javasolt következő kiadás

Először visszajelzés a napi használatból: K és KS firmware, Main/Perf/Patches/Setup, inaktív állapotok, hosszú patchnevek, több GUI-méret. Új vizuális változtatás előtt a v1.0 érintetlenül megmarad. A javításokhoz kis, áttekinthető eltérések és tesztek készüljenek; a grafikai ízlésváltoztatást válasszuk külön a programhibák javításától.
