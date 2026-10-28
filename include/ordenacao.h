#ifndef ORDENACAO_H
#define ORDENACAO_H

#include "estruturas.h"

typedef enum {
    ACAO_IA_HABILIDADE = 0,
    ACAO_IA_MOVER = 1,
    ACAO_IA_DESCANSAR = 2
} TipoAcaoIA;

typedef struct {
    TipoAcaoIA tipo_acao;
    int linha_alvo;
    int coluna_alvo;
    int indice_habilidade;
    int pontuacao;
} CandidatoAcao;

// Ordena por velocidade efetiva decrescente, incluindo Motivação e Veneno.
void ordenarFilaPorVelocidade(Combatente *fila[], int totalVivos);
// Ordena candidatos do Tactical Blackboard por pontuação decrescente.
void ordenarCandidatosIA(CandidatoAcao vetor[], int total);

#endif
