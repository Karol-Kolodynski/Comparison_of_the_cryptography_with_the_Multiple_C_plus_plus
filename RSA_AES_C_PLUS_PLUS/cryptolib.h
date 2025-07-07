#pragma once
#ifndef CRYPTOLIB_H
#define CRYPTOLIB_H

#include "cryptlib.h"
#include "osrng.h"
#include "rsa.h"
#include "hex.h"
#include <iostream>
#include <string>
#include <rijndael.h>
#include <modes.h>
#include <files.h>
#include <chrono>

using namespace CryptoPP;
using namespace std;

string generateRandomText();
void EncryptDecryptRSA(string plaintext, int bytes);
void EncryptDecryptAES(string plaintext);

#endif /* CRYPTOLIB_H */