#include "interface.h"

#include "dados_jogo.h"
#include "raylib.h"

#include <string.h>

// Inicia o protótipo e executa o ciclo principal da interface Raylib.
int main(void)
{
    InterfaceJogo interfaceJogo;
    Tabuleiro3x6 mapa;
    Combatente piratasDisponiveis[5];
    int totalDisponiveis = 5;
    int indicePirata;
    int linha;
    int coluna;
    memset(&mapa, 0, sizeof(mapa));
    memset(piratasDisponiveis, 0, sizeof(piratasDisponiveis));
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
