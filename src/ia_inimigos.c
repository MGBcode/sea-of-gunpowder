#include "ia_inimigos.h"

#include "raylib.h"

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static Tabuleiro3x6 *mapaReservas;
static bool reservasStun[LINHAS_TABULEIRO][COLUNAS_TABULEIRO];

// Calcula a resposta de uma curva para uma entrada normalizada entre zero e um.
static double aplicarCurvaResposta(CurvaRespostaIA curva, double valor)
{
    if (valor < 0.0) {
        valor = 0.0;
    } else if (valor > 1.0) {
        valor = 1.0;
    }
    if (curva == CURVE_INVERSE) {
        return 1.0 / (1.0 + 4.0 * valor);
    }
    if (curva == CURVE_EXPONENTIAL) {
        return exp(-4.0 * valor);
    }
    if (curva == CURVE_LOGISTIC) {
        return 1.0 / (1.0 + exp(8.0 * (valor - 0.5)));
    }
    return valor;
}

// Atualiza reservas compartilhadas de stun e os alvos aliados vivos.
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

// Pontua a prioridade de um alvo com base em vida e distância Manhattan.
static int pontuarAlvo(const Combatente *inimigo, const Combatente *alvo)
{
    int distancia = abs(inimigo->linha - alvo->linha) + abs(inimigo->coluna - alvo->coluna);
    int pontuacao = 100 - alvo->vida_atual + 20 - distancia * 3;
    if (pontuacao < 1) {
        pontuacao = 1;
    }
    return pontuacao;
}

// Insere uma ação positiva sem exceder a capacidade do vetor de candidatos.
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

// Estima o dano líquido de uma habilidade contra um alvo.
static int estimarDano(const Combatente *inimigo, const Combatente *alvo,
                       const Habilidade *habilidade)
{
    int dano = inimigo->dano_base + habilidade->dano - alvo->defesa;
    if (inimigo->turnos_veneno > 0) {
        dano -= (dano * 15) / 100;
    }
    return dano < 1 ? 1 : dano;
}

// Gera ações candidatas, ordena-as e escolhe uma entre as melhores ponderadamente.
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
    int velocidadeEfetiva;
    int passoMaximo;
    if (mapa == NULL || inimigo == NULL || escolha == NULL || !inimigo->vivo) {
        return false;
    }
    atualizarTacticalBlackboard(&blackboard, mapa, inimigo);
    habilidades = inimigo->habilidades_desbloqueadas;
    if (habilidades > MAX_HABILIDADES) {
        habilidades = MAX_HABILIDADES;
    }
    for (indice = 0; indice < habilidades; indice++) {
        if (inimigo->turnos_desde_uso[indice] >= 0 &&
            inimigo->turnos_desde_uso[indice] < INT_MAX) {
            inimigo->turnos_desde_uso[indice]++;
        }
    }
    for (indice = 0; indice < habilidades; indice++) {
        Habilidade *habilidade = &inimigo->habilidades[indice];
        int alvoIndice;
        for (alvoIndice = 0; alvoIndice < blackboard.total_alvos; alvoIndice++) {
            Combatente *alvo = blackboard.alvos[alvoIndice];
            int distancia = abs(inimigo->linha - alvo->linha) +
                            abs(inimigo->coluna - alvo->coluna);
            int dano;
            int pontuacao;
            if (distancia > habilidade->alcance) {
                continue;
            }
            alvoAoAlcance = true;
            if (habilidade->custo_estamina < 0 ||
                inimigo->estamina_atual < habilidade->custo_estamina) {
                continue;
            }
            if (habilidade->efeito == EFEITO_ESTUNAR &&
                (alvo->turnos_atordoado > 0 ||
                 blackboard.stun_reservado[alvo->linha][alvo->coluna])) {
                continue;
            }
            dano = estimarDano(inimigo, alvo, habilidade);
            pontuacao = 40 + pontuarAlvo(inimigo, alvo) + dano / 2;
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
    if (!alvoAoAlcance && blackboard.linha_aliado_mais_fraco >= 0) {
        int linha;
        int coluna;
        Combatente *alvo = mapa->celulas[blackboard.linha_aliado_mais_fraco]
                                        [blackboard.coluna_aliado_mais_fraco];
        int distanciaAtual = abs(inimigo->linha - alvo->linha) +
                             abs(inimigo->coluna - alvo->coluna);
        velocidadeEfetiva = inimigo->velocidade;
        if (inimigo->turnos_motivacao > 0) {
            velocidadeEfetiva += 2;
        }
        if (inimigo->turnos_veneno > 0) {
            velocidadeEfetiva -= 3;
        }
        passoMaximo = velocidadeEfetiva >= 7 ? 2 : 1;
        for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
            for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                int custo = abs(linha - inimigo->linha) + abs(coluna - inimigo->coluna);
                int distanciaNova;
                int pontuacao;
                if (custo < 1 || custo > passoMaximo || custo > inimigo->estamina_atual ||
                    mapa->celulas[linha][coluna] != NULL) {
                    continue;
                }
                distanciaNova = abs(linha - alvo->linha) + abs(coluna - alvo->coluna);
                if (distanciaNova >= distanciaAtual) {
                    continue;
                }
                pontuacao = 40 + (distanciaAtual - distanciaNova) * 30 - distanciaNova * 2;
                adicionarCandidato(candidatos, &total, ACAO_IA_MOVER,
                                   linha, coluna, -1, pontuacao);
            }
        }
    }
    if (inimigo->estamina_atual <= 1) {
        double proporcao = inimigo->estamina_maxima > 0
                               ? (double)inimigo->estamina_atual / inimigo->estamina_maxima
                               : 0.0;
        double resposta = (aplicarCurvaResposta(CURVE_INVERSE, proporcao) +
                           aplicarCurvaResposta(CURVE_EXPONENTIAL, proporcao)) / 2.0;
        int pontuacaoDescanso = (int)(100.0 * resposta);
        adicionarCandidato(candidatos, &total, ACAO_IA_DESCANSAR,
                           inimigo->linha, inimigo->coluna, -1, pontuacaoDescanso);
    }
    if (total == 0) {
        return false;
    }
    ordenarCandidatosIA(candidatos, total);
    limite = total < 3 ? total : 3;
    melhorPontuacao = candidatos[0].pontuacao;
    margem = melhorPontuacao * 20 / 100;
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
