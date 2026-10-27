#include "combate.h"

#include "dados_jogo.h"
#include "raylib.h"

#include <stdlib.h>

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
        alvo->em_descanso = false;
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

// Posiciona um aliado em uma das duas colunas iniciais do tabuleiro.
bool posicionarPirataPreBatalha(Tabuleiro3x6 *mapa, Combatente *pirata, int linha, int coluna)
{
    if (mapa == NULL || pirata == NULL || !pirata->aliado ||
        !coordenadaValida(linha, coluna) || coluna > 1 ||
        (mapa->celulas[linha][coluna] != NULL && mapa->celulas[linha][coluna] != pirata)) {
        return false;
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

// Move uma unidade adjacente e consome estamina conforme o deslocamento.
bool moverCombatente3x6(Tabuleiro3x6 *mapa, Combatente *unidade, int destinoLinha, int destinoColuna)
{
    int deslocamentoLinha;
    int deslocamentoColuna;
    int custo;
    if (mapa == NULL || unidade == NULL || !unidade->vivo ||
        !coordenadaValida(unidade->linha, unidade->coluna) ||
        !coordenadaValida(destinoLinha, destinoColuna) ||
        mapa->celulas[unidade->linha][unidade->coluna] != unidade ||
        mapa->celulas[destinoLinha][destinoColuna] != NULL) {
        return false;
    }
    deslocamentoLinha = abs(destinoLinha - unidade->linha);
    deslocamentoColuna = abs(destinoColuna - unidade->coluna);
    if (deslocamentoLinha > 1 || deslocamentoColuna > 1 ||
        deslocamentoLinha + deslocamentoColuna == 0) {
        return false;
    }
    custo = deslocamentoLinha == 1 && deslocamentoColuna == 1 ? 2 : 1;
    if (unidade->estamina_atual < custo) {
        return false;
    }
    mapa->celulas[unidade->linha][unidade->coluna] = NULL;
    unidade->estamina_atual -= custo;
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

    if (habilidade->efeito == EFEITO_BOMBA && habilidade->raio_area > 0) {
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

    if (habilidade->disparo && atacante->linha == alvoLinha &&
        atacante->coluna != alvoColuna) {
        int direcao = alvoColuna > atacante->coluna ? 1 : -1;
        for (coluna = atacante->coluna + direcao; coluna != alvoColuna; coluna += direcao) {
            alvo = mapa->celulas[alvoLinha][coluna];
            if (alvo != NULL) {
                int danoAplicado = alvo->aliado == atacante->aliado ? dano / 2 : dano;
                aplicarDano(mapa, alvo, danoAplicado);
                break;
            }
        }
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
    unidade->turno_perdido = true;
}

// Aplica efeitos de status no início do turno.
void processarStatusInicioTurno(Combatente *unidade)
{
    if (unidade == NULL || !unidade->vivo) {
        return;
    }
    unidade->turno_perdido = false;
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
        liberarListaItensCombatente(&unidade->build_passivos);
        return;
    }
}
