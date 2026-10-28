#include "dados_jogo.h"

#include <stdlib.h>
#include <string.h>

int inflacaoLojasPercentual = 0;

// Preenche uma habilidade com os valores fornecidos.
static void definirHabilidade(Habilidade *habilidade, const char *nome, int dano, int alcance,
                              int custo, int critico, int raio, TipoEfeito efeito)
{
    (void)strncpy(habilidade->nome, nome, sizeof(habilidade->nome) - 1);
    habilidade->nome[sizeof(habilidade->nome) - 1] = '\0';
    habilidade->dano = dano;
    habilidade->alcance = alcance;
    habilidade->custo_estamina = custo;
    habilidade->chance_critico = critico;
    habilidade->raio_area = raio;
    habilidade->efeito = efeito;
}

// Preenche os dados comuns de um combatente.
static void definirCombatente(Combatente *unidade, int id, const char *nome, int vida,
                              int estamina, int velocidade, int defesa, int dano, bool aliado)
{
    memset(unidade, 0, sizeof(*unidade));
    unidade->id = id;
    (void)strncpy(unidade->nome, nome, sizeof(unidade->nome) - 1);
    unidade->nome[sizeof(unidade->nome) - 1] = '\0';
    unidade->vida_atual = vida;
    unidade->vida_maxima = vida;
    unidade->estamina_atual = estamina;
    unidade->estamina_maxima = estamina;
    unidade->velocidade = velocidade;
    unidade->defesa = defesa;
    unidade->dano_base = dano;
    unidade->nivel = 1;
    unidade->aliado = aliado;
    unidade->vivo = true;
    unidade->habilidades_desbloqueadas = 1;
    definirHabilidade(&unidade->habilidades[0], "Ataque basico", 0, 3, 1, 10, 0, EFEITO_DANO);
    definirHabilidade(&unidade->habilidades[1], "Habilidade Nv 3", 5, 3, 2, 15, 0, EFEITO_DANO);
    definirHabilidade(&unidade->habilidades[2], "Habilidade Nv 6", 8, 4, 3, 20, 1, EFEITO_BOMBA);
    unidade->turnos_desde_uso[0] = -1;
    unidade->turnos_desde_uso[1] = -1;
    unidade->turnos_desde_uso[2] = -1;
}

// Retorna um pirata inicial preenchido pelo índice solicitado.
bool criarPirataInicial(int indice, Combatente *destino)
{
    static const char *nomes[TOTAL_PIRATAS_PREDEFINIDOS] = {
        "Capitao Barba Rubra", "Mira Tempestade", "Tomas Mosquete",
        "Nina Remendo", "Baltazar Gancho"
    };
    static const int vidas[TOTAL_PIRATAS_PREDEFINIDOS] = { 120, 85, 95, 110, 140 };
    static const int estaminas[TOTAL_PIRATAS_PREDEFINIDOS] = { 5, 6, 5, 5, 4 };
    static const int velocidades[TOTAL_PIRATAS_PREDEFINIDOS] = { 5, 8, 7, 4, 3 };
    static const int defesas[TOTAL_PIRATAS_PREDEFINIDOS] = { 4, 2, 2, 3, 6 };
    static const int danos[TOTAL_PIRATAS_PREDEFINIDOS] = { 12, 9, 10, 7, 15 };
    if (destino == NULL || indice < 0 || indice >= TOTAL_PIRATAS_PREDEFINIDOS) {
        return false;
    }
    definirCombatente(destino, indice + 1, nomes[indice], vidas[indice], estaminas[indice],
                      velocidades[indice], defesas[indice], danos[indice], true);
    destino->ganho_estamina_nivel = estaminas[indice] >= 5 ? 2 : 1;
    definirHabilidade(&destino->habilidades[1], "Disparo certeiro", 7, 4, 2, 25, 0, EFEITO_DANO);
    destino->habilidades[1].disparo = true;
    definirHabilidade(&destino->habilidades[2], "Bomba de estilhacos", 12, 3, 3, 10, 1, EFEITO_BOMBA);
    return true;
}

// Retorna um inimigo inicial preenchido pelo índice solicitado.
bool criarInimigoInicial(int indice, Combatente *destino)
{
    static const char *nomes[TOTAL_INIMIGOS_PREDEFINIDOS] = {
        "Marinheiro da Coroa", "Corsario", "Atirador Naval", "Bruto do Porto"
    };
    static const int vidas[TOTAL_INIMIGOS_PREDEFINIDOS] = { 60, 75, 55, 100 };
    static const int estaminas[TOTAL_INIMIGOS_PREDEFINIDOS] = { 4, 5, 5, 3 };
    static const int velocidades[TOTAL_INIMIGOS_PREDEFINIDOS] = { 4, 6, 7, 2 };
    static const int defesas[TOTAL_INIMIGOS_PREDEFINIDOS] = { 1, 2, 0, 5 };
    static const int danos[TOTAL_INIMIGOS_PREDEFINIDOS] = { 7, 9, 8, 14 };
    if (destino == NULL || indice < 0 || indice >= TOTAL_INIMIGOS_PREDEFINIDOS) {
        return false;
    }
    definirCombatente(destino, 100 + indice, nomes[indice], vidas[indice], estaminas[indice],
                      velocidades[indice], defesas[indice], danos[indice], false);
    return true;
}

// Escalona os atributos de um inimigo conforme a dificuldade da run.
void escalonarInimigoPorDificuldade(Combatente *inimigo, int nivelDificuldade)
{
    int bonus;
    int aumentoVidaMaxima;
    int aumentoVidaAtual;
    int aumentoDano;
    if (inimigo == NULL || nivelDificuldade <= 1) {
        return;
    }
    bonus = nivelDificuldade - 1;
    aumentoVidaMaxima = (inimigo->vida_maxima * 15 * bonus) / 100;
    aumentoVidaAtual = (inimigo->vida_atual * 15 * bonus) / 100;
    aumentoDano = (inimigo->dano_base * 15 * bonus) / 100;
    if (aumentoVidaMaxima < 5 * bonus) {
        aumentoVidaMaxima = 5 * bonus;
    }
    if (aumentoVidaAtual < 5 * bonus) {
        aumentoVidaAtual = 5 * bonus;
    }
    if (aumentoDano < 2 * bonus) {
        aumentoDano = 2 * bonus;
    }
    inimigo->vida_maxima += aumentoVidaMaxima;
    inimigo->vida_atual += aumentoVidaAtual;
    if (inimigo->vida_atual > inimigo->vida_maxima) {
        inimigo->vida_atual = inimigo->vida_maxima;
    }
    inimigo->dano_base += aumentoDano;
}

// Retorna um mercador armado para um assalto à loja.
bool criarMercadorLoja(int indice, Combatente *destino, int nivelDificuldade)
{
    static const char *nomes[] = {
        "Mercador da Costa", "Mercador do Porto", "Mercador da Coroa"
    };
    if (destino == NULL || indice < 0 || indice >= 3) {
        return false;
    }
    definirCombatente(destino, 300 + indice, nomes[indice], 180, 7, 6, 5, 18, false);
    destino->habilidades_desbloqueadas = 2;
    definirHabilidade(&destino->habilidades[1], "Mosquete do mercador", 12, 4, 2, 15, 0,
                      EFEITO_DANO);
    destino->habilidades[1].disparo = true;
    escalonarInimigoPorDificuldade(destino, nivelDificuldade);
    return true;
}

// Retorna o chefe final preenchido com seus status iniciais.
bool criarChefeFinal(Combatente *destino)
{
    if (destino == NULL) {
        return false;
    }
    definirCombatente(destino, 999, "Almirante do Abismo", 420, 8, 6, 8, 22, false);
    destino->nivel = 6;
    destino->habilidades_desbloqueadas = 3;
    definirHabilidade(&destino->habilidades[1], "Canhao da Coroa", 18, 5, 3, 20, 0, EFEITO_DANO);
    destino->habilidades[1].disparo = true;
    definirHabilidade(&destino->habilidades[2], "Bombardeio final", 25, 4, 4, 15, 2, EFEITO_BOMBA);
    return true;
}

// Copia os doze itens iniciais para o vetor informado.
int carregarItensIniciais(Item itens[], int capacidade)
{
    static const Item catalogo[TOTAL_ITENS_PREDEFINIDOS] = {
        { 1, "Tonico de vida", ITEM_CONSUMIVEL, 1, 15, 0, 0, 0, CONSUMIVEL_CURAR_VIDA, 30 },
        { 2, "Racao salgada", ITEM_CONSUMIVEL, 1, 10, 0, 0, 0, CONSUMIVEL_RESTAURAR_ESTAMINA, 3 },
        { 3, "Bomba de bolso", ITEM_CONSUMIVEL, 1, 20, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 4, "Antidoto", ITEM_CONSUMIVEL, 1, 12, 0, 0, 0, CONSUMIVEL_REMOVER_STATUS, 0 },
        { 5, "Bota do marujo", ITEM_PASSIVO, 1, 25, 0, 1, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 6, "Colete reforcado", ITEM_PASSIVO, 1, 30, 10, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 7, "Luneta de prata", ITEM_PASSIVO, 1, 35, 0, 0, 2, CONSUMIVEL_SEM_EFEITO, 0 },
        { 8, "Anel da sorte", ITEM_PASSIVO, 1, 40, 0, 0, 1, CONSUMIVEL_SEM_EFEITO, 0 },
        { 9, "Cinto de estamina", ITEM_PASSIVO, 1, 28, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 10, "Mapa secreto", ITEM_PASSIVO, 1, 45, 5, 1, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 11, "Mosquete pesado", ITEM_ATIVO, 1, 55, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 },
        { 12, "Sabre de mare", ITEM_ATIVO, 1, 50, 0, 0, 0, CONSUMIVEL_SEM_EFEITO, 0 }
    };
    int total = capacidade < TOTAL_ITENS_PREDEFINIDOS ? capacidade : TOTAL_ITENS_PREDEFINIDOS;
    int indice;
    if (itens == NULL || capacidade <= 0) {
        return 0;
    }
    for (indice = 0; indice < total; indice++) {
        itens[indice] = catalogo[indice];
    }
    return total;
}

// Adiciona experiência e aplica os atributos dos níveis 1 a 6.
bool ganharExperienciaEvoluir(Combatente *unidade, int experiencia)
{
    int aumentoVida;
    int aumentoDano;
    if (unidade == NULL || experiencia < 0 || unidade->nivel < 1 || unidade->nivel >= 6) {
        return false;
    }
    unidade->experiencia += experiencia;
    while (unidade->nivel < 6 && unidade->experiencia >= unidade->nivel * 100) {
        unidade->experiencia -= unidade->nivel * 100;
        unidade->nivel++;
        aumentoVida = unidade->vida_maxima * 8 / 100;
        if (aumentoVida < 5) {
            aumentoVida = 5;
        }
        aumentoDano = unidade->dano_base * 8 / 100;
        if (aumentoDano < 1) {
            aumentoDano = 1;
        }
        unidade->vida_maxima += aumentoVida;
        unidade->vida_atual += aumentoVida;
        unidade->dano_base += aumentoDano;
        if (unidade->nivel == 2 || unidade->nivel == 5) {
            unidade->estamina_maxima += unidade->ganho_estamina_nivel;
            unidade->estamina_atual += unidade->ganho_estamina_nivel;
        } else if (unidade->nivel == 3) {
            unidade->habilidades_desbloqueadas = 2;
        } else if (unidade->nivel == 4) {
            unidade->passiva_aprimorada = true;
            unidade->defesa++;
        } else if (unidade->nivel == 6) {
            unidade->habilidades_desbloqueadas = 3;
        }
    }
    return true;
}

// Aplica a recompensa de XP, cura e inflação correspondente à rota concluída.
void aplicarRecompensaRota(TipoNoRota tipoRota, Combatente tripulacao[],
                           int totalTripulacao, int xpBase)
{
    int experiencia;
    int curaPercentual;
    int indice;
    if (tripulacao == NULL || totalTripulacao <= 0 || xpBase < 0) {
        return;
    }
    experiencia = xpBase;
    curaPercentual = 0;
    if (tipoRota == ROTA_CAMINHO_2) {
        experiencia = xpBase * 2;
        curaPercentual = 25;
    } else if (tipoRota == ROTA_CAMINHO_1) {
        curaPercentual = 50;
    } else if (tipoRota == ROTA_LOJA) {
        experiencia = xpBase / 2;
        inflacaoLojasPercentual += 25;
    } else {
        return;
    }
    for (indice = 0; indice < totalTripulacao; indice++) {
        Combatente *aliado = &tripulacao[indice];
        if (!aliado->vivo || !aliado->aliado) {
            continue;
        }
        if (aliado->nivel >= 6) {
            aliado->experiencia += experiencia;
        } else {
            (void)ganharExperienciaEvoluir(aliado, experiencia);
        }
        if (curaPercentual > 0) {
            aliado->vida_atual += (aliado->vida_maxima * curaPercentual) / 100;
            if (aliado->vida_atual > aliado->vida_maxima) {
                aliado->vida_atual = aliado->vida_maxima;
            }
        }
    }
}

// Ajusta o preço conforme a inflação atual das lojas.
int aplicarInflacaoLoja(int preco)
{
    if (preco <= 0 || inflacaoLojasPercentual <= 0) {
        return preco;
    }
    return preco + (preco * inflacaoLojasPercentual) / 100;
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
