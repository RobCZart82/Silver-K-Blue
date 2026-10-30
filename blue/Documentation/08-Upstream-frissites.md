# Az eredeti Silver-K fejlesztéseinek átvétele

A saját repository megőrzi az eredeti Git-történettel közös alapot: v0.4.4a, 4baaead06e23e2358729cca32b6b511c72022318. A 2026-10-29-i kezdeti összekötő merge csak ezt az ismert alapot kapcsolta a saját snapshothoz, változatlan fájltartalommal. Ez nem jelenti új upstream fejlesztések felülvizsgálat nélküli átvételét. A későbbi frissítések rendes Git-merge-et használnak, nem az induláskori ours-stratégiát.

## Napi ellenőrzés

A **Check Silver-K upstream updates** munkafolyamat naponta 08:17 UTC körül ellenőrzi:

- temaniak/Silver-K **main** ágának új commitjait: javítások és fejlesztések;
- a legújabb, GitHub szerint nem előzetes kiadást.

A GitHub ütemezése késhet. Kézi indítás: Actions → Check Silver-K upstream updates → Run workflow, a main ágon. Az eredeti kísérleti ágak és prerelease-ek nem kerülnek automatikusan átvételre. A main és a kiadás ellenőrzése külön történik: egy kiadási tag megjelenése önmagában nem garantálja, hogy annak eltérő kódja már a main ágon van.

## Új változás esetén

A bot az **automation/upstream-sync** ágra készít frissítési javaslatot, és összehasonlító linket ad a saját repositoryhoz. Erről **kézzel nyitható PR**. Nem kér PR-jóváhagyási jogosultságot, nem nyit PR-t automatikusan. A main ágat nem egyesíti automatikusan, a repositoryt nem teszi publikussá, az eredeti projekthez nem ír.

Tiszta merge esetén a PR tartalmazza az upstream módosításokat, a Blue munka megőrzésével. Ütközésnél a merge visszaáll, és csak a `blue/UPSTREAM-UPDATE.md` jelentés kerül a PR-ba, az ütköző fájlok listájával. Ilyenkor a forrásfrissítés még nincs kész; kézzel kell rendezni.

Az automatikus ág erre a célra fenntartott ág. Ha kézzel dolgozunk rajta, a bot felismeri a nem bot által írt legutolsó commitot és nem állítja vissza az ágat. A kézi rendezés és felülvizsgálat után a PR egyesíthető, a bot pedig a következő változáskor új javaslatot készíthet. Egy változatlan javaslat miatt nem nyit PR-t; ugyanaz a frissítési ág használható tovább.

## Elfogadás előtti ellenőrzés

1. Nézzük át a PR különbségeit, főleg a felület, a buildbeállítások és a GitHub-workflow-k módosításait.
2. A Windows buildet kézzel futtassuk a javaslati ágra, teszteléskor kikapcsolt Release-mentéssel. A bot PR-ja nem jelent automatikus tesztjóváhagyást.
3. Fordítsuk és ellenőrizzük a macOS-változatot is; próbáljuk a két skint, a K/KS módot, a projekt-visszatöltést és a fontos hangfunkciókat.
4. Csak a megfelelő eredmény után egyesítsük a saját main ágba; utána készülhet az új ellenőrzött kiadás.

Az upstream új verziója új ütközéseket vagy teszthibákat hozhat. A folyamat megkönnyíti az átvételt, de nem ígér minden jövőbeli verzióval automatikus kompatibilitást. A `blue/upstream-state.json` a saját main által elfogadott állapotot és az utolsó feldolgozott stabil kiadást rögzíti.

## Jogosultság és ellenőrzés

A frissítési job csak repository-tartalomírást kér a saját javaslati ágához. A repository azonosítása megmarad, de privát és nyilvános láthatósággal is működik. A bot nem nyit, hagy jóvá vagy merge-el PR-t, és nem publikál kiadást. Nincs személyes token vagy firmware a workflow-ban.

A `tools/test_sync_blue_upstream.py` ideiglenes helyi repositorykkal, GitHub-írás nélkül ellenőrzi a frissítés nélküli, tiszta merge, ütközés és release-only eseteket privát és nyilvános repositoryval, valamint az idegen repository elutasítását. A workflow minden ellenőrzés előtt lefuttatja ezt.
