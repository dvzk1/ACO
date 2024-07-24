#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <cuda_runtime.h>
#include <curand_kernel.h>

// PARÂMETROS
// se for executar outras matrizes, mudar o valor de cidades para a quantidade de cidades da matriz
// estamos executando para 100x100 (100 cidades)

#define CIDADES 100 // definir tam. da matriz

// parametros standard
#define FORMIGAS 4000
#define DIST_MAX 150
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

// Declaração de funções CUDA
__global__ void inicializarFormigasCUDA(struct formiga *formigas, int numFormigas, int numCidades, unsigned int seed, curandState *states);
__global__ void moverFormigasCUDA(struct formiga *formigas, float *distancias, float *feromonios, int numFormigas, int numCidades, float feromonioInicial, float evap, float qntdFeromonio, int maxTours, float *melhorDistancia, curandState *states);
__device__ int proximaCidadeCUDA(struct formiga *formiga, float *distancias, float *feromonios, int numCidades, curandState *state);

// Verificando se a entrada está dentro dos limites permitidos
void obterMatrizDistancias() {
    int i, j;
    float k;
    while (scanf("%i %i %f", &i, &j, &k) == 3) {
        if (i < CIDADES && j < CIDADES && k <= DIST_MAX) {
            distancias[i][j] = k;
            distancias[j][i] = k; 
            feromonios[i][j] = FEROMONIO_INICIAL;
            feromonios[j][i] = FEROMONIO_INICIAL;
        } else {
            printf("Erro: Indices fora do limite ou distancia maior que o permitido.\n");
            exit(1);
        }
    }
}

// MAIN
int main() {
    clock_t inicio, fim;
    double tempoGasto;

    // Registrar o tempo de início
    inicio = clock();

    // Obter a matriz de distâncias
    obterMatrizDistancias();

    // Alocar memória na GPU
    struct formiga *d_formigas;
    float *d_distancias;
    float *d_feromonios;
    float *d_melhorDistancia;
    curandState *d_states;
    cudaMalloc((void**)&d_formigas, FORMIGAS * sizeof(struct formiga));
    cudaMalloc((void**)&d_distancias, CIDADES * CIDADES * sizeof(float));
    cudaMalloc((void**)&d_feromonios, CIDADES * CIDADES * sizeof(float));
    cudaMalloc((void**)&d_melhorDistancia, sizeof(float));
    cudaMalloc((void**)&d_states, FORMIGAS * sizeof(curandState));

    // Copiar dados para a GPU
    cudaMemcpy(d_distancias, distancias, CIDADES * CIDADES * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_feromonios, feromonios, CIDADES * CIDADES * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_melhorDistancia, &melhorDistancia, sizeof(float), cudaMemcpyHostToDevice);

    // Inicializar formigas na GPU
    inicializarFormigasCUDA<<<(FORMIGAS + 255) / 256, 256>>>(d_formigas, FORMIGAS, CIDADES, time(NULL), d_states);
    cudaDeviceSynchronize(); // Certificar-se de que as formigas foram inicializadas

    // Mover formigas na GPU
    moverFormigasCUDA<<<(FORMIGAS + 255) / 256, 256>>>(d_formigas, d_distancias, d_feromonios, FORMIGAS, CIDADES, FEROMONIO_INICIAL, EVA, QTD_FEROMONIO, MAX_TOURS, d_melhorDistancia, d_states);
    cudaDeviceSynchronize(); // Certificar-se de que a movimentação das formigas terminou

    // Copiar resultado de volta para a CPU
    cudaMemcpy(&melhorDistancia, d_melhorDistancia, sizeof(float), cudaMemcpyDeviceToHost);

    // Copiar formigas de volta para a CPU para verificação
    cudaMemcpy(formigas, d_formigas, FORMIGAS * sizeof(struct formiga), cudaMemcpyDeviceToHost);

/*
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
*/

    // Liberar memória da GPU
    cudaFree(d_formigas);
    cudaFree(d_distancias);
    cudaFree(d_feromonios);
    cudaFree(d_melhorDistancia);
    cudaFree(d_states);

    // Registrar o tempo de fim
    fim = clock();

    // Calcular o tempo gasto
    tempoGasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;

    printf("Melhor distância: %f\n", melhorDistancia);
    printf("Tempo para gerar: %f segundos\n", tempoGasto);

    return 0;
}

// Func. CUDA

// Função para inicializar as formigas
__global__ void inicializarFormigasCUDA(struct formiga *formigas, int numFormigas, int numCidades, unsigned int seed, curandState *states) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id < numFormigas) {
        curand_init(seed, id, 0, &states[id]); 
        struct formiga *formiga = &formigas[id];
        formiga->cidadeAtual = curand(&states[id]) % numCidades; 
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
__global__ void moverFormigasCUDA(struct formiga *formigas, float *distancias, float *feromonios, int numFormigas, int numCidades, float feromonioInicial, float evap, float qntdFeromonio, int maxTours, float *melhorDistancia, curandState *states) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id < numFormigas) {
        curandState state = states[id];
        struct formiga *formiga = &formigas[id];
        for (int tour = 0; tour < maxTours; ++tour) {
            formiga->indiceCaminho = 1;
            formiga->comprimentoTour = 0.0f;
            for (int i = 0; i < numCidades; ++i) {
                formiga->visitado[i] = 0;
                formiga->caminho[i] = -1;
            }
            formiga->cidadeAtual = curand(&state) % numCidades; // Selecionar uma cidade inicial aleatória para cada tour
            formiga->visitado[formiga->cidadeAtual] = 1;
            formiga->caminho[0] = formiga->cidadeAtual;

            // Mover a formiga até que todas as cidades sejam visitadas
            while (formiga->indiceCaminho < numCidades) {
                int proxima = proximaCidadeCUDA(formiga, distancias, feromonios, numCidades, &state);
                formiga->visitado[proxima] = 1;
                formiga->caminho[formiga->indiceCaminho++] = proxima;
                formiga->comprimentoTour += distancias[formiga->cidadeAtual * numCidades + proxima];
                formiga->cidadeAtual = proxima;
            }
            formiga->comprimentoTour += distancias[formiga->cidadeAtual * numCidades + formiga->caminho[0]];

            printf("Formiga %d completou um tour com comprimento %f\n", id, formiga->comprimentoTour);

            // Evaporação dos feromônios
            for (int i = 0; i < numCidades; ++i) {
                for (int j = 0; j < numCidades; ++j) {
                    atomicExch(&feromonios[i * numCidades + j], (1.0 - evap) * feromonios[i * numCidades + j]);
                    if (feromonios[i * numCidades + j] < feromonioInicial) {
                        atomicExch(&feromonios[i * numCidades + j], feromonioInicial);
                    }
                }
            }

            // Atualização dos feromônios com base no tour da formiga
            for (int i = 0; i < numCidades - 1; ++i) {
                int de = formiga->caminho[i];
                int para = formiga->caminho[i + 1];
                atomicAdd(&feromonios[de * numCidades + para], qntdFeromonio / formiga->comprimentoTour);
                atomicAdd(&feromonios[para * numCidades + de], qntdFeromonio / formiga->comprimentoTour);
            }
            int de = formiga->caminho[numCidades - 1];
            int para = formiga->caminho[0];
            atomicAdd(&feromonios[de * numCidades + para], qntdFeromonio / formiga->comprimentoTour);
            atomicAdd(&feromonios[para * numCidades + de], qntdFeromonio / formiga->comprimentoTour);

            // Atualizar a melhor distância encontrada
            atomicMin((int *)melhorDistancia, __float_as_int(formiga->comprimentoTour));
        }
        states[id] = state;
    }
}

// Função para determinar a próxima cidade a ser visitada pela formiga
__device__ int proximaCidadeCUDA(struct formiga *formiga, float *distancias, float *feromonios, int numCidades, curandState *state) {
    int de = formiga->cidadeAtual;
    double denom = 0.0;
    for (int para = 0; para < numCidades; ++para) {
        if (formiga->visitado[para] == 0) {
            denom += pow(feromonios[de * numCidades + para], ALFA) * pow(1.0 / distancias[de * numCidades + para], BETA);
        }
    }

    // Gerar um número aleatório entre 0 e 1
    double limite = (double)(curand(state) % 1000000000) / 1000000000.0;
    double probAcumulada = 0.0;
    // Calcular a probabilidade acumulada e selecionar a próxima cidade
    for (int para = 0; para < numCidades; ++para) {
        if (formiga->visitado[para] == 0) {
            probAcumulada += pow(feromonios[de * numCidades + para], ALFA) * pow(1.0 / distancias[de * numCidades + para], BETA) / denom;
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
