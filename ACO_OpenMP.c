#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h>

// PARÂMETROS
#define CIDADES 100
#define FORMIGAS 4000
#define DIST_MAX 100
#define ALFA 1
#define BETA 5
#define EVA 0.5
#define QTD_FEROMONIO 100
#define MAX_TOURS 50
#define FEROMONIO_INICIAL (1.0/CIDADES)
#define DIST_TOTAL_MAXIMA (CIDADES * DIST_MAX)

// FORMIGA
struct formiga {
    int cidadeAtual, proxCidade, indiceCaminho;
    int visitado[CIDADES];
    int caminho[CIDADES];
    float comprimentoTour;
};

// Variáveis globais na CPU
float distancias[CIDADES][CIDADES];
float feromonios[CIDADES][CIDADES];
struct formiga formigas[FORMIGAS];
float melhorDistancia = (float)DIST_TOTAL_MAXIMA;

// Função para obter a matriz de distâncias a partir da entrada
void obterMatrizDistancias() {
    int i, j;
    float k;
    // Lendo as distâncias entre as cidades
    while (scanf("%i %i %f", &i, &j, &k) == 3) {
        distancias[i][j] = k;
        distancias[j][i] = k;
        feromonios[i][j] = FEROMONIO_INICIAL;
        feromonios[j][i] = FEROMONIO_INICIAL;
    }
}

// Função para inicializar as formigas
void inicializarFormigas(struct formiga *formigas, int numFormigas, int numCidades) {
    #pragma omp parallel for
    for (int id = 0; id < numFormigas; ++id) {
        struct formiga *formiga = &formigas[id];
        formiga->cidadeAtual = rand() % numCidades;
        formiga->indiceCaminho = 1;
        formiga->comprimentoTour = 0.0f;
        for (int i = 0; i < numCidades; ++i) {
            formiga->visitado[i] = 0;
            formiga->caminho[i] = -1;
        }
        formiga->visitado[formiga->cidadeAtual] = 1; // Marcar a cidade inicial como visitada
        formiga->caminho[0] = formiga->cidadeAtual; // Adicionar a cidade inicial ao caminho
    }
}

// Função para determinar a próxima cidade a ser visitada pela formiga
int proximaCidade(struct formiga *formiga, float distancias[CIDADES][CIDADES], float feromonios[CIDADES][CIDADES], int numCidades) {
    int de = formiga->cidadeAtual;
    double denom = 0.0;
    for (int para = 0; para < numCidades; ++para) {
        if (formiga->visitado[para] == 0) {
            denom += pow(feromonios[de][para], ALFA) * pow(1.0 / distancias[de][para], BETA);
        }
    }

    // Gerar um número aleatório entre 0 e 1
    double limite = (double)rand() / RAND_MAX;
    double probAcumulada = 0.0;
    // Calcular a probabilidade acumulada e selecionar a próxima cidade
    for (int para = 0; para < numCidades; ++para) {
        if (formiga->visitado[para] == 0) {
            probAcumulada += pow(feromonios[de][para], ALFA) * pow(1.0 / distancias[de][para], BETA) / denom;
            if (probAcumulada >= limite) {
                return para;
            }
        }
    }

    // Fallback para garantir que uma cidade não visitada seja escolhida
    for (int para = 0; para < numCidades; ++para) {
        if (formiga->visitado[para] == 0) {
            return para;
        }
    }
    return -1;
}

// Função para mover as formigas
void moverFormigas(struct formiga *formigas, float distancias[CIDADES][CIDADES], float feromonios[CIDADES][CIDADES], int numFormigas, int numCidades, float *melhorDistancia) {
    #pragma omp parallel
    {
        float melhorDistanciaLocal = *melhorDistancia;
        float localFeromonios[CIDADES][CIDADES] = {0}; // Matriz de feromônios local para cada thread

        #pragma omp for schedule(dynamic)
        for (int id = 0; id < numFormigas; ++id) {
            struct formiga *formiga = &formigas[id];
            for (int tour = 0; tour < MAX_TOURS; ++tour) {
                formiga->indiceCaminho = 1;
                formiga->comprimentoTour = 0.0f;
                for (int i = 0; i < numCidades; ++i) {
                    formiga->visitado[i] = 0;
                    formiga->caminho[i] = -1;
                }
                formiga->cidadeAtual = rand() % numCidades; // Selecionar uma cidade inicial aleatória para cada tour
                formiga->visitado[formiga->cidadeAtual] = 1;
                formiga->caminho[0] = formiga->cidadeAtual;

                // Mover a formiga até que todas as cidades sejam visitadas
                while (formiga->indiceCaminho < numCidades) {
                    int proxima = proximaCidade(formiga, distancias, feromonios, numCidades);
                    formiga->visitado[proxima] = 1;
                    formiga->caminho[formiga->indiceCaminho++] = proxima;
                    formiga->comprimentoTour += distancias[formiga->cidadeAtual][proxima];
                    formiga->cidadeAtual = proxima;
                }
                formiga->comprimentoTour += distancias[formiga->cidadeAtual][formiga->caminho[0]];

                // Atualização dos feromônios com base no tour da formiga
                for (int i = 0; i < numCidades - 1; ++i) {
                    int de = formiga->caminho[i];
                    int para = formiga->caminho[i + 1];
                    #pragma omp atomic
                    localFeromonios[de][para] += QTD_FEROMONIO / formiga->comprimentoTour;
                    #pragma omp atomic
                    localFeromonios[para][de] += QTD_FEROMONIO / formiga->comprimentoTour;
                }
                int de = formiga->caminho[numCidades - 1];
                int para = formiga->caminho[0];
                #pragma omp atomic
                localFeromonios[de][para] += QTD_FEROMONIO / formiga->comprimentoTour;
                #pragma omp atomic
                localFeromonios[para][de] += QTD_FEROMONIO / formiga->comprimentoTour;

                // Atualizar a melhor distância encontrada
                if (formiga->comprimentoTour < melhorDistanciaLocal) {
                    melhorDistanciaLocal = formiga->comprimentoTour;
                }
            }
        }

        #pragma omp critical
        {
            if (melhorDistanciaLocal < *melhorDistancia) {
                *melhorDistancia = melhorDistanciaLocal;
            }

            // Atualizar os feromônios globais com os locais
            for (int i = 0; i < numCidades; ++i) {
                for (int j = 0; j < numCidades; ++j) {
                    feromonios[i][j] += localFeromonios[i][j];
                }
            }
        }
    }

    // Evaporação dos feromônios fora da região paralela para evitar condições de corrida
    for (int i = 0; i < numCidades; ++i) {
        for (int j = 0; j < numCidades; ++j) {
            feromonios[i][j] *= (1.0 - EVA);
            if (feromonios[i][j] < FEROMONIO_INICIAL) {
                feromonios[i][j] = FEROMONIO_INICIAL;
            }
        }
    }
}

// MAIN
int main() {
    clock_t inicio, fim;
    double tempoGasto;

    // Registrar o tempo de início
    inicio = clock();

    // Inicializar aleatoriedade
    srand(time(NULL));

    // Obter a matriz de distâncias
    obterMatrizDistancias();

    // Inicializar formigas
    inicializarFormigas(formigas, FORMIGAS, CIDADES);

    // Mover formigas
    moverFormigas(formigas, distancias, feromonios, FORMIGAS, CIDADES, &melhorDistancia);

    // Registrar o tempo de fim
    fim = clock();

    // Calcular o tempo gasto
    tempoGasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;

    printf("Melhor distância: %f\n", melhorDistancia);
    printf("Tempo para gerar: %f segundos\n", tempoGasto);

    return 0;
}
