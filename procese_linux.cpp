#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

bool isPrime(int n) {
    if (n <= 1) return false;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

int main() {
    const int NUM_PROCESE = 10;
    const int MAX_NUM = 10000;
    const int RANGE = MAX_NUM / NUM_PROCESE;

    int p2c[NUM_PROCESE][2];
    int c2p[NUM_PROCESE][2];

    for (int i = 0; i < NUM_PROCESE; i++) {
        if (pipe(p2c[i]) == -1 || pipe(c2p[i]) == -1) {
            std::cerr << "Eroare pipe!\n";
            return 1;
        }
    }

    // creeze 10 de procese care cauta numere prime
    for (int i = 0; i < NUM_PROCESE; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Eroare fork!\n";
            return 1;
        } 
        else if (pid == 0) { 
            close(p2c[i][1]);
            close(c2p[i][0]);

            int start, end;
            read(p2c[i][0], &start, sizeof(start));
            read(p2c[i][0], &end, sizeof(end));
            close(p2c[i][0]);

            std::vector<int> prime_gasite;
            for (int num = start; num <= end; num++) {
                if (isPrime(num)) {
                    prime_gasite.push_back(num);
                }
            }

            // fiecare proces va comunica numerele prime catre procesul initial, care le va si afisa.
            int count = prime_gasite.size();
            write(c2p[i][1], &count, sizeof(count));
            for (int p : prime_gasite) {
                write(c2p[i][1], &p, sizeof(p));
            }
            
            close(c2p[i][1]);
            return 0;
        }
    }

    // comunica prin pipe fiecarui proces un interval de cate 1000 de numere.
    for (int i = 0; i < NUM_PROCESE; i++) {
        close(p2c[i][0]);
        
        int start = i * RANGE + 1;
        int end = (i + 1) * RANGE;

        write(p2c[i][1], &start, sizeof(start));
        write(p2c[i][1], &end, sizeof(end));
        close(p2c[i][1]);
    }

    for (int i = 0; i < NUM_PROCESE; i++) {
        wait(NULL);
        close(c2p[i][1]);

        int count;
        read(c2p[i][0], &count, sizeof(count));

        std::cout << "\n[Proces Initial] Interval " << i * RANGE + 1 << "-" << (i + 1) * RANGE << ":\n";
        
        for (int j = 0; j < count; j++) {
            int prim;
            read(c2p[i][0], &prim, sizeof(prim));
            std::cout << prim << " ";
        }
        std::cout << "\n";
        
        close(c2p[i][0]);
    }

    return 0;
}
