#include "combate.h"

#include "dados_jogo.h"
#include "raylib.h"

#include <stdlib.h>

static Combatente bossGrunts[2];
static const Combatente *venenoAtivoNesteTurno = NULL;
static const Combatente *desmoralizadoAtivoNesteTurno = NULL;
static const Combatente *motivadoAtivoNesteTurno = NULL;

// Verifica se uma coordenada pertence ao tabuleiro 3x6.
static bool coordenadaValida(int linha, int coluna)
{
    return linha >= 0 && linha < LINHAS_TABULEIRO &&
           coluna >= 0 && coluna < COLUNAS_TABULEIRO;
}

// Calcula a distância Manhattan entre duas células do tabuleiro.
static int distanciaEntre(int linhaA, int colunaA, int linhaB, int colunaB)
{
    return abs(linhaA - linhaB) + abs(colunaA - colunaB);
}

// Calcula a velocidade com os modificadores ativos de Motivação, Veneno e Timoneiro.
static int velocidadeEfetiva(const Combatente *unidade, int base)
{
    if (unidade->turnos_motivacao > 0 || unidade->motivado_capitao) {
        base += 2;
    }
    if (unidade->turnos_veneno > 0) {
        base -= 3;
    }
    if (unidade->turnos_desnorteado > 0) {
        base -= 2;
    }
    return base;
}

// Monta a fila de iniciativa e cria as três ações simultâneas do Barba Negra.
int montarFilaTurnos(Tabuleiro3x6 *mapa, EntradaIniciativa filaOut[])
{
    static const int velocidadesBoss[3] = { 8, 6, 4 };
    int linha;
    int coluna;
    int total = 0;
    if (mapa == NULL || filaOut == NULL) {
        return 0;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *unidade = mapa->celulas[linha][coluna];
            int acao;
            int quantidade;
            if (unidade == NULL || !unidade->vivo) {
                continue;
            }
            quantidade = unidade->eh_boss ? 3 : 1;
            for (acao = 0; acao < quantidade && total < MAX_ENTRADAS_INICIATIVA; acao++) {
                int base = unidade->eh_boss ? velocidadesBoss[acao] : unidade->velocidade;
                filaOut[total].ator = unidade;
                filaOut[total].velocidade_efetiva = velocidadeEfetiva(unidade, base);
                filaOut[total].indice_turno_boss = unidade->eh_boss ? acao : -1;
                total++;
            }
        }
    }
    ordenarFilaPorVelocidade(filaOut, total);
    return total;
}

// Recalcula a velocidade efetiva e reordena as ações restantes da rodada.
void atualizarVelocidadesFilaTurnos(EntradaIniciativa fila[], int total, int indiceAtual)
{
    static const int velocidadesBoss[3] = { 8, 6, 4 };
    int inicio = indiceAtual < 0 ? 0 : indiceAtual + 1;
    int indice;
    if (fila == NULL || total <= 1 || inicio >= total) {
        return;
    }
    for (indice = inicio; indice < total; indice++) {
        Combatente *unidade = fila[indice].ator;
        int indiceBoss = fila[indice].indice_turno_boss;
        int base;
        if (unidade == NULL || !unidade->vivo) {
            fila[indice].velocidade_efetiva = -999;
            continue;
        }
        if (unidade->eh_boss && indiceBoss >= 0 && indiceBoss < 3) {
            base = velocidadesBoss[indiceBoss];
        } else {
            base = unidade->velocidade;
        }
        fila[indice].velocidade_efetiva = velocidadeEfetiva(unidade, base);
    }
    ordenarFilaPorVelocidade(&fila[inicio], total - inicio);
}

// Aplica efeito de cura, estamina, tônico, evolução ou limpeza de status.
bool usarItemConsumivel(Combatente *unidade, Item *item)
{
    bool usoGratis;
    int valor;
    if (unidade == NULL || item == NULL || !unidade->vivo ||
        item->tipo != ITEM_CONSUMIVEL || item->quantidade <= 0) {
        return false;
    }
    usoGratis = unidade->aliado && unidade->classe == CLASSE_COZINHEIRO &&
                !unidade->usou_consumivel_gratis_turno;
    valor = item->valor_efeito;
    if (unidade->aliado && unidade->passiva_aprimorada &&
        unidade->classe == CLASSE_COZINHEIRO) {
        valor = valor + (valor * 20) / 100;
    }
    if (item->efeito_consumivel == CONSUMIVEL_CURAR_VIDA) {
        if (valor <= 0 || unidade->vida_atual >= unidade->vida_maxima) {
            return false;
        }
        unidade->vida_atual += valor;
        if (unidade->vida_atual > unidade->vida_maxima) {
            unidade->vida_atual = unidade->vida_maxima;
        }
    } else if (item->efeito_consumivel == CONSUMIVEL_RESTAURAR_ESTAMINA) {
        if (item->valor_efeito == 100) {
            unidade->estamina_atual = unidade->estamina_maxima;
        } else {
            unidade->estamina_atual += valor;
            if (unidade->estamina_atual > unidade->estamina_maxima) {
                unidade->estamina_atual = unidade->estamina_maxima;
            }
        }
    } else if (item->efeito_consumivel == CONSUMIVEL_REMOVER_STATUS) {
        if (unidade->turnos_veneno <= 0 && unidade->turnos_sangramento <= 0) {
            return false;
        }
        unidade->turnos_veneno = 0;
        unidade->turnos_sangramento = 0;
    } else if (item->efeito_consumivel == CONSUMIVEL_TONICO) {
        unidade->turnos_tonico = item->valor_efeito;
        unidade->bonus_tonico_percentual =
            (unidade->aliado && unidade->passiva_aprimorada &&
             unidade->classe == CLASSE_COZINHEIRO) ? 24 : 20;
    } else if (item->efeito_consumivel == CONSUMIVEL_SEM_EFEITO && item->id == 4) {
        if (!subirNivelPirata(unidade, item->valor_efeito)) {
            return false;
        }
    } else {
        return false;
    }
    item->quantidade--;
    if (usoGratis) {
        unidade->usou_consumivel_gratis_turno = true;
    }
    return true;
}

// Verifica se uma equipe foi derrotada e informa o resultado.
bool verificarFimDeCombate(Tabuleiro3x6 *mapa, bool *vitoriaAliada)
{
    int linha;
    int coluna;
    bool aliadosVivos = false;
    bool inimigosVivos = false;
    if (mapa == NULL || vitoriaAliada == NULL) {
        return false;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *unidade = mapa->celulas[linha][coluna];
            if (unidade != NULL && unidade->vivo) {
                if (unidade->aliado) {
                    aliadosVivos = true;
                } else {
                    inimigosVivos = true;
                }
            }
        }
    }
    if (aliadosVivos && inimigosVivos) {
        return false;
    }
    *vitoriaAliada = aliadosVivos && !inimigosVivos;
    return true;
}

// Avança até a próxima entrada viva e processa um novo turno do ator.
bool avancarProximoTurno(EntradaIniciativa fila[], int total, int *indiceAtual,
                         Tabuleiro3x6 *mapa)
{
    if (fila == NULL || indiceAtual == NULL || mapa == NULL || total <= 0 ||
        *indiceAtual < -1 || *indiceAtual >= total) {
        return false;
    }
    while (++(*indiceAtual) < total) {
        EntradaIniciativa *entrada = &fila[*indiceAtual];
        Combatente *unidade = entrada->ator;
        if (unidade == NULL || !unidade->vivo) {
            continue;
        }
        if (!unidade->eh_boss || entrada->indice_turno_boss == 0) {
            if (unidade->eh_boss) {
                unidade->estamina_atual = unidade->estamina_maxima;
            }
            processarStatusInicioTurno(mapa, unidade);
        }
        if (unidade->vivo && !unidade->turno_perdido) {
            return true;
        }
    }
    return false;
}

// Invoca um grunt na célula livre mais próxima da posição indicada.
static void invocarGruntProximo(Tabuleiro3x6 *mapa, Combatente *grunt,
                                int origemLinha, int origemColuna, int id)
{
    int menorDistancia = LINHAS_TABULEIRO + COLUNAS_TABULEIRO + 1;
    int melhorLinha = -1;
    int melhorColuna = -1;
    int linha;
    int coluna;
    if (!criarInimigoInicial(INIMIGO_GRUNT_ESPADA, grunt)) {
        return;
    }
    grunt->id = id;
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            int distancia = distanciaEntre(linha, coluna, origemLinha, origemColuna);
            if (mapa->celulas[linha][coluna] == NULL && distancia < menorDistancia) {
                menorDistancia = distancia;
                melhorLinha = linha;
                melhorColuna = coluna;
            }
        }
    }
    if (melhorLinha >= 0 && melhorColuna >= 0) {
        grunt->linha = melhorLinha;
        grunt->coluna = melhorColuna;
        mapa->celulas[melhorLinha][melhorColuna] = grunt;
    }
}

// Centraliza a verificação de Permadeath e limpeza da build passiva.
static void verificarMorteCombatente(Tabuleiro3x6 *mapa, Combatente *alvo)
{
    if (alvo == NULL || alvo->vida_atual > 0) {
        return;
    }
    alvo->vida_atual = 0;
    alvo->vivo = false;
    if (mapa != NULL && coordenadaValida(alvo->linha, alvo->coluna) &&
        mapa->celulas[alvo->linha][alvo->coluna] == alvo) {
        mapa->celulas[alvo->linha][alvo->coluna] = NULL;
    }
    if (alvo->aliado) {
        liberarListaItensCombatente(&alvo->build_passivos);
    }
}

// Aplica dano, remoção permanente e transição da 2ª fase do Barba Negra.
void subtrairVida(Tabuleiro3x6 *mapa, Combatente *alvo, int quantidade)
{
    int indiceGrunt;
    if (alvo == NULL || !alvo->vivo || quantidade <= 0) {
        return;
    }
    alvo->vida_atual -= quantidade;
    if (alvo->vida_atual <= 0) {
        verificarMorteCombatente(mapa, alvo);
        return;
    }
    if (!alvo->eh_boss || mapa == NULL || mapa->boss_fase_2_ativa ||
        alvo->vida_atual * 2 > alvo->vida_maxima) {
        return;
    }
    mapa->boss_fase_2_ativa = true;
    mapa->turnos_boss_fase_2 = 0;
    alvo->fase_boss = 2;
    alvo->estamina_maxima = 20;
    alvo->estamina_atual += 5;
    if (alvo->estamina_atual > alvo->estamina_maxima) {
        alvo->estamina_atual = alvo->estamina_maxima;
    }
    for (indiceGrunt = 0; indiceGrunt < 2; indiceGrunt++) {
        invocarGruntProximo(mapa, &bossGrunts[indiceGrunt],
                            indiceGrunt == 0 ? 0 : 2,
                            indiceGrunt == 0 ? 0 : 5, 110 + indiceGrunt);
    }
}

// Posiciona ou troca aliados entre as duas colunas iniciais na Fase Pré-Batalha.
bool posicionarPirataPreBatalha(Tabuleiro3x6 *mapa, Combatente *pirata, int linha, int coluna)
{
    Combatente *ocupante;
    if (mapa == NULL || pirata == NULL || !pirata->aliado ||
        !coordenadaValida(linha, coluna) || coluna > 1) {
        return false;
    }
    ocupante = mapa->celulas[linha][coluna];
    if (ocupante != NULL && ocupante != pirata &&
        (!ocupante->aliado || !coordenadaValida(pirata->linha, pirata->coluna) ||
         pirata->coluna > 1 || mapa->celulas[pirata->linha][pirata->coluna] != pirata)) {
        return false;
    }
    if (ocupante != NULL && ocupante != pirata) {
        int linhaAnterior = pirata->linha;
        int colunaAnterior = pirata->coluna;
        mapa->celulas[linhaAnterior][colunaAnterior] = ocupante;
        ocupante->linha = linhaAnterior;
        ocupante->coluna = colunaAnterior;
        pirata->linha = linha;
        pirata->coluna = coluna;
        mapa->celulas[linha][coluna] = pirata;
        return true;
    }
    if (coordenadaValida(pirata->linha, pirata->coluna) &&
        mapa->celulas[pirata->linha][pirata->coluna] == pirata) {
        mapa->celulas[pirata->linha][pirata->coluna] = NULL;
    }
    pirata->linha = linha;
    pirata->coluna = coluna;
    mapa->celulas[linha][coluna] = pirata;
    return true;
}

// Move a unidade ao custo de 1 estamina e resolve minas ativadas no destino.
bool moverCombatente3x6(Tabuleiro3x6 *mapa, Combatente *unidade, int destinoLinha,
                        int destinoColuna)
{
    int distancia;
    int passoMaximo;
    int linhaAnterior;
    int colunaAnterior;
    bool bossFaseDois;
    NoItem *item;
    if (mapa == NULL || unidade == NULL || !unidade->vivo ||
        !coordenadaValida(unidade->linha, unidade->coluna) ||
        !coordenadaValida(destinoLinha, destinoColuna) ||
        mapa->celulas[unidade->linha][unidade->coluna] != unidade ||
        mapa->celulas[destinoLinha][destinoColuna] != NULL) {
        return false;
    }
    distancia = distanciaEntre(destinoLinha, destinoColuna, unidade->linha, unidade->coluna);
    passoMaximo = (unidade->aliado && unidade->classe == CLASSE_BUCANEIRO) ? 2 : 1;
    for (item = unidade->build_passivos.inicio; item != NULL; item = item->proximo) {
        if (item->item.id == 8) {
            passoMaximo++;
        }
    }
    if (distancia < 1 || distancia > passoMaximo || unidade->estamina_atual < 1) {
        return false;
    }
    linhaAnterior = unidade->linha;
    colunaAnterior = unidade->coluna;
    bossFaseDois = unidade->eh_boss && mapa->boss_fase_2_ativa;
    unidade->direcao_linha = destinoLinha > linhaAnterior ? 1 :
                             (destinoLinha < linhaAnterior ? -1 : 0);
    unidade->direcao_coluna = destinoColuna > colunaAnterior ? 1 :
                              (destinoColuna < colunaAnterior ? -1 : 0);
    mapa->celulas[linhaAnterior][colunaAnterior] = NULL;
    unidade->estamina_atual -= 1;
    unidade->linha = destinoLinha;
    unidade->coluna = destinoColuna;
    mapa->celulas[destinoLinha][destinoColuna] = unidade;
    if (mapa->mina_ativa[destinoLinha][destinoColuna] > 0) {
        mapa->mina_ativa[destinoLinha][destinoColuna] = 0;
        subtrairVida(mapa, unidade, 50);
    }
    if (bossFaseDois && unidade->vivo) {
        mapa->turnos_boss_fase_2++;
        if (mapa->turnos_boss_fase_2 % 2 == 0) {
            mapa->mina_ativa[linhaAnterior][colunaAnterior] = 1;
        }
    }
    return true;
}

// Verifica se o alvo está protegido por um capitão aliado adjacente.
static bool protegidoPorCapitao(const Tabuleiro3x6 *mapa, const Combatente *alvo)
{
    int linha;
    int coluna;
    if (mapa == NULL || alvo == NULL) {
        return false;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *aliado = mapa->celulas[linha][coluna];
            if (aliado != NULL && aliado->vivo && aliado->aliado == alvo->aliado &&
                aliado->aliado && aliado->classe == CLASSE_CAPITAO &&
                distanciaEntre(linha, coluna, alvo->linha, alvo->coluna) == 1) {
                return true;
            }
        }
    }
    return false;
}

// Verifica proteção de dano em cruz pela passiva aprimorada do Timoneiro (Nv 4).
static bool protegidoPorTimoneiro(const Tabuleiro3x6 *mapa, const Combatente *alvo)
{
    int linha;
    int coluna;
    if (mapa == NULL || alvo == NULL) {
        return false;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *aliado = mapa->celulas[linha][coluna];
            if (aliado != NULL && aliado->vivo && aliado->aliado == alvo->aliado &&
                aliado->aliado && aliado->classe == CLASSE_TIMONEIRO &&
                aliado->passiva_aprimorada &&
                distanciaEntre(linha, coluna, alvo->linha, alvo->coluna) == 1) {
                return true;
            }
        }
    }
    return false;
}

// Verifica o bônus de +20% de dano concedido por um Capitão Nv 4 adjacente.
static bool adjacenteCapitaoAprimorado(const Tabuleiro3x6 *mapa, const Combatente *atacante)
{
    int linha;
    int coluna;
    if (mapa == NULL || atacante == NULL) {
        return false;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *aliado = mapa->celulas[linha][coluna];
            if (aliado != NULL && aliado != atacante && aliado->vivo &&
                aliado->aliado == atacante->aliado && aliado->aliado &&
                aliado->classe == CLASSE_CAPITAO && aliado->passiva_aprimorada &&
                distanciaEntre(linha, coluna, atacante->linha, atacante->coluna) == 1) {
                return true;
            }
        }
    }
    return false;
}

// Soma os bônus passivos de redução de dano, melee, ranged e precisão da build.
static void obterBonusPassivos(const Combatente *unidade, int *reducao, int *melee,
                               int *ranged, int *precisao)
{
    NoItem *item;
    *reducao = 0;
    *melee = 0;
    *ranged = 0;
    *precisao = 0;
    for (item = unidade->build_passivos.inicio; item != NULL; item = item->proximo) {
        *reducao += item->item.reducao_dano_percentual;
        *melee += item->item.bonus_dano_melee_percentual;
        *ranged += item->item.bonus_dano_ranged_percentual;
        *precisao += item->item.bonus_precisao;
    }
}

// Calcula o dano inteiro final obedecendo a fórmula padronizada C99 do contrato.
static int calcularDanoFinal(Tabuleiro3x6 *mapa, Combatente *atacante,
                             Combatente *alvo, const Habilidade *habilidade,
                             int indiceHabilidade, int distancia)
{
    int danoMin = habilidade->dano_min;
    int danoMax = habilidade->dano_max;
    int dano;
    int reducao;
    int melee;
    int ranged;
    int precisao;
    int somaPctBuffs = 0;
    bool ehCritico;
    bool ataqueRanged = habilidade->tipo_ataque == ATAQUE_RANGED ||
                        habilidade->tipo_ataque == ATAQUE_LINHA ||
                        habilidade->tipo_ataque == ATAQUE_CONE ||
                        habilidade->tipo_ataque == ATAQUE_CRUZ ||
                        habilidade->tipo_ataque == ATAQUE_AREA_3X2 ||
                        (habilidade->tipo_ataque == ATAQUE_HIBRIDO_SAQUEAR &&
                         distancia > 1);
    if (atacante->aliado && atacante->classe == CLASSE_COZINHEIRO &&
        indiceHabilidade == 1) {
        danoMin = distancia == 1 ? 30 : 15;
        danoMax = distancia == 1 ? 40 : 20;
    } else if (atacante->aliado && atacante->classe == CLASSE_CAPITAO &&
               indiceHabilidade == 0) {
        danoMin = distancia == 1 ? 30 : 20;
        danoMax = distancia == 1 ? 50 : 25;
    } else if (!atacante->aliado && atacante->tipo_inimigo == INIMIGO_MARUJO_GORDO &&
               indiceHabilidade == 0 && distancia > 1) {
        danoMin = habilidade->dano_min / 2;
        danoMax = (habilidade->dano_max * 40) / 70;
    }
    dano = GetRandomValue(danoMin, danoMax);
    obterBonusPassivos(atacante, &reducao, &melee, &ranged, &precisao);
    (void)reducao;
    (void)precisao;
    if (ataqueRanged) {
        somaPctBuffs += ranged;
    } else {
        somaPctBuffs += melee;
    }
    if (atacante->aliado && atacante->dano_base > 0) {
        somaPctBuffs += atacante->dano_base;
    }
    if (atacante->turnos_tonico > 0 || atacante->bonus_tonico_percentual > 0) {
        somaPctBuffs += atacante->bonus_tonico_percentual;
    }
    if (atacante->aliado && atacante->classe == CLASSE_BUCANEIRO &&
        (atacante->passiva_aprimorada || atacante->nivel >= 4) && alvo != NULL) {
        int prodColuna = (atacante->coluna - alvo->coluna) * alvo->direcao_coluna;
        int prodLinha = (atacante->linha - alvo->linha) * alvo->direcao_linha;
        if (atacante->coluna > alvo->coluna ||
            (alvo->direcao_coluna != 0 && prodColuna < 0) ||
            (alvo->direcao_linha != 0 && prodLinha < 0)) {
            somaPctBuffs += 40;
        }
    }
    if (atacante->aliado && atacante->classe == CLASSE_TIMONEIRO &&
        atacante->vida_atual * 100 < atacante->vida_maxima * 60) {
        somaPctBuffs += 25;
    }
    if (atacante->aliado && atacante->classe == CLASSE_GAVEA &&
        (atacante->passiva_aprimorada || atacante->nivel >= 4)) {
        int quadradosMaisPerto = 6 - distancia;
        if (quadradosMaisPerto < 1) {
            quadradosMaisPerto = 1;
        }
        somaPctBuffs += quadradosMaisPerto * 10;
    }
    if (atacante->aliado && adjacenteCapitaoAprimorado(mapa, atacante)) {
        somaPctBuffs += 20;
    }
    if (atacante->eh_boss && atacante->fase_boss == 2) {
        if (indiceHabilidade == 0) {
            somaPctBuffs += 15;
        } else if (indiceHabilidade == 1) {
            somaPctBuffs += 20;
        }
    }
    dano = (dano * (100 + somaPctBuffs)) / 100;
    if (atacante->turnos_veneno > 0 || atacante == venenoAtivoNesteTurno) {
        dano = dano - (dano * 15) / 100;
    }
    ehCritico = GetRandomValue(1, 100) <= 10;
    if (ehCritico) {
        dano = dano * 2;
    }
    if (dano < 1) {
        dano = 1;
    }
    return dano;
}

// Aplica reduções defensivas, vulnerabilidade de descanso e fogo amigo (50%).
static int aplicarDano(Tabuleiro3x6 *mapa, Combatente *atacante, Combatente *alvo,
                       int dano, bool fogoAmigo)
{
    int recebido;
    int reducao;
    int melee;
    int ranged;
    int precisao;
    bool ehFogoAmigo;
    if (alvo == NULL || !alvo->vivo || dano <= 0) {
        return 0;
    }
    ehFogoAmigo = atacante != NULL && alvo->aliado == atacante->aliado && fogoAmigo;
    recebido = dano - alvo->defesa;
    if (recebido < 1) {
        recebido = 1;
    }
    obterBonusPassivos(alvo, &reducao, &melee, &ranged, &precisao);
    (void)melee;
    (void)ranged;
    (void)precisao;
    if (reducao > 0) {
        recebido = recebido - (recebido * reducao) / 100;
    }
    if (protegidoPorTimoneiro(mapa, alvo)) {
        recebido = recebido - (recebido * 40) / 100;
    }
    if (alvo->em_descanso && !protegidoPorCapitao(mapa, alvo)) {
        recebido += (recebido * 25) / 100;
        alvo->atacado_no_descanso = true;
    }
    if (ehFogoAmigo) {
        recebido = recebido / 2;
    }
    if (recebido < 1) {
        recebido = 1;
    }
    subtrairVida(mapa, alvo, recebido);
    return recebido;
}

// Verifica se a habilidade em área afeta uma célula relativamente ao centro escolhido.
static bool habilidadeAfetaCelula(const Habilidade *habilidade, const Combatente *atacante,
                                  int alvoLinha, int alvoColuna, int linha, int coluna)
{
    int deltaLinha = alvoLinha - atacante->linha;
    int deltaColuna = alvoColuna - atacante->coluna;
    if (linha == alvoLinha && coluna == alvoColuna) {
        return true;
    }
    if (habilidade->tipo_ataque == ATAQUE_LINHA ||
        habilidade->tipo_ataque == ATAQUE_DASH_CARDEAL) {
        if (deltaLinha == 0) {
            return linha == alvoLinha && (coluna - atacante->coluna) * deltaColuna > 0 &&
                   abs(coluna - atacante->coluna) <= abs(deltaColuna);
        }
        if (deltaColuna == 0) {
            return coluna == alvoColuna && (linha - atacante->linha) * deltaLinha > 0 &&
                   abs(linha - atacante->linha) <= abs(deltaLinha);
        }
    }
    if (habilidade->tipo_ataque == ATAQUE_CRUZ) {
        return abs(linha - alvoLinha) + abs(coluna - alvoColuna) <= 1;
    }
    if (habilidade->tipo_ataque == ATAQUE_CONE) {
        if (abs(deltaLinha) > abs(deltaColuna)) {
            int sinalLinha = deltaLinha > 0 ? 1 : -1;
            int passoLinha = (linha - atacante->linha) * sinalLinha;
            return passoLinha >= 1 && passoLinha <= habilidade->alcance &&
                   abs(coluna - atacante->coluna) < passoLinha;
        }
        if (deltaColuna != 0) {
            int sinalColuna = deltaColuna > 0 ? 1 : -1;
            int passoColuna = (coluna - atacante->coluna) * sinalColuna;
            return passoColuna >= 1 && passoColuna <= habilidade->alcance &&
                   abs(linha - atacante->linha) < passoColuna;
        }
        return false;
    }
    if (habilidade->tipo_ataque == ATAQUE_AREA_1X3) {
        if (abs(deltaLinha) > abs(deltaColuna)) {
            return linha == alvoLinha && abs(coluna - alvoColuna) <= 1;
        }
        return coluna == alvoColuna && abs(linha - alvoLinha) <= 1;
    }
    if (habilidade->tipo_ataque == ATAQUE_AREA_2X3) {
        int linhaExtra = (alvoLinha == LINHAS_TABULEIRO - 1) ? -1 :
                         ((alvoLinha == 0) ? 1 : (deltaLinha >= 0 ? 1 : -1));
        return (linha == alvoLinha || linha == alvoLinha + linhaExtra ||
                linha == atacante->linha) &&
               abs(coluna - alvoColuna) <= 1;
    }
    if (habilidade->tipo_ataque == ATAQUE_AREA_3X2) {
        int colunaExtra = (alvoColuna == COLUNAS_TABULEIRO - 1) ? -1 :
                          ((alvoColuna == 0) ? 1 : (deltaColuna >= 0 ? 1 : -1));
        return linha >= 0 && linha < LINHAS_TABULEIRO &&
               (coluna == alvoColuna || coluna == alvoColuna + colunaExtra);
    }
    return false;
}

// Aplica cura, buff, debuff ou knockback axial ao combatente atingido.
static void aplicarEfeito(Tabuleiro3x6 *mapa, Combatente *atacante, Combatente *alvo,
                          const Habilidade *habilidade, int indiceHabilidade,
                          int dirAxialLinha, int dirAxialColuna)
{
    if (alvo == NULL || !alvo->vivo) {
        return;
    }
    if (habilidade->efeito == EFEITO_MOTIVACAO) {
        if (alvo->aliado == atacante->aliado) {
            if (alvo->turnos_motivacao <= 0) {
                alvo->estamina_maxima += 1;
                alvo->estamina_atual += 1;
            }
            alvo->motivado_capitao = true;
            alvo->bonus_estamina_proximo_turno += 2;
            alvo->turnos_motivacao = 2;
        } else {
            alvo->turnos_desmoralizado = 2;
        }
    } else if (habilidade->efeito == EFEITO_ESTUNAR) {
        alvo->turnos_atordoado = habilidade->duracao_efeito > 0 ?
                                  habilidade->duracao_efeito : 1;
    } else if (habilidade->efeito == EFEITO_SANGRAMENTO) {
        if (atacante->aliado && atacante->classe == CLASSE_BUCANEIRO) {
            alvo->golpes_corte_bucaneiro_recebidos++;
            if (alvo->golpes_corte_bucaneiro_recebidos >= 2) {
                alvo->turnos_sangramento = 3;
                alvo->golpes_corte_bucaneiro_recebidos = 0;
            }
        } else if (!atacante->aliado &&
                   atacante->tipo_inimigo == INIMIGO_GRUNT_ESPADA) {
            alvo->golpes_corte_grunt_recebidos++;
            if (alvo->golpes_corte_grunt_recebidos >= 3) {
                alvo->turnos_sangramento = 3;
                alvo->golpes_corte_grunt_recebidos = 0;
            }
        } else {
            alvo->turnos_sangramento = 3;
        }
    } else if (habilidade->efeito == EFEITO_VENENO) {
        if (alvo->aliado != atacante->aliado) {
            if (alvo->turnos_veneno <= 0) {
                alvo->turnos_veneno = 2;
            }
        } else {
            alvo->turnos_cura_hot = 2;
            alvo->valor_cura_hot = (alvo->vida_maxima * 10) / 100;
            if (alvo->valor_cura_hot < 1) {
                alvo->valor_cura_hot = 1;
            }
        }
    } else if (habilidade->efeito == EFEITO_DESNORTEAR ||
               (habilidade->efeito == EFEITO_KNOCKBACK && indiceHabilidade == 2)) {
        if (habilidade->efeito == EFEITO_DESNORTEAR) {
            alvo->turnos_desnorteado = habilidade->duracao_efeito > 0 ?
                                        habilidade->duracao_efeito : 2;
        }
        if ((atacante->aliado && atacante->classe == CLASSE_TIMONEIRO && indiceHabilidade == 2) ||
            habilidade->efeito == EFEITO_KNOCKBACK) {
            int destinoLin = alvo->linha + dirAxialLinha;
            int destinoCol = alvo->coluna + dirAxialColuna;
            if (coordenadaValida(destinoLin, destinoCol) &&
                mapa->celulas[destinoLin][destinoCol] == NULL) {
                mapa->celulas[alvo->linha][alvo->coluna] = NULL;
                alvo->linha = destinoLin;
                alvo->coluna = destinoCol;
                mapa->celulas[destinoLin][destinoCol] = alvo;
                if (mapa->mina_ativa[destinoLin][destinoCol] > 0) {
                    mapa->mina_ativa[destinoLin][destinoCol] = 0;
                    subtrairVida(mapa, alvo, 50);
                }
            }
        }
    } else if (habilidade->efeito == EFEITO_CURA_CONTINUA) {
        alvo->turnos_cura_hot = habilidade->duracao_efeito > 0 ?
                                habilidade->duracao_efeito : 3;
        alvo->valor_cura_hot = habilidade->dano_min;
    } else if (habilidade->efeito == EFEITO_CURA) {
        int cura = (alvo->vida_maxima * habilidade->dano_max) / 100;
        if (cura < 1) {
            cura = 1;
        }
        alvo->vida_atual += cura;
        if (alvo->vida_atual > alvo->vida_maxima) {
            alvo->vida_atual = alvo->vida_maxima;
        }
    }
}

// Verifica se a coordenada clicada é válida para o tipo de alcance da habilidade.
static bool ataqueValido(const Combatente *atacante, const Habilidade *habilidade,
                         int alvoLinha, int alvoColuna, int distancia)
{
    bool permiteZero = habilidade->efeito == EFEITO_CURA ||
                       habilidade->efeito == EFEITO_CURA_CONTINUA ||
                       habilidade->efeito == EFEITO_MOTIVACAO ||
                       habilidade->efeito == EFEITO_VENENO;
    if (habilidade->tipo_ataque == ATAQUE_AREA_3X2 && habilidade->alcance >= 5) {
        return true;
    }
    if (habilidade->tipo_ataque == ATAQUE_CONE) {
        int deltaLinha = alvoLinha - atacante->linha;
        int deltaColuna = alvoColuna - atacante->coluna;
        bool coneVertical = abs(deltaLinha) >= 1 &&
                            abs(deltaLinha) <= habilidade->alcance &&
                            abs(deltaColuna) < abs(deltaLinha);
        bool coneHorizontal = abs(deltaColuna) >= 1 &&
                              abs(deltaColuna) <= habilidade->alcance &&
                              abs(deltaLinha) < abs(deltaColuna);
        return coneVertical || coneHorizontal;
    }
    if ((permiteZero ? distancia < 0 : distancia <= 0) || distancia > habilidade->alcance) {
        return false;
    }
    if ((habilidade->tipo_ataque == ATAQUE_MELEE ||
         habilidade->tipo_ataque == ATAQUE_LINHA ||
         habilidade->tipo_ataque == ATAQUE_DASH_CARDEAL ||
         habilidade->tipo_ataque == ATAQUE_AREA_1X3) &&
        alvoLinha != atacante->linha && alvoColuna != atacante->coluna) {
        return false;
    }
    return true;
}

// Calcula a precisão final do golpe considerando distância, buffs, debuffs e itens.
static int calcularPrecisaoFinal(const Combatente *atacante, const Habilidade *habilidade,
                                 int indiceHab, int distOrigem)
{
    int reducao;
    int bonusMelee;
    int bonusRanged;
    int bonusPrecisao;
    int precisao = habilidade->precisao;
    obterBonusPassivos(atacante, &reducao, &bonusMelee, &bonusRanged, &bonusPrecisao);
    (void)reducao;
    (void)bonusMelee;
    (void)bonusRanged;
    if (atacante->aliado && atacante->classe == CLASSE_CAPITAO && indiceHab == 0) {
        precisao = distOrigem == 1 ? atacante->precisao_melee : atacante->precisao_ranged;
    }
    precisao += bonusPrecisao;
    if (atacante->aliado && atacante->classe == CLASSE_GAVEA) {
        int quadradosMaisPerto = 6 - distOrigem;
        if (quadradosMaisPerto < 1) {
            quadradosMaisPerto = 1;
        }
        precisao += quadradosMaisPerto * 5;
    }
    if (atacante->turnos_desmoralizado > 0 || atacante == desmoralizadoAtivoNesteTurno) {
        precisao = precisao - (precisao * 15) / 100;
    }
    if (atacante->motivado_capitao || atacante == motivadoAtivoNesteTurno) {
        precisao = precisao + (precisao * 20) / 100;
    }
    if (precisao > 100) {
        precisao = 100;
    }
    return precisao;
}

// Executa o Avanço Impetuoso do Bucaneiro seguindo os 4 passos do Edge Case 1.
static void executarAvancoImpetuoso(Tabuleiro3x6 *mapa, Combatente *atacante,
                                    const Habilidade *habilidade, int indiceHab,
                                    int dirLinha, int dirColuna)
{
    int origLin = atacante->linha;
    int origCol = atacante->coluna;
    int linDist2 = origLin + 2 * dirLinha;
    int colDist2 = origCol + 2 * dirColuna;
    bool inimigoEmDist2 = coordenadaValida(linDist2, colDist2) &&
                          mapa->celulas[linDist2][colDist2] != NULL &&
                          mapa->celulas[linDist2][colDist2]->vivo &&
                          mapa->celulas[linDist2][colDist2]->aliado != atacante->aliado;
    int distParada = inimigoEmDist2 ? 3 : 2;
    int curaRouboVida = 0;
    int dist;
    for (dist = 1; dist <= 2; dist++) {
        int celLin = origLin + dist * dirLinha;
        int celCol = origCol + dist * dirColuna;
        Combatente *alvo;
        int precisao;
        if (!coordenadaValida(celLin, celCol)) {
            continue;
        }
        if (mapa->mina_ativa[celLin][celCol] > 0) {
            mapa->mina_ativa[celLin][celCol] = 0;
        }
        alvo = mapa->celulas[celLin][celCol];
        if (alvo == NULL || !alvo->vivo || alvo->aliado == atacante->aliado) {
            continue;
        }
        precisao = calcularPrecisaoFinal(atacante, habilidade, indiceHab, dist);
        if (precisao >= 0 && GetRandomValue(1, 100) <= precisao) {
            int dano = calcularDanoFinal(mapa, atacante, alvo, habilidade, indiceHab, dist);
            int danoCausado = aplicarDano(mapa, atacante, alvo, dano, false);
            curaRouboVida += danoCausado / 2;
        }
    }
    if (curaRouboVida > 0 && atacante->vivo) {
        atacante->vida_atual += curaRouboVida;
        if (atacante->vida_atual > atacante->vida_maxima) {
            atacante->vida_atual = atacante->vida_maxima;
        }
    }
    if (!atacante->vivo) {
        return;
    }
    for (dist = distParada; dist >= 1; dist--) {
        int novaLin = origLin + dist * dirLinha;
        int novaCol = origCol + dist * dirColuna;
        if (coordenadaValida(novaLin, novaCol) && mapa->celulas[novaLin][novaCol] == NULL) {
            mapa->celulas[origLin][origCol] = NULL;
            atacante->linha = novaLin;
            atacante->coluna = novaCol;
            atacante->direcao_linha = dirLinha;
            atacante->direcao_coluna = dirColuna;
            mapa->celulas[novaLin][novaCol] = atacante;
            if (mapa->mina_ativa[novaLin][novaCol] > 0) {
                mapa->mina_ativa[novaLin][novaCol] = 0;
                subtrairVida(mapa, atacante, 50);
            }
            break;
        }
    }
}

// Executa uma habilidade, aplica efeitos e destrói minas atingidas.
void executarAtaqueOuHabilidade(Tabuleiro3x6 *mapa, Combatente *atacante, int indiceHab,
                                int alvoLinha, int alvoColuna)
{
    Habilidade *habilidade;
    Combatente *alvoClique;
    int distancia;
    int custo;
    int linha;
    int coluna;
    int dirAxialLinha;
    int dirAxialColuna;
    bool area;
    bool clicouEmMina;
    bool coneBloqueadoEmDist1 = false;
    if (mapa == NULL || atacante == NULL || !atacante->vivo ||
        indiceHab < 0 || indiceHab >= atacante->habilidades_desbloqueadas ||
        indiceHab >= MAX_HABILIDADES || !coordenadaValida(alvoLinha, alvoColuna)) {
        return;
    }
    habilidade = &atacante->habilidades[indiceHab];
    distancia = distanciaEntre(atacante->linha, atacante->coluna, alvoLinha, alvoColuna);
    if (!ataqueValido(atacante, habilidade, alvoLinha, alvoColuna, distancia)) {
        return;
    }
    custo = habilidade->custo_estamina;
    if (atacante->turnos_desmoralizado > 0 || atacante == desmoralizadoAtivoNesteTurno) {
        custo++;
    }
    if (atacante->estamina_atual < custo) {
        return;
    }
    alvoClique = mapa->celulas[alvoLinha][alvoColuna];
    clicouEmMina = mapa->mina_ativa[alvoLinha][alvoColuna] > 0;
    if (habilidade->efeito == EFEITO_CURA &&
        habilidade->tipo_ataque != ATAQUE_AREA_2X3 &&
        (alvoClique == NULL || alvoClique->aliado != atacante->aliado)) {
        return;
    }
    if (habilidade->efeito != EFEITO_CURA &&
        habilidade->efeito != EFEITO_MOTIVACAO &&
        habilidade->efeito != EFEITO_CURA_CONTINUA &&
        habilidade->efeito != EFEITO_VENENO &&
        habilidade->tipo_ataque != ATAQUE_AREA_1X3 &&
        habilidade->tipo_ataque != ATAQUE_AREA_2X3 &&
        habilidade->tipo_ataque != ATAQUE_AREA_3X2 &&
        habilidade->tipo_ataque != ATAQUE_CRUZ &&
        habilidade->tipo_ataque != ATAQUE_CONE &&
        habilidade->tipo_ataque != ATAQUE_LINHA &&
        habilidade->tipo_ataque != ATAQUE_DASH_CARDEAL &&
        !clicouEmMina &&
        (alvoClique == NULL || alvoClique->aliado == atacante->aliado)) {
        return;
    }
    atacante->estamina_atual -= custo;
    dirAxialLinha = (alvoLinha > atacante->linha) ? 1 :
                    ((alvoLinha < atacante->linha) ? -1 : 0);
    dirAxialColuna = (alvoColuna > atacante->coluna) ? 1 :
                     ((alvoColuna < atacante->coluna) ? -1 : 0);
    if (abs(alvoLinha - atacante->linha) > abs(alvoColuna - atacante->coluna)) {
        dirAxialColuna = 0;
    } else if (abs(alvoColuna - atacante->coluna) > abs(alvoLinha - atacante->linha)) {
        dirAxialLinha = 0;
    }
    if (dirAxialLinha != 0 || dirAxialColuna != 0) {
        atacante->direcao_linha = dirAxialLinha;
        atacante->direcao_coluna = dirAxialColuna;
    }
    if (habilidade->tipo_ataque == ATAQUE_DASH_CARDEAL) {
        executarAvancoImpetuoso(mapa, atacante, habilidade, indiceHab,
                                dirAxialLinha, dirAxialColuna);
        return;
    }
    if (clicouEmMina) {
        mapa->mina_ativa[alvoLinha][alvoColuna] = 0;
        if (alvoClique == NULL &&
            habilidade->tipo_ataque != ATAQUE_CRUZ &&
            habilidade->tipo_ataque != ATAQUE_CONE &&
            habilidade->tipo_ataque != ATAQUE_AREA_1X3 &&
            habilidade->tipo_ataque != ATAQUE_AREA_2X3 &&
            habilidade->tipo_ataque != ATAQUE_AREA_3X2 &&
            habilidade->tipo_ataque != ATAQUE_LINHA) {
            return;
        }
    }
    if (habilidade->tipo_ataque == ATAQUE_LINHA) {
        int limiteAlvos = 1;
        int alvosAtingidos = 0;
        int passo;
        if (habilidade->perfurante) {
            limiteAlvos = COLUNAS_TABULEIRO + LINHAS_TABULEIRO;
        } else if ((!atacante->aliado &&
                    (atacante->tipo_inimigo == INIMIGO_GRUNT_PISTOLA ||
                     atacante->tipo_inimigo == INIMIGO_MARUJO_GORDO) &&
                    indiceHab == 0)) {
            limiteAlvos = 2;
        }
        for (passo = 1; passo <= habilidade->alcance; passo++) {
            int celLin = atacante->linha + passo * dirAxialLinha;
            int celCol = atacante->coluna + passo * dirAxialColuna;
            Combatente *alvoLinhaAtual;
            int precisao;
            if (!coordenadaValida(celLin, celCol)) {
                break;
            }
            if (mapa->mina_ativa[celLin][celCol] > 0) {
                mapa->mina_ativa[celLin][celCol] = 0;
            }
            alvoLinhaAtual = mapa->celulas[celLin][celCol];
            if (alvoLinhaAtual == NULL || !alvoLinhaAtual->vivo) {
                continue;
            }
            if (alvoLinhaAtual->aliado == atacante->aliado && !habilidade->fogo_amigo) {
                continue;
            }
            alvosAtingidos++;
            precisao = calcularPrecisaoFinal(atacante, habilidade, indiceHab, passo);
            if (precisao >= 0 && GetRandomValue(1, 100) <= precisao) {
                int dano = calcularDanoFinal(mapa, atacante, alvoLinhaAtual,
                                             habilidade, indiceHab, passo);
                (void)aplicarDano(mapa, atacante, alvoLinhaAtual, dano,
                                  habilidade->fogo_amigo);
                aplicarEfeito(mapa, atacante, alvoLinhaAtual, habilidade, indiceHab,
                              dirAxialLinha, dirAxialColuna);
            }
            if (alvosAtingidos >= limiteAlvos || !atacante->vivo) {
                break;
            }
        }
        return;
    }
    if (habilidade->tipo_ataque == ATAQUE_CONE) {
        int linDist1 = atacante->linha + dirAxialLinha;
        int colDist1 = atacante->coluna + dirAxialColuna;
        if (coordenadaValida(linDist1, colDist1) &&
            mapa->celulas[linDist1][colDist1] != NULL &&
            mapa->celulas[linDist1][colDist1]->vivo &&
            mapa->celulas[linDist1][colDist1]->aliado != atacante->aliado) {
            coneBloqueadoEmDist1 = true;
        }
    }
    area = habilidade->tipo_ataque == ATAQUE_CRUZ ||
           habilidade->tipo_ataque == ATAQUE_CONE ||
           habilidade->tipo_ataque == ATAQUE_AREA_1X3 ||
           habilidade->tipo_ataque == ATAQUE_AREA_2X3 ||
           habilidade->tipo_ataque == ATAQUE_AREA_3X2;
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            int distOrigem = distanciaEntre(linha, coluna, atacante->linha, atacante->coluna);
            Combatente *alvoCelula;
            int precisao;
            bool acertou = true;
            if (area) {
                if (!habilidadeAfetaCelula(habilidade, atacante, alvoLinha,
                                           alvoColuna, linha, coluna)) {
                    continue;
                }
                if (habilidade->tipo_ataque == ATAQUE_CONE && coneBloqueadoEmDist1 &&
                    distOrigem > 1) {
                    continue;
                }
            } else if (linha != alvoLinha || coluna != alvoColuna) {
                continue;
            }
            if (mapa->mina_ativa[linha][coluna] > 0 && habilidade->efeito != EFEITO_CURA) {
                mapa->mina_ativa[linha][coluna] = 0;
            }
            alvoCelula = mapa->celulas[linha][coluna];
            if (alvoCelula == NULL || !alvoCelula->vivo) {
                continue;
            }
            if (habilidade->efeito == EFEITO_CURA ||
                habilidade->efeito == EFEITO_CURA_CONTINUA) {
                if (alvoCelula->aliado != atacante->aliado) {
                    continue;
                }
            } else if (alvoCelula->aliado == atacante->aliado && !habilidade->fogo_amigo &&
                       habilidade->efeito != EFEITO_MOTIVACAO &&
                       habilidade->efeito != EFEITO_VENENO) {
                continue;
            }
            if (habilidade->efeito != EFEITO_CURA &&
                habilidade->efeito != EFEITO_CURA_CONTINUA &&
                !(alvoCelula->aliado == atacante->aliado &&
                  (habilidade->efeito == EFEITO_MOTIVACAO ||
                   habilidade->efeito == EFEITO_VENENO))) {
                precisao = calcularPrecisaoFinal(atacante, habilidade, indiceHab, distOrigem);
                acertou = precisao >= 0 && GetRandomValue(1, 100) <= precisao;
            }
            if (!acertou) {
                continue;
            }
            if (habilidade->dano_max > 0 &&
                (habilidade->efeito == EFEITO_DANO ||
                 habilidade->efeito == EFEITO_BOMBA ||
                 habilidade->efeito == EFEITO_SANGRAMENTO ||
                 habilidade->efeito == EFEITO_VENENO ||
                 habilidade->efeito == EFEITO_ESTUNAR ||
                 habilidade->efeito == EFEITO_DESNORTEAR ||
                 habilidade->efeito == EFEITO_KNOCKBACK)) {
                int dano = calcularDanoFinal(mapa, atacante, alvoCelula, habilidade,
                                             indiceHab, distOrigem);
                (void)aplicarDano(mapa, atacante, alvoCelula, dano, habilidade->fogo_amigo);
            }
            if (alvoCelula->vivo && atacante->eh_boss && atacante->fase_boss == 2 &&
                (indiceHab == 0 || indiceHab == 2)) {
                if (alvoCelula->turnos_veneno <= 0) {
                    alvoCelula->turnos_veneno = 2;
                }
            }
            if (alvoCelula->vivo) {
                aplicarEfeito(mapa, atacante, alvoCelula, habilidade, indiceHab,
                              dirAxialLinha, dirAxialColuna);
            }
            if (!atacante->vivo) {
                return;
            }
        }
    }
}

// Encerra o turno e marca vulnerabilidade durante o descanso.
void iniciarDescanso(Tabuleiro3x6 *mapa, Combatente *unidade)
{
    if (unidade == NULL || !unidade->vivo) {
        return;
    }
    unidade->em_descanso = true;
    unidade->atacado_no_descanso = false;
    unidade->turno_perdido = true;
    unidade->acoes_basicas_restantes = 0;
    (void)mapa;
}

// Aplica efeitos persistentes (DoT/HoT) e recarrega recursos no início do turno.
void processarStatusInicioTurno(Tabuleiro3x6 *mapa, Combatente *unidade)
{
    if (unidade == NULL || !unidade->vivo) {
        return;
    }
    venenoAtivoNesteTurno = unidade->turnos_veneno > 0 ? unidade : NULL;
    desmoralizadoAtivoNesteTurno = unidade->turnos_desmoralizado > 0 ? unidade : NULL;
    motivadoAtivoNesteTurno = (unidade->turnos_motivacao > 0 || unidade->motivado_capitao) ?
                              unidade : NULL;
    unidade->turno_perdido = false;
    unidade->usou_consumivel_gratis_turno = false;
    if (unidade->em_descanso) {
        if (unidade->atacado_no_descanso) {
            unidade->estamina_atual += unidade->estamina_maxima / 2;
        } else {
            unidade->estamina_atual = unidade->estamina_maxima;
        }
        if (unidade->estamina_atual > unidade->estamina_maxima) {
            unidade->estamina_atual = unidade->estamina_maxima;
        }
        unidade->em_descanso = false;
        unidade->atacado_no_descanso = false;
    }
    if (unidade->bonus_estamina_proximo_turno > 0) {
        unidade->estamina_atual += unidade->bonus_estamina_proximo_turno;
        if (unidade->estamina_atual > unidade->estamina_maxima) {
            unidade->estamina_atual = unidade->estamina_maxima;
        }
        unidade->bonus_estamina_proximo_turno = 0;
    }
    if (unidade->turnos_sangramento > 0) {
        subtrairVida(mapa, unidade, DANO_SANGRAMENTO_TURNO);
        unidade->turnos_sangramento--;
    }
    if (unidade->vivo && unidade->turnos_veneno > 0) {
        subtrairVida(mapa, unidade, DANO_VENENO_TURNO);
        unidade->turnos_veneno--;
    }
    if (unidade->vivo && unidade->turnos_cura_hot > 0) {
        unidade->vida_atual += unidade->valor_cura_hot;
        if (unidade->vida_atual > unidade->vida_maxima) {
            unidade->vida_atual = unidade->vida_maxima;
        }
        unidade->turnos_cura_hot--;
    }
    if (unidade->turnos_motivacao > 0) {
        unidade->turnos_motivacao--;
        if (unidade->turnos_motivacao == 0) {
            unidade->motivado_capitao = false;
            if (unidade->estamina_maxima > 1) {
                unidade->estamina_maxima -= 1;
                if (unidade->estamina_atual > unidade->estamina_maxima) {
                    unidade->estamina_atual = unidade->estamina_maxima;
                }
            }
        }
    }
    if (unidade->turnos_desmoralizado > 0) {
        unidade->turnos_desmoralizado--;
    }
    if (unidade->turnos_desnorteado > 0) {
        unidade->turnos_desnorteado--;
    }
    if (unidade->turnos_tonico > 0) {
        unidade->turnos_tonico--;
        if (unidade->turnos_tonico == 0) {
            unidade->bonus_tonico_percentual = 0;
        }
    }
    if (unidade->turnos_atordoado > 0) {
        unidade->turnos_atordoado--;
        unidade->turno_perdido = true;
    }
}
