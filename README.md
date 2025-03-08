# Problem ucztujących filozofów

## Jak uruchomić

### Budowanie ze źródła

```bash
git clone git@github.com:jakubwiczkowski/dining-philosophers.git
cd dining-philosophers/
mkdir build/ && cd build/
cmake ..  
```

### Uruchomienie programu

```bash
cd build
./dining-philosophers
```

Jako argument programu można podać liczbę filozofów >= 1. W przypadku, gdy liczba nie zostanie podana, używana jest domyślna wartość 5.

## Opis problemu

Problem ucztujących filozofów to klasyczny problem synchronizacji w programowaniu współbieżnym. Przedstawia pięciu
(lub w przypadku tego programu >= 1) filozofów siedzących przy okrągłym stole, którzy na zmianę jedzą i rozmyślają. Każdy 
z nich ma przed sobą talerz i współdzieli widelec z sąsiadem. Aby jeść, filozof musi zdobyć dwa widelce – lewy i prawy. 
Problem polega na tym, jak zapobiec zakleszczeniom (deadlock), zagłodzeniu (starvation) oraz zapewnić efektywny dostęp 
do zasobów (widelców), aby wszyscy filozofowie mogli jeść i myśleć bez konfliktów.

## Wątki

Ze względu na specyfikacje zadania, problem może posiadać różną ilość wątków. Każdemu z filozofów zostaje przydzielony
1 wątek w którym wykonywane są funkcje odpowiadające czynnościom (myślenie, podnoszenie widelców, jedzenie oraz odkładanie
widelców), oraz wątek który zajmuje się wyświetlaniem aktualnych stanów filozofów (czy myślą, są głodni lub jedzą) oraz
ile sekund temu nastąpiła ostatnia zmiana stanu.

## Sekcje krytyczne









