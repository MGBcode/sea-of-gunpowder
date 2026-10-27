#include "interface.h"

#include "dados_jogo.h"
#include "raylib.h"

#include <string.h>

// Inicia o protótipo e executa o ciclo principal da interface Raylib.
int main(void)
{
    InterfaceJogo interfaceJogo;
    Tabuleiro3x6 mapa;
    Combatente pirata;
    int linha;
    int coluna;
    memset(&mapa, 0, sizeof(mapa));
    inicializarInterface(&interfaceJogo);
    (void)criarPirataInicial(0, &pirata);
    pirata.linha = 1;
    pirata.coluna = 0;
    mapa.celulas[pirata.linha][pirata.coluna] = &pirata;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            break;
        }
        atualizarInterface(&interfaceJogo);
        desenharInterface(&interfaceJogo, &mapa);
    }

    encerrarInterface();
    liberarListaItensCombatente(&pirata.build_passivos);
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            mapa.celulas[linha][coluna] = NULL;
        }
    }
    return 0;
}

