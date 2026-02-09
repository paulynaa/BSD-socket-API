# BSD-socket-API

C programa, kuri naudoja lizdus (sockets), vienu metu aptarnauja kelis klientus ir naudoja įvairias su lizdais susijusias funkcijas.

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
```
## Serverio logika

- `socket()` – sukuriamas klausymo lizdas  
- `bind()` – pririšimas prie porto  
- `listen()` – įjungiamas pasyvus klausymas  
- `select()` – kelių klientų aptarnavimas be blokavimo  
- `accept()` – naujų klientų priėmimas  
- `recv()` – žinučių gavimas  
- `send()` – žinučių siuntimas (echo)  
- `close()` – lizdo uždarymas, kai klientas atsijungia  

## Kliento logika

- `socket()` – sukuriamas lizdas  
- `connect()` – jungiamasi prie serverio  
- `select()` – vienu metu skaito vartotojo įvestį ir priima žinutes iš serverio  
- Žinutės rašomos vartotojo ir siunčiamos serveriui, kuris jas gražina atgal (echo)  
- `shutdown()` ir `close()` – tvarkingas uždarymas  

## Peer-to-Peer mazgo funkcionalumas

### Paleidimas su klausymo portu:

```bash
./p2p <port>
```
 ### Prisijungimas prie kitų mazgų:
 
```bash
connect <IP> <port>
```

## Naudojama `select()` valdyti:

- `stdin` – vartotojo komandoms  
- klausymo socketą – naujiems peer prisijungimams  
- esamus socketus – žinutėms iš kitų mazgų  

Žinutės perduodamos broadcast principu visiems peer’ams.

## Komandos:

- `quit` / `exit` – išeiti iš programos  
- Bet kokia kita tekstinė žinutė siunčiama visiems peer’ams  

Visur naudojamos `htonl`, `htons`, `ntohl`, `ntohs` funkcijos baitų tvarkai konvertuoti.

## Atliktos užduoties paaiškinimas

### Kliento-serverio architektūra

**Serveris:**

- Laukia jungčių  
- Aptarnauja neribotą klientų skaičių  
- Spausdina ir grąžina žinutes klientams  

**Klientas:**

- Jungiasi prie serverio  
- Siunčia žinutes  
- Laukia atsakymų  

### Peer-to-Peer architektūra

- Kiekvienas mazgas veikia kaip serveris ir klientas vienu metu  
- Turi klausymo lizdą ir gali jungtis prie kitų peer’ų  
- Gali priimti naujas jungtis, siųsti žinutes, retransliuoti jas ir dinamiškai keisti aktyvių peer’ų sąrašą  
- Kodo logika išsamiai dokumentuota komentaruose  

## Darbo rezultatai

Visos programos sėkmingai kompiliuojamos su komanda:

```bash
make all
```
<img width="968" height="237" alt="image" src="https://github.com/user-attachments/assets/6cf3cf7f-5341-4550-961f-dc648b4a084b" />

Testavimas

Serveris vienu metu priima ir aptarnauja kelis klientus neblokuodamas jų tarpusavyje.
<img width="968" height="391" alt="image" src="https://github.com/user-attachments/assets/9c72db02-cef4-4466-b61d-b53e09b14c0a" />

Žinutės siunčiamos ir gaunamos iš kelių klientų vienu metu.

Peer-to-Peer tinkle mazgai veikia kaip ir serveriai, ir klientai, keičiasi žinutėmis broadcast principu.

<img width="1245" height="233" alt="image" src="https://github.com/user-attachments/assets/5c205e92-b019-4824-9f31-70e57e52f38c" />

