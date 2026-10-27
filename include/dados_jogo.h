#ifndef DADOS_JOGO_H
#define DADOS_JOGO_H

#include "estruturas.h"

#define TOTAL_PIRATAS_PREDEFINIDOS 5
#define TOTAL_INIMIGOS_PREDEFINIDOS 4
#define TOTAL_ITENS_PREDEFINIDOS 12

// Retorna um pirata inicial preenchido pelo índice solicitado.
bool criarPirataInicial(int indice, Combatente *destino);
// Retorna um inimigo inicial preenchido pelo índice solicitado.
bool criarInimigoInicial(int indice, Combatente *destino);
// Retorna o chefe final preenchido com seus status iniciais.
bool criarChefeFinal(Combatente *destino);
// Copia os doze itens iniciais para o vetor informado.
int carregarItensIniciais(Item itens[], int capacidade);
// Adiciona experiência e aplica os atributos dos níveis 1 a 6.
bool ganharExperienciaEvoluir(Combatente *unidade, int experiencia);
// Libera os nós da build passiva do combatente.
void liberarListaItensCombatente(ListaItens *lista);

#endif

