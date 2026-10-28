# Silver-K 0.4.4a – forrásalapú skin API-ellenőrzés

Ellenőrzés: 2026-10-24. A GitHub `v0.4.4a` tagjának feloldott commitja: `4baaead06e23e2358729cca32b6b511c72022318`. A tag annotált objektuma `f7d4ca17bc66614da63fdf049de123959035cfa9`; a kódot a feloldott commiton olvastuk. Az alábbi megállapítások ehhez a változathoz tartoznak.

## Elsődleges források

- [Skinleírás – doc/SKINS.md](https://github.com/temaniak/Silver-K/blob/4baaead06e23e2358729cca32b6b511c72022318/doc/SKINS.md)
- [Betöltő és filmstrip-rajzoló – silverSkin.cpp](https://github.com/temaniak/Silver-K/blob/4baaead06e23e2358729cca32b6b511c72022318/source/novation/silverK/plugin/silverSkin.cpp)
- [Natív rotary és paraméterkötés – silverEditor.cpp](https://github.com/temaniak/Silver-K/blob/4baaead06e23e2358729cca32b6b511c72022318/source/novation/silverK/plugin/silverEditor.cpp)
- [Performance vezérlők – silverPerformance.cpp](https://github.com/temaniak/Silver-K/blob/4baaead06e23e2358729cca32b6b511c72022318/source/novation/silverK/plugin/silverPerformance.cpp)
- [K paraméterkatalógus](https://github.com/temaniak/Silver-K/blob/4baaead06e23e2358729cca32b6b511c72022318/source/novation/kstation/kstationJucePlugin/kstationControls.h)
- [KS paraméterkatalógus](https://github.com/temaniak/Silver-K/blob/4baaead06e23e2358729cca32b6b511c72022318/source/novation/ks/ksJucePlugin/ksControls.h)
- [A megfelelő kiadás](https://github.com/temaniak/Silver-K/releases/tag/v0.4.4a)

## Ténylegesen támogatott API

A manifest `schemaVersion: 1`, `engine: "silver-k-juce"`. A `layout` és `theme` relatív fájlutak. A layout és theme a beépített alapokkal egyesül: az objektumok öröklődnek, a tömbök teljesen cserélődnek. Ezért a csomag két mezős layoutja megtartja az összes gyári vezérlőt és kötést, miközben az új skinnevet és a studio elrendezést kéri.

A theme `images` mezője tetszőleges erőforrásnevet rendel egy helyi bitmapfájlhoz. A `knobStrip` tartalma `image`, `frames`, `orientation`. 2–1024 képkocka, függőleges vagy vízszintes elrendezés támogatott; a hosszú képméretnek oszthatónak kell lennie a képkockaszámmal. Rácsos atlasz, több sor/oszlop együttes címzése nincs.

A `SkinLookAndFeel::drawKnobStrip` a kiválasztott képkockát `roundToInt(position * (frames-1))` szerint vágja ki, és a rotary rajzterületére skálázza. A `components.<slider ID>.knobStrip` külön képet választhat, de a frames/orientation a globális beállításból jön. A komponensazonosító és a teljes szülő/gyermek útvonal egyaránt támogatott, az útvonal specifikusabb. Egy konténer stílusa nem öröklődik automatikusan a sliderre.

A Silver-K valódi skin-rajzolója a `silverEditor.cpp` és `silverSkin.cpp` fájlokban van. A keretrendszer másik `juceUiLib/rotaryStyle.cpp` implementációja nem a jelen csomag manifest API-ja.

`panelKnob`, `knob`, `classic`, `miniKnob`, illetve a rotaryként kialakított mixer/compact vezérlők ugyanazt a natív rotary rajzolót használják; a filmstrip a beépített műanyagtestet, LED-eket és flat rotary rajzot is megelőzi. A fader és a row/extraRow lineáris slider külön rajzolási útvonalat kap. Enum paraméterből a `Control` menüt vagy a kijelölt natív választóvezérlőt készíti; a filmstrip nem írja felül ezt a döntést.

## Paraméterkövetés és nullapont

Main/Extra alatt a slider nyers egészérték-tartománya a paraméterleíróból származik. A számkijelzés `raw + displayOffset`. Ha ez a tartomány negatív és pozitív értékeket is tartalmaz, a konstruktor `bipolarZero` tulajdonságot állít be.

A rajzoló a nyers normalizált `p` helyzetet így alakítja:

```text
z = (-displayOffset - minimum) / (maximum - minimum)
u = p <= z ? 0.5 * p / z : 0.5 + 0.5 * (p-z) / (1-z)
frame = round(127 * u)
```

Unipolárisnál `u=p`. Nulla esetén `u=0.5`, kiválasztott frame = 64. A csomag 64. képkockáján a mutató pontosan 12 órán áll. A pointer pályája a natív 4π/3…8π/3 paraméterezést követi, 240°-os íven. A teljes kupak forgása, az egybefüggő kék mutató és a LED-logika a PNG-be van sütve, a skin API nem kap külön LED-vezérlési görbét.

A Performance `Parameter` osztálya ebben a kiadásban nem állít `bipolarZero` tulajdonságot. A Transpose nyers tartománya 40…88, nulla 64, vagyis középen van. A Fine 0…127, nulla 64, ami kissé eltér az aritmetikai féltől. A mellékelt erőforrások 64. indexű nullája és a nyolc explicit Performance slider-hozzárendelés ezt az eltérést kezelik. A Main/Extra aszimmetrikus tartományain a középre normalizálást továbbra is a plugin végzi.

A frissítési útvonal az audio-paraméter aktuális értékét olvassa; az editor időzítője 20 Hz-en fut. Így presetváltás és hostautomatizáció is megjelenhet, a UI normál késleltetésével és kvantálásával. Egérhúzás vagy szöveges szerkesztés közben a kód kerül bizonyos külső visszaírásokat. A LED-ek a megjelenített paraméterpozíciót követik, nem a DSP belső modulációját, LFO-fázisát vagy audiojelszintjét.

## Lényeges technikai korlátok

1. **8192 px képszél a gyári loaderben.** A `silverSkin.cpp` képbetöltési ellenőrzése 8192-nél nagyobb szélességet vagy magasságot elutasít. 128 egyenes képkockánál ebből legfeljebb 64 px/képkocka következik. A 128-as sáv támogatása önmagában nem garantál normál méretű Retina-gombot.
2. **Nincs `@2x`-atlasz vagy rácsos filmstrip API.** A JSON nem tudja a 128 képkockát pl. 16×8-as rácsból kivágni. Az orientáció horizontálisra cserélése a méretkorlátot nem oldja meg.
3. **Retina nagy gombokhoz módosított build kell.** A mellékelt patch a 128-szoros képarányú sávokat 256 px/képkockáig engedi. Az alternatív Stock32 skin kisebb gombokkal teljesíti a 2× felbontást gyári binárison.
4. **Memória.** Egy RGBA64 sáv 2 MiB, egy RGBA192 sáv 18 MiB, egy RGBA256 sáv 32 MiB nyers pixeladat. Négy külön erőforrásra ez 8/72/128 MiB; az azonos képtartalom automatikus deduplikálására nincs garancia. A PNG mérete és a dekódolt memória különböző. A tényleges pluginmemória ennél nagyobb lehet. A nagy változatokból csak a használt skint töltsd be.
5. **Kvantálás.** 128 grafikai állapot van; 24 vagy 30 lépéses paraméter egyes képkockákat kihagy. A diszkrét LED-szegmensek több egymás utáni képkockán azonosak lehetnek, miközben a mutató mozog. A felső nullapont miatti mintavétel enyhén aszimmetrikus.
6. **Tartományt nem módosít a skin.** A volume nem kap mesterséges logaritmikus leképezést; a tényleges DSP-hangerőgörbe nem a grafikai API része. Az egyedi paramétertartományt a plugin definiálja.
7. **Dekoráció nem dinamikus LED.** A layout dekorációi statikusak; a paraméterkövető LED-eket a forgó filmstripbe sütöttük. Nem külön áttetsző LED-kép mozog a gomb felett.
8. **Bitmap és nagyítás.** PNG-s filmstripet skáláz a JUCE, tehát nem végtelenül nagyítható vektor. Retina-minőség csak a megadott mintavételi határig áll fenn. A skin nem írja át a host DPI-ját.
9. **Összes forgó megjelenítést lefedő készlet, nem minden paraméter forgósítása.** A katalógus K: 154 bejegyzés, ebből 108 nem-enum; KS: 182 bejegyzés, ebből 129 nem-enum. A bipoláris nem-enum bejegyzések száma 52/64. Ez nem a képernyőn egyszerre látható gombok darabszáma. A menük, lineáris faderek és gombok natív típusa megmarad.
10. **Futtatási ellenőrzés határa.** A csomag formátumát, hivatkozásait, összes filmstripjét, katalóguslefedettségét és a patch alkalmazhatóságát ellenőriztük. Nem történt VST3-fordítás, natív skin-loader tesztfuttatás, hostautomatizációs vagy DAW-ban végzett vizuális teszt. A mellékelt előnézet az asseteket mutatja, nem a futó Silver-K-t.

## V2 grafikai korrekció

A teljes kupak minden képkockán elfordul. A széles sötét LED-körsáv és a diódák pozíciója helyben marad; csak a világítás változik. A kék mutató a generált kupak része, a középrésszel együtt forog. A JSON/API és a Retina-betöltési korlát nem változott.
