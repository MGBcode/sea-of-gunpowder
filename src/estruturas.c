#include "estruturas.h"

#include "dados_jogo.h"

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
        no->loja = criarEtapaRota(dificuldade, 4, ROTA_LOJA);
        if (no->loja != NULL) {
            no->loja->caminho1 = criarEtapaRota(dificuldade, 5, ROTA_CHEFE);
        }
    }
    if ((etapa < 4) &&
        (no->caminho1 == NULL || no->caminho2 == NULL || no->loja == NULL)) {
        liberarArvoreRotas(no);
        return NULL;
    }
    if (etapa == 3 && no->loja->caminho1 == NULL) {
        liberarArvoreRotas(no);
        return NULL;
    }
    return no;
}

// Adiciona uma coordenada e seu indicador de fogo amigo à lista.
static bool adicionarCelula(ListaAlcance *lista, int linha, int coluna, int perigoFogoAmigo)
{
    NoCelula *nova = malloc(sizeof(*nova));
    NoCelula **fim;
    if (nova == NULL) {
        return false;
    }
    nova->linha = linha;
    nova->coluna = coluna;
    nova->perigo_fogo_amigo = perigoFogoAmigo;
    nova->proximo = NULL;
    fim = &lista->inicio;
    while (*fim != NULL) {
        fim = &(*fim)->proximo;
    }
    *fim = nova;
    lista->tamanho++;
    return true;
}

// Conta os itens presentes na lista para impor o limite real da build.
static int contarItens(const ListaItens *lista)
{
    const NoItem *atual;
    int total = 0;
    if (lista == NULL) {
        return 0;
    }
    for (atual = lista->inicio; atual != NULL; atual = atual->proximo) {
        total++;
    }
    return total;
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

// Encerra a aventura, apaga o inventário solto e incrementa a dificuldade.
NoRota *fugirDaAventura(NoRota *raiz, ListaItens *inventarioGeral, int *nivelDificuldade)
{
    if (contadorFugas >= 2) {
        return NULL;
    }
    liberarArvoreRotas(raiz);
    contadorFugas++;
    if (inventarioGeral != NULL) {
        liberarListaItens(inventarioGeral);
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
    NoItem **fimBuild;
    NoItem *item;
    int itensNaBuild;
    if (inventario == NULL || buildPirata == NULL) {
        return false;
    }
    itensNaBuild = contarItens(buildPirata);
    if (itensNaBuild >= MAX_ITENS_PASSIVOS) {
        return false;
    }
    elo = &inventario->inicio;
    while (*elo != NULL && (*elo)->item.id != idItem) {
        elo = &(*elo)->proximo;
    }
    if (*elo == NULL || (*elo)->item.tipo != ITEM_PASSIVO) {
        return false;
    }
    if ((*elo)->item.quantidade > 1) {
        item = malloc(sizeof(*item));
        if (item == NULL) {
            return false;
        }
        item->item = (*elo)->item;
        item->item.quantidade = 1;
        (*elo)->item.quantidade--;
    } else {
        item = *elo;
        *elo = item->proximo;
        inventario->tamanho = contarItens(inventario);
    }
    item->proximo = NULL;
    fimBuild = &buildPirata->inicio;
    while (*fimBuild != NULL) {
        fimBuild = &(*fimBuild)->proximo;
    }
    *fimBuild = item;
    buildPirata->tamanho = itensNaBuild + 1;
    return true;
}

// Aplica ou remove os bônus diretos de um item passivo.
static void alterarAtributosItem(Combatente *pirata, const Item *item, int fator)
{
    pirata->vida_maxima += fator * item->bonus_vida;
    pirata->vida_atual += fator * item->bonus_vida;
    pirata->velocidade += fator * item->bonus_velocidade;
    pirata->dano_base += fator * item->bonus_dano;
    pirata->estamina_maxima += fator * item->bonus_estamina;
    pirata->estamina_atual += fator * item->bonus_estamina;
    pirata->precisao_melee += fator * item->bonus_precisao;
    pirata->precisao_ranged += fator * item->bonus_precisao;
    if (pirata->vida_maxima < 1) {
        pirata->vida_maxima = 1;
    }
    if (pirata->vida_atual > pirata->vida_maxima) {
        pirata->vida_atual = pirata->vida_maxima;
    }
    if (pirata->vida_atual < 0) {
        pirata->vida_atual = 0;
    }
    if (pirata->estamina_maxima < 1) {
        pirata->estamina_maxima = 1;
    }
    if (pirata->estamina_atual > pirata->estamina_maxima) {
        pirata->estamina_atual = pirata->estamina_maxima;
    }
    if (pirata->estamina_atual < 0) {
        pirata->estamina_atual = 0;
    }
}

// Equipa um item passivo no combatente e aplica seus bônus.
bool aplicarItemPassivoNoPirata(ListaItens *inventario, Combatente *pirata, int idItem)
{
    NoItem *item;
    Item itemCopia;
    if (inventario == NULL || pirata == NULL ||
        contarItens(&pirata->build_passivos) >= MAX_ITENS_PASSIVOS) {
        return false;
    }
    item = inventario->inicio;
    while (item != NULL && item->item.id != idItem) {
        item = item->proximo;
    }
    if (item == NULL || item->item.tipo != ITEM_PASSIVO) {
        return false;
    }
    itemCopia = item->item;
    if (!equiparItemPassivoBuild(inventario, &pirata->build_passivos, idItem)) {
        return false;
    }
    alterarAtributosItem(pirata, &itemCopia, 1);
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
    pirata->build_passivos.tamanho = contarItens(&pirata->build_passivos);
    alterarAtributosItem(pirata, &item->item, -1);
    item->proximo = inventario->inicio;
    inventario->inicio = item;
    inventario->tamanho = contarItens(inventario);
    return true;
}

// Verifica se uma célula fica dentro do efeito centrado no alvo indicado.
static bool celulaNaArea(const Habilidade *habilidade, const Combatente *origem,
                         int alvoLinha, int alvoColuna, int linha, int coluna)
{
    int deltaLinha = alvoLinha - origem->linha;
    int deltaColuna = alvoColuna - origem->coluna;
    if (linha == alvoLinha && coluna == alvoColuna) {
        return true;
    }
    if (habilidade->tipo_ataque == ATAQUE_LINHA ||
        habilidade->tipo_ataque == ATAQUE_DASH_CARDEAL) {
        return (alvoLinha == origem->linha && linha == origem->linha &&
                (coluna - origem->coluna) * deltaColuna > 0 &&
                abs(coluna - origem->coluna) <= habilidade->alcance) ||
               (alvoColuna == origem->coluna && coluna == origem->coluna &&
                (linha - origem->linha) * deltaLinha > 0 &&
                abs(linha - origem->linha) <= habilidade->alcance);
    }
    if (habilidade->tipo_ataque == ATAQUE_CONE) {
        if (abs(deltaLinha) > abs(deltaColuna)) {
            int sinalLinha = deltaLinha > 0 ? 1 : -1;
            int passoLinha = (linha - origem->linha) * sinalLinha;
            return passoLinha >= 1 && passoLinha <= habilidade->alcance &&
                   abs(coluna - origem->coluna) < passoLinha;
        }
        if (deltaColuna != 0) {
            int sinalColuna = deltaColuna > 0 ? 1 : -1;
            int passoColuna = (coluna - origem->coluna) * sinalColuna;
            return passoColuna >= 1 && passoColuna <= habilidade->alcance &&
                   abs(linha - origem->linha) < passoColuna;
        }
        return false;
    }
    if (habilidade->tipo_ataque == ATAQUE_CRUZ) {
        return abs(linha - alvoLinha) + abs(coluna - alvoColuna) <= 1;
    }
    if (habilidade->tipo_ataque == ATAQUE_AREA_1X3) {
        if (alvoColuna != origem->coluna) {
            return coluna == alvoColuna && abs(linha - origem->linha) <= 1;
        }
        return linha == alvoLinha && abs(coluna - origem->coluna) <= 1;
    }
    if (habilidade->tipo_ataque == ATAQUE_AREA_2X3) {
        int linhaExtra = (alvoLinha != origem->linha) ? alvoLinha :
                         (origem->linha < LINHAS_TABULEIRO - 1 ?
                          origem->linha + 1 : origem->linha - 1);
        return (linha == origem->linha || linha == linhaExtra) &&
               abs(coluna - origem->coluna) <= 1;
    }
    if (habilidade->tipo_ataque == ATAQUE_AREA_3X2) {
        int colunaExtra = (alvoColuna == COLUNAS_TABULEIRO - 1) ? -1 :
                          ((alvoColuna == 0) ? 1 : (deltaColuna >= 0 ? 1 : -1));
        return linha >= 0 && linha < LINHAS_TABULEIRO &&
               (coluna == alvoColuna || coluna == alvoColuna + colunaExtra);
    }
    return false;
}

// Lista as células selecionáveis e os aliados ameaçados por ataque em área.
ListaAlcance *calcularAreaAtuacao3x6(Tabuleiro3x6 *mapa, Combatente *origem, int indiceHabilidade)
{
    ListaAlcance *lista;
    Habilidade *habilidade;
    bool alcancavel[LINHAS_TABULEIRO][COLUNAS_TABULEIRO] = { { false } };
    bool perigoso[LINHAS_TABULEIRO][COLUNAS_TABULEIRO] = { { false } };
    int alvoLinha;
    int alvoColuna;
    int linha;
    int coluna;
    if (mapa == NULL || origem == NULL || origem->linha < 0 ||
        origem->linha >= LINHAS_TABULEIRO || origem->coluna < 0 ||
        origem->coluna >= COLUNAS_TABULEIRO || indiceHabilidade < 0 ||
        indiceHabilidade >= origem->habilidades_desbloqueadas ||
        indiceHabilidade >= MAX_HABILIDADES) {
        return NULL;
    }
    habilidade = &origem->habilidades[indiceHabilidade];
    for (alvoLinha = 0; alvoLinha < LINHAS_TABULEIRO; alvoLinha++) {
        for (alvoColuna = 0; alvoColuna < COLUNAS_TABULEIRO; alvoColuna++) {
            int deltaLinha = alvoLinha - origem->linha;
            int deltaColuna = alvoColuna - origem->coluna;
            int distancia = abs(deltaLinha) + abs(deltaColuna);
            bool cardinal = alvoLinha == origem->linha || alvoColuna == origem->coluna;
            bool permitePropriaCelula = habilidade->efeito == EFEITO_CURA ||
                                        habilidade->efeito == EFEITO_CURA_CONTINUA ||
                                        habilidade->efeito == EFEITO_MOTIVACAO ||
                                        habilidade->efeito == EFEITO_VENENO;
            bool dentroAlcance = (permitePropriaCelula ? distancia >= 0 : distancia > 0) &&
                                 distancia <= habilidade->alcance && cardinal;
            if (habilidade->tipo_ataque == ATAQUE_CONE) {
                bool coneVertical = abs(deltaLinha) >= 1 &&
                                    abs(deltaLinha) <= habilidade->alcance &&
                                    abs(deltaColuna) < abs(deltaLinha);
                bool coneHorizontal = abs(deltaColuna) >= 1 &&
                                      abs(deltaColuna) <= habilidade->alcance &&
                                      abs(deltaLinha) < abs(deltaColuna);
                dentroAlcance = coneVertical || coneHorizontal;
            } else if (habilidade->tipo_ataque == ATAQUE_AREA_1X3) {
                dentroAlcance = (alvoLinha != origem->linha || alvoColuna != origem->coluna) &&
                                abs(deltaLinha) <= 1 && abs(deltaColuna) <= 1;
            } else if (habilidade->tipo_ataque == ATAQUE_AREA_2X3) {
                dentroAlcance = abs(deltaLinha) <= 1 && abs(deltaColuna) <= 1;
            } else if (habilidade->tipo_ataque == ATAQUE_AREA_3X2) {
                dentroAlcance = habilidade->alcance >= 5 ||
                                ((permitePropriaCelula ? distancia >= 0 : distancia > 0) &&
                                 distancia <= habilidade->alcance);
            } else if (dentroAlcance && distancia > 1 &&
                       !habilidade->perfurante &&
                       habilidade->tipo_ataque != ATAQUE_DASH_CARDEAL &&
                       habilidade->tipo_ataque != ATAQUE_CRUZ &&
                       habilidade->tipo_ataque != ATAQUE_AREA_1X3) {
                int dirLin = (deltaLinha > 0) ? 1 : ((deltaLinha < 0) ? -1 : 0);
                int dirCol = (deltaColuna > 0) ? 1 : ((deltaColuna < 0) ? -1 : 0);
                int passo;
                for (passo = 1; passo < distancia; passo++) {
                    int celLin = origem->linha + passo * dirLin;
                    int celCol = origem->coluna + passo * dirCol;
                    Combatente *ocupante = mapa->celulas[celLin][celCol];
                    if (ocupante != NULL && ocupante->vivo &&
                        (ocupante->aliado != origem->aliado || habilidade->fogo_amigo)) {
                        dentroAlcance = false;
                        break;
                    }
                }
            }
            if (dentroAlcance) {
                alcancavel[alvoLinha][alvoColuna] = true;
                for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
                    for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                        Combatente *unidade = mapa->celulas[linha][coluna];
                        if (unidade != NULL && unidade->vivo &&
                            unidade != origem &&
                            unidade->aliado == origem->aliado &&
                            habilidade->efeito != EFEITO_CURA &&
                            habilidade->efeito != EFEITO_MOTIVACAO &&
                            habilidade->efeito != EFEITO_VENENO &&
                            celulaNaArea(habilidade, origem, alvoLinha, alvoColuna,
                                         linha, coluna)) {
                            perigoso[linha][coluna] = true;
                        }
                    }
                }
            }
        }
    }
    lista = calloc(1, sizeof(*lista));
    if (lista == NULL) {
        return NULL;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            if ((alcancavel[linha][coluna] || perigoso[linha][coluna]) &&
                !adicionarCelula(lista, linha, coluna, perigoso[linha][coluna] ? 1 : 0)) {
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
