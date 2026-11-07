#ifndef COMBATE_H
#define COMBATE_H

#include "ordenacao.h"

// Monta a fila de iniciativa com as ações vivas do turno.
int montarFilaTurnos(Tabuleiro3x6 *mapa, EntradaIniciativa filaOut[]);
// Recalcula a velocidade efetiva e reordena as ações restantes da rodada.
void atualizarVelocidadesFilaTurnos(EntradaIniciativa fila[], int total, int indiceAtual);
// Aplica e consome um consumível válido.
bool usarItemConsumivel(Combatente *unidade, Item *item);
// Verifica se uma equipe foi derrotada.
bool verificarFimDeCombate(Tabuleiro3x6 *mapa, bool *vitoriaAliada);
// Avança até a próxima entrada viva da fila.
bool avancarProximoTurno(EntradaIniciativa fila[], int total, int *indiceAtual,
                         Tabuleiro3x6 *mapa);
// Aplica dano, morte permanente e transição da fase do chefe.
void subtrairVida(Tabuleiro3x6 *mapa, Combatente *alvo, int quantidade);
// Posiciona ou troca aliados entre as duas colunas iniciais.
bool posicionarPirataPreBatalha(Tabuleiro3x6 *mapa, Combatente *pirata, int linha, int coluna);
// Move a unidade e resolve minas ativadas em sua célula de destino.
bool moverCombatente3x6(Tabuleiro3x6 *mapa, Combatente *unidade, int destinoLinha,
                        int destinoColuna);
// Executa uma habilidade e aplica seus efeitos e dano.
void executarAtaqueOuHabilidade(Tabuleiro3x6 *mapa, Combatente *atacante, int indiceHab,
                                int alvoLinha, int alvoColuna);
// Encerra o turno da unidade e marca sua vulnerabilidade.
void iniciarDescanso(Tabuleiro3x6 *mapa, Combatente *unidade);
// Aplica efeitos de status no início do turno.
void processarStatusInicioTurno(Tabuleiro3x6 *mapa, Combatente *unidade);
// Remove todos os buffs, debuffs e acúmulos temporários ao encerrar um combate.
void limparEfeitosPosCombate(Combatente *unidade);

#endif
