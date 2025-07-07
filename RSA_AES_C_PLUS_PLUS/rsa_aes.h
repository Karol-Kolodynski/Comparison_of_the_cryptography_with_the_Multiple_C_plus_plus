#pragma once          // Dyrektywa kompilatora zapobiegaj¹ca wielokrotnemu do³¹czaniu
#ifndef RSA_AES_H     // Tradycyjne zabezpieczenie przed podwójnym include
#define RSA_AES_H

#include <string>     // Biblioteka do obs³ugi ci¹gów znaków (std::string)
#include <chrono>     // Biblioteka do pomiarów czasu (u¿ywana w implementacji)

/**
 * Szyfruje i deszyfruje tekst algorytmem AES
 * @param plaintext Tekst wejœciowy do zaszyfrowania
 * @param keySize Rozmiar klucza w bitach - dopuszczalne wartoœci: 128, 192, 256
 * @warning Funkcja wyœwietla wyniki bezpoœrednio na konsolê
 */
void EncryptDecryptAES(const std::string& plaintext, int keySize);

/**
 * Szyfruje i deszyfruje tekst algorytmem RSA
 * @param plaintext Tekst wejœciowy do zaszyfrowania
 * @param bytes D³ugoœæ klucza w bitach - dopuszczalne wartoœci: 512-4096
 * @note Generuje now¹ parê kluczy przy ka¿dym wywo³aniu
 */
void EncryptDecryptRSA(const std::string& plaintext, int bytes);

/**
 * Generuje losowy tekst do testów
 * @return Losowy ci¹g znaków alfanumerycznych i specjalnych
 */
std::string generateRandomText();

/**
 * G³ówna funkcja interfejsu u¿ytkownika
 * @brief Umo¿liwia wybór algorytmu, wprowadzanie tekstu oraz wyœwietla statystyki
 * @details W przypadku wyboru RSA oferuje:
 * - Generacjê wielu testów
 * - W³asny tekst wejœciowy
 * - Pomiar czasu i pamiêci
 */
void szyfrowanie();

#endif // RSA_AES_H