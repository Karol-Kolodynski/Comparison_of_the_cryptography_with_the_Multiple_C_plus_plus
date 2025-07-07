#pragma once          // Dyrektywa kompilatora zapobiegaj�ca wielokrotnemu do��czaniu
#ifndef RSA_AES_H     // Tradycyjne zabezpieczenie przed podw�jnym include
#define RSA_AES_H

#include <string>     // Biblioteka do obs�ugi ci�g�w znak�w (std::string)
#include <chrono>     // Biblioteka do pomiar�w czasu (u�ywana w implementacji)

/**
 * Szyfruje i deszyfruje tekst algorytmem AES
 * @param plaintext Tekst wej�ciowy do zaszyfrowania
 * @param keySize Rozmiar klucza w bitach - dopuszczalne warto�ci: 128, 192, 256
 * @warning Funkcja wy�wietla wyniki bezpo�rednio na konsol�
 */
void EncryptDecryptAES(const std::string& plaintext, int keySize);

/**
 * Szyfruje i deszyfruje tekst algorytmem RSA
 * @param plaintext Tekst wej�ciowy do zaszyfrowania
 * @param bytes D�ugo�� klucza w bitach - dopuszczalne warto�ci: 512-4096
 * @note Generuje now� par� kluczy przy ka�dym wywo�aniu
 */
void EncryptDecryptRSA(const std::string& plaintext, int bytes);

/**
 * Generuje losowy tekst do test�w
 * @return Losowy ci�g znak�w alfanumerycznych i specjalnych
 */
std::string generateRandomText();

/**
 * G��wna funkcja interfejsu u�ytkownika
 * @brief Umo�liwia wyb�r algorytmu, wprowadzanie tekstu oraz wy�wietla statystyki
 * @details W przypadku wyboru RSA oferuje:
 * - Generacj� wielu test�w
 * - W�asny tekst wej�ciowy
 * - Pomiar czasu i pami�ci
 */
void szyfrowanie();

#endif // RSA_AES_H