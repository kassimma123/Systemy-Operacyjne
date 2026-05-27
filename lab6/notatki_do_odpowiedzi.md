# Notatki do odpowiedzi (Lab 6 - Wątki i Synchronizacja)

## 1. Wątki (Podstawy)
- **Co to jest wątek?** Działa w przestrzeni adresowej procesu, wykonuje się współbieżnie. Każdy wątek ma swój **odrębny stos**.
- **Co wątki współdzielą?**
  - Przestrzeń adresową (zmienne globalne!).
  - PID, UID, PPID.
  - Deskryptory plików.
  - Sposób obsługi sygnałów.
- **Czego wątki NIE współdzielą (mają własne)?**
  - TID (Thread ID).
  - Maskę sygnałów.
  - Wartość `errno`.
  - **Stos** (zmienne lokalne w funkcjach).
- **Zakończenie procesu a wątki:** Jeśli wątek główny (`main`) zwróci wartość, kończą się wszystkie wątki. Podobnie po wywołaniu `exit`.
- **Zakończenie samego wątku:** Funkcja `pthread_exit()` lub po prostu powrót (return) z funkcji wątku.

## 2. Tworzenie i obsługa wątków
- **Tworzenie:** `pthread_create(thread, attr, start_routine, arg)`
  - Uruchamia nową funkcję (`start_routine`) przekazując jej wskaźnik `arg`.
  - **UWAGA:** Nie przekazujemy adresów zmiennych lokalnych z wątku tworzącego, jeśli mogą one zniknąć (zostać usunięte ze stosu) przed startem nowego wątku.
- **Pobieranie własnego ID:** `pthread_self()`
- **Czekanie na zakończenie wątku:** `pthread_join(thread, &retval)` (usypia wątek wywołujący do czasu zakończenia wskazanego wątku i pozwala odebrać to, co ten wątek zwrócił).
- **Anulowanie wątku:** `pthread_cancel(tid)` - pozwala innemu wątkowi zatrzymać działanie danego wątku (zależnie od jego ustawień).
- **Wątek odłączony (detached):** Można go stworzyć od razu odłączonego (przez atrybuty `PTHREAD_CREATE_DETACHED`) albo odłączyć za pomocą `pthread_detach(tid)`. Z takiego wątku nie da się "dołączyć" przez `pthread_join` (zasoby są zwalniane natychmiast po jego zakończeniu).

## 3. Mutexy (Wzajemne Wykluczanie - Sekcje Krytyczne)
- **Co to?** Blokada, którą w danej chwili może trzymać **tylko jeden wątek**. Służy do bezpiecznego modyfikowania zasobów współdzielonych (zmiennych globalnych).
- **Jak używamy?**
  1. Zablokuj mutex (`pthread_mutex_lock`).
  2. Zmodyfikuj/odczytaj zmienną globalną.
  3. Odblokuj mutex (`pthread_mutex_unlock`).
- **Rodzaje mutexów:**
  - *Fast* (szybki, domyślny) - przy próbie ponownego zajęcia przez ten sam wątek doprowadzi do zakleszczenia (deadlock).
  - *Recursive* (rekursywny) - pozwala temu samemu wątkowi zajmować go wiele razy (wymaga tyle samo odblokowań).
  - *Error-checking* - zgłosi błąd `EDEADLK` zamiast zawieszać program.
- **Zakleszczenie (Deadlock):** Kiedy np. wątek A trzyma Mutex 1 i czeka na Mutex 2, a wątek B trzyma Mutex 2 i czeka na Mutex 1. Albo gdy wątek wchodzi do funkcji obsługi sygnału i próbuje zamknąć mutex, który wcześniej sam zablokował.
- **Ważne:** Nie wolno używać mutexów wewnątrz procedur obsługi sygnałów (signal handlers)!

## 4. Zmienne Warunkowe (Condition Variables)
- **Problem, który rozwiązują:** Aktywne czekanie w pętli (np. `while(x <= y)`) "zżera" procesor i jest nieefektywne.
- **Rozwiązanie:** `pthread_cond_wait(&cond, &mutex)` usypia wątek, zwalniając przy tym zajęty mutex. Pozwala to innemu wątkowi zmienić warunek.
- **Jak się używa (schemat):**
  - **Oczekujący:**
    ```c
    pthread_mutex_lock(&mutex);
    while (warunek_nie_jest_spelniony) { // ZAWSZE pętla while, nie if!
        pthread_cond_wait(&cond, &mutex); // usypia wątek i ZWALNIA mutex. Budząc się, PONOWNIE ZAJMUJE mutex.
    }
    // robimy swoje
    pthread_mutex_unlock(&mutex);
    ```
  - **Budzący:**
    ```c
    pthread_mutex_lock(&mutex);
    // zmieniamy zmienne wpływające na warunek
    pthread_cond_signal(&cond); // lub pthread_cond_broadcast(&cond) jeśli chcemy obudzić wszystkich
    pthread_mutex_unlock(&mutex);
    ```

## 5. Priorytety (Rozszerzenia Real-Time)
- **Protokół dziedziczenia priorytetów (Priority Inheritance):** Gdy wątek o wysokim priorytecie czeka na mutex zajęty przez wątek o niskim priorytecie, wątek o niskim priorytecie chwilowo *dziedziczy* ten wysoki priorytet, żeby jak najszybciej zwolnić mutex (chroni to przed inwersją priorytetów).
- **Protokół ochrony (Priority Protect):** Mutexowi przypisuje się maksymalny próg priorytetu z góry.

## 6. Sygnały w systemach wielowątkowych
- Maska sygnałów jest **indywidualna dla każdego wątku** (ustawiamy przez `pthread_sigmask`).
- Sposób obsługi sygnału (np. handler) jest **współdzielony dla całego procesu**.
- Sygnały sprzętowe (np. dzielenie przez zero, segfault) idą do wątku, który je wywołał. Reszta trafia w miarę losowo do wątku, który ich nie blokuje.
- Aby wysłać sygnał do konkretnego wątku, używamy `pthread_kill(tid, sygnał)`.
