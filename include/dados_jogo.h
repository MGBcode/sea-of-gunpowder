#ifndef DADOS_JOGO_H
#define DADOS_JOGO_H

#include "estruturas.h"

#define TOTAL_PIRATAS_PREDEFINIDOS 5
#define TOTAL_INIMIGOS_PREDEFINIDOS 4
#define TOTAL_ITENS_PREDEFINIDOS 11

extern int inflacaoLojasPercentual;
extern int moedasTripulacao;
extern int contadorFugas;
extern int contadorSaques;

// Retorna um pirata inicial preenchido pelo índice solicitado.
bool criarPirataInicial(int indice, Combatente *destino);
// Retorna um inimigo inicial preenchido pelo índice solicitado.
bool criarInimigoInicial(int indice, Combatente *destino);
// Retorna Barba Negra preenchido com seus status iniciais.
bool criarChefeFinal(Combatente *destino);
// Copia os onze itens iniciais para o vetor informado.
int carregarItensIniciais(Item itens[], int capacidade);
// Evolui o combatente e aplica atributos e desbloqueios por nível.
bool subirNivelPirata(Combatente *unidade, int niveisGanhos);
// Converte XP acumulado em níveis ganhos.
bool ganharExperienciaEvoluir(Combatente *unidade, int experiencia);
// Aplica nível, moedas, cura, itens e inflação conforme a rota.
bool aplicarRecompensaRota(TipoNoRota tipoRota, Combatente tripulacao[],
                           int totalTripulacao, ListaItens *inventario, int xpBase);
// Ajusta o preço conforme a inflação atual das lojas.
int aplicarInflacaoLoja(int preco);
// Aplica a progressão normal, fuga ou saque de loja ao inimigo.
void escalonarInimigoPorDificuldade(Combatente *inimigo, int nivelDificuldade);
// Aplica os modificadores multiplicativos de uma rota e histórico de run.
void escalonarInimigoPorRota(Combatente *inimigo, TipoNoRota tipoRota, int etapa,
                             int fugasNaRun, bool saqueouLoja);
void aplicarBuffElite(Combatente *inimigo);
bool podeGerarEliteNaEtapa(int etapa, int fugasNaRun);
// Gera inimigos determinísticos para a rota e retorna quantos foram criados.
int sortearInimigosRota(TipoNoRota tipoRota, unsigned int semente,
                        int indices[], int capacidade);
// Sorteia inimigos obedecendo as regras obrigatórias da etapa (Rota 1, 2 ou 3).
int sortearInimigosPorEtapa(TipoNoRota tipoRota, int etapaRota, unsigned int semente,
                            int indices[], int capacidade);
// Sorteia os 3 itens da loja da rota com 2 unidades de estoque cada.
int sortearItensLoja(int etapaRota, unsigned int semente, Item itensLoja[],
                     int estoques[], int capacidade);
// Processa a compra de uma unidade de um slot da loja.
bool comprarItemNaLoja(Item itensLoja[], int estoques[], int totalLoja, int indiceSlot,
                       ListaItens *inventario, int *moedas);
// Transfere gratuitamente todo o estoque restante da loja após vencer um saque.
bool saquearItensRestantesLoja(const Item itensLoja[], int estoques[], int totalLoja,
                               ListaItens *inventario);
// Libera os nós da build passiva do combatente.
void liberarListaItensCombatente(ListaItens *lista);

#endif
