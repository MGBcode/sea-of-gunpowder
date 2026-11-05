#ifndef ORDENACAO_H
#define ORDENACAO_H

#include "estruturas.h"

#define MAX_ENTRADAS_INICIATIVA (LINHAS_TABULEIRO * COLUNAS_TABULEIRO + 2)

typedef struct {
    Combatente *ator;
    int velocidade_efetiva;
    int indice_turno_boss;
} EntradaIniciativa;

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

// Ordena entradas de iniciativa por velocidade efetiva decrescente.
void ordenarFilaPorVelocidade(EntradaIniciativa fila[], int total);
// Ordena candidatos do Tactical Blackboard por pontuação decrescente.
void ordenarCandidatosIA(CandidatoAcao vetor[], int total);

#endif
