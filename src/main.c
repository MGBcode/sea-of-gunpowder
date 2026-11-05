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
    if (bucaneiro.vida_maxima != 100 || bucaneiro.velocidade != 9 ||
        bucaneiro.precisao_melee != 90 || bucaneiro.estamina_maxima != 4 ||
        timoneiro.vida_maxima != 160 || timoneiro.velocidade != 4 ||
        timoneiro.precisao_melee != 60 || timoneiro.estamina_maxima != 4 ||
        cozinheiro.vida_maxima != 130 || cozinheiro.velocidade != 6 ||
        cozinheiro.precisao_melee != 60 || cozinheiro.estamina_maxima != 5 ||
        capitao.vida_maxima != 120 || capitao.velocidade != 7 ||
        capitao.precisao_melee != 70 || capitao.precisao_ranged != 60 ||
        gavea.vida_maxima != 70 || gavea.velocidade != 5 ||
        gavea.precisao_melee != 40 || gavea.estamina_maxima != 5) {
        fprintf(stderr, "Falha sanitaria: atributos base dos 5 piratas incorretos.\n");
        return false;
    }

    (void)subirNivelPirata(&gavea, 1);
    if (gavea.nivel != 2 || gavea.estamina_maxima != 6 || gavea.vida_maxima != 84 ||
        gavea.habilidades[0].dano_min != 24 || gavea.habilidades[0].dano_max != 48) {
        fprintf(stderr, "Falha sanitaria: Gavea Nv 2 (estamina 6 e +20%% vida/dano).\n");
        return false;
    }
    (void)subirNivelPirata(&gavea, 2);
    if (gavea.nivel != 4 || gavea.estamina_maxima != 7) {
        fprintf(stderr, "Falha sanitaria: Gavea Nv 4 deve ter 7 de estamina.\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    bucaneiro.linha = 1;
    bucaneiro.coluna = 0;
    mapa.celulas[1][0] = &bucaneiro;
    if (!moverCombatente3x6(&mapa, &bucaneiro, 1, 2) ||
        bucaneiro.coluna != 2 || bucaneiro.estamina_atual != 3) {
        fprintf(stderr, "Falha sanitaria: passiva Nv 1 do Bucaneiro (andar 2 casas).\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_BUCANEIRO, &bucaneiro);
    (void)subirNivelPirata(&bucaneiro, 5);
    if (bucaneiro.nivel != 6 || bucaneiro.habilidades_desbloqueadas != 3 ||
        bucaneiro.estamina_maxima != 8) {
        fprintf(stderr, "Falha sanitaria: progressao Nv 6 do Bucaneiro.\n");
        return false;
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
    inimigoA.vida_atual = 300;
    inimigoA.linha = 1;
    inimigoA.coluna = 3;
    mapa.celulas[1][1] = &bucaneiro;
    mapa.celulas[1][3] = &inimigoA;
    mapa.mina_ativa[1][2] = 1;
    executarAtaqueOuHabilidade(&mapa, &bucaneiro, 2, 1, 3);
    if (bucaneiro.coluna != 4 || mapa.mina_ativa[1][2] != 0 ||
        inimigoA.vida_atual >= 300 || bucaneiro.vida_atual <= 50) {
        fprintf(stderr, "Falha sanitaria: Avanco Impetuoso (dash dist 3 + lifesteal + mina).\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_TIMONEIRO, &timoneiro);
    (void)subirNivelPirata(&timoneiro, 5);
    timoneiro.habilidades[2].precisao = 100;
    timoneiro.linha = 1;
    timoneiro.coluna = 1;
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoA);
    inimigoA.linha = 1;
    inimigoA.coluna = 2;
    mapa.celulas[1][1] = &timoneiro;
    mapa.celulas[1][2] = &inimigoA;
    executarAtaqueOuHabilidade(&mapa, &timoneiro, 2, 1, 2);
    if (inimigoA.coluna != 3 || inimigoA.turnos_desnorteado != 2) {
        fprintf(stderr, "Falha sanitaria: Giro do Timao (knockback + desnortear).\n");
        return false;
    }

    (void)memset(&mapa, 0, sizeof(mapa));
    (void)criarPirataInicial(CLASSE_COZINHEIRO, &cozinheiro);
    (void)subirNivelPirata(&cozinheiro, 5);
    cozinheiro.habilidades[1].precisao = 100;
    cozinheiro.linha = 1;
    cozinheiro.coluna = 1;
    bucaneiro.vida_atual = 20;
    bucaneiro.linha = 0;
    bucaneiro.coluna = 2;
    mapa.celulas[1][1] = &cozinheiro;
    mapa.celulas[0][2] = &bucaneiro;
    executarAtaqueOuHabilidade(&mapa, &cozinheiro, 2, 0, 2);
    if (bucaneiro.vida_atual <= 20) {
        fprintf(stderr, "Falha sanitaria: Hora do Rango (cura em area 2x3).\n");
        return false;
    }
    cozinheiro.estamina_atual = cozinheiro.estamina_maxima;
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoA);
    (void)criarInimigoInicial(INIMIGO_GRUNT_ESPADA, &inimigoB);
    inimigoA.linha = 1;
    inimigoA.coluna = 2;
    inimigoB.linha = 0;
    inimigoB.coluna = 3;
    mapa.celulas[1][2] = &inimigoA;
    mapa.celulas[0][3] = &inimigoB;
    executarAtaqueOuHabilidade(&mapa, &cozinheiro, 1, 0, 3);
    if (inimigoA.vida_atual >= inimigoA.vida_maxima ||
        inimigoB.vida_atual != inimigoB.vida_maxima) {
        fprintf(stderr, "Falha sanitaria: Bacamarte (cone bloqueado na distancia 1).\n");
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
    inimigoA.vida_maxima = 500;
    inimigoA.vida_atual = 500;
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
    if (bucaneiro.turnos_sangramento != 3) {
        fprintf(stderr, "Falha sanitaria: 3 cortes do Grunt devem aplicar Sangramento.\n");
        return false;
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
    int moedas = 300;
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
        interfaceJogo.total_selecionados != 2 ||
        interfaceJogo.total_sobreviventes_fuga != 2 ||
        piratas[CLASSE_BUCANEIRO].nivel != 3 ||
        piratas[CLASSE_BUCANEIRO].build_passivos.tamanho != 1 ||
        !piratas[CLASSE_TIMONEIRO].vivo ||
        piratas[CLASSE_TIMONEIRO].nivel != 1 ||
        piratas[CLASSE_TIMONEIRO].vida_maxima != 160 ||
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
        if (!usarItemConsumivel(&piratas[CLASSE_COZINHEIRO], &bandagemTeste) ||
            piratas[CLASSE_COZINHEIRO].vida_atual != 98 ||
            !piratas[CLASSE_COZINHEIRO].usou_consumivel_gratis_turno ||
            bandagemTeste.quantidade != 0) {
            fprintf(stderr, "Falha sanitaria: passiva do Cozinheiro (+20%% e gratis).\n");
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
        itensLoja[0].raridade != RARIDADE_LENDARIO || estoquesLoja[0] != 2 ||
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
    (void)memset(&mapa, 0, sizeof(mapa));
    boss.vida_atual = boss.vida_maxima;
    boss.vivo = true;
    boss.linha = 1;
    boss.coluna = 3;
    mapa.celulas[1][3] = &boss;
    subtrairVida(&mapa, &boss, boss.vida_maxima / 2);
    if (!mapa.boss_fase_2_ativa || boss.fase_boss != 2 ||
        boss.estamina_maxima != 20) {
        fprintf(stderr, "Falha sanitaria: transicao de fase do chefe.\n");
        goto limpeza;
    }
    {
        int grunts = 0;
        int linha;
        int coluna;
        for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
            for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                Combatente *unidade = mapa.celulas[linha][coluna];
                if (unidade != NULL && !unidade->aliado &&
                    unidade->tipo_inimigo == INIMIGO_GRUNT_ESPADA) {
                    grunts++;
                }
            }
        }
        if (grunts != 2) {
            fprintf(stderr, "Falha sanitaria: invocacao de grunts na fase dois.\n");
            goto limpeza;
        }
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
        gruntPistola.habilidades[0].dano_min = 30;
        gruntPistola.habilidades[0].dano_max = 40;
        gruntPistola.linha = 1;
        gruntPistola.coluna = 4;
        alvoTeste.linha = 1;
        alvoTeste.coluna = 1;
        mapaTeste.celulas[1][4] = &gruntPistola;
        mapaTeste.celulas[1][1] = &alvoTeste;

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
    while (!WindowShouldClose()) {
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
    encerrarInterface();
    liberarEstruturasJogo(raizRotas, &inventario, NULL);
    for (indice = 0; indice < TOTAL_PIRATAS_PREDEFINIDOS; indice++) {
        liberarListaItensCombatente(&piratasDisponiveis[indice].build_passivos);
    }
    return 0;
}
