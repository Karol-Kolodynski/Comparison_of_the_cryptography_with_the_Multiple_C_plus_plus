#include <iostream>
#include "liczenie_slow.h"
#include "rsa_aes.h"

int main() {
    std::cout << "Wybierz opcje:\n";
    std::cout << "1. Liczenie s³ów w pliku\n";
    std::cout << "2. Szyfrowanie RSA/AES\n";
    int wybor;
    std::cin >> wybor;

    if (wybor == 1) {
        liczenie_slow();
    }
    else if (wybor == 2) {
        szyfrowanie();
    }
    else {
        std::cout << "Nieprawidlowy wybor." << std::endl;
    }

    return 0;
}