// Algoritmo para encontrar o menor custo de um percurso entre todas as cidades de um mapa 25x25, 
// será utilizado para ver se a nossa solução é fidedigna

#include <stdio.h>
#include <limits.h>

#define N 25
#define VISITED_ALL (1<<N) - 1

int dist[N][N];
int memo[N][1<<N];

void readMatrix(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("File could not be opened.\n");
        return;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            dist[i][j] = (i == j) ? 0 : INT_MAX; 
        }
    }

    int i, j;
    double d;
    while (fscanf(file, "%d %d %lf", &i, &j, &d) == 3) {
        dist[i][j] = (int)d; 
        dist[j][i] = (int)d;  
    }

    fclose(file);
}

int tsp(int pos, int mask) {
    if (mask == VISITED_ALL) {
        return dist[pos][0];  
    }

    if (memo[pos][mask] != -1) {
        return memo[pos][mask];
    }

    int ans = INT_MAX;
    for (int city = 0; city < N; city++) {
        if ((mask & (1 << city)) == 0 && dist[pos][city] != INT_MAX) {
            int newAns = dist[pos][city] + tsp(city, mask | (1 << city));
            ans = (ans > newAns) ? newAns : ans;
        }
    }

    return memo[pos][mask] = ans;
}

int main() {
    readMatrix("map25.txt"); 

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < (1 << N); j++) {
            memo[i][j] = -1;
        }
    }

    printf("Custo Mínimo: %d\n", tsp(0, 1)); 
    return 0;
}