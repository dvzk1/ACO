#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

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

// Funções auxiliares
void inicializarFormigas(struct formiga *formigas, int numFormigas, int numCidades);
void moverFormigas(struct formiga *formigas, float distancias[CIDADES][CIDADES], float feromonios[CIDADES][CIDADES], int numFormigas, int numCidades, float feromonioInicial, float evap, float qntdFeromonio, int maxTours, float *melhorDistancia);
int proximaCidade(struct formiga *formiga, float distancias[CIDADES][CIDADES], float feromonios[CIDADES][CIDADES], int numCidades);

// Função para obter a matriz de distâncias a partir do arquivo
void obterMatrizDistancias() {
    // nome do arquivo
    FILE *file = fopen("C:\\Users\\moura\\Searches\\ACO\\MAPS\\map100.txt", "r");
    if (!file) {
        printf("Erro ao abrir o arquivo de distâncias.\n");
        exit(1);
    }
    int i, j;
    float k;
    while (fscanf(file, "%d %d %f", &i, &j, &k) == 3) {
        distancias[i][j] = k;
        distancias[j][i] = k;
        feromonios[i][j] = FEROMONIO_INICIAL;
        feromonios[j][i] = FEROMONIO_INICIAL;
    }
    fclose(file);
}

// MAIN
int main() {
    clock_t inicio, fim;
    double tempoGasto;

    // Registrar o tempo de início
    inicio = clock();

    // Obter a matriz de distâncias
    obterMatrizDistancias();

    // Inicializar formigas
    inicializarFormigas(formigas, FORMIGAS, CIDADES);

    // Mover formigas
    moverFormigas(formigas, distancias, feromonios, FORMIGAS, CIDADES, FEROMONIO_INICIAL, EVA, QTD_FEROMONIO, MAX_TOURS, &melhorDistancia);

    // Imprimir alguns exemplos das variáveis na CPU
    for (int i = 0; i < 5; ++i) {
        printf("Formiga %d: Comprimento do Tour: %f\n", i, formigas[i].comprimentoTour);
        printf("Caminho: ");
        for (int j = 0; j < CIDADES; ++j) {
            if (formigas[i].caminho[j] != -1)
                printf("%d ", formigas[i].caminho[j]);
        }
        printf("\nVisitado: ");
        for (int j = 0; j < CIDADES; ++j) {
            printf("%d ", formigas[i].visitado[j]);
        }
        printf("\n");
    }

    // Registrar o tempo de fim
    fim = clock();

    // Calcular o tempo gasto
    tempoGasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;

    printf("Melhor distância: %f\n", melhorDistancia);
    printf("Tempo para gerar: %f segundos\n", tempoGasto);

    return 0;
}

// Função para inicializar as formigas
void inicializarFormigas(struct formiga *formigas, int numFormigas, int numCidades) {
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

        printf("Formiga %d inicializada na cidade %d\n", id, formiga->cidadeAtual);
    }
}

// Função para mover as formigas
void moverFormigas(struct formiga *formigas, float distancias[CIDADES][CIDADES], float feromonios[CIDADES][CIDADES], int numFormigas, int numCidades, float feromonioInicial, float evap, float qntdFeromonio, int maxTours, float *melhorDistancia) {
    for (int id = 0; id < numFormigas; ++id) {
        struct formiga *formiga = &formigas[id];
        for (int tour = 0; tour < maxTours; ++tour) {
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

            printf("Formiga %d completou um tour com comprimento %f\n", id, formiga->comprimentoTour);

            // Evaporação dos feromônios
            for (int i = 0; i < numCidades; ++i) {
                for (int j = 0; j < numCidades; ++j) {
                    feromonios[i][j] = (1.0 - evap) * feromonios[i][j];
                    if (feromonios[i][j] < feromonioInicial) {
                        feromonios[i][j] = feromonioInicial;
                    }
                }
            }

            // Atualização dos feromônios com base no tour da formiga
            for (int i = 0; i < numCidades - 1; ++i) {
                int de = formiga->caminho[i];
                int para = formiga->caminho[i + 1];
                feromonios[de][para] += qntdFeromonio / formiga->comprimentoTour;
                feromonios[para][de] += qntdFeromonio / formiga->comprimentoTour;
            }
            int de = formiga->caminho[numCidades - 1];
            int para = formiga->caminho[0];
            feromonios[de][para] += qntdFeromonio / formiga->comprimentoTour;
            feromonios[para][de] += qntdFeromonio / formiga->comprimentoTour;

            // Atualizar a melhor distância encontrada
            if (formiga->comprimentoTour < *melhorDistancia) {
                *melhorDistancia = formiga->comprimentoTour;
            }
        }
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
