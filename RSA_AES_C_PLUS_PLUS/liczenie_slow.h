#pragma once         // Dyrektywa kompilatora zapobiegaj�ca wielokrotnemu do��czaniu tego samego nag��wka
#ifndef LICZENIE_SLOW_H  // Tradycyjny spos�b zabezpieczenia przed wielokrotnym include (tzw. include guards)
#define LICZENIE_SLOW_H

#include <string>    // Biblioteka do obs�ugi ci�g�w znak�w (std::string)
#include <vector>    // Kontener wektor do przechowywania dynamicznych tablic
#include <cstddef>   // Definicje standardowe (np. size_t)

// Struktura przechowuj�ca wyniki wraz z metrykami wydajno�ciowymi
struct Metrics {
    int count;         // Liczba znalezionych wyst�pie� s�owa
    double czas;       // Ca�kowity czas wykonania w sekundach (czas - pol. "time")
    double cpu_usage;  // Procentowe u�ycie procesora podczas wykonania (0-100%)
    size_t ram_usage;  // Zu�yta pami�� operacyjna w bajtach (r�nica przed/po wykonaniu)
};

// Deklaracje funkcji:

// Wersja sekwencyjna - zlicza s�owa w pojedynczym w�tku
Metrics liczba_slow_sekwencyjny(
    const std::string& sciezka_pliku,  // �cie�ka do analizowanego pliku
    const std::string& slowo           // Szukane s�owo
);

// Wersja wielow�tkowa z u�yciem std::thread
Metrics liczba_slow_Thread(
    const std::string& sciezka_pliku,  // �cie�ka do pliku
    const std::string& slowo,          // Szukane s�owo
    int liczba_watkow                  // Liczba w�tk�w do uruchomienia
);

// Wersja r�wnoleg�a z u�yciem OpenMP
Metrics liczba_slow_OpenMP(
    const std::string& sciezka_pliku,  // �cie�ka do pliku
    const std::string& slowo,          // Szukane s�owo
    int liczba_watkow                  // Liczba w�tk�w do uruchomienia
);

// G��wna funkcja interfejsu u�ytkownika
void liczenie_slow();  // Funkcja inicjuj�ca proces zliczania i wy�wietlaj�ca wyniki

#endif // LICZENIE_SLOW_H