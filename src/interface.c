#include "interface.h"

#include "raylib.h"

// Converte o estado atual para seu rótulo de exibição.
static const char *nomeEstado(EstadoJogo estado)
{
    static const char *nomes[] = {
        "Menu", "Selecao do grupo", "Mapa de rotas", "Pre-combate",
        "Combate", "Loja", "Inventario"
    };
    if (estado < ESTADO_MENU || estado > ESTADO_INVENTARIO) {
        return "Desconhecido";
    }
    return nomes[estado];
}

// Inicializa o estado visual e abre a janela Raylib.
void inicializarInterface(InterfaceJogo *interfaceJogo)
{
    if (interfaceJogo == NULL) {
        return;
    }
    interfaceJogo->estado = ESTADO_MENU;
    interfaceJogo->linha_selecionada = -1;
    interfaceJogo->coluna_selecionada = -1;
    interfaceJogo->largura_celula = 130;
    interfaceJogo->altura_celula = 130;
    interfaceJogo->margem_esquerda = 250;
    interfaceJogo->margem_superior = 210;
    InitWindow(1280, 720, "Sea of Gunpowder - RPG Tatico");
    SetTargetFPS(60);
}

// Processa os cliques de seleção do tabuleiro.
void atualizarInterface(InterfaceJogo *interfaceJogo)
{
    Vector2 mouse;
    int linha;
    int coluna;
    if (interfaceJogo == NULL) {
        return;
    }
    if (IsKeyPressed(KEY_F1)) {
        interfaceJogo->estado = ESTADO_MENU;
    } else if (IsKeyPressed(KEY_F2)) {
        interfaceJogo->estado = ESTADO_SELECAO_GRUPO;
    } else if (IsKeyPressed(KEY_F3)) {
        interfaceJogo->estado = ESTADO_MAPA_ROTAS;
    } else if (IsKeyPressed(KEY_F4)) {
        interfaceJogo->estado = ESTADO_PRE_COMBATE;
    } else if (IsKeyPressed(KEY_F5)) {
        interfaceJogo->estado = ESTADO_COMBATE;
    } else if (IsKeyPressed(KEY_F6)) {
        interfaceJogo->estado = ESTADO_LOJA;
    } else if (IsKeyPressed(KEY_F7)) {
        interfaceJogo->estado = ESTADO_INVENTARIO;
    }
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }
    mouse = GetMousePosition();
    coluna = ((int)mouse.x - interfaceJogo->margem_esquerda) / interfaceJogo->largura_celula;
    linha = ((int)mouse.y - interfaceJogo->margem_superior) / interfaceJogo->altura_celula;
    if (mouse.x >= interfaceJogo->margem_esquerda &&
        mouse.y >= interfaceJogo->margem_superior &&
        linha >= 0 && linha < LINHAS_TABULEIRO &&
        coluna >= 0 && coluna < COLUNAS_TABULEIRO) {
        interfaceJogo->linha_selecionada = linha;
        interfaceJogo->coluna_selecionada = coluna;
        interfaceJogo->estado = ESTADO_COMBATE;
    }
}

// Desenha o estado atual e o tabuleiro interativo 3x6.
void desenharInterface(const InterfaceJogo *interfaceJogo, const Tabuleiro3x6 *mapa)
{
    int linha;
    int coluna;
    if (interfaceJogo == NULL) {
        return;
    }
    BeginDrawing();
    ClearBackground((Color){ 18, 28, 39, 255 });
    DrawText("SEA OF GUNPOWDER", 430, 42, 34, GOLD);
    DrawText("RPG tatico roguelike - prototipo C99", 438, 88, 20, RAYWHITE);
    DrawText(TextFormat("Estado: %s  |  F1-F7 troca de tela",
                        nomeEstado(interfaceJogo->estado)), 410, 125, 18, LIGHTGRAY);
    DrawText("Clique em uma celula para selecionar | ESC para sair", 380, 155, 18, LIGHTGRAY);
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            int posicaoX = interfaceJogo->margem_esquerda + coluna * interfaceJogo->largura_celula;
            int posicaoY = interfaceJogo->margem_superior + linha * interfaceJogo->altura_celula;
            Color corCelula = (Color){ 42, 63, 76, 255 };
            Combatente *unidade = mapa == NULL ? NULL : mapa->celulas[linha][coluna];
            if (interfaceJogo->linha_selecionada == linha &&
                interfaceJogo->coluna_selecionada == coluna) {
                corCelula = (Color){ 92, 113, 62, 255 };
            }
            DrawRectangle(posicaoX, posicaoY, interfaceJogo->largura_celula - 4,
                          interfaceJogo->altura_celula - 4, corCelula);
            DrawRectangleLines(posicaoX, posicaoY, interfaceJogo->largura_celula - 4,
                               interfaceJogo->altura_celula - 4, (Color){ 180, 193, 184, 255 });
            if (unidade != NULL && unidade->vivo) {
                DrawText(unidade->nome, posicaoX + 8, posicaoY + 28, 14,
                         unidade->aliado ? SKYBLUE : ORANGE);
                DrawText(TextFormat("PV: %d", unidade->vida_atual), posicaoX + 8, posicaoY + 55, 16, RAYWHITE);
            } else {
                DrawText(TextFormat("%d,%d", linha, coluna), posicaoX + 8, posicaoY + 12, 14, LIGHTGRAY);
            }
        }
    }
    if (interfaceJogo->linha_selecionada >= 0) {
        DrawText(TextFormat("Selecionada: linha %d, coluna %d",
                            interfaceJogo->linha_selecionada,
                            interfaceJogo->coluna_selecionada), 485, 620, 20, RAYWHITE);
    }
    EndDrawing();
}

// Encerra a janela aberta pela interface.
void encerrarInterface(void)
{
    CloseWindow();
}
