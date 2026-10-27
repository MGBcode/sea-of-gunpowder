#ifndef COMBATE_H
#define COMBATE_H

#include "estruturas.h"

// Posiciona um aliado em uma das duas colunas iniciais do tabuleiro.
bool posicionarPirataPreBatalha(Tabuleiro3x6 *mapa, Combatente *pirata, int linha, int coluna);
// Move uma unidade adjacente e consome estamina conforme o deslocamento.
bool moverCombatente3x6(Tabuleiro3x6 *mapa, Combatente *unidade, int destinoLinha, int destinoColuna);
// Executa uma habilidade, aplica efeitos e remove aliados derrotados permanentemente.
void executarAtaqueOuHabilidade(Tabuleiro3x6 *mapa, Combatente *atacante, int indiceHab, int alvoLinha, int alvoColuna);
// Encerra o turno da unidade e marca sua vulnerabilidade.
void iniciarDescanso(Combatente *unidade);
// Aplica efeitos de status no início do turno.
void processarStatusInicioTurno(Tabuleiro3x6 *mapa, Combatente *unidade);

#endif
