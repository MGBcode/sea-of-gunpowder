#include "interface.h"

#include "combate.h"
#include "dados_jogo.h"
#include "ordenacao.h"
#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constrói a lista de inventário com os itens do catálogo inicial.
static bool carregarInventarioSanitario(ListaItens *inventario, const Item itens[], int totalItens)
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

// Exercita o ciclo de vida das estruturas antes de abrir a janela.
static bool executarTestesSanitarios(NoRota **raiz, ListaItens *inventario,
                                    ListaAlcance **alcance, Combatente *pirataTeste)
{
    Item itens[TOTAL_ITENS_PREDEFINIDOS];
    Tabuleiro3x6 mapa;
    Combatente aliado;
    Combatente inimigo;
    Combatente *fila[LINHAS_TABULEIRO * COLUNAS_TABULEIRO];
    NoRota *rota;
    NoCelula *celula;
    int totalItens;
    int totalFila;
    int nivelDificuldade = 1;
    int indice;
    bool encontrouAliadoPerigoso = false;
    bool encontrouInimigoSeguro = false;

    memset(&mapa, 0, sizeof(mapa));
    if (raiz == NULL || inventario == NULL || alcance == NULL || pirataTeste == NULL ||
        *raiz != NULL) {
        fprintf(stderr, "Falha no teste sanitario: estado inicial invalido.\n");
        return false;
    }

    *raiz = construirArvoreRotas(1);
    if (*raiz == NULL || (*raiz)->nivel_dificuldade != 1) {
        fprintf(stderr, "Falha no teste sanitario: construcao da arvore de rotas.\n");
        return false;
    }
    rota = avancarParaRota(*raiz, 1);
    rota = avancarParaRota(rota, 3);
    if (rota == NULL || rota->etapa != 2 || rota->tipo != ROTA_LOJA) {
        fprintf(stderr, "Falha no teste sanitario: navegacao da arvore de rotas.\n");
        return false;
    }

    totalItens = carregarItensIniciais(itens, TOTAL_ITENS_PREDEFINIDOS);
    if (totalItens != TOTAL_ITENS_PREDEFINIDOS ||
        !carregarInventarioSanitario(inventario, itens, totalItens) ||
        !criarPirataInicial(0, pirataTeste)) {
        fprintf(stderr, "Falha no teste sanitario: carga do inventario.\n");
        return false;
    }
    for (indice = 5; indice <= 9; indice++) {
        if (!aplicarItemPassivoNoPirata(inventario, pirataTeste, indice)) {
            fprintf(stderr, "Falha no teste sanitario: equipar passivo.\n");
            return false;
        }
    }
    if (pirataTeste->build_passivos.tamanho != MAX_ITENS_PASSIVOS ||
        aplicarItemPassivoNoPirata(inventario, pirataTeste, 10)) {
        fprintf(stderr, "Falha no teste sanitario: limite de passivos.\n");
        return false;
    }
    if (!desequiparItemPassivoBuild(inventario, pirataTeste, 5) ||
        !aplicarItemPassivoNoPirata(inventario, pirataTeste, 10) ||
        pirataTeste->build_passivos.tamanho != MAX_ITENS_PASSIVOS) {
        fprintf(stderr, "Falha no teste sanitario: desequipar e substituir passivo.\n");
        return false;
    }

    if (!criarPirataInicial(1, &aliado) || !criarInimigoInicial(0, &inimigo)) {
        fprintf(stderr, "Falha no teste sanitario: criacao de combatentes.\n");
        return false;
    }
    pirataTeste->linha = 1;
    pirataTeste->coluna = 1;
    aliado.linha = 0;
    aliado.coluna = 2;
    inimigo.linha = 1;
    inimigo.coluna = 3;
    pirataTeste->habilidades_desbloqueadas = 3;
    pirataTeste->turnos_motivacao = 1;
    aliado.velocidade = 9;
    inimigo.velocidade = 6;
    mapa.celulas[1][1] = pirataTeste;
    mapa.celulas[0][2] = &aliado;
    mapa.celulas[1][3] = &inimigo;

    *alcance = calcularAreaAtuacao3x6(&mapa, pirataTeste, 2);
    if (*alcance == NULL || (*alcance)->tamanho <= 0) {
        fprintf(stderr, "Falha no teste sanitario: calculo de alcance.\n");
        return false;
    }
    for (celula = (*alcance)->inicio; celula != NULL; celula = celula->proximo) {
        if (celula->linha == aliado.linha && celula->coluna == aliado.coluna &&
            celula->perigo_fogo_amigo == 1) {
            encontrouAliadoPerigoso = true;
        }
        if (celula->linha == inimigo.linha && celula->coluna == inimigo.coluna &&
            celula->perigo_fogo_amigo == 0) {
            encontrouInimigoSeguro = true;
        }
    }
    if (!encontrouAliadoPerigoso || !encontrouInimigoSeguro) {
        fprintf(stderr, "Falha no teste sanitario: celulas ou flags de fogo amigo.\n");
        return false;
    }

    totalFila = montarFilaTurnos(&mapa, fila);
    if (totalFila != 3) {
        fprintf(stderr, "Falha no teste sanitario: montagem da fila de turnos.\n");
        return false;
    }
    fila[0] = pirataTeste;
    fila[1] = &inimigo;
    fila[2] = &aliado;
    ordenarFilaPorVelocidade(fila, totalFila);
    if (fila[0] != &aliado || fila[1] != pirataTeste || fila[2] != &inimigo) {
        fprintf(stderr, "Falha no teste sanitario: ordenacao da fila de turnos.\n");
        return false;
    }

    *raiz = fugirDaAventura(*raiz, inventario, &nivelDificuldade);
    if (*raiz == NULL || nivelDificuldade != 2 || (*raiz)->nivel_dificuldade != 2 ||
        inventario->tamanho != 1 || inventario->inicio == NULL ||
        inventario->inicio->item.id != 5 || pirataTeste->build_passivos.tamanho != 5) {
        fprintf(stderr, "Falha no teste sanitario: fuga e reinicio da aventura.\n");
        return false;
    }
    puts("Testes sanitarios de estruturas e ordenacao aprovados.");
    return true;
}

// Inicia o protótipo e executa o ciclo principal da interface Raylib.
int main(void)
{
    InterfaceJogo interfaceJogo;
    Tabuleiro3x6 mapa;
    Combatente piratasDisponiveis[5];
    Combatente pirataTeste;
    NoRota *raizRotas = NULL;
    ListaItens inventarioGeral = { NULL, 0 };
    ListaAlcance *alcanceTeste = NULL;
    int totalDisponiveis = 5;
    int indicePirata;
    int linha;
    int coluna;
    memset(&mapa, 0, sizeof(mapa));
    memset(piratasDisponiveis, 0, sizeof(piratasDisponiveis));
    memset(&pirataTeste, 0, sizeof(pirataTeste));
    if (!executarTestesSanitarios(&raizRotas, &inventarioGeral, &alcanceTeste,
                                  &pirataTeste)) {
        liberarEstruturasJogo(raizRotas, &inventarioGeral, alcanceTeste);
        liberarListaItensCombatente(&pirataTeste.build_passivos);
        return 1;
    }
    for (indicePirata = 0; indicePirata < totalDisponiveis; indicePirata++) {
        (void)criarPirataInicial(indicePirata, &piratasDisponiveis[indicePirata]);
    }
    inicializarInterface(&interfaceJogo);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            break;
        }
        atualizarInterface(&interfaceJogo, &mapa, piratasDisponiveis, totalDisponiveis);
        if (interfaceJogo.estado == ESTADO_SELECAO_GRUPO) {
            desenharTelaSelecaoGrupo(&interfaceJogo, piratasDisponiveis, totalDisponiveis);
        } else {
            desenharInterface(&interfaceJogo, &mapa, NULL);
        }
    }

    encerrarInterface();
    liberarEstruturasJogo(raizRotas, &inventarioGeral, alcanceTeste);
    liberarListaItensCombatente(&pirataTeste.build_passivos);
    for (indicePirata = 0; indicePirata < totalDisponiveis; indicePirata++) {
        liberarListaItensCombatente(&piratasDisponiveis[indicePirata].build_passivos);
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            mapa.celulas[linha][coluna] = NULL;
        }
    }
    return 0;
}
