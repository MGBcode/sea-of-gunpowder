#include "interface.h"

#include "combate.h"
#include "dados_jogo.h"
#include "ia_inimigos.h"
#include "ordenacao.h"
#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Adiciona o catálogo inteiro ao inventário de teste.
static bool carregarInventarioSanitario(ListaItens *inventario, const Item itens[],
                                         int totalItens)
{
    int indice;
    for (indice = 0; indice < totalItens; indice++) {
        NoItem *novo = malloc(sizeof(*novo));
        if (novo == NULL) {
            return false;
        }
        novo->item = itens[indice];
        novo->proximo = inventario->inicio;
        inventario->inicio = novo;
        inventario->tamanho++;
    }
    return true;
}

// Testa todas as classes de piratas, habilidades, passivas Nv 1/4 e geometrias AoE.
static bool testarClassesEMecanicasCombate(void)
{
    Tabuleiro3x6 mapa;
    Combatente bucaneiro;
    Combatente timoneiro;
    Combatente cozinheiro;
    Combatente capitao;
    Combatente gavea;
    Combatente inimigoA;
    Combatente inimigoB;
    Combatente inimigoC;
    Item catalogo[TOTAL_ITENS_PREDEFINIDOS];

    (void)carregarItensIniciais(catalogo, TOTAL_ITENS_PREDEFINIDOS);
    (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiro);
    (void)criarPirataInicial(CLASSE_TIMONEIRO, &timoneiro);
    (void)criarPirataInicial(CLASSE_COZINHEIRO, &cozinheiro);
    (void)criarPirataInicial(CLASSE_CAPITAO, &capitao);
    (void)criarPirataInicial(CLASSE_GAVEA, &gavea);
    if (bucaneiro.vida_maxima != 150 || bucaneiro.velocidade != 9 ||
        bucaneiro.precisao_melee != 90 || bucaneiro.estamina_maxima != 5 ||
        bucaneiro.habilidades[0].dano_min != 60 || bucaneiro.habilidades[0].dano_max != 90 ||
        bucaneiro.habilidades[2].dano_min != 120 || bucaneiro.habilidades[2].dano_max != 150 ||
        timoneiro.vida_maxima != 250 || timoneiro.velocidade != 5 ||
        timoneiro.precisao_melee != 60 || timoneiro.estamina_maxima != 5 ||
        timoneiro.habilidades[0].dano_min != 70 || timoneiro.habilidades[0].dano_max != 100 ||
        timoneiro.habilidades[1].dano_min != 90 || timoneiro.habilidades[1].dano_max != 120 ||
        timoneiro.habilidades[2].dano_min != 50 || timoneiro.habilidades[2].dano_max != 85 ||
        cozinheiro.vida_maxima != 200 || cozinheiro.velocidade != 5 ||
        cozinheiro.precisao_melee != 100 || cozinheiro.estamina_maxima != 5 ||
        cozinheiro.habilidades[0].dano_max != 20 ||
        cozinheiro.habilidades[1].dano_min != 60 || cozinheiro.habilidades[1].dano_max != 100 ||
        capitao.vida_maxima != 170 || capitao.velocidade != 7 ||
        capitao.precisao_melee != 70 || capitao.precisao_ranged != 60 ||
        capitao.habilidades[0].dano_min != 75 || capitao.habilidades[0].dano_max != 100 ||
        capitao.habilidades[1].custo_estamina != 1 ||
        capitao.habilidades[2].dano_min != 100 || capitao.habilidades[2].dano_max != 130 ||
        gavea.vida_maxima != 120 || gavea.velocidade != 6 ||
        gavea.precisao_melee != 40 || gavea.estamina_maxima != 5 ||
        gavea.habilidades[0].dano_min != 60 || gavea.habilidades[0].dano_max != 110 ||
        gavea.habilidades[2].dano_min != 150 || gavea.habilidades[2].dano_max != 200) {
        fprintf(stderr, "Falha sanitaria: atributos base dos 5 piratas incorretos.\n");
        return false;
    }

    (void)subirNivelPirata(&gavea, 1);
    if (gavea.nivel != 2 || gavea.estamina_maxima != 6 || gavea.vida_maxima != 144 ||
        gavea.habilidades[0].dano_min != 72 || gavea.habilidades[0].dano_max != 132) {
        fprintf(stderr, "Falha sanitaria: Gavea Nv 2 (estamina 6 e +20%% vida/dano).\n");
        return false;
    }
    (void)subirNivelPirata(&gavea, 2);
    if (gavea.nivel != 4 || gavea.estamina_maxima != 7) {
        fprintf(stderr, "Falha sanitaria: Gavea Nv 4 deve ter 7 de estamina.\n");
        return false;
    }
    {
        int distancia;
        static const int danoEsperado[] = { 115, 110, 105 };
        for (distancia = 1; distancia <= 3; distancia++) {
            Combatente atacante;
            Combatente alvo;
            (void)memset(&mapa, 0, sizeof(mapa));
            (void)criarPirataInicial(CLASSE_GAVEA, &atacante);
            (void)subirNivelPirata(&atacante, 3);
            atacante.passiva_aprimorada = true;
            atacante.linha = 1;
            atacante.coluna = 0;
            atacante.habilidades[0].precisao = 100;
            atacante.habilidades[0].dano_min = 100;
            atacante.habilidades[0].dano_max = 100;
            (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &alvo);
            alvo.vida_maxima = 10000;
            alvo.vida_atual = 10000;
            alvo.defesa = 0;
            alvo.linha = 1;
            alvo.coluna = distancia;
            mapa.celulas[1][0] = &atacante;
            mapa.celulas[1][distancia] = &alvo;
            executarAtaqueOuHabilidade(&mapa, &atacante, 0, 1, distancia);
            if (alvo.vida_atual != 10000 - danoEsperado[distancia - 1]) {
                fprintf(stderr, "Falha sanitaria: passiva do Gavea deve conceder 15%%, 10%% e 5%% de dano a 1, 2 e 3 casas.\n");
                return false;
            }
        }
    }

    (void)subirNivelPirata(&bucaneiro, 1);
    if (bucaneiro.estamina_maxima != 7) {
        fprintf(stderr, "Falha sanitaria: Bucaneiro Nv 2 deve ter 7 de estamina.\n");
        return false;
    }
    (void)subirNivelPirata(&bucaneiro, 2);
    if (bucaneiro.estamina_maxima != 10) {
        fprintf(stderr, "Falha sanitaria: Bucaneiro Nv 4 deve ter 10 de estamina.\n");
        return false;
    }

    (void)subirNivelPirata(&timoneiro, 1);
    if (timoneiro.estamina_maxima != 6) {
        fprintf(stderr, "Falha sanitaria: Timoneiro Nv 2 deve ter 6 de estamina.\n");
        return false;
    }
    (void)subirNivelPirata(&timoneiro, 2);
    if (timoneiro.estamina_maxima != 7) {
        fprintf(stderr, "Falha sanitaria: Timoneiro Nv 4 deve ter 7 de estamina.\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiro);
    bucaneiro.linha = 1;
    bucaneiro.coluna = 0;
    mapa.celulas[1][0] = &bucaneiro;
    if (!moverCombatente3x6(&mapa, &bucaneiro, 1, 2) ||
        bucaneiro.coluna != 2 || bucaneiro.estamina_atual != 4) {
        fprintf(stderr, "Falha sanitaria: passiva Nv 1 do Bucaneiro (andar 2 casas).\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiro);
    (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoA);
    inimigoA.vida_maxima = 900;
    inimigoA.vida_atual = 900;
    bucaneiro.linha = 1;
    bucaneiro.coluna = 1;
    inimigoA.linha = 1;
    inimigoA.coluna = 2;
    mapa.celulas[1][1] = &bucaneiro;
    mapa.celulas[1][2] = &inimigoA;
    executarAtaqueOuHabilidade(&mapa, &bucaneiro, 0, 1, 2);
    if (inimigoA.turnos_sangramento != 0 ||
        inimigoA.golpes_corte_bucaneiro_recebidos != 1) {
        fprintf(stderr, "Falha sanitaria: 1o Ataque 1 do Bucaneiro nao deve sangrar ainda.\n");
        return false;
    }
    bucaneiro.estamina_atual = bucaneiro.estamina_maxima;
    executarAtaqueOuHabilidade(&mapa, &bucaneiro, 0, 1, 2);
    if (inimigoA.turnos_sangramento != 2) {
        fprintf(stderr, "Falha sanitaria: 2o Ataque 1 do Bucaneiro no mesmo inimigo deve causar Sangramento por 2 turnos.\n");
        return false;
    }
    {
        int vidaAntesSangr = inimigoA.vida_atual;
        int danoSangrEsperado = (inimigoA.vida_maxima * 15) / 100;
        processarStatusInicioTurno(&mapa, &inimigoA);
        if (inimigoA.vida_atual != vidaAntesSangr - danoSangrEsperado ||
            inimigoA.turnos_sangramento != 1) {
            fprintf(stderr, "Falha sanitaria: Sangramento deve causar -15%% da vida maxima por turno durante 2 turnos.\n");
            return false;
        }
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiro);
    (void)subirNivelPirata(&bucaneiro, 5);
    if (bucaneiro.nivel != 6 || bucaneiro.habilidades_desbloqueadas != 3 ||
        bucaneiro.estamina_maxima != 10) {
        fprintf(stderr, "Falha sanitaria: progressao Nv 6 do Bucaneiro.\n");
        return false;
    }
    {
        int vidaNivel6 = bucaneiro.vida_maxima;
        int danoNivel6 = bucaneiro.habilidades[0].dano_max;
        int estaminaNivel6 = bucaneiro.estamina_maxima;
        int vidaEsperada = vidaNivel6;
        int danoEsperado = danoNivel6;
        int passo;
        for (passo = 0; passo < 2; passo++) {
            int aumentoVida = (vidaEsperada * 5) / 100;
            int aumentoDano = (danoEsperado * 5) / 100;
            vidaEsperada += aumentoVida > 0 ? aumentoVida : 1;
            danoEsperado += aumentoDano > 0 ? aumentoDano : 1;
        }
        (void)subirNivelPirata(&bucaneiro, 2);
        if (bucaneiro.nivel != 8 || bucaneiro.habilidades_desbloqueadas != 3 ||
            bucaneiro.vida_maxima != vidaEsperada ||
            bucaneiro.habilidades[0].dano_max != danoEsperado ||
            bucaneiro.estamina_maxima != estaminaNivel6) {
            fprintf(stderr, "Falha sanitaria: níveis acima de 6 devem melhorar status sem liberar habilidades.\n");
            return false;
        }
    }
    bucaneiro.habilidades[1].precisao = 100;
    bucaneiro.habilidades[2].precisao = 100;
    (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoA);
    (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoB);
    bucaneiro.linha = 1;
    bucaneiro.coluna = 1;
    inimigoA.linha = 0;
    inimigoA.coluna = 2;
    inimigoB.linha = 2;
    inimigoB.coluna = 2;
    mapa.celulas[1][1] = &bucaneiro;
    mapa.celulas[0][2] = &inimigoA;
    mapa.celulas[2][2] = &inimigoB;
    executarAtaqueOuHabilidade(&mapa, &bucaneiro, 1, 1, 2);
    if (inimigoA.turnos_atordoado != 1 || inimigoB.turnos_atordoado != 1) {
        fprintf(stderr, "Falha sanitaria: Bolsa de Areia (1x3 perpendicular).\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    bucaneiro.estamina_atual = bucaneiro.estamina_maxima;
    bucaneiro.vida_atual = 50;
    bucaneiro.linha = 1;
    bucaneiro.coluna = 1;
    inimigoA.vida_maxima = 600;
    inimigoA.vida_atual = 600;
    inimigoA.linha = 1;
    inimigoA.coluna = 3;
    mapa.celulas[1][1] = &bucaneiro;
    mapa.celulas[1][3] = &inimigoA;
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoB);
    inimigoB.vida_maxima = 600;
    inimigoB.vida_atual = 600;
    inimigoB.linha = 1;
    inimigoB.coluna = 2;
    mapa.celulas[1][2] = &inimigoB;
    mapa.mina_ativa[1][2] = 1;
    executarAtaqueOuHabilidade(&mapa, &bucaneiro, 2, 1, 3);
    if (bucaneiro.coluna != 4 || mapa.mina_ativa[1][2] != 0 ||
        inimigoA.vida_atual >= 600 || inimigoB.vida_atual >= 600 ||
        bucaneiro.vida_atual <= 50) {
        fprintf(stderr, "Falha sanitaria: Avanco Impetuoso deve causar dano garantido em área.\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_TIMONEIRO, &timoneiro);
    (void)subirNivelPirata(&timoneiro, 5);
    timoneiro.habilidades[2].precisao = 100;
    timoneiro.linha = 1;
    timoneiro.coluna = 1;
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoA);
    inimigoA.vida_maxima = 500;
    inimigoA.vida_atual = 500;
    inimigoA.linha = 1;
    inimigoA.coluna = 2;
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoB);
    inimigoB.vida_maxima = 500;
    inimigoB.vida_atual = 500;
    inimigoB.linha = 0;
    inimigoB.coluna = 2;
    mapa.celulas[1][1] = &timoneiro;
    mapa.celulas[1][2] = &inimigoA;
    mapa.celulas[0][2] = &inimigoB;
    executarAtaqueOuHabilidade(&mapa, &timoneiro, 2, 1, 2);
    if (inimigoA.coluna != 3 || inimigoA.turnos_desnorteado != 2 ||
        inimigoB.vida_atual >= 500) {
        fprintf(stderr, "Falha sanitaria: Giro do Timão deve causar dano garantido em área.\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_COZINHEIRO, &cozinheiro);
    cozinheiro.habilidades_desbloqueadas = 2;
    cozinheiro.linha = 1;
    cozinheiro.coluna = 1;
    (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoA);
    (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoB);
    inimigoA.vida_maxima = 300;
    inimigoA.vida_atual = 300;
    inimigoB.vida_maxima = 300;
    inimigoB.vida_atual = 300;
    inimigoA.linha = 1;
    inimigoA.coluna = 2;
    inimigoB.linha = 1;
    inimigoB.coluna = 3;
    mapa.celulas[1][1] = &cozinheiro;
    mapa.celulas[1][2] = &inimigoA;
    mapa.celulas[1][3] = &inimigoB;
    executarAtaqueOuHabilidade(&mapa, &cozinheiro, 1, 1, 3);
    if (inimigoA.vida_atual != 200 || inimigoB.vida_atual != 240 ||
        inimigoA.coluna != 2 || inimigoB.coluna != 3) {
        fprintf(stderr, "Falha sanitaria: Bacamarte deve atingir a linha de 2 quadrados (100 perto e 60 longe) sem empurrar.\n");
        return false;
    }
    (void)subirNivelPirata(&cozinheiro, 5);
    cozinheiro.estamina_atual = cozinheiro.estamina_maxima;
    bucaneiro.vida_atual = 20;
    bucaneiro.linha = 0;
    bucaneiro.coluna = 2;
    mapa.celulas[0][2] = &bucaneiro;
    executarAtaqueOuHabilidade(&mapa, &cozinheiro, 2, 0, 2);
    if (bucaneiro.vida_atual <= 20) {
        fprintf(stderr, "Falha sanitaria: Hora do Rango (cura em area 2x3).\n");
        return false;
    }
    (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiro);
    bucaneiro.vida_atual = 20;
    bucaneiro.linha = 0;
    bucaneiro.coluna = 1;
    mapa.celulas[0][1] = &bucaneiro;
    cozinheiro.estamina_atual = cozinheiro.estamina_maxima;
    executarAtaqueOuHabilidade(&mapa, &cozinheiro, 0, 0, 1);
    if (bucaneiro.vida_atual != 50) {
        fprintf(stderr, "Falha sanitaria: Comida da Boa deve curar 20%% da Vida Máxima.\n");
        return false;
    }
    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_COZINHEIRO, &cozinheiro);
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoA);
    cozinheiro.linha = 1;
    cozinheiro.coluna = 1;
    inimigoA.linha = 1;
    inimigoA.coluna = 3;
    inimigoA.vida_atual = inimigoA.vida_maxima;
    mapa.celulas[1][1] = &cozinheiro;
    mapa.celulas[1][3] = &inimigoA;
    executarAtaqueOuHabilidade(&mapa, &cozinheiro, 0, 1, 3);
    if (inimigoA.turnos_veneno != 2 ||
        inimigoA.vida_atual != inimigoA.vida_maxima) {
        fprintf(stderr, "Falha sanitaria: Comida da Boa deve envenenar inimigos sem perder a cura aliada.\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_CAPITAO, &capitao);
    capitao.linha = 1;
    capitao.coluna = 1;
    (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoA);
    inimigoA.vida_maxima = 300;
    inimigoA.vida_atual = 300;
    inimigoA.linha = 1;
    inimigoA.coluna = 2;
    mapa.celulas[1][1] = &capitao;
    mapa.celulas[1][2] = &inimigoA;
    executarAtaqueOuHabilidade(&mapa, &capitao, 0, 1, 2);
    if (inimigoA.vida_atual != 200) {
        fprintf(stderr, "Falha sanitaria: Saquear do Capitao deve dar 100 de perto.\n");
        return false;
    }
    mapa.celulas[1][2] = NULL;
    inimigoA.linha = 1;
    inimigoA.coluna = 3;
    mapa.celulas[1][3] = &inimigoA;
    capitao.estamina_atual = capitao.estamina_maxima;
    executarAtaqueOuHabilidade(&mapa, &capitao, 0, 1, 3);
    if (inimigoA.vida_atual != 125) {
        fprintf(stderr, "Falha sanitaria: Saquear do Capitao deve dar 75 de longe.\n");
        return false;
    }
    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_CAPITAO, &capitao);
    (void)subirNivelPirata(&capitao, 5);
    capitao.habilidades[1].precisao = 100;
    capitao.habilidades[2].precisao = 100;
    capitao.linha = 1;
    capitao.coluna = 0;
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoA);
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoB);
    inimigoA.vida_maxima = 900;
    inimigoA.vida_atual = 900;
    inimigoB.vida_maxima = 900;
    inimigoB.vida_atual = 900;
    inimigoA.linha = 0;
    inimigoA.coluna = 5;
    inimigoB.linha = 2;
    inimigoB.coluna = 4;
    mapa.celulas[1][0] = &capitao;
    mapa.celulas[0][5] = &inimigoA;
    mapa.celulas[2][4] = &inimigoB;
    executarAtaqueOuHabilidade(&mapa, &capitao, 2, 0, 5);
    if (inimigoA.vida_atual >= inimigoA.vida_maxima ||
        inimigoB.vida_atual >= inimigoB.vida_maxima) {
        fprintf(stderr, "Falha sanitaria: Enxurrada de Canhoes 3x2 na borda.\n");
        return false;
    }
    capitao.estamina_atual = capitao.estamina_maxima;
    inimigoB.linha = 1;
    inimigoB.coluna = 2;
    mapa.celulas[2][4] = NULL;
    mapa.celulas[1][2] = &inimigoB;
    executarAtaqueOuHabilidade(&mapa, &capitao, 1, 1, 2);
    if (inimigoB.turnos_desmoralizado != 2) {
        fprintf(stderr, "Falha sanitaria: Discursar em inimigo (Desmoralizar).\n");
        return false;
    }
    processarStatusInicioTurno(&mapa, &inimigoB);
    processarStatusInicioTurno(&mapa, &inimigoB);
    inimigoB.estamina_atual = 5;
    bucaneiro.vida_atual = 100;
    bucaneiro.linha = 1;
    bucaneiro.coluna = 1;
    mapa.celulas[1][1] = &bucaneiro;
    executarAtaqueOuHabilidade(&mapa, &inimigoB, 0, 1, 1);
    if (inimigoB.estamina_atual != 2) {
        fprintf(stderr, "Falha sanitaria: Desmoralizado (+1 custo de estamina no 2o turno).\n");
        return false;
    }
    {
        Combatente aliadoMotivado;
        EntradaIniciativa filaMot[8];
        int totalMot;
        int idxMot;
        int velMotivado = 0;
        (void)criarPirataInicial(CLASSE_TIMONEIRO, &aliadoMotivado);
        aliadoMotivado.linha = 0;
        aliadoMotivado.coluna = 0;
        mapa.celulas[0][0] = &aliadoMotivado;
        capitao.estamina_atual = capitao.estamina_maxima;
        executarAtaqueOuHabilidade(&mapa, &capitao, 1, 0, 0);
        totalMot = montarFilaTurnos(&mapa, filaMot);
        for (idxMot = 0; idxMot < totalMot; idxMot++) {
            if (filaMot[idxMot].ator == &aliadoMotivado) {
                velMotivado = filaMot[idxMot].velocidade_efetiva;
            }
        }
        if (aliadoMotivado.turnos_motivacao != 3 ||
            !aliadoMotivado.motivado_capitao ||
            aliadoMotivado.estamina_maxima != 6 ||
            velMotivado != 7) {
            fprintf(stderr, "Falha sanitaria: Discursar em aliado (+40%% precisao, +2 velocidade, +1 estamina max por 3 turnos).\n");
            return false;
        }
        processarStatusInicioTurno(&mapa, &aliadoMotivado);
        processarStatusInicioTurno(&mapa, &aliadoMotivado);
        processarStatusInicioTurno(&mapa, &aliadoMotivado);
        if (!aliadoMotivado.motivado_capitao || aliadoMotivado.estamina_maxima != 6) {
            fprintf(stderr, "Falha sanitaria: Motivacao deve permanecer ativa durante o 3o turno completo.\n");
            return false;
        }
        processarStatusInicioTurno(&mapa, &aliadoMotivado);
        if (aliadoMotivado.motivado_capitao || aliadoMotivado.estamina_maxima != 5) {
            fprintf(stderr, "Falha sanitaria: Motivacao deve expirar apos os 3 turnos.\n");
            return false;
        }
        mapa.celulas[0][0] = NULL;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_GAVEA, &gavea);
    (void)subirNivelPirata(&gavea, 5);
    gavea.habilidades[1].precisao = 100;
    gavea.habilidades[2].precisao = 100;
    gavea.linha = 1;
    gavea.coluna = 0;
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoA);
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoB);
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoC);
    inimigoA.vida_maxima = 900;
    inimigoA.vida_atual = 900;
    inimigoB.vida_maxima = 900;
    inimigoB.vida_atual = 900;
    inimigoC.vida_maxima = 900;
    inimigoC.vida_atual = 900;
    inimigoA.linha = 1;
    inimigoA.coluna = 2;
    inimigoB.linha = 1;
    inimigoB.coluna = 4;
    inimigoC.linha = 1;
    inimigoC.coluna = 5;
    mapa.celulas[1][0] = &gavea;
    mapa.celulas[1][2] = &inimigoA;
    mapa.celulas[1][4] = &inimigoB;
    mapa.celulas[1][5] = &inimigoC;
    executarAtaqueOuHabilidade(&mapa, &gavea, 2, 1, 5);
    if (inimigoA.vida_atual >= inimigoA.vida_maxima ||
        inimigoB.vida_atual >= inimigoB.vida_maxima ||
        inimigoC.vida_atual >= inimigoC.vida_maxima) {
        fprintf(stderr, "Falha sanitaria: Bala Perfurante Especial (atravessar linha).\n");
        return false;
    }
    gavea.estamina_atual = gavea.estamina_maxima;
    executarAtaqueOuHabilidade(&mapa, &gavea, 1, 1, 2);
    if (inimigoA.turnos_veneno != 2) {
        fprintf(stderr, "Falha sanitaria: Zarabatana em inimigo (Veneno 2t).\n");
        return false;
    }
    {
        int vidaAntesVeneno = inimigoA.vida_atual;
        int danoVenenoEsperado = (inimigoA.vida_maxima * 10) / 100;
        processarStatusInicioTurno(&mapa, &inimigoA);
        if (inimigoA.vida_atual != vidaAntesVeneno - danoVenenoEsperado ||
            inimigoA.turnos_veneno != 1) {
            fprintf(stderr, "Falha sanitaria: Envenenamento deve causar -10%% da vida maxima por turno durante 2 turnos.\n");
            return false;
        }
    }
    gavea.estamina_atual = gavea.estamina_maxima;
    executarAtaqueOuHabilidade(&mapa, &gavea, 1, 1, 0);
    if (gavea.turnos_cura_hot != 2 ||
        gavea.valor_cura_hot != (gavea.vida_maxima * 10) / 100) {
        fprintf(stderr, "Falha sanitaria: Zarabatana em aliado (HoT 10%% por 2t).\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiro);
    (void)criarPirataInicial(CLASSE_CAPITAO, &capitao);
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoA);
    inimigoA.habilidades[0].precisao = 100;
    bucaneiro.linha = 1;
    bucaneiro.coluna = 1;
    capitao.linha = 0;
    capitao.coluna = 1;
    inimigoA.linha = 1;
    inimigoA.coluna = 2;
    mapa.celulas[1][1] = &bucaneiro;
    mapa.celulas[0][1] = &capitao;
    mapa.celulas[1][2] = &inimigoA;
    iniciarDescanso(&mapa, &bucaneiro);
    executarAtaqueOuHabilidade(&mapa, &inimigoA, 0, 1, 1);
    if (bucaneiro.atacado_no_descanso) {
        fprintf(stderr, "Falha sanitaria: passiva Nv 1 do Capitao no descanso.\n");
        return false;
    }
    inimigoA.estamina_atual = 5;
    bucaneiro.vida_atual = 100;
    executarAtaqueOuHabilidade(&mapa, &inimigoA, 0, 1, 1);
    inimigoA.estamina_atual = 5;
    bucaneiro.vida_atual = 100;
    executarAtaqueOuHabilidade(&mapa, &inimigoA, 0, 1, 1);
    if (bucaneiro.turnos_sangramento != 2) {
        fprintf(stderr, "Falha sanitaria: 3 cortes do Grunt devem aplicar Sangramento por 2 turnos.\n");
        return false;
    }

    {
        Combatente atiradorMagrelo;
        Combatente marujoGordo;
        Combatente aliadoInimigo;
        (void)criarInimigoInicial(INIMIGO_ATIRADOR_MAGRELO, &atiradorMagrelo);
        if (atiradorMagrelo.habilidades[0].alcance != 3) {
            fprintf(stderr, "Falha sanitaria: Atirador Magrelo deve ter 3 quadrados de alcance.\n");
            return false;
        }
        (void)memset(&mapa, 0, sizeof(mapa));
        (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &marujoGordo);
        (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &aliadoInimigo);
        (void)criarPirataInicial(CLASSE_TIMONEIRO, &timoneiro);
        marujoGordo.linha = 1;
        marujoGordo.coluna = 3;
        aliadoInimigo.vida_maxima = 300;
        aliadoInimigo.vida_atual = 300;
        aliadoInimigo.linha = 1;
        aliadoInimigo.coluna = 2;
        timoneiro.vida_maxima = 300;
        timoneiro.vida_atual = 300;
        timoneiro.linha = 1;
        timoneiro.coluna = 1;
        mapa.celulas[1][3] = &marujoGordo;
        mapa.celulas[1][2] = &aliadoInimigo;
        mapa.celulas[1][1] = &timoneiro;
        executarAtaqueOuHabilidade(&mapa, &marujoGordo, 0, 1, 1);
        if (aliadoInimigo.vida_atual != 265 || timoneiro.vida_atual != 250) {
            fprintf(stderr, "Falha sanitaria: Escopeta do Marujo Gordo deve funcionar igual ao Bacamarte (70 perto, 50 longe, 50%% em aliado).\n");
            return false;
        }
        marujoGordo.estamina_atual = marujoGordo.estamina_maxima;
        aliadoInimigo.vida_maxima = 200;
        aliadoInimigo.vida_atual = 100;
        executarAtaqueOuHabilidade(&mapa, &marujoGordo, 1, 1, 2);
        if (aliadoInimigo.turnos_cura_hot != 3 ||
            aliadoInimigo.valor_cura_hot != 20) {
            fprintf(stderr, "Falha sanitaria: Bandagens do Marujo Gordo devem curar 10%% da vida total por 3 turnos.\n");
            return false;
        }
    }

    {
        Combatente tripulacaoRota[1];
        Combatente bucTesteAcoes;
        ListaItens invRota = { NULL, 0 };
        int moedasSalvas = moedasTripulacao;
        int idxRum;
        (void)criarPirataInicial(CLASSE_BUCANEIRO, &tripulacaoRota[0]);
        moedasTripulacao = 0;
        if (!aplicarRecompensaRota(ROTA_CAMINHO_1, tripulacaoRota, 1, &invRota, 1) ||
            moedasTripulacao != 700) {
            fprintf(stderr, "Falha sanitaria: Caminho 1 deve dar 700 ouros.\n");
            moedasTripulacao = moedasSalvas;
            liberarEstruturasJogo(NULL, &invRota, NULL);
            return false;
        }
        moedasTripulacao = 0;
        if (!aplicarRecompensaRota(ROTA_CAMINHO_2, tripulacaoRota, 1, &invRota, 1) ||
            moedasTripulacao != 300) {
            fprintf(stderr, "Falha sanitaria: Caminho 2 deve dar 300 ouros.\n");
            moedasTripulacao = moedasSalvas;
            liberarEstruturasJogo(NULL, &invRota, NULL);
            return false;
        }
        for (idxRum = 0; idxRum < 20; idxRum++) {
            if (!aplicarRecompensaRota(ROTA_CAMINHO_1, tripulacaoRota, 1,
                                       &invRota, 1)) {
                fprintf(stderr, "Falha sanitaria: recompensa de combate invalida.\n");
                moedasTripulacao = moedasSalvas;
                liberarEstruturasJogo(NULL, &invRota, NULL);
                return false;
            }
        }
        {
            NoItem *item;
            for (item = invRota.inicio; item != NULL; item = item->proximo) {
                if (item->item.id == 11) {
                    fprintf(stderr, "Falha sanitaria: Rum de Poseidon nao pode vir de recompensas de combate.\n");
                    moedasTripulacao = moedasSalvas;
                    liberarEstruturasJogo(NULL, &invRota, NULL);
                    return false;
                }
            }
        }
        moedasTripulacao = moedasSalvas;
        liberarEstruturasJogo(NULL, &invRota, NULL);

        (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucTesteAcoes);
        if (calcularAcoesPorTurno(&bucTesteAcoes) != 2) {
            fprintf(stderr, "Falha sanitaria: Bucaneiro Nv 1 deve ter 2 acoes por turno.\n");
            return false;
        }
        (void)subirNivelPirata(&bucTesteAcoes, 3);
        if (!bucTesteAcoes.passiva_aprimorada || calcularAcoesPorTurno(&bucTesteAcoes) != 3) {
            fprintf(stderr, "Falha sanitaria: Passiva Nv 4 do Bucaneiro deve conceder +1 acao por turno (total 3).\n");
            return false;
        }
        for (idxRum = 0; idxRum < 5; idxRum++) {
            NoItem *noRum = malloc(sizeof(*noRum));
            if (noRum == NULL) {
                liberarListaItensCombatente(&bucTesteAcoes.build_passivos);
                return false;
            }
            noRum->item = catalogo[10];
            noRum->item.quantidade = 1;
            noRum->proximo = bucTesteAcoes.build_passivos.inicio;
            bucTesteAcoes.build_passivos.inicio = noRum;
            bucTesteAcoes.build_passivos.tamanho++;
            if (calcularAcoesPorTurno(&bucTesteAcoes) != 3 + idxRum + 1) {
                fprintf(stderr, "Falha sanitaria: acumulo de Rum de Poseidon com passiva Nv 4 do Bucaneiro.\n");
                liberarListaItensCombatente(&bucTesteAcoes.build_passivos);
                return false;
            }
        }
        if (calcularAcoesPorTurno(&bucTesteAcoes) != 8) {
            fprintf(stderr, "Falha sanitaria: Bucaneiro Nv 4 com 5 Runs de Poseidon deve ter 8 acoes por turno.\n");
            liberarListaItensCombatente(&bucTesteAcoes.build_passivos);
            return false;
        }
        liberarListaItensCombatente(&bucTesteAcoes.build_passivos);
    }

    {
        Item travesseiro = catalogo[2];
        Item livroOrigem = catalogo[3];
        Item tonico = catalogo[1];
        bucaneiro.estamina_atual = 0;
        if (!usarItemConsumivel(&bucaneiro, &travesseiro) ||
            bucaneiro.estamina_atual != bucaneiro.estamina_maxima ||
            !usarItemConsumivel(&bucaneiro, &livroOrigem) ||
            bucaneiro.nivel != 3 ||
            !usarItemConsumivel(&bucaneiro, &tonico) ||
            bucaneiro.turnos_tonico != 2 ||
            bucaneiro.bonus_tonico_percentual != 20) {
            fprintf(stderr, "Falha sanitaria: itens Travesseiro, Livro de Origem e Tonico.\n");
            return false;
        }
    }
    {
        Combatente elite;
        if (podeGerarEliteNaEtapa(1, 0) ||
            !podeGerarEliteNaEtapa(2, 0) ||
            !podeGerarEliteNaEtapa(1, 1)) {
            fprintf(stderr, "Falha sanitaria: elites só surgem a partir da segunda rota ou após fuga.\n");
            return false;
        }
        if (!criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &elite)) {
            fprintf(stderr, "Falha sanitaria: criacao de elite.\n");
            return false;
        }
        aplicarBuffElite(&elite);
        if (!elite.eh_elite || elite.vida_maxima != 250 ||
            elite.habilidades[0].dano_min != 43 ||
            elite.habilidades[0].dano_max != 62 ||
            elite.estamina_maxima != 8 || calcularAcoesPorTurno(&elite) != 3) {
            fprintf(stderr, "Falha sanitaria: elite deve ter +25%% vida/dano, +3 estamina e 3 acoes.\n");
            return false;
        }
    }
    {
        Tabuleiro3x6 mapaDescanso;
        Combatente descansando;
        Combatente atacante;
        (void)memset(&mapaDescanso, 0, sizeof(mapaDescanso));
        (void)criarPirataInicial(CLASSE_BUCANEIRO, &descansando);
        (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &atacante);
        descansando.linha = 1;
        descansando.coluna = 1;
        descansando.estamina_atual = 0;
        atacante.linha = 1;
        atacante.coluna = 2;
        atacante.habilidades[0].dano_min = 40;
        atacante.habilidades[0].dano_max = 40;
        atacante.habilidades[0].precisao = 100;
        mapaDescanso.celulas[1][1] = &descansando;
        mapaDescanso.celulas[1][2] = &atacante;
        iniciarDescanso(&mapaDescanso, &descansando);
        executarAtaqueOuHabilidade(&mapaDescanso, &atacante, 0, 1, 1);
        if (!descansando.atacado_no_descanso ||
            descansando.vida_atual != descansando.vida_maxima - 50) {
            fprintf(stderr, "Falha sanitaria: descansar mantém vulnerabilidade a dano.\n");
            return false;
        }
        processarStatusInicioTurno(&mapaDescanso, &descansando);
        if (descansando.estamina_atual != descansando.estamina_maxima) {
            fprintf(stderr, "Falha sanitaria: descanso recupera toda a estamina após ataque.\n");
            return false;
        }
    }
    {
        Tabuleiro3x6 mapaChefe;
        Combatente bucaneiroChefe;
        Combatente chefe;
        (void)memset(&mapaChefe, 0, sizeof(mapaChefe));
        (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiroChefe);
        (void)subirNivelPirata(&bucaneiroChefe, 5);
        (void)criarChefeFinal(&chefe);
        bucaneiroChefe.linha = 1;
        bucaneiroChefe.coluna = 1;
        bucaneiroChefe.estamina_atual = 10;
        chefe.linha = 1;
        chefe.coluna = 2;
        mapaChefe.celulas[1][1] = &bucaneiroChefe;
        mapaChefe.celulas[1][2] = &chefe;
        executarAtaqueOuHabilidade(&mapaChefe, &bucaneiroChefe, 1, 1, 2);
        executarAtaqueOuHabilidade(&mapaChefe, &bucaneiroChefe, 1, 1, 2);
        if (!mapaChefe.boss_atordoado_nesta_rodada ||
            chefe.turnos_atordoado != 1) {
            fprintf(stderr, "Falha sanitaria: Bucaneiro só atordoa Barba Negra uma vez por rodada.\n");
            return false;
        }
        (void)memset(&mapaChefe, 0, sizeof(mapaChefe));
        chefe.linha = 1;
        chefe.coluna = 3;
        chefe.estamina_atual = 2;
        mapaChefe.celulas[1][3] = &chefe;
        if (!moverCombatente3x6(&mapaChefe, &chefe, 1, 5) ||
            chefe.coluna != 5 || chefe.estamina_atual != 1) {
            fprintf(stderr, "Falha sanitaria: Barba Negra pode mover duas casas por ação.\n");
            return false;
        }
    }
    return true;
}

// Testa a reposição de piratas mortos após desistência (Fuga da Aventura).
static bool testarReposicaoTripulacaoPosFuga(void)
{
    InterfaceJogo interfaceJogo;
    Tabuleiro3x6 mapa;
    Combatente piratas[TOTAL_PIRATAS_PREDEFINIDOS];
    ListaItens inventario = { NULL, 0 };
    Item itens[TOTAL_ITENS_PREDEFINIDOS];
    NoRota *raizRotas = construirArvoreRotas(1);
    NoRota *rotaAtual = raizRotas;
    int nivelDificuldade = 1;
    int moedas = 1234;
    int indice;
    int totalItens;

    if (raizRotas == NULL) {
        return false;
    }
    (void)memset(&interfaceJogo, 0, sizeof(interfaceJogo));
    (void)memset(&mapa, 0, sizeof(mapa));
    totalItens = carregarItensIniciais(itens, TOTAL_ITENS_PREDEFINIDOS);
    (void)carregarInventarioSanitario(&inventario, itens, totalItens);
    for (indice = 0; indice < TOTAL_PIRATAS_PREDEFINIDOS; indice++) {
        (void)criarPirataInicial(indice, &piratas[indice]);
    }
    (void)subirNivelPirata(&piratas[CLASSE_BUCANEIRO], 2);
    (void)aplicarItemPassivoNoPirata(&inventario, &piratas[CLASSE_BUCANEIRO], 5);
    (void)subirNivelPirata(&piratas[CLASSE_TIMONEIRO], 3);
    (void)aplicarItemPassivoNoPirata(&inventario, &piratas[CLASSE_TIMONEIRO], 6);

    interfaceJogo.raiz_rotas = &raizRotas;
    interfaceJogo.rota_atual = &rotaAtual;
    interfaceJogo.inventario = &inventario;
    interfaceJogo.moedas = &moedas;
    interfaceJogo.nivel_dificuldade = &nivelDificuldade;
    interfaceJogo.piratas_selecionados_ids[0] = piratas[CLASSE_BUCANEIRO].id;
    interfaceJogo.piratas_selecionados_ids[1] = piratas[CLASSE_TIMONEIRO].id;
    interfaceJogo.piratas_selecionados_ids[2] = piratas[CLASSE_COZINHEIRO].id;
    interfaceJogo.total_selecionados = 3;

    subtrairVida(&mapa, &piratas[CLASSE_TIMONEIRO], 999);
    if (piratas[CLASSE_TIMONEIRO].vivo) {
        liberarEstruturasJogo(raizRotas, &inventario, NULL);
        return false;
    }

    processarFugaETrocaTripulacao(&interfaceJogo, &mapa, piratas,
                                  TOTAL_PIRATAS_PREDEFINIDOS);

    if (interfaceJogo.estado != TELA_SELECAO_TRIPULACAO ||
        moedas != 1234 ||
        interfaceJogo.total_selecionados != 2 ||
        interfaceJogo.total_sobreviventes_fuga != 2 ||
        piratas[CLASSE_BUCANEIRO].nivel != 3 ||
        piratas[CLASSE_BUCANEIRO].build_passivos.tamanho != 1 ||
        !piratas[CLASSE_TIMONEIRO].vivo ||
        piratas[CLASSE_TIMONEIRO].nivel != 1 ||
        piratas[CLASSE_TIMONEIRO].vida_maxima != 250 ||
        piratas[CLASSE_TIMONEIRO].build_passivos.tamanho != 0) {
        fprintf(stderr, "Falha sanitaria: reposicao pos-fuga e reset de mortos ao Nv 1.\n");
        for (indice = 0; indice < TOTAL_PIRATAS_PREDEFINIDOS; indice++) {
            liberarListaItensCombatente(&piratas[indice].build_passivos);
        }
        liberarEstruturasJogo(raizRotas, &inventario, NULL);
        return false;
    }

    for (indice = 0; indice < TOTAL_PIRATAS_PREDEFINIDOS; indice++) {
        liberarListaItensCombatente(&piratas[indice].build_passivos);
    }
    liberarEstruturasJogo(raizRotas, &inventario, NULL);
    return true;
}

// Verifica dados de Britto, progressão Nv 1-6, loja, IA, estruturas e os 10 edge cases.
static bool executarTestesSanitarios(void)
{
    Item itens[TOTAL_ITENS_PREDEFINIDOS];
    Item itensLoja[3];
    int estoquesLoja[3];
    int indicesSorteados[4];
    ListaItens inventario = { NULL, 0 };
    ListaAlcance *alcance;
    NoRota *raiz;
    NoRota *novaRaiz;
    Combatente piratas[TOTAL_PIRATAS_PREDEFINIDOS];
    Combatente boss;
    Combatente inimigo;
    Tabuleiro3x6 mapa;
    EntradaIniciativa fila[MAX_ENTRADAS_INICIATIVA];
    CandidatoAcao acaoIA;
    int totalItens;
    int totalFila;
    int dificuldade = 1;
    int indice;
    int moedasTeste = 1000;
    bool encontrouPassivoAliado;
    bool sucesso = false;

    (void)memset(piratas, 0, sizeof(piratas));
    (void)memset(&boss, 0, sizeof(boss));
    (void)memset(&inimigo, 0, sizeof(inimigo));
    (void)memset(&mapa, 0, sizeof(mapa));
    totalItens = carregarItensIniciais(itens, TOTAL_ITENS_PREDEFINIDOS);
    if (totalItens != 11) {
        fprintf(stderr, "Falha sanitaria: catalogo deveria conter 11 itens.\n");
        return false;
    }
    for (indice = 0; indice < TOTAL_PIRATAS_PREDEFINIDOS; indice++) {
        if (!criarPirataInicial(indice, &piratas[indice]) ||
            piratas[indice].classe != (ClassePirata)indice) {
            fprintf(stderr, "Falha sanitaria: classe jogavel invalida.\n");
            goto limpeza;
        }
    }
    for (indice = 0; indice < TOTAL_INIMIGOS_PREDEFINIDOS; indice++) {
        if (!criarInimigoInicial(indice, &inimigo) ||
            inimigo.tipo_inimigo != (TipoInimigo)indice) {
            fprintf(stderr, "Falha sanitaria: inimigo base invalido.\n");
            goto limpeza;
        }
    }
    for (indice = 4; indice < totalItens; indice++) {
        if (itens[indice].id < 5 || itens[indice].tipo != ITEM_PASSIVO) {
            fprintf(stderr, "Falha sanitaria: itens 5 a 11 devem ser passivos.\n");
            goto limpeza;
        }
    }
    if (!carregarInventarioSanitario(&inventario, itens, totalItens) ||
        !criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigo) ||
        !criarChefeFinal(&boss)) {
        fprintf(stderr, "Falha sanitaria: criacao de dados de jogo.\n");
        goto limpeza;
    }
    if (!aplicarItemPassivoNoPirata(&inventario, &piratas[0], 5)) {
        fprintf(stderr, "Falha sanitaria: equipar build passiva.\n");
        goto limpeza;
    }
    if (!subirNivelPirata(&piratas[CLASSE_COZINHEIRO], 3) ||
        piratas[CLASSE_COZINHEIRO].nivel != 4 ||
        !piratas[CLASSE_COZINHEIRO].passiva_aprimorada ||
        piratas[CLASSE_COZINHEIRO].estamina_maxima != 8) {
        fprintf(stderr, "Falha sanitaria: progressao de nivel e passiva Nv 4.\n");
        goto limpeza;
    }
    piratas[CLASSE_COZINHEIRO].vida_atual = 50;
    {
        Item bandagemTeste = itens[0];
        Item bandagemBase = itens[0];
        Combatente bucaneiroCura;
        int curaEsperadaCozinheiro = (piratas[CLASSE_COZINHEIRO].vida_maxima * 48) / 100;
        (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiroCura);
        bucaneiroCura.vida_atual = 50;
        if (!usarItemConsumivel(&bucaneiroCura, &bandagemBase) ||
            bucaneiroCura.vida_atual != 50 + (bucaneiroCura.vida_maxima * 40) / 100 ||
            !usarItemConsumivel(&piratas[CLASSE_COZINHEIRO], &bandagemTeste) ||
            piratas[CLASSE_COZINHEIRO].vida_atual != 50 + curaEsperadaCozinheiro ||
            !piratas[CLASSE_COZINHEIRO].usou_consumivel_gratis_turno ||
            bandagemTeste.quantidade != 0) {
            fprintf(stderr, "Falha sanitaria: Bandagem (40%% da vida max) e passiva do Cozinheiro (+20%% e gratis).\n");
            goto limpeza;
        }
    }
    if (sortearInimigosPorEtapa(ROTA_CAMINHO_1, 1, 42U, indicesSorteados, 4) != 4 ||
        sortearInimigosPorEtapa(ROTA_CAMINHO_2, 2, 42U, indicesSorteados, 4) != 3 ||
        indicesSorteados[0] != INIMIGO_ATIRADOR_MAGRELO ||
        sortearInimigosPorEtapa(ROTA_CAMINHO_1, 3, 42U, indicesSorteados, 4) != 4 ||
        indicesSorteados[0] != INIMIGO_MARUJO_GORDO) {
        fprintf(stderr, "Falha sanitaria: sorteio deterministico de inimigos por rota.\n");
        goto limpeza;
    }
    if (sortearItensLoja(3, 99U, itensLoja, estoquesLoja, 3) != 3 ||
        itensLoja[0].raridade != RARIDADE_LENDARIO || itensLoja[0].id != 11 ||
        estoquesLoja[0] != 2 ||
        !comprarItemNaLoja(itensLoja, estoquesLoja, 3, 0, &inventario, &moedasTeste) ||
        estoquesLoja[0] != 1) {
        fprintf(stderr, "Falha sanitaria: sorteio e compra na Loja da Rota 3.\n");
        goto limpeza;
    }
    piratas[CLASSE_CAPITAO].habilidades_desbloqueadas = 3;
    piratas[CLASSE_CAPITAO].linha = 1;
    piratas[CLASSE_CAPITAO].coluna = 1;
    piratas[CLASSE_CAPITAO].habilidades[2].alcance = 5;
    piratas[CLASSE_BUCANEIRO].linha = 0;
    piratas[CLASSE_BUCANEIRO].coluna = 3;
    inimigo.linha = 1;
    inimigo.coluna = 3;
    mapa.celulas[1][1] = &piratas[CLASSE_CAPITAO];
    mapa.celulas[0][3] = &piratas[CLASSE_BUCANEIRO];
    mapa.celulas[1][3] = &inimigo;
    alcance = calcularAreaAtuacao3x6(&mapa, &piratas[CLASSE_CAPITAO], 2);
    if (alcance == NULL || alcance->tamanho <= 0) {
        fprintf(stderr, "Falha sanitaria: area de fogo amigo.\n");
        liberarEstruturasJogo(NULL, NULL, alcance);
        goto limpeza;
    }
    encontrouPassivoAliado = false;
    {
        NoCelula *celula;
        for (celula = alcance->inicio; celula != NULL; celula = celula->proximo) {
            if (celula->linha == 0 && celula->coluna == 3 &&
                celula->perigo_fogo_amigo == 1) {
                encontrouPassivoAliado = true;
            }
        }
    }
    if (!encontrouPassivoAliado) {
        fprintf(stderr, "Falha sanitaria: aliado na area nao foi sinalizado.\n");
        liberarEstruturasJogo(NULL, NULL, alcance);
        goto limpeza;
    }
    liberarEstruturasJogo(NULL, NULL, alcance);
    if (!selecionarAcaoIA(&mapa, &inimigo, &acaoIA)) {
        fprintf(stderr, "Falha sanitaria: Utility AI deveria selecionar uma acao valida.\n");
        goto limpeza;
    }
    {
        Combatente inimigoFuga;
        if (!criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoFuga)) {
            fprintf(stderr, "Falha sanitaria: criacao de inimigo para teste de fuga.\n");
            goto limpeza;
        }
        escalonarInimigoPorRota(&inimigoFuga, ROTA_CAMINHO_1, 1, 2, false);
        if (inimigoFuga.vida_maxima != 350) {
            fprintf(stderr, "Falha sanitaria: a segunda fuga deve conceder +75%% de status aos inimigos.\n");
            goto limpeza;
        }
    }
    (void)memset(&mapa, 0, sizeof(mapa));
    if (boss.vida_maxima != 3000 || boss.estamina_maxima != 20 ||
        boss.habilidades[1].custo_estamina != 3) {
        fprintf(stderr, "Falha sanitaria: status iniciais e custo do Barba Negra.\n");
        goto limpeza;
    }
    boss.vida_atual = boss.vida_maxima;
    boss.vivo = true;
    boss.linha = 1;
    boss.coluna = COLUNAS_TABULEIRO - 1;
    boss.etapa_rota = 3;
    mapa.celulas[1][COLUNAS_TABULEIRO - 1] = &boss;
    subtrairVida(&mapa, &boss, boss.vida_maxima / 2);
    if (!mapa.boss_fase_2_ativa || boss.fase_boss != 2 ||
        boss.estamina_maxima != 30 || boss.estamina_atual != 30) {
        fprintf(stderr, "Falha sanitaria: transicao de fase do chefe.\n");
        goto limpeza;
    }
    {
        int grunts = 0;
        bool gruntsEscalados = true;
        int linha;
        int coluna;
        for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
            for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                Combatente *unidade = mapa.celulas[linha][coluna];
                if (unidade != NULL && !unidade->aliado &&
                    unidade->tipo_inimigo == INIMIGO_GRUNT_ESPADA) {
                    grunts++;
                    if (unidade->etapa_rota != 3 || unidade->vida_maxima < 312 ||
                        unidade->estamina_maxima != 9) {
                        gruntsEscalados = false;
                    }
                }
            }
        }
        if (grunts != 2 || !gruntsEscalados) {
            fprintf(stderr, "Falha sanitaria: grunts da fase dois devem escalar com a rota.\n");
            goto limpeza;
        }
    }
    {
        Combatente *gruntA = NULL;
        Combatente *gruntB = NULL;
        int linha;
        int coluna;
        int vidaBoss = boss.vida_atual;
        for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
            for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                Combatente *unidade = mapa.celulas[linha][coluna];
                if (unidade != NULL && unidade->id == 110) {
                    gruntA = unidade;
                } else if (unidade != NULL && unidade->id == 111) {
                    gruntB = unidade;
                }
            }
        }
        subtrairVida(&mapa, &boss, 100);
        if (gruntA == NULL || gruntB == NULL || boss.vida_atual != vidaBoss) {
            fprintf(stderr, "Falha sanitaria: chefe deve ser imune enquanto os dois grunts vivem.\n");
            goto limpeza;
        }
        subtrairVida(&mapa, gruntA, gruntA->vida_atual);
        subtrairVida(&mapa, &boss, 100);
        if (boss.vida_atual != vidaBoss) {
            fprintf(stderr, "Falha sanitaria: chefe deve permanecer imune enquanto um grunt vive.\n");
            goto limpeza;
        }
        subtrairVida(&mapa, gruntB, gruntB->vida_atual);
        subtrairVida(&mapa, &boss, 100);
        if (boss.vida_atual != vidaBoss - 100) {
            fprintf(stderr, "Falha sanitaria: chefe deve voltar a receber dano após derrotar os grunts.\n");
            goto limpeza;
        }
    }
    boss.estamina_atual = 0;
    boss.em_descanso = true;
    boss.atacado_no_descanso = false;
    processarStatusInicioTurno(&mapa, &boss);
    if (boss.estamina_atual != boss.estamina_maxima || boss.em_descanso) {
        fprintf(stderr, "Falha sanitaria: Barba Negra recupera toda estamina ao descansar sem ser atacado.\n");
        goto limpeza;
    }
    boss.estamina_atual = 0;
    boss.em_descanso = true;
    boss.atacado_no_descanso = true;
    processarStatusInicioTurno(&mapa, &boss);
    if (boss.estamina_atual != boss.estamina_maxima || boss.em_descanso) {
        fprintf(stderr, "Falha sanitaria: Barba Negra recupera toda estamina ao descansar mesmo atacado.\n");
        goto limpeza;
    }
    (void)memset(&mapa, 0, sizeof(mapa));
    mapa.celulas[1][1] = &piratas[CLASSE_CAPITAO];
    mapa.celulas[0][3] = NULL;
    mapa.celulas[1][3] = &inimigo;
    mapa.celulas[2][5] = &boss;
    boss.linha = 2;
    boss.coluna = 5;
    totalFila = montarFilaTurnos(&mapa, fila);
    if (totalFila != 5) {
        fprintf(stderr, "Falha sanitaria: chefe deve ter tres entradas de iniciativa.\n");
        goto limpeza;
    }
    {
        int entradasBoss = 0;
        for (indice = 0; indice < totalFila; indice++) {
            if (fila[indice].ator == &boss) {
                entradasBoss++;
            }
        }
        if (entradasBoss != 3) {
            fprintf(stderr, "Falha sanitaria: entradas do Barba Negra.\n");
            goto limpeza;
        }
    }
    raiz = construirArvoreRotas(dificuldade);
    if (raiz == NULL) {
        fprintf(stderr, "Falha sanitaria: arvore de rotas.\n");
        goto limpeza;
    }
    {
        int fugasAntes = contadorFugas;
        novaRaiz = fugirDaAventura(raiz, &inventario, &dificuldade);
        if (novaRaiz == NULL || dificuldade != 2 || inventario.tamanho != 0 ||
            contadorFugas != fugasAntes + 1 ||
            piratas[0].build_passivos.tamanho != 1) {
            fprintf(stderr, "Falha sanitaria: fuga precisa limpar inventario e escalar run.\n");
            liberarEstruturasJogo(novaRaiz, &inventario, NULL);
            goto limpeza;
        }
        liberarEstruturasJogo(novaRaiz, &inventario, NULL);
    }
    if (!testarClassesEMecanicasCombate() || !testarReposicaoTripulacaoPosFuga()) {
        goto limpeza;
    }
    {
        int fugasSalvas = contadorFugas;
        int dificuldadeSalva = dificuldade;
        NoRota *tentativaFuga = construirArvoreRotas(dificuldade);
        NoRota *fugaBloqueada;
        if (tentativaFuga == NULL) {
            fprintf(stderr, "Falha sanitaria: arvore para validar limite de fugas.\n");
            goto limpeza;
        }
        contadorFugas = 2;
        fugaBloqueada = fugirDaAventura(tentativaFuga, NULL, &dificuldade);
        if (fugaBloqueada != NULL || contadorFugas != 2 ||
            dificuldade != dificuldadeSalva) {
            fprintf(stderr, "Falha sanitaria: a terceira fuga deve ser bloqueada.\n");
            if (fugaBloqueada != NULL) {
                liberarEstruturasJogo(fugaBloqueada, NULL, NULL);
            } else {
                liberarEstruturasJogo(tentativaFuga, NULL, NULL);
            }
            contadorFugas = fugasSalvas;
            goto limpeza;
        }
        liberarEstruturasJogo(tentativaFuga, NULL, NULL);
        contadorFugas = fugasSalvas;
    }
    {
        Tabuleiro3x6 mapaTeste;
        Combatente gruntPistola;
        Combatente alvoTeste;
        Combatente pA;
        Combatente pB;
        Combatente pC;
        Combatente pD;
        Combatente pE;
        EntradaIniciativa filaOrd[5];
        ListaItens mochilaTeste = { NULL, 0 };
        int iter;
        int contMin = 0;
        int contMax = 0;
        int vx = 0;
        int vy = 0;

        (void)memset(&mapaTeste, 0, sizeof(mapaTeste));
        (void)criarInimigoInicial(INIMIGO_GRUNT_PISTOLA, &gruntPistola);
        (void)criarPirataInicial(CLASSE_TIMONEIRO, &alvoTeste);
        if (gruntPistola.habilidades[0].alcance != 2) {
            fprintf(stderr, "Falha sanitaria: Grunt com Pistola deve ter alcance 2 com a pistola.\n");
            goto limpeza;
        }
        gruntPistola.habilidades[0].dano_min = 30;
        gruntPistola.habilidades[0].dano_max = 40;
        gruntPistola.linha = 1;
        gruntPistola.coluna = 4;
        alvoTeste.vida_maxima = 200;
        alvoTeste.vida_atual = 200;
        alvoTeste.linha = 1;
        alvoTeste.coluna = 1;
        mapaTeste.celulas[1][4] = &gruntPistola;
        mapaTeste.celulas[1][1] = &alvoTeste;
        executarAtaqueOuHabilidade(&mapaTeste, &gruntPistola, 0, 1, 1);
        if (alvoTeste.vida_atual != 200) {
            fprintf(stderr, "Falha sanitaria: Grunt com Pistola nao deve alcancar 3 quadrados.\n");
            goto limpeza;
        }
        mapaTeste.celulas[1][4] = NULL;
        gruntPistola.coluna = 3;
        mapaTeste.celulas[1][3] = &gruntPistola;

        gruntPistola.precisao_ranged = 0;
        gruntPistola.habilidades[0].precisao = 0;
        for (iter = 0; iter < 30; iter++) {
            int vidaAntes = 200;
            int danoCausado;
            alvoTeste.vida_maxima = 200;
            alvoTeste.vida_atual = vidaAntes;
            gruntPistola.estamina_atual = gruntPistola.estamina_maxima;
            executarAtaqueOuHabilidade(&mapaTeste, &gruntPistola, 0, 1, 1);
            danoCausado = vidaAntes - alvoTeste.vida_atual;
            if (danoCausado != 30) {
                fprintf(stderr, "Falha sanitaria: precisao 0%% deve causar dano minimo (30), causou %d.\n", danoCausado);
                goto limpeza;
            }
        }
        gruntPistola.precisao_ranged = 100;
        gruntPistola.habilidades[0].precisao = 100;
        for (iter = 0; iter < 30; iter++) {
            int vidaAntes = 200;
            int danoCausado;
            alvoTeste.vida_maxima = 200;
            alvoTeste.vida_atual = vidaAntes;
            gruntPistola.estamina_atual = gruntPistola.estamina_maxima;
            executarAtaqueOuHabilidade(&mapaTeste, &gruntPistola, 0, 1, 1);
            danoCausado = vidaAntes - alvoTeste.vida_atual;
            if (danoCausado != 40) {
                fprintf(stderr, "Falha sanitaria: precisao 100%% deve causar dano maximo (40), causou %d.\n", danoCausado);
                goto limpeza;
            }
        }
        gruntPistola.precisao_ranged = 60;
        gruntPistola.habilidades[0].precisao = 60;
        for (iter = 0; iter < 120; iter++) {
            int vidaAntes = 200;
            int danoCausado;
            alvoTeste.vida_maxima = 200;
            alvoTeste.vida_atual = vidaAntes;
            gruntPistola.estamina_atual = gruntPistola.estamina_maxima;
            executarAtaqueOuHabilidade(&mapaTeste, &gruntPistola, 0, 1, 1);
            danoCausado = vidaAntes - alvoTeste.vida_atual;
            if (danoCausado == 30) {
                contMin++;
            } else if (danoCausado == 40) {
                contMax++;
            } else {
                fprintf(stderr, "Falha sanitaria: precisao 60%% causou valor intermediario ou erro (%d).\n", danoCausado);
                goto limpeza;
            }
        }
        if (contMin == 0 || contMax == 0) {
            fprintf(stderr, "Falha sanitaria: distribuicao de precisao 60%% invalida.\n");
            goto limpeza;
        }

        (void)criarPirataInicial(CLASSE_BUCANEIRO, &pA);
        (void)criarPirataInicial(CLASSE_CAPITAO, &pB);
        (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &pC);
        (void)criarPirataInicial(CLASSE_TIMONEIRO, &pD);
        (void)criarPirataInicial(CLASSE_COZINHEIRO, &pE);
        pA.vida_atual = 80;
        pA.estamina_atual = 4;
        pB.vida_atual = 50;
        pB.estamina_atual = 4;
        pC.vida_atual = 10;
        pC.estamina_atual = 9;
        pD.vida_atual = 80;
        pD.estamina_atual = 7;
        pE.vida_atual = 90;
        pE.estamina_atual = 9;
        filaOrd[0].ator = &pC;
        filaOrd[0].velocidade_efetiva = 7;
        filaOrd[1].ator = &pA;
        filaOrd[1].velocidade_efetiva = 7;
        filaOrd[2].ator = &pB;
        filaOrd[2].velocidade_efetiva = 7;
        filaOrd[3].ator = &pD;
        filaOrd[3].velocidade_efetiva = 7;
        filaOrd[4].ator = &pE;
        filaOrd[4].velocidade_efetiva = 9;
        ordenarFilaPorVelocidade(filaOrd, 5);
        if (filaOrd[0].ator != &pE || filaOrd[1].ator != &pB ||
            filaOrd[2].ator != &pD || filaOrd[3].ator != &pA ||
            filaOrd[4].ator != &pC) {
            fprintf(stderr, "Falha sanitaria: desempate de velocidade (vel > jogavel > menos vida > mais estamina).\n");
            goto limpeza;
        }

        if (!carregarInventarioSanitario(&mochilaTeste, &itens[4], 1) ||
            mochilaTeste.tamanho != 1 ||
            !aplicarItemPassivoNoPirata(&mochilaTeste, &pA, itens[4].id) ||
            mochilaTeste.tamanho != 0 || pA.build_passivos.tamanho != 1 ||
            !desequiparItemPassivoBuild(&mochilaTeste, &pA, itens[4].id) ||
            mochilaTeste.tamanho != 1 || pA.build_passivos.tamanho != 0 ||
            !aplicarItemPassivoNoPirata(&mochilaTeste, &pB, itens[4].id) ||
            mochilaTeste.tamanho != 0 || pB.build_passivos.tamanho != 1) {
            fprintf(stderr, "Falha sanitaria: troca de item passivo entre mochila e personagens.\n");
            liberarListaItensCombatente(&pA.build_passivos);
            liberarListaItensCombatente(&pB.build_passivos);
            liberarEstruturasJogo(NULL, &mochilaTeste, NULL);
            goto limpeza;
        }
        liberarListaItensCombatente(&pA.build_passivos);
        liberarListaItensCombatente(&pB.build_passivos);
        liberarEstruturasJogo(NULL, &mochilaTeste, NULL);

        converterCoordenadaMouseVirtual(960, 540, 1920, 1080, &vx, &vy);
        if (vx != 640 || vy != 360) {
            fprintf(stderr, "Falha sanitaria: escala virtual em tela cheia 1920x1080.\n");
            goto limpeza;
        }
        if (converterCoordenadaMouseVirtual(-1, 540, 1920, 1080, &vx, &vy) ||
            converterCoordenadaMouseVirtual(1920, 540, 1920, 1080, &vx, &vy)) {
            fprintf(stderr, "Falha sanitaria: cliques fora da janela nao podem ativar botoes de borda.\n");
            goto limpeza;
        }
        converterCoordenadaMouseVirtual(592, 985, 1920, 1080, &vx, &vy);
        if (vx < 300 || vx >= 490 || vy < 630 || vy >= 685) {
            fprintf(stderr, "Falha sanitaria: area clicavel da fuga deve acompanhar o botao em 1080p.\n");
            goto limpeza;
        }
        converterCoordenadaMouseVirtual(960, 712, 1920, 1080, &vx, &vy);
        if (vx < 470 || vx >= 810 || vy < 445 || vy >= 505) {
            fprintf(stderr, "Falha sanitaria: area clicavel de sair deve acompanhar o botao em 1080p.\n");
            goto limpeza;
        }
        converterCoordenadaMouseVirtual(1919, 1008, 1920, 1009, &vx, &vy);
        if (vx != 1279 || vy != 719) {
            fprintf(stderr, "Falha sanitaria: escala virtual em janela maximizada 1920x1009.\n");
            goto limpeza;
        }
        converterCoordenadaMouseVirtual(1280, 720, 2560, 1440, &vx, &vy);
        if (vx != 640 || vy != 360) {
            fprintf(stderr, "Falha sanitaria: escala virtual em 2560x1440.\n");
            goto limpeza;
        }

        {
            Item lojaA[3];
            Item lojaB[3];
            Item lojaC[3];
            int estA[3];
            int estB[3];
            int estC[3];
            int ouroCompra = 5000;
            Combatente pirataLoja;
            Combatente inimigoFrente;
            Combatente inimigoTras;
            (void)criarPirataInicial(CLASSE_BUCANEIRO, &pirataLoja);
            if (sortearItensLoja(1, 77U, lojaA, estA, 3) != 3 ||
                sortearItensLoja(1, 77U, lojaB, estB, 3) != 3 ||
                sortearItensLoja(2, 77U, lojaC, estC, 3) != 3 ||
                (lojaA[0].id == lojaB[0].id &&
                 lojaA[1].id == lojaB[1].id &&
                 lojaA[2].id == lojaB[2].id) ||
                (lojaB[0].id == lojaC[0].id &&
                 lojaB[1].id == lojaC[1].id &&
                 lojaB[2].id == lojaC[2].id)) {
                fprintf(stderr, "Falha sanitaria: rotacao obrigatoria de itens da loja.\n");
                goto limpeza;
            }
            if (!comprarItemNaLoja(lojaA, estA, 3, 0, &mochilaTeste, &ouroCompra) ||
                estA[0] != 1 || mochilaTeste.inicio == NULL) {
                fprintf(stderr, "Falha sanitaria: compra antes de saque da loja.\n");
                liberarEstruturasJogo(NULL, &mochilaTeste, NULL);
                goto limpeza;
            }
            if (mochilaTeste.inicio->item.tipo == ITEM_PASSIVO) {
                if (!aplicarItemPassivoNoPirata(&mochilaTeste, &pirataLoja,
                                                mochilaTeste.inicio->item.id) ||
                    pirataLoja.build_passivos.tamanho != 1) {
                    fprintf(stderr, "Falha sanitaria: equipar passivo antes do saque da loja.\n");
                    liberarListaItensCombatente(&pirataLoja.build_passivos);
                    liberarEstruturasJogo(NULL, &mochilaTeste, NULL);
                    goto limpeza;
                }
            } else {
                pirataLoja.vida_atual = 10;
                pirataLoja.estamina_atual = 0;
                pirataLoja.turnos_sangramento = 2;
                if (!usarItemConsumivel(&pirataLoja, &mochilaTeste.inicio->item)) {
                    fprintf(stderr, "Falha sanitaria: usar consumivel antes do saque da loja.\n");
                    liberarEstruturasJogo(NULL, &mochilaTeste, NULL);
                    goto limpeza;
                }
            }
            if (!saquearItensRestantesLoja(lojaA, estA, 3, &mochilaTeste) ||
                estA[0] != 0 || estA[1] != 0 || estA[2] != 0) {
                fprintf(stderr, "Falha sanitaria: saque dos itens restantes da loja.\n");
                liberarListaItensCombatente(&pirataLoja.build_passivos);
                liberarEstruturasJogo(NULL, &mochilaTeste, NULL);
                goto limpeza;
            }
            liberarListaItensCombatente(&pirataLoja.build_passivos);
            liberarEstruturasJogo(NULL, &mochilaTeste, NULL);

            (void)memset(&mapaTeste, 0, sizeof(mapaTeste));
            (void)criarPirataInicial(CLASSE_CAPITAO, &pirataLoja);
            (void)criarInimigoInicial(INIMIGO_GRUNT_PISTOLA, &gruntPistola);
            (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoFrente);
            (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoTras);
            pirataLoja.habilidades_desbloqueadas = 3;
            pirataLoja.linha = 1;
            pirataLoja.coluna = 1;
            inimigoFrente.vida_maxima = 500;
            inimigoFrente.vida_atual = 500;
            inimigoFrente.linha = 0;
            inimigoFrente.coluna = 2;
            mapaTeste.celulas[1][1] = &pirataLoja;
            mapaTeste.celulas[0][2] = &inimigoFrente;
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 0, 0, 2);
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 1, 0, 2);
            if (inimigoFrente.vida_atual != 500 || inimigoFrente.turnos_desmoralizado != 0) {
                fprintf(stderr, "Falha sanitaria: ataque na diagonal deveria ser bloqueado.\n");
                goto limpeza;
            }

            (void)memset(&mapaTeste, 0, sizeof(mapaTeste));
            (void)criarPirataInicial(CLASSE_GAVEA, &pirataLoja);
            pirataLoja.linha = 1;
            pirataLoja.coluna = 0;
            inimigoFrente.vida_atual = 500;
            inimigoFrente.linha = 1;
            inimigoFrente.coluna = 1;
            inimigoTras.vida_maxima = 500;
            inimigoTras.vida_atual = 500;
            inimigoTras.linha = 1;
            inimigoTras.coluna = 2;
            mapaTeste.celulas[1][0] = &pirataLoja;
            mapaTeste.celulas[1][1] = &inimigoFrente;
            mapaTeste.celulas[1][2] = &inimigoTras;
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 0, 1, 2);
            if (inimigoTras.vida_atual != 500 || inimigoFrente.vida_atual != 500) {
                fprintf(stderr, "Falha sanitaria: ataque nao perfurante nao pode mirar atras de inimigo.\n");
                goto limpeza;
            }
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 0, 1, 1);
            if (inimigoFrente.vida_atual >= 500 || inimigoTras.vida_atual != 500) {
                fprintf(stderr, "Falha sanitaria: ataque 1 do Gavea nao pode atravessar o 1o inimigo.\n");
                goto limpeza;
            }
            gruntPistola.linha = 1;
            gruntPistola.coluna = 3;
            mapaTeste.celulas[1][3] = &gruntPistola;
            inimigoFrente.aliado = true;
            inimigoTras.aliado = true;
            inimigoFrente.vida_atual = 500;
            inimigoTras.vida_atual = 500;
            executarAtaqueOuHabilidade(&mapaTeste, &gruntPistola, 0, 1, 2);
            if (inimigoTras.vida_atual >= 500 || inimigoFrente.vida_atual != 500) {
                fprintf(stderr, "Falha sanitaria: Tiro do Grunt Pistola nao deve atravessar o 1o alvo.\n");
                goto limpeza;
            }

            (void)memset(&mapaTeste, 0, sizeof(mapaTeste));
            (void)criarPirataInicial(CLASSE_GAVEA, &pirataLoja);
            (void)criarPirataInicial(CLASSE_TIMONEIRO, &alvoTeste);
            (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoFrente);
            (void)criarInimigoInicial(INIMIGO_MARUJO_GORDO, &inimigoTras);
            pirataLoja.habilidades_desbloqueadas = 3;
            pirataLoja.precisao_ranged = 100;
            pirataLoja.habilidades[2].precisao = 100;
            pirataLoja.linha = 1;
            pirataLoja.coluna = 0;
            alvoTeste.vida_maxima = 500;
            alvoTeste.vida_atual = 500;
            alvoTeste.linha = 1;
            alvoTeste.coluna = 1;
            inimigoFrente.aliado = false;
            inimigoFrente.vida_maxima = 500;
            inimigoFrente.vida_atual = 500;
            inimigoFrente.linha = 1;
            inimigoFrente.coluna = 2;
            inimigoTras.aliado = false;
            inimigoTras.vida_maxima = 500;
            inimigoTras.vida_atual = 500;
            inimigoTras.linha = 1;
            inimigoTras.coluna = 3;
            mapaTeste.celulas[1][0] = &pirataLoja;
            mapaTeste.celulas[1][1] = &alvoTeste;
            mapaTeste.celulas[1][2] = &inimigoFrente;
            mapaTeste.celulas[1][3] = &inimigoTras;
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 2, 1, 2);
            if (inimigoFrente.vida_atual != 300 || inimigoTras.vida_atual != 300 ||
                alvoTeste.vida_atual != 400 ||
                mapaTeste.ultimo_dano_celula[1][1] != 100 ||
                mapaTeste.ultimo_dano_celula[1][2] != 200 ||
                mapaTeste.ultimo_dano_celula[1][3] != 200) {
                fprintf(stderr, "Falha sanitaria: Ultimate Gavea deve atingir todos os inimigos (100%%) e aliados (50%%) na linha.\n");
                goto limpeza;
            }

            (void)memset(&mapaTeste, 0, sizeof(mapaTeste));
            (void)criarPirataInicial(CLASSE_BUCANEIRO, &pirataLoja);
            pirataLoja.habilidades_desbloqueadas = 3;
            pirataLoja.vida_maxima = 300;
            pirataLoja.vida_atual = 50;
            pirataLoja.estamina_maxima = 10;
            pirataLoja.estamina_atual = 10;
            pirataLoja.precisao_melee = 100;
            pirataLoja.habilidades[2].precisao = 100;
            pirataLoja.linha = 1;
            pirataLoja.coluna = 0;
            alvoTeste.vida_atual = 500;
            alvoTeste.linha = 1;
            alvoTeste.coluna = 1;
            inimigoFrente.vida_atual = 500;
            inimigoFrente.linha = 1;
            inimigoFrente.coluna = 2;
            mapaTeste.celulas[1][0] = &pirataLoja;
            mapaTeste.celulas[1][1] = &alvoTeste;
            mapaTeste.celulas[1][2] = &inimigoFrente;
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 2, 1, 2);
            if (inimigoFrente.vida_atual != 350 || alvoTeste.vida_atual != 425 ||
                pirataLoja.vida_atual != 50 + ((150 + 75) * 70) / 100) {
                fprintf(stderr, "Falha sanitaria: Ultimate Bucaneiro deve atingir inimigos (100%%), aliados (50%%) e ter 70%% de roubo de vida total.\n");
                goto limpeza;
            }

            {
                ListaItens invBuild = { NULL, 0 };
                Combatente bucBuild;
                (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucBuild);
                if (!carregarInventarioSanitario(&invBuild, itens, totalItens) ||
                    !aplicarItemPassivoNoPirata(&invBuild, &bucBuild, 5) ||
                    bucBuild.build_passivos.tamanho != 1 ||
                    bucBuild.build_passivos.inicio == NULL ||
                    bucBuild.build_passivos.inicio->item.id != 5 ||
                    !aplicarItemPassivoNoPirata(&invBuild, &bucBuild, 6) ||
                    bucBuild.build_passivos.tamanho != 2 ||
                    bucBuild.build_passivos.inicio == NULL ||
                    bucBuild.build_passivos.inicio->item.id != 5 ||
                    bucBuild.build_passivos.inicio->proximo == NULL ||
                    bucBuild.build_passivos.inicio->proximo->item.id != 6 ||
                    !aplicarItemPassivoNoPirata(&invBuild, &bucBuild, 10) ||
                    bucBuild.build_passivos.tamanho != 3 ||
                    bucBuild.estamina_maxima != 7) {
                    fprintf(stderr, "Falha sanitaria: montagem de build com 2 ou mais equipamentos no Bucaneiro.\n");
                    liberarListaItensCombatente(&bucBuild.build_passivos);
                    liberarEstruturasJogo(NULL, &invBuild, NULL);
                    goto limpeza;
                }
                liberarListaItensCombatente(&bucBuild.build_passivos);
                liberarEstruturasJogo(NULL, &invBuild, NULL);
            }

            (void)memset(&mapaTeste, 0, sizeof(mapaTeste));
            (void)criarPirataInicial(CLASSE_TIMONEIRO, &pirataLoja);
            pirataLoja.habilidades_desbloqueadas = 3;
            pirataLoja.estamina_maxima = 10;
            pirataLoja.estamina_atual = 10;
            pirataLoja.precisao_melee = 100;
            pirataLoja.habilidades[2].precisao = 100;
            pirataLoja.linha = 1;
            pirataLoja.coluna = 1;
            alvoTeste.vida_atual = 500;
            alvoTeste.linha = 0;
            alvoTeste.coluna = 2;
            inimigoFrente.vida_atual = 500;
            inimigoFrente.linha = 1;
            inimigoFrente.coluna = 2;
            inimigoTras.vida_atual = 500;
            inimigoTras.linha = 2;
            inimigoTras.coluna = 2;
            mapaTeste.celulas[1][1] = &pirataLoja;
            mapaTeste.celulas[0][2] = &alvoTeste;
            mapaTeste.celulas[1][2] = &inimigoFrente;
            mapaTeste.celulas[2][2] = &inimigoTras;
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 2, 1, 2);
            if (inimigoFrente.vida_atual != 415 || inimigoTras.vida_atual != 415 ||
                alvoTeste.vida_atual != 458) {
                fprintf(stderr, "Falha sanitaria: Ultimate Timoneiro deve atingir toda a faixa 1x3 (inimigos 100%%, aliados 50%%).\n");
                goto limpeza;
            }

            (void)memset(&mapaTeste, 0, sizeof(mapaTeste));
            (void)criarPirataInicial(CLASSE_CAPITAO, &pirataLoja);
            pirataLoja.habilidades_desbloqueadas = 3;
            pirataLoja.estamina_maxima = 10;
            pirataLoja.estamina_atual = 10;
            pirataLoja.precisao_ranged = 100;
            pirataLoja.habilidades[2].precisao = 100;
            pirataLoja.linha = 1;
            pirataLoja.coluna = 0;
            alvoTeste.vida_atual = 500;
            alvoTeste.linha = 0;
            alvoTeste.coluna = 3;
            inimigoFrente.vida_atual = 500;
            inimigoFrente.linha = 1;
            inimigoFrente.coluna = 3;
            inimigoTras.vida_atual = 500;
            inimigoTras.linha = 2;
            inimigoTras.coluna = 3;
            mapaTeste.celulas[1][0] = &pirataLoja;
            mapaTeste.celulas[0][3] = &alvoTeste;
            mapaTeste.celulas[1][3] = &inimigoFrente;
            mapaTeste.celulas[2][3] = &inimigoTras;
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 2, 1, 3);
            if (inimigoFrente.vida_atual != 370 || inimigoTras.vida_atual != 370 ||
                alvoTeste.vida_atual != 435) {
                fprintf(stderr, "Falha sanitaria: Ultimate Capitao deve atingir inimigos (100%%) e aliados (50%%) na cruz.\n");
                goto limpeza;
            }

            (void)memset(&mapaTeste, 0, sizeof(mapaTeste));
            (void)criarPirataInicial(CLASSE_COZINHEIRO, &pirataLoja);
            pirataLoja.habilidades_desbloqueadas = 3;
            pirataLoja.estamina_maxima = 10;
            pirataLoja.estamina_atual = 10;
            pirataLoja.linha = 1;
            pirataLoja.coluna = 0;
            alvoTeste.vida_atual = 500;
            alvoTeste.linha = 1;
            alvoTeste.coluna = 1;
            inimigoFrente.vida_atual = 500;
            inimigoFrente.linha = 1;
            inimigoFrente.coluna = 2;
            inimigoTras.vida_atual = 500;
            inimigoTras.linha = 0;
            inimigoTras.coluna = 2;
            mapaTeste.celulas[1][0] = &pirataLoja;
            mapaTeste.celulas[1][1] = &alvoTeste;
            mapaTeste.celulas[1][2] = &inimigoFrente;
            mapaTeste.celulas[0][2] = &inimigoTras;
            executarAtaqueOuHabilidade(&mapaTeste, &pirataLoja, 1, 1, 2);
            if (alvoTeste.vida_atual != 450 || inimigoFrente.vida_atual != 440 ||
                inimigoTras.vida_atual != 500) {
                fprintf(stderr, "Falha sanitaria: Bacamarte deve ser linha reta de 2 quadrados (aliados 50%%, inimigos 100%%, fora da linha ileso).\n");
                goto limpeza;
            }

            alvoTeste.golpes_corte_grunt_recebidos = 2;
            alvoTeste.golpes_corte_bucaneiro_recebidos = 1;
            alvoTeste.turnos_sangramento = 2;
            alvoTeste.turnos_veneno = 2;
            alvoTeste.turnos_motivacao = 3;
            alvoTeste.turnos_tonico = 2;
            limparEfeitosPosCombate(&alvoTeste);
            if (alvoTeste.golpes_corte_grunt_recebidos != 0 ||
                alvoTeste.golpes_corte_bucaneiro_recebidos != 0 ||
                alvoTeste.turnos_sangramento != 0 ||
                alvoTeste.turnos_veneno != 0 ||
                alvoTeste.turnos_motivacao != 0 ||
                alvoTeste.turnos_tonico != 0) {
                fprintf(stderr, "Falha sanitaria: limpeza de buffs, debuffs e acumulacoes pos-combate.\n");
                goto limpeza;
            }
        }
    }
    sucesso = true;
    puts("Testes sanitarios de classes, mecanicas, reposicao pos-fuga, IA e estruturas aprovados.");

limpeza:
    for (indice = 0; indice < TOTAL_PIRATAS_PREDEFINIDOS; indice++) {
        liberarListaItensCombatente(&piratas[indice].build_passivos);
    }
    liberarListaItensCombatente(&boss.build_passivos);
    return sucesso;
}

// Inicia os testes e executa o ciclo principal da interface Raylib.
int main(int argc, char *argv[])
{
    InterfaceJogo interfaceJogo;
    Tabuleiro3x6 mapa;
    Combatente piratasDisponiveis[TOTAL_PIRATAS_PREDEFINIDOS];
    NoRota *raizRotas;
    NoRota *rotaAtual;
    ListaItens inventario = { NULL, 0 };
    int nivelDificuldade = 1;
    int indice;
    (void)memset(&mapa, 0, sizeof(mapa));
    (void)memset(piratasDisponiveis, 0, sizeof(piratasDisponiveis));
    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
        return executarTestesSanitarios() ? 0 : 1;
    }
    if (!executarTestesSanitarios()) {
        return 1;
    }
    contadorFugas = 0;
    contadorSaques = 0;
    inflacaoLojasPercentual = 0;
    moedasTripulacao = 300;
    raizRotas = construirArvoreRotas(nivelDificuldade);
    rotaAtual = raizRotas;
    if (raizRotas == NULL) {
        fprintf(stderr, "Falha ao inicializar a aventura.\n");
        liberarEstruturasJogo(raizRotas, &inventario, NULL);
        return 1;
    }
    for (indice = 0; indice < TOTAL_PIRATAS_PREDEFINIDOS; indice++) {
        if (!criarPirataInicial(indice, &piratasDisponiveis[indice])) {
            fprintf(stderr, "Falha ao inicializar pirata %d.\n", indice);
            liberarEstruturasJogo(raizRotas, &inventario, NULL);
            return 1;
        }
    }
    inicializarInterface(&interfaceJogo);
    while (!WindowShouldClose() && !interfaceJogo.sair_solicitado) {
        atualizarInterface(&interfaceJogo, &mapa, piratasDisponiveis,
                           TOTAL_PIRATAS_PREDEFINIDOS, &raizRotas, &rotaAtual,
                           &inventario, &moedasTripulacao, &nivelDificuldade);
        if (interfaceJogo.estado == TELA_SELECAO_TRIPULACAO) {
            desenharTelaSelecaoGrupo(&interfaceJogo, piratasDisponiveis,
                                     TOTAL_PIRATAS_PREDEFINIDOS);
        } else {
            desenharInterface(&interfaceJogo, &mapa, NULL);
        }
    }
    descarregarInterface(&interfaceJogo);
    liberarEstruturasJogo(raizRotas, &inventario, NULL);
    for (indice = 0; indice < TOTAL_PIRATAS_PREDEFINIDOS; indice++) {
        liberarListaItensCombatente(&piratasDisponiveis[indice].build_passivos);
    }
    return 0;
}
