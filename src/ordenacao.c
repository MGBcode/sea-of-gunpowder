#include "ordenacao.h"

// Calcula a velocidade após os efeitos temporários de status.
static int calcularVelocidadeEfetiva(const Combatente *unidade)
{
    int velocidade;
    if (unidade == NULL) {
        return -1;
    }
    velocidade = unidade->velocidade;
    if (unidade->turnos_motivacao > 0) {
        velocidade += 2;
    }
    if (unidade->turnos_veneno > 0) {
        velocidade -= 3;
    }
    return velocidade;
}

// Ordena por velocidade efetiva decrescente com InsertionSort.
void ordenarFilaPorVelocidade(Combatente *fila[], int totalVivos)
{
    int indice;
    if (fila == NULL || totalVivos < 2) {
        return;
    }
    for (indice = 1; indice < totalVivos; indice++) {
        Combatente *unidade = fila[indice];
        int posicao = indice;
        while (posicao > 0 &&
               calcularVelocidadeEfetiva(fila[posicao - 1]) < calcularVelocidadeEfetiva(unidade)) {
            fila[posicao] = fila[posicao - 1];
            posicao--;
        }
        fila[posicao] = unidade;
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
