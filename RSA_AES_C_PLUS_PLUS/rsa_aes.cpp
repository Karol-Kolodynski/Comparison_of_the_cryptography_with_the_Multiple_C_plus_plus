
#include "cryptlib.h"   // G³ówna biblioteka Crypto++
#include "osrng.h"      // Generator liczb losowych
#include "aes.h"        // Algorytm AES
#include "modes.h"      // Tryby pracy szyfrów blokowych
#include "filters.h"    // Filtry do przetwarzania danych
#include "hex.h"        // Kodowanie heksadecymalne
#include "rsa.h"        // Algorytm RSA
#include "pem.h"        // Obs³uga kluczy PEM
#include <iostream>      // Obs³uga wejœcia i wyjœcia
#include "files.h"      // Obs³uga plików

// Obs³uga konfliktów z definicjami Windows
#ifdef _WIN32
#undef BOOLEAN              // Usuñ konfliktuj¹c¹ definicjê
#endif


#include <windows.h>     // Windows API
#include <psapi.h>       // Monitorowanie zasobów

// Przywróæ oryginaln¹ definicjê BOOLEAN dla Windows
#define BOOLEAN WINBOOL

// Linkowanie z bibliotekami

#pragma comment(lib, "psapi.lib")      // Biblioteka do monitorowania procesów
#pragma comment(lib, "cryptlib.lib")   // Biblioteka Crypto++

// Alias dla przestrzeni nazw CryptoPP
namespace CP = CryptoPP;

using namespace std;


void GetProcessorTimes(FILETIME& userTime, FILETIME& kernelTime);
double FileTimeToSeconds(const FILETIME& ft);
void PrintResourceUsage(double cpuUsage, const PROCESS_MEMORY_COUNTERS& pmc);


// Implementacja AES
// =================

/**
 * Szyfruje i deszyfruje tekst przy u¿yciu AES-CTR
 * @param plaintext Tekst do zaszyfrowania
 * @param keySize Rozmiar klucza (128, 192, 256)
 */
void EncryptDecryptAES(string plaintext, int keySize) {
    CP::AutoSeededRandomPool prng; // Kryptograficzny generator liczb losowych

    // Walidacja rozmiaru klucza
    if (keySize != 128 && keySize != 192 && keySize != 256) {
        cerr << "Nieprawidlowy rozmiar klucza AES." << endl;
        return;
    }
    // Generacja klucza i IV
    int keyLength = keySize / 8; // Konwersja bitów na bajty
    CP::SecByteBlock key(keyLength); // Klucz AES
    CP::SecByteBlock iv(CP::AES::BLOCKSIZE); // Wektor inicjalizuj¹cy (IV)

    prng.GenerateBlock(key, key.size()); // Generowanie losowego klucza AES
    prng.GenerateBlock(iv, iv.size());   // Generowanie losowego IV // Wektor inicjuj¹cy (16 bajtów)

    // Konwersja klucza do hex
    string strKey;
    CP::HexEncoder keyEncoder(new CP::StringSink(strKey));
    keyEncoder.Put(key, key.size());
    keyEncoder.MessageEnd();

    cout << "Klucz AES (szesnastkowo): " << endl << strKey << endl;

    string cipher, recovered;
    cout << "Tekst oryginalny: " << plaintext << endl;

    try {
        // Inicjalizacja szyfrowania AES w trybie CTR
        CP::CTR_Mode<CP::AES>::Encryption e;
        e.SetKeyWithIV(key, key.size(), iv);

        // Szyfrowanie tekstu // Kodowanie tekstu jawnego do hex przed szyfrowaniem
        string encodedPlain;
        CP::StringSource(plaintext, true,
            new CP::HexEncoder(
                new CP::StringSink(encodedPlain)
            )
        );
        // Przeprocesowanie szyfrowania
        CP::StringSource(encodedPlain, true,
            new CP::StreamTransformationFilter(e,
                new CP::StringSink(cipher)
            )
        );

        cout << "Zaszyfrowany tekst (w heksadecymalnym): ";
        CP::StringSource(cipher, true,
            new CP::HexEncoder(
                new CP::FileSink(cout)
            )
        );
        cout << endl;

        // Deszyfrowanie
        CP::CTR_Mode<CP::AES>::Decryption d;
        d.SetKeyWithIV(key, key.size(), iv);
        CP::StringSource(cipher, true,
            new CP::StreamTransformationFilter(d,
                new CP::StringSink(recovered)
            )
        );
        // Dekodowanie z hex
        string decodedRecovered;
        CP::StringSource(recovered, true,
            new CP::HexDecoder(
                new CP::StringSink(decodedRecovered)
            )
        );
        // Wyœwietl IV
        string strIV;
        CP::HexEncoder ivEncoder(new CP::StringSink(strIV));
        ivEncoder.Put(iv, iv.size());
        ivEncoder.MessageEnd();

        cout << "Wektor inicjujacy (IV) (szesnastkowo): " << endl << strIV << endl;
        cout << "Tekst odszyfrowany: " << decodedRecovered << endl;
        // Weryfikacja poprawnoœci
        if (plaintext != decodedRecovered) {
            cout << "Blad: Tekst odszyfrowany rozni sie od oryginalnego!" << endl;
        }
    }
    catch (const CP::Exception& e) {
        cerr << "CryptoPP blad: " << e.what() << endl;
        exit(1);
    }
}
// Implementacja RSA
// =================

/**
 * Szyfruje i deszyfruje tekst przy u¿yciu RSA-OAEP
 * @param plaintext Tekst do zaszyfrowania
 * @param bytes D³ugoœæ klucza w bitach (512-4096)
 */
void EncryptDecryptRSA(string plaintext, int bytes) {
    CP::AutoSeededRandomPool rng;// Generator kryptograficzny
    
    // Walidacja d³ugoœci klucza
    if (bytes < 512 || bytes > 4096) {
        cerr << "Nieprawidlowa dlugosc klucza RSA" << endl;
        return;
    }

    // Generacja pary kluczy
    CP::InvertibleRSAFunction params;
    params.GenerateRandomWithKeySize(rng, bytes);  // Generuj parametry RSA

    CP::RSA::PrivateKey privateKey(params);  // Klucz prywatny
    CP::RSA::PublicKey publicKey(params);     // Klucz publiczny
    
    // Eksport kluczy do hex
    string strPrivateKey, strPublicKey;
    CP::HexEncoder privateKeyEncoder(new CP::StringSink(strPrivateKey));
    CP::HexEncoder publicKeyEncoder(new CP::StringSink(strPublicKey));

    privateKey.Save(privateKeyEncoder);
    publicKey.Save(publicKeyEncoder);

    cout << "Prywatny klucz RSA (szesnastkowo): " << endl << strPrivateKey << endl;
    cout << "Publiczny klucz RSA (szesnastkowo): " << endl << strPublicKey << endl;

    string encrypted, decrypted;
    cout << "Tekst oryginalny: " << plaintext << endl;

    try {
        // Szyfrowanie z OAEP-SHA
        CP::RSAES_OAEP_SHA_Encryptor e(publicKey);
        CP::StringSource(plaintext, true,
            new CP::PK_EncryptorFilter(rng, e,
                new CP::StringSink(encrypted)
            )
        );
        // Konwersja zaszyfrowanych danych na hex
        string encoded;
        CP::HexEncoder encoder(new CP::StringSink(encoded));
        encoder.Put(reinterpret_cast<const CP::byte*>(encrypted.data()), encrypted.size());
        encoder.MessageEnd();

        cout << "Zaszyfrowany tekst (w heksadecymalnym): " << encoded << endl;
        // Deszyfrowanie
        CP::RSAES_OAEP_SHA_Decryptor d(privateKey);
        CP::StringSource(encoded, true,
            new CP::HexDecoder(
                new CP::PK_DecryptorFilter(rng, d,
                    new CP::StringSink(decrypted)
                )
            )
        );

        cout << "Tekst odszyfrowany: " << decrypted << endl;

        if (plaintext != decrypted) {
            cout << "Blad: Tekst odszyfrowany rozni sie od oryginalnego!" << endl;
        }
    }
    catch (const CP::Exception& e) {
        cerr << "CryptoPP blad: " << e.what() << endl;
        exit(1);
    }
}
// Generacja losowego tekstu
// =========================

/**
 * Generuje losowy tekst o zadanej d³ugoœci
 * @param min_length Minimalna d³ugoœæ
 * @param max_length Maksymalna d³ugoœæ
 * @return Losowy ci¹g znaków
 */
string generateRandomText(int min_length, int max_length) {
    const string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()";
    const int charactersLength = characters.length();

    int length = rand() % (max_length - min_length + 1) + min_length;
    string randomText;

    for (int i = 0; i < length; ++i) {
        randomText += characters[rand() % charactersLength];
    }
    return randomText;
}

// Deklaracje funkcji pomocniczych
// ===============================

/**
 * Pobiera czas procesora zu¿yty przez proces
 * @param userTime[out] Czas w trybie u¿ytkownika
 * @param kernelTime[out] Czas w trybie j¹dra
 */
void GetProcessorTimes(FILETIME& userTime, FILETIME& kernelTime) {
    FILETIME creationTime, exitTime;
    GetProcessTimes(
        GetCurrentProcess(),   // Obecny proces
        &creationTime,         // Czas utworzenia (ignorowany)
        &exitTime,             // Czas zakoñczenia (ignorowany)
        &kernelTime,           // Czas w kernel mode
        &userTime              // Czas w user mode
    );
}

double FileTimeToSeconds(const FILETIME& ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;   // Dolne 32 bity
    uli.HighPart = ft.dwHighDateTime;  // Górne 32 bity
    return uli.QuadPart / 1e7;        // 100-ns -> sekundy
}

/**
 * Wyœwietla zu¿ycie zasobów
 * @param cpuUsage Procentowe u¿ycie CPU
 * @param pmc Struktura z informacjami o pamiêci
 */
void PrintResourceUsage(double cpuUsage, const PROCESS_MEMORY_COUNTERS& pmc) {
    cout << "Uzycie procesora: " << cpuUsage << "%" << endl;
    cout << "Uzycie pamieci RAM: " << pmc.WorkingSetSize << " B" << endl;
}

// G³ówna funkcja interfejsu
// =========================
void szyfrowanie()
{
    srand(time(0));// Inicjalizacja generatora
    cout << "Wybierz rodzaj szyfrowania: RSA lub AES" << endl;
    string choice;
    cin >> choice;

    if (choice == "RSA" || choice == "rsa" || choice == "r" || choice == "R") {
        cout << "Czy chcesz podac wlasny tekst? (T/N)" << endl;
        char yn;
        cin >> yn;
        // Wspólne zmienne pomiarowe
        FILETIME userStart, kernelStart, userEnd, kernelEnd;
        PROCESS_MEMORY_COUNTERS pmcStart, pmcEnd;
        chrono::time_point<chrono::high_resolution_clock> start, end;

        if (yn == 'T' || yn == 't') {
            // Obs³uga w³asnego tekstu
            string plaintext;
            cout << "Podaj tekst: ";
            cin.ignore();
            getline(cin, plaintext);

            int bytes;
            cout << "Podaj dlugosc klucza 512, 1024, 2048 lub 4096: ";
            cin >> bytes;

            while (bytes < 512 || bytes > 4096) {
                cout << "Nieprawidlowa dlugosc! Podaj wartosc 512, 1024, 2048 lub 4096: ";
                cin >> bytes;
            }
            // Rozpocznij pomiary
            GetProcessorTimes(userStart, kernelStart);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmcStart, sizeof(pmcStart));
            start = chrono::high_resolution_clock::now();

            EncryptDecryptRSA(plaintext, bytes);
            // Zakoñcz pomiary
            end = chrono::high_resolution_clock::now();
            GetProcessorTimes(userEnd, kernelEnd);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmcEnd, sizeof(pmcEnd));
            // Oblicz statystyki
            chrono::duration<double> duration = end - start;
            double userTime = FileTimeToSeconds(userEnd) - FileTimeToSeconds(userStart);
            double kernelTime = FileTimeToSeconds(kernelEnd) - FileTimeToSeconds(kernelStart);
            double cpuUsage = ((userTime + kernelTime) / duration.count()) * 100.0;

            cout << "\n=== Statystyki wydajnosci ===" << endl;
            cout << "Czas wykonania: " << duration.count() << " sekund" << endl;
            PrintResourceUsage(cpuUsage, pmcEnd);
        }
        else {
            // Tryb generacji losowych danych
            int ilosc, bytes, min_len, max_len;

            cout << "Podaj ilosc hasel: ";
            cin >> ilosc;
            while (ilosc <= 0) {
                cout << "Ilosc musi byc wieksza od 0! Podaj ponownie: ";
                cin >> ilosc;
            }

            cout << "Podaj dlugosc klucza (512, 1024, 2048 lub 4096): ";
            cin >> bytes;
            while (bytes < 512 || bytes > 4096) {
                cout << "Nieprawidlowa dlugosc klucza RSA. Dlugosc musi byc 512-4096 bitow: ";
                cin >> bytes;
            }

            cout << "Podaj minimalna dlugosc tekstu (>=1): ";
            cin >> min_len;
            cout << "Podaj maksymalna dlugosc tekstu: ";
            cin >> max_len;
            while (min_len < 1 || max_len < min_len) {
                cout << "Nieprawidlowy zakres! Podaj ponownie:\n";
                cout << "Minimalna dlugosc (>=1): ";
                cin >> min_len;
                cout << "Maksymalna dlugosc (>= " << min_len << "): ";
                cin >> max_len;
            }

            // Rozpocznij testy
            GetProcessorTimes(userStart, kernelStart);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmcStart, sizeof(pmcStart));
            start = chrono::high_resolution_clock::now();

            for (int i = 0; i < ilosc; i++) {
                string text = generateRandomText(min_len, max_len);
                EncryptDecryptRSA(text, bytes);
            }
            // Zakoñcz i oblicz statystyki
            end = chrono::high_resolution_clock::now();
            GetProcessorTimes(userEnd, kernelEnd);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmcEnd, sizeof(pmcEnd));

            chrono::duration<double> duration = end - start;
            double userTime = FileTimeToSeconds(userEnd) - FileTimeToSeconds(userStart);
            double kernelTime = FileTimeToSeconds(kernelEnd) - FileTimeToSeconds(kernelStart);
            double cpuUsage = ((userTime + kernelTime) / duration.count()) * 100.0;

            cout << "\n=== Statystyki wydajnosci ===" << endl;
            cout << "Calkowity czas: " << duration.count() << " sekund" << endl;
            PrintResourceUsage(cpuUsage, pmcEnd);
        }
    }
    else if (choice == "AES" || choice == "aes" || choice == "a" || choice == "A") {
        cout << "Czy chcesz podac wlasny tekst? (T/N)" << endl;
        char yn;
        cin >> yn;

        FILETIME userStart, kernelStart, userEnd, kernelEnd;
        PROCESS_MEMORY_COUNTERS pmcStart, pmcEnd;
        chrono::time_point<chrono::high_resolution_clock> start, end;

        if (yn == 'T' || yn == 't') {
            string plaintext;
            cout << "Podaj tekst: ";
            cin.ignore();
            getline(cin, plaintext);

            int keySize;
            cout << "Podaj rozmiar klucza 128, 192 lub 256: ";
            cin >> keySize;

            while (keySize != 128 && keySize != 192 && keySize != 256) {
                cout << "Nieprawidlowa dlugosc klucza AES. Dopuszczalne wartosci: 128, 192, 256: ";
                cin >> keySize;
            }

            GetProcessorTimes(userStart, kernelStart);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmcStart, sizeof(pmcStart));
            start = chrono::high_resolution_clock::now();

            EncryptDecryptAES(plaintext, keySize);

            end = chrono::high_resolution_clock::now();
            GetProcessorTimes(userEnd, kernelEnd);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmcEnd, sizeof(pmcEnd));

            chrono::duration<double> duration = end - start;
            double userTime = FileTimeToSeconds(userEnd) - FileTimeToSeconds(userStart);
            double kernelTime = FileTimeToSeconds(kernelEnd) - FileTimeToSeconds(kernelStart);
            double cpuUsage = ((userTime + kernelTime) / duration.count()) * 100.0;

            cout << "\n=== Statystyki wydajnosci ===" << endl;
            cout << "Czas wykonania: " << duration.count() << " sekund" << endl;
            PrintResourceUsage(cpuUsage, pmcEnd);
        }
        else {
            int ilosc, keySize, min_len, max_len;

            cout << "Podaj ilosc hasel: ";
            cin >> ilosc;
            while (ilosc <= 0) {
                cout << "Ilosc musi byc wieksza od 0! Podaj ponownie: ";
                cin >> ilosc;
            }

            cout << "Podaj rozmiar klucza 128, 192 lub 256: ";
            cin >> keySize;
            while (keySize != 128 && keySize != 192 && keySize != 256) {
                cout << "Nieprawidlowa dlugosc klucza AES. Dopuszczalne wartosci: 128, 192, 256: ";
                cin >> keySize;
            }

            cout << "Podaj minimalna dlugosc tekstu (>=1): ";
            cin >> min_len;
            cout << "Podaj maksymalna dlugosc tekstu: ";
            cin >> max_len;
            while (min_len < 1 || max_len < min_len) {
                cout << "Nieprawidlowy zakres! Podaj ponownie:\n";
                cout << "Minimalna dlugosc (>=1): ";
                cin >> min_len;
                cout << "Maksymalna dlugosc (>= " << min_len << "): ";
                cin >> max_len;
            }

            GetProcessorTimes(userStart, kernelStart);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmcStart, sizeof(pmcStart));
            start = chrono::high_resolution_clock::now();

            for (int i = 0; i < ilosc; i++) {
                string text = generateRandomText(min_len, max_len);
                EncryptDecryptAES(text, keySize);
            }

            end = chrono::high_resolution_clock::now();
            GetProcessorTimes(userEnd, kernelEnd);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmcEnd, sizeof(pmcEnd));

            chrono::duration<double> duration = end - start;
            double userTime = FileTimeToSeconds(userEnd) - FileTimeToSeconds(userStart);
            double kernelTime = FileTimeToSeconds(kernelEnd) - FileTimeToSeconds(kernelStart);
            double cpuUsage = ((userTime + kernelTime) / duration.count()) * 100.0;

            cout << "\n=== Statystyki wydajnosci ===" << endl;
            cout << "Calkowity czas: " << duration.count() << " sekund" << endl;
            PrintResourceUsage(cpuUsage, pmcEnd);
        }
    }
    else {
        cout << "Nieprawidlowy wybor szyfrowania!" << endl;
    }
}