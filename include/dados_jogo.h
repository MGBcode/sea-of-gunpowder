#ifndef DADOS_JOGO_H
#define DADOS_JOGO_H

#include "estruturas.h"

#define TOTAL_PIRATAS_PREDEFINIDOS 5
#define TOTAL_INIMIGOS_PREDEFINIDOS 4
#define TOTAL_ITENS_PREDEFINIDOS 12

extern int inflacaoLojasPercentual;

// Retorna um pirata inicial preenchido pelo índice solicitado.
bool criarPirataInicial(int indice, Combatente *destino);
// Retorna um inimigo inicial preenchido pelo índice solicitado.
bool criarInimigoInicial(int indice, Combatente *destino);
// Retorna um mercador armado para um assalto à loja.
bool criarMercadorLoja(int indice, Combatente *destino, int nivelDificuldade);
// Retorna o chefe final preenchido com seus status iniciais.
bool criarChefeFinal(Combatente *destino);
// Copia os doze itens iniciais para o vetor informado.
int carregarItensIniciais(Item itens[], int capacidade);
// Adiciona experiência e aplica os atributos dos níveis 1 a 6.
bool ganharExperienciaEvoluir(Combatente *unidade, int experiencia);
// Aplica a recompensa de XP, cura e inflação correspondente à rota concluída.
void aplicarRecompensaRota(TipoNoRota tipoRota, Combatente tripulacao[],
                           int totalTripulacao, int xpBase);
// Ajusta o preço conforme a inflação atual das lojas.
int aplicarInflacaoLoja(int preco);
// Escalona os atributos de um inimigo conforme a dificuldade da run.
void escalonarInimigoPorDificuldade(Combatente *inimigo, int nivelDificuldade);
// Libera os nós da build passiva do combatente.
void liberarListaItensCombatente(ListaItens *lista);

#endif
