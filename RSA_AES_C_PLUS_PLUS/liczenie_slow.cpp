// Sekcja includ�w - do��czanie niezb�dnych bibliotek
#include "liczenie_slow.h"  // Nasz w�asny nag��wek z deklaracjami funkcji i struktur
#include <iostream>         // Operacje wej�cia/wyj�cia (cout, cin)
#include <fstream>          // Operacje na plikach (ifstream)
#include <vector>           // Kontener vector do przechowywania fragment�w
#include <string>           // Klasa string do obs�ugi napis�w
#include <thread>           // Biblioteka w�tk�w (std::thread)
#include <chrono>           // Pomiar czasu (high_resolution_clock)
#include <omp.h>            // Biblioteka OpenMP do r�wnoleg�o�ci
#include <string_view>      // Lekka reprezentacja fragment�w string�w
#include <windows.h>        // Funkcje specyficzne dla Windows
#include <psapi.h>          // API do monitorowania zasob�w systemowych

#pragma comment(lib, "psapi.lib")  // Linkowanie z bibliotek� psapi

// Sta�e kontroluj�ce prac� programu
constexpr size_t ROZMIAR_FRAGMENTU = 2 * 1024 * 1024; // Rozmiar fragmentu pliku (2MB)
constexpr size_t OVERLAP_SIZE = 256;  // Rozmiar nak�adki mi�dzy fragmentami (zapobiega przeoczeniu s��w na granicach)

// Struktura przechowuj�ca preprocesowane informacje dla algorytmu KMP
struct KMP_Preprocessed {
    std::vector<int> lps;    // Tablica najd�u�szych prefiks�w-sufiks�w (Longest Prefix Suffix)
    std::string pattern;     // Szukany wzorzec (s�owo)
};

// Funkcje pomocnicze do monitorowania zasob�w (Windows-specific)
// =============================================================

// Zwraca czas CPU zu�yty przez proces w sekundach
static double get_cpu_time() {
    FILETIME createTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER user, kernel;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        return (user.QuadPart + kernel.QuadPart) * 1e-7; // Konwersja 100-nanosekundowych jednostek na sekundy
    }
    return 0.0;
}

// Zwraca zu�ycie pami�ci prywatnej procesu w bajtach
static size_t get_memory_usage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return pmc.PrivateUsage;  // Pami�� prywatna alokowana przez proces
    }
    return 0;
}


// Funkcje pomocnicze do przetwarzania plik�w
// ==========================================

// Odczytuje ca�� zawarto�� pliku do pojedynczego stringa
std::string odczytaj_caly_plik(const std::string& sciezka) {
    std::ifstream file(sciezka, std::ios::binary | std::ios::ate);  // Otw�rz plik od ko�ca
    if (!file) return "";  // B��d otwarcia pliku

    std::streamsize size = file.tellg();  // Pobierz rozmiar pliku
    file.seekg(0, std::ios::beg);         // Przewi� na pocz�tek

    std::string buffer(size, '\0');       // Alokuj bufor na ca�y plik
    file.read(buffer.data(), size);       // Wczytaj ca�� zawarto��
    return buffer;
}

// Dzieli bufor na fragmenty z nak�adkami (overlap)
std::vector<std::string_view> podziel_na_fragmenty(const std::string& buffer) {
    std::vector<std::string_view> fragmenty;
    size_t poz = 0;
    while (poz < buffer.size()) {
        // Oblicz zakres fragmentu z uwzgl�dnieniem nak�adki
        size_t start = (poz > OVERLAP_SIZE) ? (poz - OVERLAP_SIZE) : 0;
        size_t end = min(poz + ROZMIAR_FRAGMENTU, buffer.size());
        fragmenty.emplace_back(buffer.data() + start, end - start);// Dodaj widok fragmentu (bez kopiowania danych)
        poz = end;// Przesu� pozycj� ko�ca fragmentu
    }
    return fragmenty;
}

// Implementacja algorytmu KMP (Knuth-Morris-Pratt)
// ================================================

// Przygotowuje struktur� KMP dla zadanego wzorca

KMP_Preprocessed przygotuj_wzorzec(const std::string& slowo) {
    KMP_Preprocessed result;
    result.pattern = slowo;
    size_t m = slowo.size();
    result.lps.resize(m, 0);// Inicjalizacja tablicy LPS zerami

    // Budowa tablicy LPS
    for (size_t i = 1, len = 0; i < m;) {
        if (slowo[i] == slowo[len]) {
            result.lps[i++] = ++len;// Dopasowanie - zwi�ksz d�ugo�� prefiksu
        }
        else {
            if (len != 0) len = result.lps[len - 1];// Cofnij si� w tablicy LPS
            else result.lps[i++] = 0;// Brak dopasowania - zerujemy
        }
    }
    return result;
}

// Zlicza wyst�pienia wzorca we fragmencie tekstu u�ywaj�c KMP
int liczba_slow_we_fragmencie(std::string_view fragment, const KMP_Preprocessed& wzorzec) {
    if (wzorzec.pattern.empty()) return 0;  // Zabezpieczenie przed pustym wzorcem

    int count = 0;  // Licznik wyst�pie�
    const size_t m = wzorzec.pattern.size();
    const size_t n = fragment.size();
    size_t i = 0, j = 0;  // Indeksy dla tekstu (i) i wzorca (j)

    while (i < n) {
        if (wzorzec.pattern[j] == fragment[i]) {// Dopasowanie znak�w
            ++i;
            ++j;
        }

        if (j == m) {// Ca�e s�owo dopasowane
            ++count;
            j = wzorzec.lps[j - 1];// Kontynuuj szukanie od pozycji w LPS
        }
        else if (i < n && wzorzec.pattern[j] != fragment[i]) {
            // Brak dopasowania - wykorzystaj tablic� LPS do optymalnego przesuni�cia
            j ? j = wzorzec.lps[j - 1] : ++i;
        }
    }
    return count;
}

// Implementacje r�nych wersji zliczania
// ======================================

// Wersja sekwencyjna
Metrics liczba_slow_sekwencyjny(const std::string& sciezka_pliku, const std::string& slowo) {
    // Rozpocznij pomiary
    auto start_cpu = get_cpu_time();
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t start_mem = get_memory_usage();

    // G��wne przetwarzanie
    std::string buffer = odczytaj_caly_plik(sciezka_pliku);
    auto fragmenty = podziel_na_fragmenty(buffer);
    auto wzorzec = przygotuj_wzorzec(slowo);

    int total = 0;
    for (const auto& fragment : fragmenty) {// Przetwarzaj fragment po fragmencie
        total += liczba_slow_we_fragmencie(fragment, wzorzec);
    }

    // Zako�cz pomiary
    auto end_time = std::chrono::high_resolution_clock::now();
    auto end_cpu = get_cpu_time();
    size_t end_mem = get_memory_usage();

    // Oblicz metryki
    double czas = std::chrono::duration<double>(end_time - start_time).count();
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int num_cpus = sysInfo.dwNumberOfProcessors;  // Pobierz liczb� rdzeni

    double cpu_usage = (end_cpu - start_cpu) / (czas * num_cpus) * 100.0;  // Uwzgl�dnij liczb� rdzeni
    size_t ram_usage = end_mem - start_mem;

    return { total, czas, cpu_usage, ram_usage };
}

// Wersja z u�yciem std::thread
Metrics liczba_slow_Thread(const std::string& sciezka_pliku, const std::string& slowo, int liczba_watkow) {
    // Rozpocznij pomiary
    auto start_cpu = get_cpu_time();
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t start_mem = get_memory_usage();

    // Przygotuj dane
    std::string buffer = odczytaj_caly_plik(sciezka_pliku);
    auto fragmenty = podziel_na_fragmenty(buffer);
    auto wzorzec = przygotuj_wzorzec(slowo);

    // Inicjalizacja struktur dla w�tk�w
    std::vector<std::thread> watki;
    std::vector<int> wyniki(liczba_watkow, 0);// Wyniki cz�stkowe
    const size_t frag_na_watek = (fragmenty.size() + liczba_watkow - 1) / liczba_watkow;// Podzia� pracy

    // Funkcja robocza dla w�tk�w
    auto worker = [&](int id) {
        size_t start = id * frag_na_watek;
        size_t end = min(start + frag_na_watek, fragmenty.size());
        int local_count = 0;

        for (size_t i = start; i < end; ++i) {// Przetwarzaj przypisane fragmenty
            local_count += liczba_slow_we_fragmencie(fragmenty[i], wzorzec);
        }
        wyniki[id] = local_count;// Zapisz wynik cz�stkowy
        };

    // Uruchom w�tki
    for (int i = 0; i < liczba_watkow; ++i) {
        watki.emplace_back(worker, i);  // Ka�dy w�tek dostaje sw�j ID
    }

    // Czekaj na zako�czenie w�tk�w
    for (auto& t : watki) {
        t.join();
    }

    // Sumuj wyniki
    int total = 0;
    for (int x : wyniki) {
        total += x;
    }

    // Zako�cz pomiary
    auto end_time = std::chrono::high_resolution_clock::now();
    auto end_cpu = get_cpu_time();
    size_t end_mem = get_memory_usage();

    // Oblicz metryki
    double czas = std::chrono::duration<double>(end_time - start_time).count();

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int num_cpus = sysInfo.dwNumberOfProcessors;  // Pobierz liczb� rdzeni

    double cpu_usage = (end_cpu - start_cpu) / (czas * num_cpus) * 100.0;  // Uwzgl�dnij liczb� rdzeni
    size_t ram_usage = end_mem - start_mem;

    return { total, czas, cpu_usage, ram_usage };
}

// Wersja z u�yciem OpenMP
Metrics liczba_slow_OpenMP(const std::string& sciezka_pliku, const std::string& slowo, int liczba_watkow) {
    // Rozpocznij pomiary
    auto start_cpu = get_cpu_time();
    auto start_time = std::chrono::high_resolution_clock::now();
    size_t start_mem = get_memory_usage();

    // Przygotuj dane
    std::string buffer = odczytaj_caly_plik(sciezka_pliku);
    auto fragmenty = podziel_na_fragmenty(buffer);
    auto wzorzec = przygotuj_wzorzec(slowo);

    int total = 0;
    omp_set_num_threads(liczba_watkow);// Ustaw liczb� w�tk�w

    // R�wnoleg�a p�tla z redukcj� wyniku
#pragma omp parallel for reduction(+:total) schedule(static, 10)
    for (int i = 0; i < static_cast<int>(fragmenty.size()); ++i) {
        total += liczba_slow_we_fragmencie(fragmenty[i], wzorzec);
    }

    // Zako�cz pomiary
    auto end_time = std::chrono::high_resolution_clock::now();
    auto end_cpu = get_cpu_time();
    size_t end_mem = get_memory_usage();

    // Oblicz metryki
    double czas = std::chrono::duration<double>(end_time - start_time).count();

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int num_cpus = sysInfo.dwNumberOfProcessors;  // Pobierz liczb� rdzeni

    double cpu_usage = (end_cpu - start_cpu) / (czas * num_cpus) * 100.0;  // Uwzgl�dnij liczb� rdzeni
    size_t ram_usage = end_mem - start_mem;

    return { total, czas, cpu_usage, ram_usage };
}

// G��wna funkcja interfejsu u�ytkownika
// =====================================

void liczenie_slow() {
    // Ustawienie kodowania UTF-8 dla konsoli
    system("chcp 65001");

    // Pobierz dane od u�ytkownika
    std::string nazwa_uzytkownika;
    std::cout << "Podaj nazwe uzytkownika: ";
    std::cin >> nazwa_uzytkownika;

    int liczba_watkow;
    std::cout << "Podaj ilosc watkow: ";
    std::cin >> liczba_watkow;

    int liczba_slow;
    std::cout << "Podaj ilosc slow do sprawdzenia: ";
    std::cin >> liczba_slow;

    std::vector<std::string> slowa(liczba_slow);
    for (int i = 0; i < liczba_slow; ++i) {
        std::cout << "Podaj " << i + 1 << ". slowo: ";
        std::cin >> slowa[i];
    }

    // Skonstruuj pe�n� �cie�k� do pliku
    std::string sciezka_pliku;
    std::cout << "Podaj sciezke do pliku: ";
    std::cin >> sciezka_pliku;
    sciezka_pliku = "C:\\Users\\" + nazwa_uzytkownika + "\\Desktop\\" + sciezka_pliku;

    // Zmienne do podsumowania
    int total_seq = 0, total_thr = 0, total_omp = 0;
    double time_seq = 0, time_thr = 0, time_omp = 0;
    double cpu_seq = 0, cpu_thr = 0, cpu_omp = 0;
    size_t ram_seq = 0, ram_thr = 0, ram_omp = 0;

    for (const auto& slowo : slowa) {
        // Wywo�aj wszystkie implementacje
        auto result_seq = liczba_slow_sekwencyjny(sciezka_pliku, slowo);
        auto result_thr = liczba_slow_Thread(sciezka_pliku, slowo, liczba_watkow);
        auto result_omp = liczba_slow_OpenMP(sciezka_pliku, slowo, liczba_watkow);

        // Wy�wietl wyniki dla bie��cego s�owa
        std::cout << "\nSlowo: " << slowo
            << "\nSekwencyjnie: " << result_seq.count << " (czas: " << result_seq.czas << "s, CPU: " << result_seq.cpu_usage << "%, RAM: " << result_seq.ram_usage << " B)"
            << "\nThreading: " << result_thr.count << " (czas: " << result_thr.czas << "s, CPU: " << result_thr.cpu_usage << "%, RAM: " << result_thr.ram_usage << " B)"
            << "\nOpenMP: " << result_omp.count << " (czas: " << result_omp.czas << "s, CPU: " << result_omp.cpu_usage << "%, RAM: " << result_omp.ram_usage << " B)\n";

        // Aktualizuj statystyki podsumowuj�ce
        total_seq += result_seq.count;
        total_thr += result_thr.count;
        total_omp += result_omp.count;

        time_seq += result_seq.czas;
        time_thr += result_thr.czas;
        time_omp += result_omp.czas;

        cpu_seq += result_seq.cpu_usage;
        cpu_thr += result_thr.cpu_usage;
        cpu_omp += result_omp.cpu_usage;

        ram_seq += result_seq.ram_usage;
        ram_thr += result_thr.ram_usage;
        ram_omp += result_omp.ram_usage;
    }
    // Wy�wietl podsumowanie globalne
    std::cout << "\nPodsumowanie:"
        << "\nSekwencyjnie: " << total_seq << " (czas: " << time_seq << "s, �rednie CPU: " << cpu_seq / slowa.size() << "%, RAM: " << ram_seq << " B)"
        << "\nThreading: " << total_thr << " (czas: " << time_thr << "s, �rednie CPU: " << cpu_thr / slowa.size() << "%, RAM: " << ram_thr << " B)"
        << "\nOpenMP: " << total_omp << " (czas: " << time_omp << "s, �rednie CPU: " << cpu_omp / slowa.size() << "%, RAM: " << ram_omp << " B)\n";
}
