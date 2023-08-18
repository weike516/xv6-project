/*
#include "kernel/types.h"
#include "user/user.h"

#define MAX_NUM 35

// Function to filter primes
void filter_primes(int in_fd) {
    int num;
    int primes[2];

    if (read(in_fd, &num, sizeof(num)) == sizeof(num)) {
        printf("prime %d\n", num);

        int p[2];
        pipe(p);

        if (fork() == 0) {
            close(p[0]);
            filter_primes(p[1]);
        } else {
            close(p[1]);
            while (read(in_fd, &primes[0], sizeof(primes[0])) == sizeof(primes[0])) {
                if (primes[0] % num != 0) {
                    write(p[0], &primes[0], sizeof(primes[0]));
                }
            }
            close(p[0]);
            wait(0);
        }
    }

    exit(0);
}

int main(int argc, char *argv[]) {
    int p[2];

    pipe(p);

    if (fork() == 0) {
        close(p[0]);
        for (int i = 2; i <= MAX_NUM; i++) {
            write(p[1], &i, sizeof(i));
        }
        close(p[1]);
        exit(0);
    } else {
        close(p[1]);
        filter_primes(p[0]);
        close(p[0]);
        wait(0);
        exit(0);
    }

    return 0;
}

*/
//Primes Function. Print prime numbers from 2~35

#include "kernel/types.h"
#include "user/user.h"

void primes(int p[2]){
    int n, prime;
    int p1[2];
    close(p[1]);
    if(read(p[0], &prime, 4) == 0){
        close(p[0]);
        exit(1);
    } else {
        printf("prime %d\n", prime);
        pipe(p1);
        if(fork() == 0) {
            close(p[0]);
            primes(p1);
        } else {
            close(p1[0]);
            while(1){
                if(read(p[0],&n, 4) == 0) break;
                if(n % prime != 0) write(p1[1], &n, 4);
            }
            close(p[0]);
            close(p1[1]);
            wait(0);
        }
    }
    exit(0);
}

int
main(int argc, char *argv[]){
    if(argc != 1){
        fprintf(2," Usage: error.\n");
        exit(1);
    }
    int n;
    int p[2];
    pipe(p);
    if(fork() == 0) {
        primes(p);
    } else {
        close(p[0]);
        for(n = 2 ; n <= 35 ; n ++ ){
            write(p[1], &n, 4);
        }
        close(p[1]);
        wait(0);
    }
    exit(0);
}

