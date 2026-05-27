# Notatki do odpowiedzi (Lab 7 - Gniazda / Sockets)

## 1. Gniazda (Sockets) - Wprowadzenie
Gniazda to uniwersalny mechanizm komunikacji między procesami, pozwalający wymieniać dane zarówno na jednej maszynie, jak i przez sieć.

- **Dziedziny (Address Families):**
  - `AF_INET` - komunikacja przez sieć (IPv4)
  - `AF_INET6` - komunikacja przez sieć (IPv6)
  - `AF_UNIX` / `AF_LOCAL` - komunikacja lokalnie na jednej maszynie (z wykorzystaniem specjalnego pliku w systemie)

- **Tryby komunikacji:**
  - `SOCK_STREAM` (strumieniowy) - np. TCP. Jest **niezawodny** (obsługuje utratę pakietów), **uporządkowany** i wymaga nawiązania **połączenia**.
  - `SOCK_DGRAM` (datagramowy) - np. UDP. Jest **zawodny** (pakiety mogą zaginąć bez informacji zwrotnej), **nieuporządkowany** i działa **bez połączenia**.

## 2. Ważne struktury adresowe
- Gniazda używają ogólnej struktury `struct sockaddr`. 
- Dla sieci IPv4 (`AF_INET`) używa się rzutowanej struktury `struct sockaddr_in`:
  - `sin_family` - rodzina (AF_INET)
  - `sin_port` - port podany w formacie *network byte order*
  - `sin_addr.s_addr` - adres IP (np. `INADDR_ANY` dla wszystkich interfejsów lub sprecyzowany adres po konwersji).

## 3. Kolejność bajtów (Endianness)
Maszyny różnie zapisują dane w pamięci (Little-endian vs Big-endian), ale protokoły sieciowe zawsze wymagają kolejności **Big-endian (Network Byte Order)**.
Dlatego przed wysłaniem/zbindowaniem wartości portów i adresów musimy użyć:
- `htons()` (Host To Network Short) - dla portów (liczby 16-bitowe).
- `htonl()` (Host To Network Long) - dla adresów IPv4 (liczby 32-bitowe).
Oraz funkcji konwertujących tekstowy adres IP na liczbowy:
- `inet_pton(AF_INET, "127.0.0.1", &struktura)` - Presentation to Network (konwersja ze stringa).
- `inet_ntop()` - Network to Presentation (odwrotnie, z liczby na string).

## 4. Cykl życia gniazda TCP (Połączeniowego)

### SERWER TCP
1. `socket()` - tworzy nowe gniazdo.
2. `bind()` - przypisuje wygenerowane gniazdo do konkretnego portu (np. 9000) i adresu (np. na wszystkich interfejsach).
3. `listen()` - przestawia gniazdo w tryb pasywny, zaczyna "nasłuchiwać" połączeń przychodzących od klientów (kolejka `backlog`).
4. `accept()` - odbiera połączenie z kolejki nasłuchującej. Zwraca **nowy deskryptor** gniazda przypisany specjalnie do tego pojedynczego klienta.
5. `recv()` / `send()` - odczytywanie zapytania i wysyłanie odpowiedzi za pomocą wyżej zwróconego nowego deskryptora.
6. `close()` - zamykanie gniazda klienta po zakończonej pracy.

*Uwaga: Warto użyć flagi `SO_REUSEADDR` przez `setsockopt()`, aby zapobiec błędowi "Address already in use" przy restartowaniu serwera.*

### KLIENT TCP
1. `socket()` - tworzy gniazdo.
2. `connect()` - łączy się aktywnie ze wskazanym IP oraz portem serwera. Po tej operacji gniazdo (w trybie strumieniowym) jest połączone z serwerem i ma automatycznie przydzielony losowy port wychodzący (efemeryczny).
3. `send()` / `recv()` - przesyłanie poleceń do serwera i czekanie na odpowiedź.
4. `close()` - zamknięcie połączenia.

## 5. Przydatne opcje i funkcje
- `recv()` domyślnie blokuje wykonanie do momentu przyjścia jakichkolwiek danych (chyba że podano np. flagę `MSG_DONTWAIT` lub gniazdo ustawiono na non-blocking za pomocą opcji `SOCK_NONBLOCK`).
- Gniazda po `accept()` posiadają status takiego samego zablokowania oraz parametry z bazowego gniazda, które na nich oczekiwało.
- Multipleksowanie: Oczekiwanie na wiadomości z wielu połączonych deskryptorów bez blokowania się realizuje się za pomocą mechanizmów `epoll()`, `poll()` lub najstarszego `select()`. Pozwalają jednemu wątkowi obsługiwać wielu klientów TCP bez problemu oczekiwania w `recv` na jednego z nich.
