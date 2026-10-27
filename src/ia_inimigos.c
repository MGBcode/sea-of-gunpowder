#include "ia_inimigos.h"

#include "raylib.h"
#include "raymath.h"

#include <stdlib.h>

// Atualiza o contexto tático com os alvos aliados vivos.
void atualizarTacticalBlackboard(TacticalBlackboard *blackboard, Tabuleiro3x6 *mapa, Combatente *inimigo)
{
    int linha;
    int coluna;
    if (blackboard == NULL || mapa == NULL) {
        return;
    }
    blackboard->inimigo = inimigo;
    blackboard->total_alvos = 0;
    blackboard->linha_aliado_mais_fraco = -1;
    blackboard->coluna_aliado_mais_fraco = -1;
    blackboard->aliado_mais_fraco_vida = 0;
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *alvo = mapa->celulas[linha][coluna];
            if (alvo != NULL && alvo->vivo && alvo->aliado != (inimigo != NULL && inimigo->aliado)) {
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

// Evita gerar uma ação contra uma unidade na mesma célula.
static bool mesmaCelulaRaymath(const Combatente *primeira, const Combatente *segunda)
{
    Vector2 posicaoPrimeira = { (float)primeira->coluna, (float)primeira->linha };
    Vector2 posicaoSegunda = { (float)segunda->coluna, (float)segunda->linha };
    return Vector2Equals(posicaoPrimeira, posicaoSegunda);
}

// Pontua o alvo com cálculos inteiros de vida e distância Manhattan.
static int pontuarAlvo(const Combatente *inimigo, const Combatente *alvo)
{
    int distancia = abs(inimigo->linha - alvo->linha) + abs(inimigo->coluna - alvo->coluna);
    int pontuacao = 100 - alvo->vida_atual + 20 - distancia * 3;
    if (pontuacao < 1) {
        pontuacao = 1;
    }
    return pontuacao;
}

// Gera ações candidatas, ordena-as e escolhe uma entre as melhores ponderadamente.
bool selecionarAcaoIA(Tabuleiro3x6 *mapa, Combatente *inimigo, CandidatoAcao *escolha)
{
    TacticalBlackboard blackboard = { 0 };
    CandidatoAcao candidatos[MAX_CANDIDATOS_IA];
    int total = 0;
    int indice;
    int limite;
    int pesoTotal = 0;
    int sorteio;
    if (mapa == NULL || inimigo == NULL || escolha == NULL) {
        return false;
    }
    atualizarTacticalBlackboard(&blackboard, mapa, inimigo);
    for (indice = 0; indice < blackboard.total_alvos && total < MAX_CANDIDATOS_IA; indice++) {
        Combatente *alvo = blackboard.alvos[indice];
        if (mesmaCelulaRaymath(inimigo, alvo)) {
            continue;
        }
        CandidatoAcao candidato = { 0, alvo->linha, alvo->coluna, 0, pontuarAlvo(inimigo, alvo) };
        candidatos[total++] = candidato;
    }
    if (total == 0) {
        return false;
    }
    ordenarCandidatosIA(candidatos, total);
    limite = total < 3 ? total : 3;
    for (indice = 0; indice < limite; indice++) {
        pesoTotal += limite - indice;
    }
    sorteio = GetRandomValue(1, pesoTotal);
    for (indice = 0; indice < limite; indice++) {
        sorteio -= limite - indice;
        if (sorteio <= 0) {
            *escolha = candidatos[indice];
            return true;
        }
    }
    *escolha = candidatos[0];
    return true;
}
