#include "ordenacao.h"

// Ordena entradas de iniciativa por velocidade efetiva decrescente.
void ordenarFilaPorVelocidade(EntradaIniciativa fila[], int total)
{
    int indice;
    if (fila == NULL || total < 2) {
        return;
    }
    for (indice = 1; indice < total; indice++) {
        EntradaIniciativa entrada = fila[indice];
        int posicao = indice;
        while (posicao > 0 &&
               fila[posicao - 1].velocidade_efetiva < entrada.velocidade_efetiva) {
            fila[posicao] = fila[posicao - 1];
            posicao--;
        }
        fila[posicao] = entrada;
    }
}

// Ordena candidatos do Tactical Blackboard por pontuação decrescente.
void ordenarCandidatosIA(CandidatoAcao vetor[], int total)
{
    int indice;
    if (vetor == NULL || total < 2) {
        return;
    }
    for (indice = 1; indice < total; indice++) {
        CandidatoAcao candidato = vetor[indice];
        int posicao = indice;
        while (posicao > 0 && vetor[posicao - 1].pontuacao < candidato.pontuacao) {
            vetor[posicao] = vetor[posicao - 1];
            posicao--;
        }
        vetor[posicao] = candidato;
    }
}
