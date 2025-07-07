#pragma once         // Dyrektywa kompilatora zapobiegaj¹ca wielokrotnemu do³¹czaniu tego samego nag³ówka
#ifndef LICZENIE_SLOW_H  // Tradycyjny sposób zabezpieczenia przed wielokrotnym include (tzw. include guards)
#define LICZENIE_SLOW_H

#include <string>    // Biblioteka do obs³ugi ci¹gów znaków (std::string)
#include <vector>    // Kontener wektor do przechowywania dynamicznych tablic
#include <cstddef>   // Definicje standardowe (np. size_t)

// Struktura przechowuj¹ca wyniki wraz z metrykami wydajnoœciowymi
struct Metrics {
    int count;         // Liczba znalezionych wyst¹pieñ s³owa
    double czas;       // Ca³kowity czas wykonania w sekundach (czas - pol. "time")
    double cpu_usage;  // Procentowe u¿ycie procesora podczas wykonania (0-100%)
    size_t ram_usage;  // Zu¿yta pamiêæ operacyjna w bajtach (ró¿nica przed/po wykonaniu)
};

// Deklaracje funkcji:

// Wersja sekwencyjna - zlicza s³owa w pojedynczym w¹tku
Metrics liczba_slow_sekwencyjny(
    const std::string& sciezka_pliku,  // Œcie¿ka do analizowanego pliku
    const std::string& slowo           // Szukane s³owo
);

// Wersja wielow¹tkowa z u¿yciem std::thread
Metrics liczba_slow_Thread(
    const std::string& sciezka_pliku,  // Œcie¿ka do pliku
    const std::string& slowo,          // Szukane s³owo
    int liczba_watkow                  // Liczba w¹tków do uruchomienia
);

// Wersja równoleg³a z u¿yciem OpenMP
Metrics liczba_slow_OpenMP(
    const std::string& sciezka_pliku,  // Œcie¿ka do pliku
    const std::string& slowo,          // Szukane s³owo
    int liczba_watkow                  // Liczba w¹tków do uruchomienia
);

// G³ówna funkcja interfejsu u¿ytkownika
void liczenie_slow();  // Funkcja inicjuj¹ca proces zliczania i wyœwietlaj¹ca wyniki

#endif // LICZENIE_SLOW_H