# BSD-socket-API
# Laboratorinio darbo aprašymas

## Darbo tikslas

Sukurti programą C kalba, kuri naudoja lizdus (sockets), vienu metu aptarnauja kelis klientus ir naudoja įvairias su lizdais susijusias funkcijas.

## Reikalavimai

Programa turi naudoti šias BSD socket API funkcijas:  
`select(2)`, `socket(2)`, `send(2)`, `recv(2)`, `close(2)`, `shutdown(2)`, `accept(2)`, `listen(2)`, `bind(2)`, `connect(2)`.

Taip pat tinklo vardų išrišimo funkcijas:  
`hton*`, `ntoh*`.

Programa turi palaikyti kelių klientų aptarnavimą vienu metu be blokavimo.  
Negalima naudoti išorinių bibliotekų – tik BSD socket API funkcijas.

## Architektūra

- Kliento-serverio architektūra, realizuotos atskiros kliento ir serverio programos.
- Peer-to-Peer (P2P) architektūra, kur mazgas vienu metu gali būti ir serveris, ir klientas.

## Darbo eiga

Sukurta direktorija su C programomis ir `Makefile`. Įgyvendintos trys C programos:

* `server.c` – TCP serveris, naudojantis `select()` kelių klientų aptarnavimui.
* `client.c` – TCP klientas, jungiantis prie serverio.
* `p2p.c` – peer-to-peer mazgas, galintis ir klausyti, ir jungtis prie kitų mazgų.

`Makefile` leidžia automatiškai sukompiliuoti visas programas komanda:

```bash
make all
