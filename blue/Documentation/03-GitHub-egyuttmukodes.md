# Későbbi együttműködés az eredeti fejlesztéssel

Eredeti projekt: https://github.com/temaniak/Silver-K . A csomag rögzített alapja v0.4.4a; nem állítjuk, hogy a 2026-10-25 utáni upstream változásokat tartalmazza. A munka helyben készült, nem küldtünk issue-t, PR-t vagy üzenetet.

## Javasolt menet

1. A tulajdonossal először a célokat és a hozzájárulási szokásokat egyeztessük. Mutassuk meg a két skin nevét, az előtte/utána képeket, a pontos hibát és azt, melyik fejlesztés általánosítható.
2. A tényleges hozzájárulás előtt frissítsük az upstream állapot ismeretét, olvassuk el az aktuális CONTRIBUTING/AGENTS/README és licencszabályokat. A most mellékelt teljes patch dokumentált átadási alap, nem egy kész, minden mai ágra alkalmazható PR.
3. Forkban, külön ágakon kis változásokat javasoljunk: gyorstipp-sarok; hover-javítás; inaktív képes vezérlők; majd opcionális Retina/API és LCD. A hangmotor módosítását ne vonjuk be ebbe a felületi munkába.
4. Minden PR legyen a fogadó ág friss állapotára illesztve, régi skineket megőrző alapértékekkel, célzott teszttel és egyértelmű képi bizonyítékkal. A nagy, közös patch szétválasztása és felülvizsgálata még feladat.
5. A skin és a forráskód jogállását külön nézzük át. A mellékelt upstream licencet és eredeti megjelöléseket tartsuk meg. A felhasználói grafikai források publikus terjesztési jogát egyeztessük; firmware-t vagy bankot ne tegyünk a PR-ba.

## Hogyan maradunk kapcsolatban az eredeti fejlesztéssel?

A fork upstream távoli forrást kap, a helyi munkát kis saját commitokban őrzi. Új upstream kiadáskor az új kódot be lehet emelni, az ütközéseket meg kell oldani és újra tesztelni/fordítani. Ez nem automatikus binárisfrissítés. Ha a karbantartó beolvasztja a javításokat, azokat későbbi hivatalos kiadásban is használhatjuk; a fejlesztő dönt a befogadásról és az API formájáról.

A gyári kompatibilis Standard addig is önálló grafikai csomagként használható. A Retina helyi bővítéseit csak megfelelő binárissal lehet futtatni.
