#include "combate.h"

#include "dados_jogo.h"
#include "ordenacao.h"
#include "raylib.h"

#include <stdlib.h>

// Monta a fila da rodada com combatentes vivos ordenados por velocidade.
int montarFilaTurnos(Tabuleiro3x6 *mapa, Combatente *filaOut[])
{
    int linha;
    int coluna;
    int totalVivos = 0;
    if (mapa == NULL || filaOut == NULL) {
        return 0;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *unidade = mapa->celulas[linha][coluna];
            if (unidade != NULL && unidade->vivo) {
                filaOut[totalVivos++] = unidade;
            }
        }
    }
    ordenarFilaPorVelocidade(filaOut, totalVivos);
    return totalVivos;
}

// Aplica e consome um item de cura, estamina ou remoção de status.
bool usarItemConsumivel(Combatente *unidade, Item *item)
{
    if (unidade == NULL || item == NULL || !unidade->vivo ||
        item->tipo != ITEM_CONSUMIVEL || item->quantidade <= 0) {
        return false;
    }
    if (item->efeito_consumivel == CONSUMIVEL_CURAR_VIDA) {
        if (item->valor_efeito <= 0 || unidade->vida_atual >= unidade->vida_maxima) {
            return false;
        }
        unidade->vida_atual += item->valor_efeito;
        if (unidade->vida_atual > unidade->vida_maxima) {
            unidade->vida_atual = unidade->vida_maxima;
        }
    } else if (item->efeito_consumivel == CONSUMIVEL_RESTAURAR_ESTAMINA) {
        if (item->valor_efeito <= 0 || unidade->estamina_atual >= unidade->estamina_maxima) {
            return false;
        }
        unidade->estamina_atual += item->valor_efeito;
        if (unidade->estamina_atual > unidade->estamina_maxima) {
            unidade->estamina_atual = unidade->estamina_maxima;
        }
    } else if (item->efeito_consumivel == CONSUMIVEL_REMOVER_STATUS) {
        if (unidade->turnos_veneno <= 0 && unidade->turnos_sangramento <= 0) {
            return false;
        }
        unidade->turnos_veneno = 0;
        unidade->turnos_sangramento = 0;
    } else {
        return false;
    }
    item->quantidade--;
    return true;
}

// Verifica se uma equipe foi derrotada e informa se os aliados venceram.
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

// Avança até a próxima unidade viva que possa agir.
bool avancarProximoTurno(Combatente *fila[], int totalVivos, int *indiceAtual, Tabuleiro3x6 *mapa)
{
    if (fila == NULL || indiceAtual == NULL || mapa == NULL || totalVivos <= 0 ||
        *indiceAtual < -1 || *indiceAtual >= totalVivos) {
        return false;
    }
    while (++(*indiceAtual) < totalVivos) {
        Combatente *unidade = fila[*indiceAtual];
        if (unidade == NULL || !unidade->vivo) {
            continue;
        }
        processarStatusInicioTurno(mapa, unidade);
        if (unidade->vivo && !unidade->turno_perdido) {
            return true;
        }
    }
    return false;
}

// Verifica se uma coordenada pertence ao tabuleiro.
static bool coordenadaValida(int linha, int coluna)
{
    return linha >= 0 && linha < LINHAS_TABULEIRO &&
           coluna >= 0 && coluna < COLUNAS_TABULEIRO;
}

// Aplica dano e remove permanentemente um aliado derrotado.
static void aplicarDano(Tabuleiro3x6 *mapa, Combatente *alvo, int dano)
{
    int danoRecebido;
    if (alvo == NULL || !alvo->vivo || dano <= 0) {
        return;
    }
    danoRecebido = dano - alvo->defesa;
    if (danoRecebido < 1) {
        danoRecebido = 1;
    }
    if (alvo->em_descanso) {
        danoRecebido += danoRecebido / 2;
        alvo->atacado_no_descanso = true;
    }
    alvo->vida_atual -= danoRecebido;
    if (alvo->vida_atual <= 0) {
        alvo->vida_atual = 0;
        alvo->vivo = false;
        if (mapa != NULL && coordenadaValida(alvo->linha, alvo->coluna) &&
            mapa->celulas[alvo->linha][alvo->coluna] == alvo) {
            mapa->celulas[alvo->linha][alvo->coluna] = NULL;
        }
        liberarListaItensCombatente(&alvo->build_passivos);
    }
}

// Aplica um efeito de habilidade ao combatente atingido.
static void aplicarEfeito(Combatente *alvo, TipoEfeito efeito)
{
    if (alvo == NULL || !alvo->vivo) {
        return;
    }
    if (efeito == EFEITO_MOTIVACAO) {
        alvo->turnos_motivacao = 2;
        if (alvo->estamina_atual < alvo->estamina_maxima) {
            alvo->estamina_atual++;
        }
    } else if (efeito == EFEITO_ESTUNAR) {
        alvo->turnos_atordoado = 1;
    } else if (efeito == EFEITO_SANGRAMENTO) {
        alvo->turnos_sangramento = 3;
    } else if (efeito == EFEITO_VENENO) {
        alvo->turnos_veneno = 2;
    }
}

// Calcula dano final com crítico e redução por envenenamento.
static int calcularDanoFinal(Combatente *atacante, const Habilidade *habilidade)
{
    int dano = habilidade->dano + atacante->dano_base;
    if (atacante->turnos_veneno > 0) {
        dano -= (dano * 15) / 100;
    }
    if (habilidade->chance_critico > 0 &&
        GetRandomValue(1, 100) <= habilidade->chance_critico) {
        dano *= 2;
    }
    return dano < 0 ? 0 : dano;
}

// Posiciona ou troca aliados entre as duas colunas iniciais do tabuleiro.
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

// Move uma unidade até seu alcance permitido e consome estamina pela distância.
bool moverCombatente3x6(Tabuleiro3x6 *mapa, Combatente *unidade, int destinoLinha, int destinoColuna)
{
    int distancia;
    int velocidadeEfetiva;
    int passoMaximo;
    if (mapa == NULL || unidade == NULL || !unidade->vivo ||
        !coordenadaValida(unidade->linha, unidade->coluna) ||
        !coordenadaValida(destinoLinha, destinoColuna) ||
        mapa->celulas[unidade->linha][unidade->coluna] != unidade ||
        mapa->celulas[destinoLinha][destinoColuna] != NULL) {
        return false;
    }
    distancia = abs(destinoLinha - unidade->linha) + abs(destinoColuna - unidade->coluna);
    velocidadeEfetiva = unidade->velocidade;
    if (unidade->turnos_motivacao > 0) {
        velocidadeEfetiva += 2;
    }
    if (unidade->turnos_veneno > 0) {
        velocidadeEfetiva -= 3;
    }
    passoMaximo = velocidadeEfetiva >= 7 ? 2 : 1;
    if (distancia < 1 || distancia > passoMaximo) {
        return false;
    }
    if (unidade->estamina_atual < distancia) {
        return false;
    }
    mapa->celulas[unidade->linha][unidade->coluna] = NULL;
    unidade->estamina_atual -= distancia;
    unidade->linha = destinoLinha;
    unidade->coluna = destinoColuna;
    mapa->celulas[destinoLinha][destinoColuna] = unidade;
    return true;
}

// Executa uma habilidade, aplica efeitos e remove aliados derrotados permanentemente.
void executarAtaqueOuHabilidade(Tabuleiro3x6 *mapa, Combatente *atacante, int indiceHab, int alvoLinha, int alvoColuna)
{
    Habilidade *habilidade;
    int dano;
    int linha;
    int coluna;
    Combatente *alvo;
    if (mapa == NULL || atacante == NULL || !atacante->vivo ||
        indiceHab < 0 || indiceHab >= atacante->habilidades_desbloqueadas ||
        !coordenadaValida(alvoLinha, alvoColuna)) {
        return;
    }
    habilidade = &atacante->habilidades[indiceHab];
    if (atacante->estamina_atual < habilidade->custo_estamina ||
        abs(alvoLinha - atacante->linha) + abs(alvoColuna - atacante->coluna) > habilidade->alcance) {
        return;
    }
    if (habilidade->disparo &&
        (alvoLinha != atacante->linha || alvoColuna == atacante->coluna)) {
        return;
    }
    alvo = mapa->celulas[alvoLinha][alvoColuna];
    if (habilidade->efeito != EFEITO_BOMBA &&
        (alvo == NULL || !alvo->vivo || (alvo->aliado == atacante->aliado &&
                          habilidade->efeito != EFEITO_MOTIVACAO))) {
        return;
    }
    if (habilidade->efeito == EFEITO_MOTIVACAO &&
        (alvo == NULL || !alvo->vivo || alvo->aliado != atacante->aliado)) {
        return;
    }
    atacante->estamina_atual -= habilidade->custo_estamina;
    dano = calcularDanoFinal(atacante, habilidade);

    if (habilidade->efeito == EFEITO_MOTIVACAO) {
        aplicarEfeito(alvo, habilidade->efeito);
        return;
    }

    if (habilidade->efeito == EFEITO_BOMBA) {
        for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
            for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                int distancia = abs(linha - alvoLinha) + abs(coluna - alvoColuna);
                alvo = mapa->celulas[linha][coluna];
                if (alvo != NULL && distancia <= habilidade->raio_area) {
                    int danoAplicado = alvo->aliado == atacante->aliado ? dano / 2 : dano;
                    aplicarDano(mapa, alvo, danoAplicado);
                    aplicarEfeito(alvo, habilidade->efeito);
                }
            }
        }
        return;
    }

    if (habilidade->disparo) {
        int direcao = alvoColuna > atacante->coluna ? 1 : -1;
        for (coluna = atacante->coluna + direcao;; coluna += direcao) {
            alvo = mapa->celulas[alvoLinha][coluna];
            if (alvo != NULL) {
                int danoAplicado = alvo->aliado == atacante->aliado ? dano / 2 : dano;
                aplicarDano(mapa, alvo, danoAplicado);
                aplicarEfeito(alvo, habilidade->efeito);
            }
            if (coluna == alvoColuna) {
                break;
            }
        }
        return;
    }
    alvo = mapa->celulas[alvoLinha][alvoColuna];
    if (alvo != NULL) {
        aplicarDano(mapa, alvo, dano);
        aplicarEfeito(alvo, habilidade->efeito);
    }
}

// Encerra o turno da unidade e marca sua vulnerabilidade.
void iniciarDescanso(Combatente *unidade)
{
    if (unidade == NULL || !unidade->vivo) {
        return;
    }
    unidade->em_descanso = true;
    unidade->atacado_no_descanso = false;
    unidade->turno_perdido = true;
}

// Aplica efeitos de status no início do turno.
void processarStatusInicioTurno(Tabuleiro3x6 *mapa, Combatente *unidade)
{
    if (unidade == NULL || !unidade->vivo) {
        return;
    }
    unidade->turno_perdido = false;
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
    if (unidade->turnos_sangramento > 0) {
        unidade->vida_atual -= 10;
        unidade->turnos_sangramento--;
    }
    if (unidade->turnos_veneno > 0) {
        unidade->vida_atual -= 5;
        unidade->turnos_veneno--;
    }
    if (unidade->turnos_motivacao > 0) {
        unidade->turnos_motivacao--;
    }
    if (unidade->turnos_atordoado > 0) {
        unidade->turnos_atordoado--;
        unidade->turno_perdido = true;
    }
    if (unidade->vida_atual <= 0) {
        unidade->vida_atual = 0;
        unidade->vivo = false;
        if (mapa != NULL && coordenadaValida(unidade->linha, unidade->coluna) &&
            mapa->celulas[unidade->linha][unidade->coluna] == unidade) {
            mapa->celulas[unidade->linha][unidade->coluna] = NULL;
        }
        liberarListaItensCombatente(&unidade->build_passivos);
        return;
    }
}
