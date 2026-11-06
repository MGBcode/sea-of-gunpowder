#include "dados_jogo.h"

#include "raylib.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

int inflacaoLojasPercentual = 0;
int moedasTripulacao = 300;
int contadorFugas = 0;
int contadorSaques = 0;

// Gera um número pseudoaleatório determinístico a partir do estado informado.
static unsigned int avancarSemente(unsigned int *estado)
{
    if (*estado == 0U) {
        *estado = 2463534242U;
    }
    *estado ^= *estado << 13;
    *estado ^= *estado >> 17;
    *estado ^= *estado << 5;
    return *estado;
}

// Preenche uma habilidade com seus parâmetros definitivos.
static void definirHabilidade(Habilidade *habilidade, const char *nome, int danoMin,
                              int danoMax, int precisao, int alcance, int custo,
                              TipoEfeito efeito, TipoAlcanceAtaque ataque)
{
    (void)strncpy(habilidade->nome, nome, sizeof(habilidade->nome) - 1);
    habilidade->nome[sizeof(habilidade->nome) - 1] = '\0';
    habilidade->dano_min = danoMin;
    habilidade->dano_max = danoMax;
    habilidade->precisao = precisao;
    habilidade->alcance = alcance;
    habilidade->custo_estamina = custo;
    habilidade->efeito = efeito;
    habilidade->tipo_ataque = ataque;
    habilidade->fogo_amigo = false;
    habilidade->perfurante = false;
    habilidade->duracao_efeito = 0;
}

// Inicializa os campos comuns de combatentes.
static void definirCombatente(Combatente *unidade, int id, const char *nome, int vida,
                              int estamina, int velocidade, int precisao, bool aliado)
{
    int indice;
    (void)memset(unidade, 0, sizeof(*unidade));
    unidade->id = id;
    (void)strncpy(unidade->nome, nome, sizeof(unidade->nome) - 1);
    unidade->nome[sizeof(unidade->nome) - 1] = '\0';
    unidade->vida_atual = vida;
    unidade->vida_maxima = vida;
    unidade->estamina_atual = estamina;
    unidade->estamina_maxima = estamina;
    unidade->velocidade = velocidade;
    unidade->precisao_melee = precisao;
    unidade->precisao_ranged = precisao;
    unidade->nivel = 1;
    unidade->aliado = aliado;
    unidade->vivo = true;
    unidade->direcao_coluna = aliado ? 1 : -1;
    unidade->habilidades_desbloqueadas = 1;
    for (indice = 0; indice < MAX_HABILIDADES; indice++) {
        unidade->turnos_desde_uso[indice] = -1;
    }
}

// Retorna o pirata jogável preenchido pelo índice solicitado.
bool criarPirataInicial(int indice, Combatente *destino)
{
    static const char *nomes[TOTAL_PIRATAS_PREDEFINIDOS] = {
        "Bucaneiro", "Timoneiro", "Cozinheiro", "Capitao", "Gavea"
    };
    static const int vidas[TOTAL_PIRATAS_PREDEFINIDOS] = { 150, 250, 200, 170, 120 };
    static const int velocidades[TOTAL_PIRATAS_PREDEFINIDOS] = { 9, 4, 6, 7, 5 };
    static const int precisao[TOTAL_PIRATAS_PREDEFINIDOS] = { 90, 60, 100, 70, 40 };
    static const int estamina[TOTAL_PIRATAS_PREDEFINIDOS][6] = {
        { 5, 7, 7, 10, 10, 10 },
        { 5, 6, 6, 7, 7, 7 },
        { 5, 6, 6, 8, 8, 8 },
        { 5, 6, 6, 7, 7, 7 },
        { 5, 6, 6, 7, 7, 7 }
    };
    int nivel;
    if (destino == NULL || indice < 0 || indice >= TOTAL_PIRATAS_PREDEFINIDOS) {
        return false;
    }
    definirCombatente(destino, indice + 1, nomes[indice], vidas[indice],
                      estamina[indice][0], velocidades[indice], precisao[indice], true);
    destino->classe = (ClassePirata)indice;
    destino->dano_base = 0;
    for (nivel = 0; nivel < 6; nivel++) {
        destino->estamina_por_nivel[nivel] = estamina[indice][nivel];
    }
    if (indice == CLASSE_BUCANEIRO) {
        definirHabilidade(&destino->habilidades[0], "Corte Sangrento", 60, 90, 90, 1, 2,
                          EFEITO_SANGRAMENTO, ATAQUE_MELEE);
        definirHabilidade(&destino->habilidades[1], "Bolsa de Areia", 0, 0, 90, 1, 4,
                          EFEITO_ESTUNAR, ATAQUE_AREA_1X3);
        definirHabilidade(&destino->habilidades[2], "Avanco Impetuoso", 120, 150, 90, 2, 6,
                          EFEITO_DANO, ATAQUE_DASH_CARDEAL);
        destino->habilidades[0].duracao_efeito = 3;
        destino->habilidades[1].duracao_efeito = 1;
    } else if (indice == CLASSE_TIMONEIRO) {
        definirHabilidade(&destino->habilidades[0], "Pancada", 70, 100, 60, 1, 2,
                          EFEITO_DANO, ATAQUE_MELEE);
        definirHabilidade(&destino->habilidades[1], "Esmagar", 90, 120, 60, 1, 3,
                          EFEITO_ESTUNAR, ATAQUE_MELEE);
        definirHabilidade(&destino->habilidades[2], "Giro do Timao", 50, 85, 60, 1, 4,
                          EFEITO_DESNORTEAR, ATAQUE_AREA_1X3);
        destino->habilidades[1].duracao_efeito = 1;
        destino->habilidades[2].duracao_efeito = 2;
    } else if (indice == CLASSE_COZINHEIRO) {
        definirHabilidade(&destino->habilidades[0], "Comida da Boa", 0, 35, 100, 2, 2,
                          EFEITO_CURA, ATAQUE_RANGED);
        definirHabilidade(&destino->habilidades[1], "Bacamarte", 60, 100, 100, 2, 3,
                          EFEITO_DANO, ATAQUE_CONE);
        definirHabilidade(&destino->habilidades[2], "Hora do Rango", 0, 70, 100, 2, 6,
                          EFEITO_CURA, ATAQUE_AREA_2X3);
    } else if (indice == CLASSE_CAPITAO) {
        destino->precisao_ranged = 60;
        definirHabilidade(&destino->habilidades[0], "Saquear", 75, 100, 70, 2, 2,
                          EFEITO_DANO, ATAQUE_HIBRIDO_SAQUEAR);
        definirHabilidade(&destino->habilidades[1], "Discursar", 0, 0, 70, 2, 3,
                          EFEITO_MOTIVACAO, ATAQUE_RANGED);
        definirHabilidade(&destino->habilidades[2], "Enxurrada de Canhoes", 100, 130, 70, 5, 6,
                          EFEITO_BOMBA, ATAQUE_AREA_3X2);
        destino->habilidades[1].duracao_efeito = 2;
        destino->habilidades[2].fogo_amigo = true;
    } else {
        definirHabilidade(&destino->habilidades[0], "Disparo de Longa Distancia", 60, 110,
                          40, 3, 2, EFEITO_DANO, ATAQUE_LINHA);
        definirHabilidade(&destino->habilidades[1], "Zarabatana", 0, 0, 40, 2, 1,
                          EFEITO_VENENO, ATAQUE_RANGED);
        definirHabilidade(&destino->habilidades[2], "Bala Perfurante Especial", 150, 200,
                          40, 6, 4, EFEITO_DANO, ATAQUE_LINHA);
        destino->habilidades[1].duracao_efeito = 2;
        destino->habilidades[2].fogo_amigo = true;
        destino->habilidades[2].perfurante = true;
    }
    return true;
}

// Retorna um inimigo comum preenchido pelo índice solicitado.
bool criarInimigoInicial(int indice, Combatente *destino)
{
    static const char *nomes[TOTAL_INIMIGOS_PREDEFINIDOS] = {
        "Grunt com Espada", "Grunt com Pistola", "Atirador Magrelo", "Marujo Gordo"
    };
    static const int vidas[TOTAL_INIMIGOS_PREDEFINIDOS] = { 200, 160, 120, 300 };
    static const int estaminas[3][TOTAL_INIMIGOS_PREDEFINIDOS] = {
        { 5, 4, 4, 5 }, { 7, 5, 5, 6 }, { 9, 6, 6, 7 }
    };
    static const int velocidades[TOTAL_INIMIGOS_PREDEFINIDOS] = { 7, 6, 6, 4 };
    static const int precisao[TOTAL_INIMIGOS_PREDEFINIDOS] = { 80, 60, 35, 80 };
    int nivel;
    if (destino == NULL || indice < 0 || indice >= TOTAL_INIMIGOS_PREDEFINIDOS) {
        return false;
    }
    definirCombatente(destino, 100 + indice, nomes[indice], vidas[indice],
                      estaminas[0][indice], velocidades[indice], precisao[indice], false);
    destino->tipo_inimigo = (TipoInimigo)indice;
    destino->habilidades_desbloqueadas = (indice == INIMIGO_GRUNT_PISTOLA ||
                                          indice == INIMIGO_MARUJO_GORDO) ? 2 : 1;
    for (nivel = 0; nivel < 3; nivel++) {
        destino->estamina_por_nivel[nivel] = estaminas[nivel][indice];
    }
    if (indice == INIMIGO_GRUNT_ESPADA) {
        definirHabilidade(&destino->habilidades[0], "Corte", 35, 50, 80, 1, 2,
                          EFEITO_SANGRAMENTO, ATAQUE_MELEE);
        destino->habilidades[0].duracao_efeito = 3;
    } else if (indice == INIMIGO_GRUNT_PISTOLA) {
        definirHabilidade(&destino->habilidades[0], "Tiro", 20, 30, 60, 3, 1,
                          EFEITO_DANO, ATAQUE_LINHA);
        definirHabilidade(&destino->habilidades[1], "Granada", 40, 55, 60, 2, 4,
                          EFEITO_BOMBA, ATAQUE_CRUZ);
        destino->habilidades[0].fogo_amigo = true;
        destino->habilidades[1].fogo_amigo = true;
    } else if (indice == INIMIGO_ATIRADOR_MAGRELO) {
        definirHabilidade(&destino->habilidades[0], "Disparo Pesado", 25, 60, 35, 4, 2,
                          EFEITO_DANO, ATAQUE_LINHA);
    } else {
        definirHabilidade(&destino->habilidades[0], "Escopeta", 50, 70, 80, 2, 3,
                          EFEITO_DANO, ATAQUE_LINHA);
        definirHabilidade(&destino->habilidades[1], "Bandagens", 15, 15, 80, 1, 1,
                          EFEITO_CURA_CONTINUA, ATAQUE_RANGED);
        destino->habilidades[1].duracao_efeito = 3;
    }
    return true;
}

// Retorna Barba Negra preenchido com seus status iniciais.
bool criarChefeFinal(Combatente *destino)
{
    if (destino == NULL) {
        return false;
    }
    definirCombatente(destino, 999, "Barba Negra", 1500, 15, 8, 90, false);
    destino->eh_boss = true;
    destino->fase_boss = 1;
    destino->tipo_inimigo = INIMIGO_BOSS_BARBA_NEGRA;
    destino->precisao_ranged = 60;
    destino->nivel = 6;
    destino->estamina_por_nivel[0] = 15;
    destino->estamina_por_nivel[1] = 20;
    destino->estamina_maxima = 15;
    destino->estamina_atual = 15;
    destino->habilidades_desbloqueadas = 3;
    definirHabilidade(&destino->habilidades[0], "Ruina dos Covardes", 60, 80, 90, 1, 2,
                      EFEITO_DANO, ATAQUE_MELEE);
    definirHabilidade(&destino->habilidades[1], "Destruidor de Familias", 40, 55, 60, 3, 2,
                      EFEITO_DANO, ATAQUE_RANGED);
    definirHabilidade(&destino->habilidades[2], "Tirano dos Mares", 50, 70, 60, 5, 13,
                      EFEITO_BOMBA, ATAQUE_AREA_3X2);
    return true;
}

// Copia os onze itens iniciais para o vetor informado.
int carregarItensIniciais(Item itens[], int capacidade)
{
    static const Item catalogo[TOTAL_ITENS_PREDEFINIDOS] = {
        { 1, "Bandagem", ITEM_CONSUMIVEL, RARIDADE_COMUM, 1, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, CONSUMIVEL_CURAR_VIDA, 40 },
        { 2, "Tonico Enfurecedor", ITEM_CONSUMIVEL, RARIDADE_COMUM, 1, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, CONSUMIVEL_TONICO, 2 },
        { 3, "Travesseiro", ITEM_CONSUMIVEL, RARIDADE_COMUM, 1, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, CONSUMIVEL_RESTAURAR_ESTAMINA, 100 },
        { 4, "Livro de Origem", ITEM_CONSUMIVEL, RARIDADE_EPICO, 1, 500, 0, 0, 0, 0, 0, 0, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 2 },
        { 5, "Colete de Couro", ITEM_PASSIVO, RARIDADE_RARO, 1, 250, 0, 0, 0, 0, 0, 15, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 6, "Pedra de Amolar", ITEM_PASSIVO, RARIDADE_RARO, 1, 250, 0, 0, 0, 0, 0, 0, 20, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 7, "Lentes de Contato", ITEM_PASSIVO, RARIDADE_RARO, 1, 250, 0, 0, 0, 0, 25, 0, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 8, "Botas Leves", ITEM_PASSIVO, RARIDADE_RARO, 1, 250, 0, 0, 0, 0, 0, 0, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 9, "Balas Perfurantes", ITEM_PASSIVO, RARIDADE_EPICO, 1, 500, 0, 0, 0, 0, 0, 0, 0, 35, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 10, "Colar dos Mares", ITEM_PASSIVO, RARIDADE_EPICO, 1, 500, 0, 0, 0, 2, 0, 0, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 11, "Rum de Poseidon", ITEM_PASSIVO, RARIDADE_LENDARIO, 1, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 1, CONSUMIVEL_SEM_EFEITO, 0 }
    };
    int total;
    if (itens == NULL || capacidade <= 0) {
        return 0;
    }
    total = capacidade < TOTAL_ITENS_PREDEFINIDOS ? capacidade : TOTAL_ITENS_PREDEFINIDOS;
    (void)memcpy(itens, catalogo, (size_t)total * sizeof(*itens));
    return total;
}

// Evolui níveis aplicando ganho de +20% de vida e +20% de dano de ataque por nível.
bool subirNivelPirata(Combatente *unidade, int niveisGanhos)
{
    int passo;
    int bonusColar = 0;
    int habIndice;
    NoItem *item;
    if (unidade == NULL || niveisGanhos < 0 || unidade->nivel < 1) {
        return false;
    }
    for (item = unidade->build_passivos.inicio; item != NULL; item = item->proximo) {
        if (item->item.id == 10) {
            bonusColar += 2;
        }
    }
    for (passo = 0; passo < niveisGanhos; passo++) {
        int aumentoVida;
        int nivelNovo;
        int indiceTabela;
        int estaminaNova;
        int estaminaBonus;
        if (unidade->nivel >= 6) {
            break;
        }
        aumentoVida = (unidade->vida_maxima * 20) / 100;
        if (aumentoVida < 1) {
            aumentoVida = 1;
        }
        nivelNovo = unidade->nivel + 1;
        indiceTabela = nivelNovo - 1;
        estaminaNova = unidade->estamina_por_nivel[indiceTabela] + bonusColar;
        unidade->nivel = nivelNovo;
        unidade->vida_maxima += aumentoVida;
        unidade->vida_atual += aumentoVida;
        if (unidade->vida_atual > unidade->vida_maxima) {
            unidade->vida_atual = unidade->vida_maxima;
        }
        unidade->dano_base += 20;
        for (habIndice = 0; habIndice < MAX_HABILIDADES; habIndice++) {
            if (unidade->habilidades[habIndice].efeito != EFEITO_CURA &&
                unidade->habilidades[habIndice].efeito != EFEITO_CURA_CONTINUA) {
                if (unidade->habilidades[habIndice].dano_min > 0) {
                    unidade->habilidades[habIndice].dano_min +=
                        (unidade->habilidades[habIndice].dano_min * 20) / 100;
                }
                if (unidade->habilidades[habIndice].dano_max > 0) {
                    unidade->habilidades[habIndice].dano_max +=
                        (unidade->habilidades[habIndice].dano_max * 20) / 100;
                }
            }
        }
        estaminaBonus = estaminaNova - unidade->estamina_maxima;
        if (estaminaBonus > 0) {
            unidade->estamina_maxima = estaminaNova;
            unidade->estamina_atual += estaminaBonus;
        } else {
            unidade->estamina_maxima = estaminaNova;
        }
        if (unidade->estamina_atual > unidade->estamina_maxima) {
            unidade->estamina_atual = unidade->estamina_maxima;
        }
        if (unidade->nivel >= 3 && unidade->habilidades_desbloqueadas < 2) {
            unidade->habilidades_desbloqueadas = 2;
        }
        if (unidade->nivel >= 4) {
            unidade->passiva_aprimorada = true;
        }
        if (unidade->nivel >= 6) {
            unidade->habilidades_desbloqueadas = 3;
        }
    }
    return true;
}

// Converte XP acumulado em níveis ganhos.
bool ganharExperienciaEvoluir(Combatente *unidade, int experiencia)
{
    int niveis = 0;
    if (unidade == NULL || experiencia < 0 || unidade->nivel < 1) {
        return false;
    }
    unidade->experiencia += experiencia;
    while (unidade->nivel + niveis < 6 &&
           unidade->experiencia >= (unidade->nivel + niveis) * 100) {
        unidade->experiencia -= (unidade->nivel + niveis) * 100;
        niveis++;
    }
    return subirNivelPirata(unidade, niveis);
}

// Adiciona uma cópia de item ao inventário da tripulação.
static bool adicionarItemInventario(ListaItens *inventario, int id, int quantidade)
{
    Item catalogo[TOTAL_ITENS_PREDEFINIDOS];
    NoItem *atual;
    NoItem *novo;
    int total;
    int indice;
    int copia;
    if (inventario == NULL || quantidade <= 0) {
        return false;
    }
    total = carregarItensIniciais(catalogo, TOTAL_ITENS_PREDEFINIDOS);
    for (indice = 0; indice < total && catalogo[indice].id != id; indice++) {
    }
    if (indice == total) {
        return false;
    }
    if (catalogo[indice].tipo == ITEM_CONSUMIVEL) {
        for (atual = inventario->inicio; atual != NULL; atual = atual->proximo) {
            if (atual->item.id == id) {
                atual->item.quantidade += quantidade;
                return true;
            }
        }
        novo = malloc(sizeof(*novo));
        if (novo == NULL) {
            return false;
        }
        novo->item = catalogo[indice];
        novo->item.quantidade = quantidade;
        novo->proximo = inventario->inicio;
        inventario->inicio = novo;
        inventario->tamanho++;
        return true;
    }
    for (copia = 0; copia < quantidade; copia++) {
        novo = malloc(sizeof(*novo));
        if (novo == NULL) {
            return false;
        }
        novo->item = catalogo[indice];
        novo->item.quantidade = 1;
        novo->proximo = inventario->inicio;
        inventario->inicio = novo;
        inventario->tamanho++;
    }
    return true;
}

// Sorteia um ID de item conforme a tabela de probabilidades de recompensa de combate.
static int sortearIdRecompensaCombate(unsigned int *semente)
{
    static const int itensComuns[3] = { 1, 2, 3 };
    static const int itensRaros[4] = { 5, 6, 7, 8 };
    static const int itensEpicos[3] = { 4, 9, 10 };
    int rolagem = (int)(avancarSemente(semente) % 100U);
    if (rolagem < 40) {
        return itensComuns[avancarSemente(semente) % 3U];
    }
    if (rolagem < 70) {
        return itensRaros[avancarSemente(semente) % 4U];
    }
    if (rolagem < 90) {
        return itensEpicos[avancarSemente(semente) % 3U];
    }
    return 11;
}

// Aplica evolução, moedas, cura e itens sorteados conforme a rota vencida.
bool aplicarRecompensaRota(TipoNoRota tipoRota, Combatente tripulacao[],
                           int totalTripulacao, ListaItens *inventario, int xpBase)
{
    static unsigned int sementeRecompensa = 987654321U;
    int niveis;
    int cura;
    int quantidadeItens;
    int indice;
    if (tripulacao == NULL || totalTripulacao <= 0 || inventario == NULL || xpBase < 0) {
        return false;
    }
    sementeRecompensa ^= (unsigned int)(moedasTripulacao * 31 + contadorSaques * 17 +
                                        contadorFugas * 13 + xpBase + 1);
    if (tipoRota == ROTA_CAMINHO_1) {
        niveis = 2;
        moedasTripulacao += 300;
        cura = 15;
        quantidadeItens = 1;
    } else if (tipoRota == ROTA_CAMINHO_2) {
        niveis = 1;
        moedasTripulacao += 700;
        cura = 50;
        quantidadeItens = 2;
    } else if (tipoRota == ROTA_LOJA) {
        niveis = 0;
        cura = 0;
        quantidadeItens = 2;
        inflacaoLojasPercentual += 25;
        contadorSaques++;
    } else {
        return false;
    }
    for (indice = 0; indice < totalTripulacao; indice++) {
        Combatente *pirata = &tripulacao[indice];
        if (!pirata->aliado || !pirata->vivo) {
            continue;
        }
        if (niveis > 0) {
            if (!subirNivelPirata(pirata, niveis)) {
                return false;
            }
            pirata->experiencia += xpBase;
        }
        if (cura > 0) {
            pirata->vida_atual += (pirata->vida_maxima * cura) / 100;
            if (pirata->vida_atual > pirata->vida_maxima) {
                pirata->vida_atual = pirata->vida_maxima;
            }
        }
    }
    for (indice = 0; indice < quantidadeItens; indice++) {
        int idSorteado = sortearIdRecompensaCombate(&sementeRecompensa);
        if (!adicionarItemInventario(inventario, idSorteado, 1)) {
            return false;
        }
    }
    return true;
}

// Ajusta o preço conforme a inflação cumulativa de +25% por saque na loja.
int aplicarInflacaoLoja(int preco)
{
    int precoAjustado = preco;
    int totalSaques = contadorSaques;
    int indiceSaque;
    if (preco <= 0) {
        return preco;
    }
    if (totalSaques <= 0 && inflacaoLojasPercentual > 0) {
        totalSaques = inflacaoLojasPercentual / 25;
    }
    for (indiceSaque = 0; indiceSaque < totalSaques; indiceSaque++) {
        precoAjustado = precoAjustado + (precoAjustado * 25) / 100;
    }
    return precoAjustado;
}

// Multiplica saúde, dano-base e faixas de habilidade por um percentual inteiro.
static void aplicarMultiplicadorInimigo(Combatente *inimigo, int fator)
{
    int indice;
    inimigo->vida_maxima = (inimigo->vida_maxima * fator) / 100;
    inimigo->vida_atual = (inimigo->vida_atual * fator) / 100;
    inimigo->dano_base = (inimigo->dano_base * fator) / 100;
    for (indice = 0; indice < MAX_HABILIDADES; indice++) {
        if (inimigo->habilidades[indice].efeito != EFEITO_CURA_CONTINUA) {
            inimigo->habilidades[indice].dano_min =
                (inimigo->habilidades[indice].dano_min * fator) / 100;
            inimigo->habilidades[indice].dano_max =
                (inimigo->habilidades[indice].dano_max * fator) / 100;
        }
    }
}

// Aplica o multiplicador de dificuldade da run.
void escalonarInimigoPorDificuldade(Combatente *inimigo, int nivelDificuldade)
{
    int rotaIndice;
    int multRota = contadorFugas > 0 ? 150 : 125;
    if (inimigo == NULL || nivelDificuldade <= 1) {
        return;
    }
    for (rotaIndice = 1; rotaIndice < nivelDificuldade; rotaIndice++) {
        aplicarMultiplicadorInimigo(inimigo, multRota);
    }
}

// Aplica multiplicadores de rota, fuga e saque usando matemática 100% inteira.
void escalonarInimigoPorRota(Combatente *inimigo, TipoNoRota tipoRota, int etapa,
                             bool houveFuga, bool saqueouLoja)
{
    int rotaIndice;
    int multRota = houveFuga ? 150 : 125;
    (void)tipoRota;
    if (inimigo == NULL) {
        return;
    }
    if (inimigo->eh_boss) {
        if (houveFuga) {
            aplicarMultiplicadorInimigo(inimigo, 150);
        }
        return;
    }
    if (etapa >= 1 && etapa <= 3) {
        inimigo->estamina_maxima = inimigo->estamina_por_nivel[etapa - 1];
        inimigo->estamina_atual = inimigo->estamina_maxima;
    }
    for (rotaIndice = 1; rotaIndice < etapa; rotaIndice++) {
        aplicarMultiplicadorInimigo(inimigo, multRota);
    }
    if (saqueouLoja) {
        aplicarMultiplicadorInimigo(inimigo, 200);
    }
}

// Sorteia inimigos obedecendo as regras obrigatórias da etapa (Rota 1, 2 ou 3) em O(1).
int sortearInimigosPorEtapa(TipoNoRota tipoRota, int etapaRota, unsigned int semente,
                            int indices[], int capacidade)
{
    unsigned int estado = semente == 0U ? 1U : semente;
    int quantidade = tipoRota == ROTA_CAMINHO_2 ? 3 : 4;
    int maxTipoPermitido;
    int qtdPorTipo[4] = { 0, 0, 0, 0 };
    int slotInicial = 0;
    int slot;
    if (indices == NULL || capacidade < quantidade ||
        (tipoRota != ROTA_CAMINHO_1 && tipoRota != ROTA_CAMINHO_2 &&
         tipoRota != ROTA_LOJA)) {
        return 0;
    }
    if (etapaRota <= 1) {
        maxTipoPermitido = 2;
    } else if (etapaRota == 2) {
        maxTipoPermitido = 3;
        indices[0] = INIMIGO_ATIRADOR_MAGRELO;
        qtdPorTipo[INIMIGO_ATIRADOR_MAGRELO]++;
        slotInicial = 1;
    } else {
        maxTipoPermitido = 4;
        indices[0] = INIMIGO_MARUJO_GORDO;
        qtdPorTipo[INIMIGO_MARUJO_GORDO]++;
        slotInicial = 1;
    }
    for (slot = slotInicial; slot < quantidade; slot++) {
        int candidatos[4];
        int totalCandidatos = 0;
        int tipo;
        int escolhido;
        for (tipo = 0; tipo < maxTipoPermitido; tipo++) {
            if (qtdPorTipo[tipo] < 2) {
                candidatos[totalCandidatos++] = tipo;
            }
        }
        if (totalCandidatos == 0) {
            return slot;
        }
        escolhido = candidatos[avancarSemente(&estado) % (unsigned int)totalCandidatos];
        indices[slot] = escolhido;
        qtdPorTipo[escolhido]++;
    }
    return quantidade;
}

// Gera inimigos determinísticos para a rota e retorna quantos foram criados.
int sortearInimigosRota(TipoNoRota tipoRota, unsigned int semente,
                        int indices[], int capacidade)
{
    int etapaInferida = 1;
    if (tipoRota == ROTA_CAMINHO_2) {
        etapaInferida = 2;
    } else if (tipoRota == ROTA_LOJA) {
        etapaInferida = 3;
    }
    return sortearInimigosPorEtapa(tipoRota, etapaInferida, semente, indices, capacidade);
}

// Sorteia os 3 itens da loja da rota com 2 unidades de estoque cada.
int sortearItensLoja(int etapaRota, unsigned int semente, Item itensLoja[],
                     int estoques[], int capacidade)
{
    Item catalogo[TOTAL_ITENS_PREDEFINIDOS];
    unsigned int estado = semente == 0U ? 777U : semente;
    int totalCatalogo;
    int usados[TOTAL_ITENS_PREDEFINIDOS] = { 0 };
    int slotInicial = 0;
    int slot;
    if (itensLoja == NULL || estoques == NULL || capacidade < 3) {
        return 0;
    }
    totalCatalogo = carregarItensIniciais(catalogo, TOTAL_ITENS_PREDEFINIDOS);
    if (totalCatalogo < TOTAL_ITENS_PREDEFINIDOS) {
        return 0;
    }
    if (etapaRota >= 3) {
        itensLoja[0] = catalogo[10];
        estoques[0] = 2;
        usados[10] = 1;
        slotInicial = 1;
    }
    for (slot = slotInicial; slot < 3; slot++) {
        int rolagem = (int)(avancarSemente(&estado) % 100U);
        RaridadeItem raridadeAlvo;
        int candidatos[TOTAL_ITENS_PREDEFINIDOS];
        int totalCandidatos = 0;
        int indice;
        int indiceCatalogo;
        if (rolagem < 30) {
            raridadeAlvo = RARIDADE_COMUM;
        } else if (rolagem < 70) {
            raridadeAlvo = RARIDADE_RARO;
        } else {
            raridadeAlvo = RARIDADE_EPICO;
        }
        for (indice = 0; indice < totalCatalogo; indice++) {
            if (!usados[indice] && catalogo[indice].raridade == raridadeAlvo) {
                candidatos[totalCandidatos++] = indice;
            }
        }
        if (totalCandidatos == 0) {
            for (indice = 0; indice < totalCatalogo - 1; indice++) {
                if (!usados[indice]) {
                    candidatos[totalCandidatos++] = indice;
                }
            }
        }
        indiceCatalogo = candidatos[avancarSemente(&estado) % (unsigned int)totalCandidatos];
        usados[indiceCatalogo] = 1;
        itensLoja[slot] = catalogo[indiceCatalogo];
        estoques[slot] = 2;
    }
    return 3;
}

// Processa a compra de uma unidade de um slot da loja.
bool comprarItemNaLoja(Item itensLoja[], int estoques[], int totalLoja, int indiceSlot,
                       ListaItens *inventario, int *moedas)
{
    int precoFinal;
    if (itensLoja == NULL || estoques == NULL || inventario == NULL || moedas == NULL ||
        indiceSlot < 0 || indiceSlot >= totalLoja || estoques[indiceSlot] <= 0) {
        return false;
    }
    precoFinal = aplicarInflacaoLoja(itensLoja[indiceSlot].preco);
    if (*moedas < precoFinal) {
        return false;
    }
    if (!adicionarItemInventario(inventario, itensLoja[indiceSlot].id, 1)) {
        return false;
    }
    *moedas -= precoFinal;
    estoques[indiceSlot]--;
    return true;
}

// Transfere gratuitamente todo o estoque restante da loja após vencer um saque.
bool saquearItensRestantesLoja(const Item itensLoja[], int estoques[], int totalLoja,
                               ListaItens *inventario)
{
    int slot;
    if (itensLoja == NULL || estoques == NULL || inventario == NULL || totalLoja <= 0) {
        return false;
    }
    for (slot = 0; slot < totalLoja; slot++) {
        while (estoques[slot] > 0) {
            if (!adicionarItemInventario(inventario, itensLoja[slot].id, 1)) {
                return false;
            }
            estoques[slot]--;
        }
    }
    contadorSaques++;
    inflacaoLojasPercentual = contadorSaques * 25;
    return true;
}

// Libera os nós da build passiva do combatente.
void liberarListaItensCombatente(ListaItens *lista)
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
