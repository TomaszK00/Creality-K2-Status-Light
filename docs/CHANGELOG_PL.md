# Historia zmian — LayerGlow

## v2.27 — wersja testowa (23 października 2026)

- Rozdzielono okna Sieć i Drukarka. Jeden przycisk zapisuje ustawienia Wi-Fi i rozpoczyna połączenie.
- Dodano bieżący odczyt stanu drukarki, postępu i nazwy pliku, rozłączanie oraz wybór jednej z wielu wykrytych drukarek.
- Wyszukiwanie sprawdza także porty Moonraker 7125 i 80 w lokalnej podsieci oraz wyświetla procent od 0 do 100.
- W mobilnym wyborze języka przywrócono flagi; panel nosi nazwę LayerGlow.
- Kompilacja GitHub Actions zakończona pomyślnie; OTA i wykrywanie wymagają testu na urządzeniu.

Poniższe opisy pochodzą z instrukcji README dołączonych do oryginalnych paczek oraz z historii projektu. Wyjaśniają, **po co przygotowano kolejną wersję**. Nie oznacza to, że każda została ponownie przetestowana na fizycznej drukarce i module ESP32.

**Wersja ogłaszana przez GitHub OTA: 2.15.** Wersja **2.16** jest zachowana w archiwum, ale nie została ustawiona jako aktualna w `Software/update.json`.

| Wersja | Dlaczego powstała |
| --- | --- |
| **2.16** | Poprawa skanowania Wi-Fi w trybie AP `K2-LED-Setup`, blokada prób ponownego łączenia na czas skanowania, rozróżnianie błędu od pustej listy sieci. Dodana **wewnętrzna kopia ustawień LED** i przyciski zapisu/przywracania. |
| **2.15** | Rozwiązanie problemów z odzyskiwaniem połączenia Wi-Fi: automatyczne uruchamianie AP po ok. 8 sekundach, ponowne próby łączenia, reset modułu radiowego bez utraty konfiguracji, automatyczne zamykanie AP po powrocie domowej sieci. |
| **2.14** | Czerwone tło przycisku **Aktualizacja** i biały tekst po wykryciu nowej wersji. Nowe efekty LED: kometa, wędrujące wypełnienie i naprzemienne pary. |
| **2.13** | Nowy efekt **Losowe pary** — niezależne włączanie i wyłączanie par LED w losowych, szybkich odstępach, również w podglądzie i teście. |
| **2.12** | Zmieniony sygnał niebieskiej diody informujący o aktualizacji: **dwa długie błyski, a następnie jeden krótki**. Uzupełniono FAQ. |
| **2.11** | Możliwość wyłączenia świecenia osobno dla każdego etapu; automatyczny zapis ustawień, usunięcie zbędnego efektu „boki–środek–boki” i poprawa układu na telefonach. |
| **2.10** | Naprawa wykrywania aktualizacji, gdy na GitHubie brakowało pliku `update.json`. Dodano alternatywne sprawdzanie plików BIN oraz sygnały stanu Wi-Fi/aktualizacji niebieską diodą. |
| **2.9** | Uporządkowanie panelu aktualizacji GitHub i osobne okno **Aktualizacja**; poprawki wyglądu nagłówka, stopki i podglądu LED. |
| **2.8** | Rozwinięcie systemu aktualizacji GitHub przez manifest z numerem wersji i SHA-256, rozwój efektów LED oraz tłumaczeń interfejsu. |
| **2.7** | Automatyczne odświeżenie panelu po OTA, trwałe komunikaty o braku połączenia oraz nowe efekty od środka, od boków i fali. |
| **2.6** | Przebudowa wyglądu strony i ustawień dla poszczególnych stanów drukarki; efekt **Knight Rider** i powiadomienia w prawym dolnym rogu. |
| **2.5** | Oddzielne pulsowanie dla stanów drukarki, automatyczne odtwarzanie postępu w teście, pełne resetowanie konfiguracji LED oraz czytelniejsze komunikaty podczas OTA. |
| **2.4** | Automatyczne zapisywanie kolorów i ustawień LED, przywracanie domyślnych kolorów oraz instrukcja zmiany domowej sieci Wi-Fi. |
| **2.3** | Wybór 12/24 diod, obsługa pulsowania i zapętlonych testów, status niebieskiej diody, cztery języki oraz osobny obraz USB do pierwszej instalacji. |
| **2.2** | Stara paczka ZIP istnieje w spisie plików, ale nie udało się uzyskać jej oryginalnej zawartości. Nie publikuję zmyślonej listy zmian. |
| **2.1** | Skanowanie sieci Wi-Fi w panelu, podstawowe EN/PL, aktualizacja .bin przez WWW oraz podgląd i test diod. |
| **2.0** | Pierwszy prototyp ESP32-C3 z panelem WWW i odczytem postępu z Moonraker, jeszcze **bez fizycznego paska LED**. |

## Gdzie są pliki?

- [Kod źródłowy Arduino](../Arduino/README.md) — `.ino` dla wersji 2.0, 2.1, 2.3–2.16.
- [Oprogramowanie archiwalne](../Software/archive/README.md) — pozostałe pliki `.bin`, w tym 2.3 USB.
- [Aktualny manifest OTA](../Software/update.json) — nadal wskazuje 2.15.
- [Główna strona projektu](../README.md).

## Ważne

Archiwum nie jest automatyczną listą aktualizacji. Stare pliki `_OTA.bin` wymagają zgodnego układu partycji i zainstalowanego programu z funkcją OTA. Pierwsze wgrywanie przez USB opisano w [instrukcji instalacji](INSTALLATION.md).

Nie każdy kod historyczny ma takie same opcje i zabezpieczenia. Nigdy nie udostępniaj panelu ESP32 bezpośrednio w Internecie. Archiwalny firmware może mieć domyślne hasło AP i panel bez uwierzytelniania.
