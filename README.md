
# ChatServer

Prosty wielowątkowy komunikator tekstowy napisany w C++ z wykorzystaniem gniazd TCP.

Aplikacja składa się z serwera obsługującego wielu klientów oraz klienta terminalowego.

### Funkcjonalności

- Komunikacja tekstowa między użytkownikami.
- Tworzenie pokojów i dołączanie do nich.
- Wiadomości prywatne.
- Lista aktywnych użytkowników i pokojów.
- Statusy użytkowników.
- Moderacja pokojów (KICK, MUTE).

### Kompilacja

Projekt przeznaczony jest do uruchamiania w środowisku Linux (również WSL).

```bash
cmake -S . -B build
cmake --build build -j
```

### Uruchomienie

**Terminal 1 — serwer:**

```bash
./build/server --host 127.0.0.1 --port 5555
```

**Terminal 2 — klient:**

```bash
./build/client --host 127.0.0.1 --port 5555
```

Aby połączyć kolejnego użytkownika, należy uruchomić klienta w osobnym terminalu. Po połączeniu program poprosi o podanie nicku.

### Przykładowe komendy

| Komenda                 | Działanie                  |
| ----------------------- | --------------------------- |
| `/JOIN #test`         | Dołączenie do pokoju      |
| `/MSG #test Cześć!` | Wiadomość do pokoju       |
| `/PRIV Jan Cześć!`  | Wiadomość prywatna        |
| `/LIST ROOMS`         | Lista pokojów              |
| `/LIST USERS ONLINE`  | Lista użytkowników online |
| `/WHOAMI`             | Informacje o użytkowniku   |
| `/STATUS AWAY`        | Zmiana statusu              |
| `/QUIT`               | Rozłączenie               |

### Informacja o projekcie

Projekt został opracowany pierwotnie w środowisku Replit, a następnie przeniesiony na GitHub. Z tego powodu repozytorium nie zawiera oryginalnej historii rozwoju projektu.
