#include "ia_inimigos.h"

#include "raylib.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static Tabuleiro3x6 *mapaReservas;
static bool reservasStun[LINHAS_TABULEIRO][COLUNAS_TABULEIRO];

// Calcula uma curva matemática de resposta (0 a 100) usando aritmética 100% inteira.
static int aplicarCurvaResposta(CurvaRespostaIA curva, int valorPercentual)
{
    if (valorPercentual < 0) {
        valorPercentual = 0;
    } else if (valorPercentual > 100) {
        valorPercentual = 100;
    }
    if (curva == CURVE_INVERSE) {
        return 10000 / (100 + 4 * valorPercentual);
    }
    if (curva == CURVE_EXPONENTIAL) {
        return ((100 - valorPercentual) * (100 - valorPercentual)) / 100;
    }
    if (curva == CURVE_LOGISTIC) {
        if (valorPercentual <= 50) {
            return 100 - (2 * valorPercentual * valorPercentual) / 100;
        }
        return (2 * (100 - valorPercentual) * (100 - valorPercentual)) / 100;
    }
    return valorPercentual;
}

// Atualiza reservas compartilhadas de stun e os alvos aliados vivos no Blackboard.
void atualizarTacticalBlackboard(TacticalBlackboard *blackboard, Tabuleiro3x6 *mapa,
                                 Combatente *inimigo)
{
    int linha;
    int coluna;
    if (blackboard == NULL || mapa == NULL) {
        return;
    }
    if (mapaReservas != mapa) {
        mapaReservas = mapa;
        (void)memset(reservasStun, 0, sizeof(reservasStun));
    }
    blackboard->inimigo = inimigo;
    blackboard->total_alvos = 0;
    blackboard->linha_aliado_mais_fraco = -1;
    blackboard->coluna_aliado_mais_fraco = -1;
    blackboard->aliado_mais_fraco_vida = 0;
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *alvo = mapa->celulas[linha][coluna];
            if (reservasStun[linha][coluna] &&
                (alvo == NULL || !alvo->vivo || alvo->turnos_atordoado <= 0)) {
                reservasStun[linha][coluna] = false;
            }
            blackboard->stun_reservado[linha][coluna] = reservasStun[linha][coluna];
            if (alvo != NULL && alvo->vivo && inimigo != NULL &&
                alvo->aliado != inimigo->aliado) {
                blackboard->alvos[blackboard->total_alvos++] = alvo;
                if (blackboard->linha_aliado_mais_fraco < 0 ||
                    alvo->vida_atual < blackboard->aliado_mais_fraco_vida) {
                    blackboard->linha_aliado_mais_fraco = linha;
                    blackboard->coluna_aliado_mais_fraco = coluna;
                    blackboard->aliado_mais_fraco_vida = alvo->vida_atual;
                }
            }
        }
    }
}

// Pontua a prioridade tática de um alvo com base em curva de vida e distância Manhattan.
static int pontuarAlvo(const Combatente *inimigo, const Combatente *alvo)
{
    int distancia = abs(inimigo->linha - alvo->linha) + abs(inimigo->coluna - alvo->coluna);
    int pctVida = alvo->vida_maxima > 0 ? (alvo->vida_atual * 100) / alvo->vida_maxima : 100;
    int urgenciaAlvo = aplicarCurvaResposta(CURVE_EXPONENTIAL, pctVida);
    int pontuacao = 30 + urgenciaAlvo + 20 - distancia * 4;
    if (pontuacao < 1) {
        pontuacao = 1;
    }
    return pontuacao;
}

// Insere uma ação candidata positiva sem exceder a capacidade do vetor.
static void adicionarCandidato(CandidatoAcao candidatos[], int *total,
                               TipoAcaoIA tipo, int linha, int coluna,
                               int habilidade, int pontuacao)
{
    if (*total >= MAX_CANDIDATOS_IA || pontuacao <= 0) {
        return;
    }
    candidatos[*total].tipo_acao = tipo;
    candidatos[*total].linha_alvo = linha;
    candidatos[*total].coluna_alvo = coluna;
    candidatos[*total].indice_habilidade = habilidade;
    candidatos[*total].pontuacao = pontuacao;
    (*total)++;
}

// Estima o dano médio líquido de uma habilidade contra um alvo.
static int estimarDano(const Combatente *inimigo, const Combatente *alvo,
                       const Habilidade *habilidade, int indiceHab, int distancia)
{
    int danoMin = habilidade->dano_min;
    int danoMax = habilidade->dano_max;
    int dano;
    if (inimigo->tipo_inimigo == INIMIGO_MARUJO_GORDO && indiceHab == 0) {
        danoMin = distancia == 1 ? habilidade->dano_max : habilidade->dano_min;
        danoMax = danoMin;
    }
    dano = inimigo->dano_base + (danoMin + danoMax) / 2 - alvo->defesa;
    if (inimigo->eh_boss && inimigo->fase_boss == 2) {
        if (indiceHab == 0) {
            dano = (dano * 115) / 100;
        } else if (indiceHab == 1) {
            dano = (dano * 120) / 100;
        }
    }
    return dano < 1 ? 1 : dano;
}

// Calibra o bônus tático conforme o arquétipo do inimigo ou a fase do Barba Negra.
static int calibrarBonusArquetipo(const Tabuleiro3x6 *mapa, const Combatente *inimigo,
                                  const Combatente *alvo, const Habilidade *habilidade,
                                  int indiceHab, int distancia)
{
    int bonus = 0;
    int linha;
    int coluna;
    if (inimigo->tipo_inimigo == INIMIGO_GRUNT_ESPADA) {
        bonus += alvo->golpes_corte_grunt_recebidos * 25;
    } else if (inimigo->tipo_inimigo == INIMIGO_GRUNT_PISTOLA) {
        if (indiceHab == 0) {
            int dirLin = alvo->linha > inimigo->linha ? 1 : (alvo->linha < inimigo->linha ? -1 : 0);
            int dirCol = alvo->coluna > inimigo->coluna ? 1 : (alvo->coluna < inimigo->coluna ? -1 : 0);
            int passo;
            for (passo = 1; passo <= habilidade->alcance; passo++) {
                int celLin = inimigo->linha + passo * dirLin;
                int celCol = inimigo->coluna + passo * dirCol;
                if (celLin >= 0 && celLin < LINHAS_TABULEIRO &&
                    celCol >= 0 && celCol < COLUNAS_TABULEIRO &&
                    mapa->celulas[celLin][celCol] != NULL &&
                    mapa->celulas[celLin][celCol]->vivo) {
                    if (mapa->celulas[celLin][celCol]->aliado == inimigo->aliado) {
                        bonus -= 45;
                    } else if (mapa->celulas[celLin][celCol] != alvo) {
                        bonus += 30;
                    }
                }
            }
        } else if (indiceHab == 1) {
            for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
                for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                    if (abs(linha - alvo->linha) + abs(coluna - alvo->coluna) <= 1 &&
                        mapa->celulas[linha][coluna] != NULL &&
                        mapa->celulas[linha][coluna]->vivo) {
                        if (mapa->celulas[linha][coluna]->aliado == inimigo->aliado) {
                            bonus -= 40;
                        } else if (mapa->celulas[linha][coluna] != alvo) {
                            bonus += 35;
                        }
                    }
                }
            }
        }
    } else if (inimigo->tipo_inimigo == INIMIGO_ATIRADOR_MAGRELO) {
        int pctVida = alvo->vida_maxima > 0 ? (alvo->vida_atual * 100) / alvo->vida_maxima : 100;
        bonus += aplicarCurvaResposta(CURVE_LOGISTIC, pctVida) / 2;
        if (distancia >= 3) {
            bonus += 20;
        }
    } else if (inimigo->tipo_inimigo == INIMIGO_MARUJO_GORDO) {
        if (indiceHab == 0 && distancia == 1) {
            bonus += 35;
        }
    } else if (inimigo->eh_boss) {
        if (indiceHab == 0 && inimigo->fase_boss == 2 && alvo->turnos_veneno <= 0) {
            bonus += 35;
        } else if (indiceHab == 2) {
            int colunaExtra = alvo->coluna >= inimigo->coluna ? 1 : -1;
            int piratasNaArea = 0;
            for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
                for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                    if (abs(linha - alvo->linha) <= 1 &&
                        (coluna == alvo->coluna || coluna == alvo->coluna + colunaExtra) &&
                        mapa->celulas[linha][coluna] != NULL &&
                        mapa->celulas[linha][coluna]->vivo &&
                        mapa->celulas[linha][coluna]->aliado != inimigo->aliado) {
                        piratasNaArea++;
                    }
                }
            }
            bonus += (piratasNaArea - 1) * 45;
        }
    }
    return bonus;
}

// Gera ações candidatas, ordena-as com Insertion Sort e escolhe no Top-3 ponderado.
bool selecionarAcaoIA(Tabuleiro3x6 *mapa, Combatente *inimigo, CandidatoAcao *escolha)
{
    TacticalBlackboard blackboard = { 0 };
    CandidatoAcao candidatos[MAX_CANDIDATOS_IA];
    int total = 0;
    int habilidades;
    int indice;
    int limite;
    int melhorPontuacao;
    int margem;
    int pesoTotal = 0;
    int sorteio;
    bool alvoAoAlcance = false;
    int penalidadeDesmoralizado;
    if (mapa == NULL || inimigo == NULL || escolha == NULL || !inimigo->vivo) {
        return false;
    }
    atualizarTacticalBlackboard(&blackboard, mapa, inimigo);
    habilidades = inimigo->habilidades_desbloqueadas;
    if (habilidades > MAX_HABILIDADES) {
        habilidades = MAX_HABILIDADES;
    }
    penalidadeDesmoralizado = inimigo->turnos_desmoralizado > 0 ? 1 : 0;
    for (indice = 0; indice < habilidades; indice++) {
        if (inimigo->turnos_desde_uso[indice] >= 0 &&
            inimigo->turnos_desde_uso[indice] < INT_MAX) {
            inimigo->turnos_desde_uso[indice]++;
        }
    }
    for (indice = 0; indice < habilidades; indice++) {
        Habilidade *habilidade = &inimigo->habilidades[indice];
        int custoReal = habilidade->custo_estamina + penalidadeDesmoralizado;
        int alvoIndice;
        if (habilidade->custo_estamina < 0 || inimigo->estamina_atual < custoReal) {
            continue;
        }
        if (habilidade->efeito == EFEITO_CURA_CONTINUA) {
            int linha;
            int coluna;
            for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
                for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                    Combatente *aliadoFerido = mapa->celulas[linha][coluna];
                    int distAliado;
                    int pctVidaAliado;
                    int urgenciaCura;
                    if (aliadoFerido == NULL || aliadoFerido == inimigo ||
                        !aliadoFerido->vivo || aliadoFerido->aliado != inimigo->aliado ||
                        aliadoFerido->vida_atual >= aliadoFerido->vida_maxima ||
                        aliadoFerido->turnos_cura_hot > 0) {
                        continue;
                    }
                    distAliado = abs(inimigo->linha - linha) + abs(inimigo->coluna - coluna);
                    if (distAliado <= habilidade->alcance &&
                        (linha == inimigo->linha || coluna == inimigo->coluna)) {
                        pctVidaAliado = (aliadoFerido->vida_atual * 100) /
                                        aliadoFerido->vida_maxima;
                        urgenciaCura = aplicarCurvaResposta(CURVE_INVERSE, pctVidaAliado);
                        adicionarCandidato(candidatos, &total, ACAO_IA_HABILIDADE,
                                           linha, coluna, indice, 70 + urgenciaCura);
                    }
                }
            }
            continue;
        }
        if (habilidade->efeito == EFEITO_CURA || habilidade->efeito == EFEITO_MOTIVACAO) {
            continue;
        }
        for (alvoIndice = 0; alvoIndice < blackboard.total_alvos; alvoIndice++) {
            Combatente *alvo = blackboard.alvos[alvoIndice];
            int distancia = abs(inimigo->linha - alvo->linha) +
                            abs(inimigo->coluna - alvo->coluna);
            bool exigeAlinhamento = habilidade->tipo_ataque != ATAQUE_AREA_3X2;
            int dano;
            int pontuacao;
            if (habilidade->tipo_ataque != ATAQUE_AREA_3X2 &&
                (distancia <= 0 || distancia > habilidade->alcance)) {
                continue;
            }
            if (exigeAlinhamento &&
                alvo->linha != inimigo->linha && alvo->coluna != inimigo->coluna) {
                continue;
            }
            if (habilidade->tipo_ataque != ATAQUE_AREA_3X2 &&
                habilidade->tipo_ataque != ATAQUE_CRUZ &&
                habilidade->tipo_ataque != ATAQUE_AREA_1X3 &&
                !habilidade->perfurante && distancia > 1) {
                int dirLin = (alvo->linha > inimigo->linha) ? 1 :
                             ((alvo->linha < inimigo->linha) ? -1 : 0);
                int dirCol = (alvo->coluna > inimigo->coluna) ? 1 :
                             ((alvo->coluna < inimigo->coluna) ? -1 : 0);
                int passo;
                bool bloqueado = false;
                for (passo = 1; passo < distancia; passo++) {
                    int celLin = inimigo->linha + passo * dirLin;
                    int celCol = inimigo->coluna + passo * dirCol;
                    Combatente *ocupante = mapa->celulas[celLin][celCol];
                    if (ocupante != NULL && ocupante->vivo &&
                        (ocupante->aliado != inimigo->aliado || habilidade->fogo_amigo)) {
                        bloqueado = true;
                        break;
                    }
                }
                if (bloqueado) {
                    continue;
                }
            }
            if (habilidade->efeito == EFEITO_ESTUNAR &&
                (alvo->turnos_atordoado > 0 ||
                 blackboard.stun_reservado[alvo->linha][alvo->coluna])) {
                continue;
            }
            alvoAoAlcance = true;
            dano = estimarDano(inimigo, alvo, habilidade, indice, distancia);
            pontuacao = 40 + pontuarAlvo(inimigo, alvo) + dano / 2;
            pontuacao += calibrarBonusArquetipo(mapa, inimigo, alvo, habilidade,
                                                indice, distancia);
            if (dano >= alvo->vida_atual) {
                pontuacao += 100;
            }
            if (alvo->em_descanso) {
                pontuacao += 35;
            }
            if (alvo->linha == blackboard.linha_aliado_mais_fraco &&
                alvo->coluna == blackboard.coluna_aliado_mais_fraco) {
                pontuacao += 20;
            }
            if (inimigo->turnos_desde_uso[indice] == 1) {
                pontuacao -= 35;
            }
            if (pontuacao < 1) {
                pontuacao = 1;
            }
            adicionarCandidato(candidatos, &total, ACAO_IA_HABILIDADE,
                               alvo->linha, alvo->coluna, indice, pontuacao);
        }
    }
    if ((!alvoAoAlcance || (inimigo->eh_boss && mapa->boss_fase_2_ativa)) &&
        blackboard.linha_aliado_mais_fraco >= 0 && inimigo->estamina_atual >= 1) {
        int linha;
        int coluna;
        Combatente *alvo = mapa->celulas[blackboard.linha_aliado_mais_fraco]
                                        [blackboard.coluna_aliado_mais_fraco];
        int distanciaAtual = abs(inimigo->linha - alvo->linha) +
                             abs(inimigo->coluna - alvo->coluna);
        bool usaAtaqueLinha = inimigo->habilidades[0].tipo_ataque == ATAQUE_LINHA;
        for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
            for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                int passoDist = abs(linha - inimigo->linha) + abs(coluna - inimigo->coluna);
                int distanciaNova;
                int pontuacao;
                bool alinhaTiro;
                if (passoDist != 1 || mapa->celulas[linha][coluna] != NULL ||
                    mapa->mina_ativa[linha][coluna] > 0) {
                    continue;
                }
                distanciaNova = abs(linha - alvo->linha) + abs(coluna - alvo->coluna);
                alinhaTiro = usaAtaqueLinha &&
                             (linha == alvo->linha || coluna == alvo->coluna) &&
                             distanciaNova <= inimigo->habilidades[0].alcance;
                if (distanciaNova >= distanciaAtual && !alinhaTiro) {
                    continue;
                }
                pontuacao = 45 + (distanciaAtual - distanciaNova) * 30 - distanciaNova * 2;
                if (alinhaTiro) {
                    pontuacao += 35;
                }
                if (inimigo->eh_boss && mapa->boss_fase_2_ativa &&
                    (mapa->turnos_boss_fase_2 + 1) % 2 == 0) {
                    pontuacao += 40;
                }
                adicionarCandidato(candidatos, &total, ACAO_IA_MOVER,
                                   linha, coluna, -1, pontuacao);
            }
        }
    }
    if (inimigo->estamina_atual <= 1 ||
        (total == 0 && inimigo->estamina_atual < inimigo->estamina_maxima)) {
        int proporcao = inimigo->estamina_maxima > 0
                            ? (inimigo->estamina_atual * 100) / inimigo->estamina_maxima
                            : 0;
        int resposta = (aplicarCurvaResposta(CURVE_INVERSE, proporcao) +
                        aplicarCurvaResposta(CURVE_EXPONENTIAL, proporcao)) / 2;
        if (resposta < 1) {
            resposta = 1;
        }
        adicionarCandidato(candidatos, &total, ACAO_IA_DESCANSAR,
                           inimigo->linha, inimigo->coluna, -1, resposta);
    }
    if (total == 0) {
        return false;
    }
    ordenarCandidatosIA(candidatos, total);
    limite = total < 3 ? total : 3;
    melhorPontuacao = candidatos[0].pontuacao;
    margem = (melhorPontuacao * 20) / 100;
    for (indice = 0; indice < limite && candidatos[indice].pontuacao >=
                                            melhorPontuacao - margem; indice++) {
        pesoTotal += candidatos[indice].pontuacao;
    }
    limite = indice;
    if (limite == 0 || pesoTotal <= 0) {
        return false;
    }
    sorteio = GetRandomValue(1, pesoTotal);
    for (indice = 0; indice < limite; indice++) {
        sorteio -= candidatos[indice].pontuacao;
        if (sorteio <= 0) {
            *escolha = candidatos[indice];
            if (escolha->tipo_acao == ACAO_IA_HABILIDADE) {
                int habilidade = escolha->indice_habilidade;
                inimigo->turnos_desde_uso[habilidade] = 0;
                if (inimigo->habilidades[habilidade].efeito == EFEITO_ESTUNAR) {
                    reservasStun[escolha->linha_alvo][escolha->coluna_alvo] = true;
                }
            }
            return true;
        }
    }
    return false;
}
