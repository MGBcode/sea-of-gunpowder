#ifndef ORDENACAO_H
#define ORDENACAO_H

#include "estruturas.h"

typedef struct {
    int tipo_acao;
    int linha_alvo;
    int coluna_alvo;
    int indice_habilidade;
    int pontuacao;
} CandidatoAcao;

// Ordena os turnos pela velocidade efetiva em ordem decrescente.
void ordenarFilaPorVelocidade(Combatente *fila[], int totalVivos);
// Ordena candidatos da IA por pontuação decrescente.
void ordenarCandidatosIA(CandidatoAcao vetor[], int total);

#endif

