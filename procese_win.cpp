#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

bool isPrime(int n) {
    if (n <= 1) return false;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc == 3) {
        HANDLE hReadP2C = (HANDLE)std::stoull(argv[1]);
        HANDLE hWriteC2P = (HANDLE)std::stoull(argv[2]);

        int start, end;
        DWORD bytesRead, bytesWritten;
        
        ReadFile(hReadP2C, &start, sizeof(start), &bytesRead, NULL);
        ReadFile(hReadP2C, &end, sizeof(end), &bytesRead, NULL);
        CloseHandle(hReadP2C);

        std::vector<int> prime_gasite;
        for (int num = start; num <= end; num++) {
            if (isPrime(num)) {
                prime_gasite.push_back(num);
            }
        }

        // fiecare proces va comunica numerele prime catre procesul initial, care le va si afisa.
        int count = prime_gasite.size();
        WriteFile(hWriteC2P, &count, sizeof(count), &bytesWritten, NULL);
        for (int p : prime_gasite) {
            WriteFile(hWriteC2P, &p, sizeof(p), &bytesWritten, NULL);
        }
        
        CloseHandle(hWriteC2P);
        return 0;
    }

    const int NUM_PROCESE = 10;
    const int MAX_NUM = 10000;
    const int RANGE = MAX_NUM / NUM_PROCESE;

    HANDLE hProcess[NUM_PROCESE];
    HANDLE hWriteP2C[NUM_PROCESE]; 
    HANDLE hReadC2P[NUM_PROCESE];  

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE; 
    sa.lpSecurityDescriptor = NULL;

    // creeze 10 de procese care cauta numere prime
    for (int i = 0; i < NUM_PROCESE; i++) {
        HANDLE hReadP2C, hWriteC2P;

        CreatePipe(&hReadP2C, &hWriteP2C[i], &sa, 0);
        SetHandleInformation(hWriteP2C[i], HANDLE_FLAG_INHERIT, 0); 

        CreatePipe(&hReadC2P[i], &hWriteC2P, &sa, 0);
        SetHandleInformation(hReadC2P[i], HANDLE_FLAG_INHERIT, 0); 

        std::string cmd = std::string(argv[0]) + " " + 
                          std::to_string((unsigned long long)hReadP2C) + " " + 
                          std::to_string((unsigned long long)hWriteC2P);
        
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            std::cerr << "Eroare creare proces!\n";
            return 1;
        }

        hProcess[i] = pi.hProcess;
        CloseHandle(pi.hThread);
        CloseHandle(hReadP2C);
        CloseHandle(hWriteC2P);

        // comunica prin pipe fiecarui proces un interval de cate 1000 de numere.
        int start = i * RANGE + 1;
        int end = (i + 1) * RANGE;
        DWORD bytesWritten;
        WriteFile(hWriteP2C[i], &start, sizeof(start), &bytesWritten, NULL);
        WriteFile(hWriteP2C[i], &end, sizeof(end), &bytesWritten, NULL);
        CloseHandle(hWriteP2C[i]);
    }

    for (int i = 0; i < NUM_PROCESE; i++) {
        WaitForSingleObject(hProcess[i], INFINITE);

        int count;
        DWORD bytesRead;
        ReadFile(hReadC2P[i], &count, sizeof(count), &bytesRead, NULL);

        std::cout << "\n[Proces Initial] Interval " << i * RANGE + 1 << "-" << (i + 1) * RANGE << ":\n";
        for (int j = 0; j < count; j++) {
            int prim;
            ReadFile(hReadC2P[i], &prim, sizeof(prim), &bytesRead, NULL);
            std::cout << prim << " ";
        }
        std::cout << "\n";

        CloseHandle(hReadC2P[i]);
        CloseHandle(hProcess[i]);
    }

    return 0;
}
