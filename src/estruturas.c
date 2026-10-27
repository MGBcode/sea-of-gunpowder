#include "estruturas.h"

#include <stdlib.h>

// Libera todos os nós de uma árvore ternária.
static void liberarArvoreRotas(NoRota *no)
{
    if (no == NULL) {
        return;
    }
    liberarArvoreRotas(no->caminho1);
    liberarArvoreRotas(no->caminho2);
    liberarArvoreRotas(no->loja);
    free(no);
}

// Libera todos os nós de uma lista de itens.
static void liberarListaItens(ListaItens *lista)
{
    NoItem *atual;
    if (lista == NULL) {
        return;
    }
    atual = lista->inicio;
    while (atual != NULL) {
        NoItem *proximo = atual->proximo;
        free(atual);
        atual = proximo;
    }
    lista->inicio = NULL;
    lista->tamanho = 0;
}

// Libera todos os nós de uma lista de coordenadas.
static void liberarListaAlcance(ListaAlcance *lista)
{
    NoCelula *atual;
    if (lista == NULL) {
        return;
    }
    atual = lista->inicio;
    while (atual != NULL) {
        NoCelula *proxima = atual->proximo;
        free(atual);
        atual = proxima;
    }
    free(lista);
}

// Cria uma etapa e seus três caminhos até o chefe final.
static NoRota *criarEtapaRota(int dificuldade, int etapa, TipoNoRota tipo)
{
    NoRota *no = malloc(sizeof(*no));
    if (no == NULL) {
        return NULL;
    }
    no->nivel_dificuldade = dificuldade;
    no->etapa = etapa;
    no->tipo = tipo;
    no->chefe_final = tipo == ROTA_CHEFE;
    no->caminho1 = NULL;
    no->caminho2 = NULL;
    no->loja = NULL;
    if (etapa < 3) {
        no->caminho1 = criarEtapaRota(dificuldade, etapa + 1, ROTA_CAMINHO_1);
        no->caminho2 = criarEtapaRota(dificuldade, etapa + 1, ROTA_CAMINHO_2);
        no->loja = criarEtapaRota(dificuldade, etapa + 1, ROTA_LOJA);
    } else if (etapa == 3) {
        no->caminho1 = criarEtapaRota(dificuldade, 4, ROTA_CHEFE);
        no->caminho2 = criarEtapaRota(dificuldade, 4, ROTA_CHEFE);
        no->loja = criarEtapaRota(dificuldade, 4, ROTA_CHEFE);
    }
    if ((etapa < 4) &&
        (no->caminho1 == NULL || no->caminho2 == NULL || no->loja == NULL)) {
        liberarArvoreRotas(no);
        return NULL;
    }
    return no;
}

// Adiciona uma coordenada ao final da lista de alcance.
static bool adicionarCelula(ListaAlcance *lista, int linha, int coluna)
{
    NoCelula *nova = malloc(sizeof(*nova));
    NoCelula **fim;
    if (nova == NULL) {
        return false;
    }
    nova->linha = linha;
    nova->coluna = coluna;
    nova->proximo = NULL;
    fim = &lista->inicio;
    while (*fim != NULL) {
        fim = &(*fim)->proximo;
    }
    *fim = nova;
    lista->tamanho++;
    return true;
}

// Constrói a árvore ternária de rotas até o confronto final.
NoRota *construirArvoreRotas(int nivelDificuldade)
{
    if (nivelDificuldade < 1) {
        nivelDificuldade = 1;
    }
    return criarEtapaRota(nivelDificuldade, 0, ROTA_RAIZ);
}

// Retorna o próximo nó conforme a rota escolhida.
NoRota *avancarParaRota(NoRota *noAtual, int escolha)
{
    if (noAtual == NULL) {
        return NULL;
    }
    if (escolha == 1) {
        return noAtual->caminho1;
    }
    if (escolha == 2) {
        return noAtual->caminho2;
    }
    if (escolha == 3) {
        return noAtual->loja;
    }
    return NULL;
}

// Encerra a aventura, libera suas rotas e reinicia a dificuldade.
NoRota *fugirDaAventura(NoRota *raiz, ListaItens *inventarioGeral, int *nivelDificuldade)
{
    NoItem **elo;
    liberarArvoreRotas(raiz);
    if (inventarioGeral != NULL) {
        elo = &inventarioGeral->inicio;
        while (*elo != NULL) {
            NoItem *item = *elo;
            if (item->item.tipo == ITEM_CONSUMIVEL || item->item.tipo == ITEM_ATIVO) {
                *elo = item->proximo;
                free(item);
                inventarioGeral->tamanho--;
            } else {
                elo = &item->proximo;
            }
        }
    }
    if (nivelDificuldade != NULL) {
        (*nivelDificuldade)++;
    }
    return construirArvoreRotas(nivelDificuldade != NULL ? *nivelDificuldade : 2);
}

// Move um item passivo do inventário para a build do personagem.
bool equiparItemPassivoBuild(ListaItens *inventario, ListaItens *buildPirata, int idItem)
{
    NoItem **elo;
    NoItem *item;
    if (inventario == NULL || buildPirata == NULL || buildPirata->tamanho >= MAX_ITENS_PASSIVOS) {
        return false;
    }
    elo = &inventario->inicio;
    while (*elo != NULL && (*elo)->item.id != idItem) {
        elo = &(*elo)->proximo;
    }
    if (*elo == NULL || (*elo)->item.tipo != ITEM_PASSIVO) {
        return false;
    }
    item = *elo;
    *elo = item->proximo;
    inventario->tamanho--;
    item->proximo = buildPirata->inicio;
    buildPirata->inicio = item;
    buildPirata->tamanho++;
    return true;
}

// Aplica ou remove os bônus diretos de um item passivo.
static void alterarAtributosItem(Combatente *pirata, const Item *item, int fator)
{
    pirata->vida_maxima += fator * item->bonus_vida;
    pirata->vida_atual += fator * item->bonus_vida;
    pirata->velocidade += fator * item->bonus_velocidade;
    pirata->dano_base += fator * item->bonus_dano;
    if (pirata->vida_maxima < 1) {
        pirata->vida_maxima = 1;
    }
    if (pirata->vida_atual > pirata->vida_maxima) {
        pirata->vida_atual = pirata->vida_maxima;
    }
    if (pirata->vida_atual < 0) {
        pirata->vida_atual = 0;
    }
}

// Equipa um item passivo no combatente e aplica seus bônus.
bool aplicarItemPassivoNoPirata(ListaItens *inventario, Combatente *pirata, int idItem)
{
    NoItem *item;
    if (inventario == NULL || pirata == NULL ||
        pirata->build_passivos.tamanho >= MAX_ITENS_PASSIVOS) {
        return false;
    }
    item = inventario->inicio;
    while (item != NULL && item->item.id != idItem) {
        item = item->proximo;
    }
    if (item == NULL || item->item.tipo != ITEM_PASSIVO) {
        return false;
    }
    if (!equiparItemPassivoBuild(inventario, &pirata->build_passivos, idItem)) {
        return false;
    }
    alterarAtributosItem(pirata, &item->item, 1);
    return true;
}

// Remove um item passivo do combatente e seus bônus.
bool desequiparItemPassivoBuild(ListaItens *inventario, Combatente *pirata, int idItem)
{
    NoItem **elo;
    NoItem *item;
    if (inventario == NULL || pirata == NULL) {
        return false;
    }
    elo = &pirata->build_passivos.inicio;
    while (*elo != NULL && (*elo)->item.id != idItem) {
        elo = &(*elo)->proximo;
    }
    if (*elo == NULL || (*elo)->item.tipo != ITEM_PASSIVO) {
        return false;
    }
    item = *elo;
    *elo = item->proximo;
    pirata->build_passivos.tamanho--;
    alterarAtributosItem(pirata, &item->item, -1);
    item->proximo = inventario->inicio;
    inventario->inicio = item;
    inventario->tamanho++;
    return true;
}

// Lista as células do tabuleiro dentro do alcance da habilidade escolhida.
ListaAlcance *calcularAreaAtuacao3x6(Tabuleiro3x6 *mapa, Combatente *origem, int indiceHabilidade)
{
    ListaAlcance *lista;
    int alcance;
    int linha;
    int coluna;
    if (mapa == NULL || origem == NULL || indiceHabilidade < 0 ||
        indiceHabilidade >= origem->habilidades_desbloqueadas) {
        return NULL;
    }
    alcance = origem->habilidades[indiceHabilidade].alcance;
    lista = calloc(1, sizeof(*lista));
    if (lista == NULL) {
        return NULL;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            int distancia = abs(linha - origem->linha) + abs(coluna - origem->coluna);
            if (distancia <= alcance &&
                (!origem->habilidades[indiceHabilidade].disparo ||
                 (linha == origem->linha && coluna != origem->coluna)) &&
                !adicionarCelula(lista, linha, coluna)) {
                liberarListaAlcance(lista);
                return NULL;
            }
        }
    }
    return lista;
}

// Libera a árvore de rotas, o inventário e a lista de alcance.
void liberarEstruturasJogo(NoRota *raiz, ListaItens *inventario, ListaAlcance *alcance)
{
    liberarArvoreRotas(raiz);
    liberarListaItens(inventario);
    liberarListaAlcance(alcance);
}
