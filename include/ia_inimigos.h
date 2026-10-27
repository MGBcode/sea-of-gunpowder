#ifndef IA_INIMIGOS_H
#define IA_INIMIGOS_H

#include "ordenacao.h"

#define MAX_CANDIDATOS_IA 64

typedef struct {
    Combatente *inimigo;
    Combatente *alvos[LINHAS_TABULEIRO * COLUNAS_TABULEIRO];
    int total_alvos;
    int linha_aliado_mais_fraco;
    int coluna_aliado_mais_fraco;
    int aliado_mais_fraco_vida;
} TacticalBlackboard;

// Atualiza o contexto tático com os alvos aliados vivos.
void atualizarTacticalBlackboard(TacticalBlackboard *blackboard, Tabuleiro3x6 *mapa, Combatente *inimigo);
// Gera ações candidatas, ordena-as e escolhe uma entre as melhores ponderadamente.
bool selecionarAcaoIA(Tabuleiro3x6 *mapa, Combatente *inimigo, CandidatoAcao *escolha);

#endif

