#include "interface.h"

#include "combate.h"
#include "raylib.h"

// Retorna o slot selecionado que corresponde ao ID do pirata.
static int indicePirataSelecionado(const InterfaceJogo *interfaceJogo, int idPirata)
{
    int indice;
    for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
        if (interfaceJogo->piratas_selecionados_ids[indice] == idPirata) {
            return indice;
        }
    }
    return -1;
}

// Converte uma posição do mouse em coordenadas válidas do tabuleiro.
static bool obterCelulaClicada(const InterfaceJogo *interfaceJogo, Vector2 mouse,
                               int *linha, int *coluna)
{
    int x;
    int y;
    if (interfaceJogo->largura_celula <= 0 || interfaceJogo->altura_celula <= 0 ||
        mouse.x < interfaceJogo->margem_esquerda ||
        mouse.y < interfaceJogo->margem_superior) {
        return false;
    }
    x = (int)mouse.x - interfaceJogo->margem_esquerda;
    y = (int)mouse.y - interfaceJogo->margem_superior;
    *coluna = x / interfaceJogo->largura_celula;
    *linha = y / interfaceJogo->altura_celula;
    return *linha >= 0 && *linha < LINHAS_TABULEIRO &&
           *coluna >= 0 && *coluna < COLUNAS_TABULEIRO &&
           x % interfaceJogo->largura_celula < interfaceJogo->largura_celula - 4 &&
           y % interfaceJogo->altura_celula < interfaceJogo->altura_celula - 4;
}

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
    interfaceJogo->piratas_selecionados_ids[0] = 0;
    interfaceJogo->piratas_selecionados_ids[1] = 0;
    interfaceJogo->piratas_selecionados_ids[2] = 0;
    interfaceJogo->total_selecionados = 0;
    interfaceJogo->pirata_posicionando_indice = -1;
    InitWindow(1280, 720, "Sea of Gunpowder - RPG Tatico");
    SetTargetFPS(60);
}

// Processa as seleções de grupo e os cliques no tabuleiro.
void atualizarInterface(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                        Combatente piratasDisponiveis[], int totalDisponiveis)
{
    Vector2 mouse;
    int linha;
    int coluna;
    int indice;
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
    if (interfaceJogo->estado == ESTADO_SELECAO_GRUPO) {
        int limiteDisponiveis = totalDisponiveis;
        int cardIndice = -1;
        if (limiteDisponiveis > 5) {
            limiteDisponiveis = 5;
        }
        for (indice = 0; indice < limiteDisponiveis; indice++) {
            int cardX = 60 + (indice % 3) * 390;
            int cardY = 145 + (indice / 3) * 205;
            if (mouse.x >= cardX && mouse.x < cardX + 350 &&
                mouse.y >= cardY && mouse.y < cardY + 180) {
                cardIndice = indice;
                break;
            }
        }
        if (cardIndice >= 0 && piratasDisponiveis != NULL) {
            int slotSelecionado = indicePirataSelecionado(interfaceJogo,
                                                          piratasDisponiveis[cardIndice].id);
            if (slotSelecionado >= 0) {
                for (indice = slotSelecionado; indice < interfaceJogo->total_selecionados - 1;
                     indice++) {
                    interfaceJogo->piratas_selecionados_ids[indice] =
                        interfaceJogo->piratas_selecionados_ids[indice + 1];
                }
                interfaceJogo->total_selecionados--;
                interfaceJogo->piratas_selecionados_ids[interfaceJogo->total_selecionados] = 0;
            } else if (interfaceJogo->total_selecionados < 3) {
                interfaceJogo->piratas_selecionados_ids[interfaceJogo->total_selecionados] =
                    piratasDisponiveis[cardIndice].id;
                interfaceJogo->total_selecionados++;
            }
            return;
        }
        if (mouse.x >= 440 && mouse.x <= 840 && mouse.y >= 590 && mouse.y <= 650 &&
            interfaceJogo->total_selecionados == 3 && mapa != NULL &&
            piratasDisponiveis != NULL) {
            int indicePirata;
            for (indicePirata = 0; indicePirata < interfaceJogo->total_selecionados;
                 indicePirata++) {
                for (indice = 0; indice < limiteDisponiveis; indice++) {
                    Combatente *pirata = &piratasDisponiveis[indice];
                    if (pirata->id == interfaceJogo->piratas_selecionados_ids[indicePirata]) {
                        (void)posicionarPirataPreBatalha(mapa, pirata, indicePirata, 0);
                        break;
                    }
                }
            }
            interfaceJogo->pirata_posicionando_indice = -1;
            interfaceJogo->estado = ESTADO_PRE_COMBATE;
        }
        return;
    }
    if (interfaceJogo->estado == ESTADO_PRE_COMBATE && mapa != NULL &&
        piratasDisponiveis != NULL &&
        obterCelulaClicada(interfaceJogo, mouse, &linha, &coluna) && coluna <= 1) {
        Combatente *unidade = mapa->celulas[linha][coluna];
        if (unidade != NULL && unidade->vivo && unidade->aliado) {
            interfaceJogo->pirata_posicionando_indice =
                indicePirataSelecionado(interfaceJogo, unidade->id);
        } else if (unidade == NULL && interfaceJogo->pirata_posicionando_indice >= 0 &&
                   interfaceJogo->pirata_posicionando_indice < interfaceJogo->total_selecionados) {
            int idPirata = interfaceJogo->piratas_selecionados_ids[
                interfaceJogo->pirata_posicionando_indice];
            for (indice = 0; indice < totalDisponiveis; indice++) {
                if (piratasDisponiveis[indice].id == idPirata) {
                    if (posicionarPirataPreBatalha(mapa, &piratasDisponiveis[indice],
                                                    linha, coluna)) {
                        interfaceJogo->pirata_posicionando_indice = -1;
                    }
                    break;
                }
            }
        }
        return;
    }
    if (interfaceJogo->estado == ESTADO_COMBATE &&
        obterCelulaClicada(interfaceJogo, mouse, &linha, &coluna)) {
        interfaceJogo->linha_selecionada = linha;
        interfaceJogo->coluna_selecionada = coluna;
    }
}

// Desenha os cards dos piratas e o controle para iniciar a aventura.
void desenharTelaSelecaoGrupo(InterfaceJogo *interfaceJogo,
                              const Combatente piratasDisponiveis[], int totalDisponiveis)
{
    int limiteDisponiveis = totalDisponiveis;
    int indice;
    if (interfaceJogo == NULL || piratasDisponiveis == NULL) {
        return;
    }
    if (limiteDisponiveis > 5) {
        limiteDisponiveis = 5;
    }
    if (limiteDisponiveis < 0) {
        limiteDisponiveis = 0;
    }
    BeginDrawing();
    ClearBackground((Color){ 18, 28, 39, 255 });
    DrawText("ESCOLHA SUA TRIPULACAO", 350, 45, 32, GOLD);
    DrawText(TextFormat("Selecione 3 piratas (%d/3)", interfaceJogo->total_selecionados),
             470, 95, 20, RAYWHITE);
    for (indice = 0; indice < limiteDisponiveis; indice++) {
        int cardX = 60 + (indice % 3) * 390;
        int cardY = 145 + (indice / 3) * 205;
        bool selecionado = indicePirataSelecionado(interfaceJogo,
                                                   piratasDisponiveis[indice].id) >= 0;
        Color corBorda = selecionado ? (Color){ 90, 220, 120, 255 } :
                                       (Color){ 150, 165, 175, 255 };
        DrawRectangle(cardX, cardY, 350, 180, (Color){ 42, 63, 76, 255 });
        DrawRectangleLinesEx((Rectangle){ (float)cardX, (float)cardY, 350.0f, 180.0f },
                             selecionado ? 4.0f : 2.0f, corBorda);
        DrawText(piratasDisponiveis[indice].nome, cardX + 16, cardY + 14, 20, RAYWHITE);
        DrawText(TextFormat("HP: %d  Estamina: %d", piratasDisponiveis[indice].vida_maxima,
                            piratasDisponiveis[indice].estamina_maxima),
                 cardX + 16, cardY + 55, 16, LIGHTGRAY);
        DrawText(TextFormat("Velocidade: %d  Defesa: %d",
                            piratasDisponiveis[indice].velocidade,
                            piratasDisponiveis[indice].defesa),
                 cardX + 16, cardY + 85, 16, LIGHTGRAY);
        DrawText(TextFormat("Dano Base: %d", piratasDisponiveis[indice].dano_base),
                 cardX + 16, cardY + 115, 16, LIGHTGRAY);
        DrawText(selecionado ? "SELECIONADO" : "Clique para selecionar",
                 cardX + 16, cardY + 148, 14, selecionado ? GOLD : GRAY);
    }
    if (interfaceJogo->total_selecionados == 3) {
        DrawRectangle(440, 590, 400, 60, (Color){ 45, 125, 70, 255 });
        DrawRectangleLines(440, 590, 400, 60, (Color){ 180, 230, 180, 255 });
        DrawText("[ INICIAR AVENTURA ]", 485, 608, 22, RAYWHITE);
    } else {
        DrawRectangle(440, 590, 400, 60, (Color){ 65, 70, 75, 255 });
        DrawRectangleLines(440, 590, 400, 60, (Color){ 110, 115, 120, 255 });
        DrawText("[ INICIAR AVENTURA ]", 485, 608, 22, DARKGRAY);
    }
    EndDrawing();
}

// Desenha a fila de turnos ordenada pela velocidade dos combatentes vivos.
static void desenharFilaTurnos(const Tabuleiro3x6 *mapa)
{
    Combatente *turnos[LINHAS_TABULEIRO * COLUNAS_TABULEIRO];
    int total = 0;
    int linha;
    int coluna;
    int indice;
    int larguraCard;
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            Combatente *unidade = mapa->celulas[linha][coluna];
            if (unidade != NULL && unidade->vivo) {
                int posicao = total;
                while (posicao > 0 &&
                       turnos[posicao - 1]->velocidade < unidade->velocidade) {
                    turnos[posicao] = turnos[posicao - 1];
                    posicao--;
                }
                turnos[posicao] = unidade;
                total++;
            }
        }
    }
    DrawRectangle(0, 0, 1280, 45, (Color){ 10, 17, 24, 240 });
    if (total == 0) {
        DrawText("FILA DE TURNOS: sem combatentes", 12, 14, 16, LIGHTGRAY);
        return;
    }
    larguraCard = (1152 - (total - 1) * 4) / total;
    DrawText("FILA DE TURNOS", 8, 15, 12, GOLD);
    for (indice = 0; indice < total; indice++) {
        int cardX = 120 + indice * (larguraCard + 4);
        int larguraTexto = larguraCard - 8;
        int maxCaracteres = larguraTexto / 6;
        if (maxCaracteres < 1) {
            maxCaracteres = 1;
        }
        DrawRectangle(cardX, 7, larguraCard, 31,
                      turnos[indice]->aliado ? (Color){ 35, 75, 105, 255 } :
                                              (Color){ 115, 55, 40, 255 });
        DrawText(TextFormat("%.*s", maxCaracteres, turnos[indice]->nome),
                 cardX + 4, 17, 10, RAYWHITE);
    }
}

// Desenha o estado atual e o tabuleiro interativo 3x6.
void desenharInterface(const InterfaceJogo *interfaceJogo, const Tabuleiro3x6 *mapa,
                       const ListaAlcance *alcance)
{
    int linha;
    int coluna;
    if (interfaceJogo == NULL) {
        return;
    }
    BeginDrawing();
    ClearBackground((Color){ 18, 28, 39, 255 });
    DrawText("SEA OF GUNPOWDER", 430, 52, 30, GOLD);
    DrawText("RPG tatico roguelike - prototipo C99", 438, 88, 18, RAYWHITE);
    DrawText(TextFormat("Estado: %s  |  F1-F7 troca de tela",
                        nomeEstado(interfaceJogo->estado)), 410, 125, 16, LIGHTGRAY);
    DrawText("Clique em uma celula para selecionar | ESC para sair", 380, 155, 16, LIGHTGRAY);
    if ((interfaceJogo->estado == ESTADO_COMBATE ||
         interfaceJogo->estado == ESTADO_PRE_COMBATE) && mapa != NULL) {
        desenharFilaTurnos(mapa);
    }
    if (interfaceJogo->estado == ESTADO_PRE_COMBATE) {
        DrawText("Zona de Preparacao Free", interfaceJogo->margem_esquerda,
                 interfaceJogo->margem_superior - 22, 16, SKYBLUE);
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            int posicaoX = interfaceJogo->margem_esquerda + coluna * interfaceJogo->largura_celula;
            int posicaoY = interfaceJogo->margem_superior + linha * interfaceJogo->altura_celula;
            Color corCelula = (Color){ 42, 63, 76, 255 };
            const NoCelula *celulaAlcance;
            bool estaNoAlcance = false;
            Combatente *unidade = mapa == NULL ? NULL : mapa->celulas[linha][coluna];
            DrawRectangle(posicaoX, posicaoY, interfaceJogo->largura_celula - 4,
                          interfaceJogo->altura_celula - 4, corCelula);
            if (interfaceJogo->estado == ESTADO_PRE_COMBATE && coluna <= 1) {
                DrawRectangle(posicaoX, posicaoY, interfaceJogo->largura_celula - 4,
                              interfaceJogo->altura_celula - 4, (Color){ 30, 90, 150, 100 });
            }
            if (alcance != NULL) {
                for (celulaAlcance = alcance->inicio; celulaAlcance != NULL;
                     celulaAlcance = celulaAlcance->proximo) {
                    if (celulaAlcance->linha == linha && celulaAlcance->coluna == coluna) {
                        estaNoAlcance = true;
                        break;
                    }
                }
            }
            if (estaNoAlcance) {
                DrawRectangle(posicaoX, posicaoY, interfaceJogo->largura_celula - 4,
                              interfaceJogo->altura_celula - 4, (Color){ 200, 150, 30, 120 });
            }
            if (interfaceJogo->linha_selecionada == linha &&
                interfaceJogo->coluna_selecionada == coluna) {
                DrawRectangle(posicaoX, posicaoY, interfaceJogo->largura_celula - 4,
                              interfaceJogo->altura_celula - 4, (Color){ 92, 113, 62, 150 });
            }
            DrawRectangleLines(posicaoX, posicaoY, interfaceJogo->largura_celula - 4,
                               interfaceJogo->altura_celula - 4, (Color){ 180, 193, 184, 255 });
            if (unidade != NULL && unidade->vivo) {
                int vidaAtual = unidade->vida_atual;
                int vidaMaxima = unidade->vida_maxima;
                int larguraVida;
                int estaminaAtual = unidade->estamina_atual;
                int estaminaMaxima = unidade->estamina_maxima;
                int indiceEstamina;
                int espacoEstamina;
                int raioEstamina;
                if (vidaAtual < 0) {
                    vidaAtual = 0;
                }
                if (vidaMaxima < 0) {
                    vidaMaxima = 0;
                }
                if (vidaAtual > vidaMaxima && vidaMaxima > 0) {
                    vidaAtual = vidaMaxima;
                }
                larguraVida = vidaMaxima > 0 ? 104 * vidaAtual / vidaMaxima : 0;
                if (estaminaMaxima < 0) {
                    estaminaMaxima = 0;
                }
                if (estaminaAtual < 0) {
                    estaminaAtual = 0;
                }
                if (estaminaAtual > estaminaMaxima) {
                    estaminaAtual = estaminaMaxima;
                }
                DrawText(unidade->nome, posicaoX + 8, posicaoY + 8, 14,
                         unidade->aliado ? SKYBLUE : ORANGE);
                DrawText(TextFormat("PV: %d/%d", unidade->vida_atual, unidade->vida_maxima),
                         posicaoX + 8, posicaoY + 27, 15, RAYWHITE);
                DrawRectangle(posicaoX + 8, posicaoY + 49, 104, 10, (Color){ 80, 35, 35, 255 });
                DrawRectangle(posicaoX + 8, posicaoY + 49, larguraVida, 10,
                              (Color){ 70, 200, 90, 255 });
                DrawText("EST", posicaoX + 8, posicaoY + 67, 12, LIGHTGRAY);
                espacoEstamina = estaminaMaxima > 1 ? 100 / (estaminaMaxima - 1) : 0;
                raioEstamina = estaminaMaxima > 1 ? espacoEstamina / 3 : 5;
                if (raioEstamina > 5) {
                    raioEstamina = 5;
                }
                if (raioEstamina < 1) {
                    raioEstamina = 1;
                }
                for (indiceEstamina = 0; indiceEstamina < estaminaMaxima; indiceEstamina++) {
                    int centroX = posicaoX + 14 + indiceEstamina * espacoEstamina;
                    DrawCircle(centroX, posicaoY + 91, raioEstamina,
                               indiceEstamina < estaminaAtual ? GOLD : DARKGRAY);
                }
            } else {
                DrawText(TextFormat("%d,%d", linha, coluna), posicaoX + 8, posicaoY + 12, 14, LIGHTGRAY);
            }
        }
    }
    if (interfaceJogo->linha_selecionada >= 0) {
        DrawText(TextFormat("Selecionada: linha %d, coluna %d",
                            interfaceJogo->linha_selecionada,
                            interfaceJogo->coluna_selecionada), 485, 596, 18, RAYWHITE);
    }
    if (interfaceJogo->estado == ESTADO_COMBATE) {
        static const char *acoes[] = { "[1] Andar", "[2] Descansar", "[3] Item", "[4] Habilidade" };
        int indiceAcao;
        DrawRectangle(100, 615, 1080, 90, (Color){ 12, 20, 29, 235 });
        for (indiceAcao = 0; indiceAcao < 4; indiceAcao++) {
            int posicaoX = 120 + indiceAcao * 265;
            DrawRectangle(posicaoX, 630, 245, 58, (Color){ 42, 63, 76, 255 });
            DrawRectangleLines(posicaoX, 630, 245, 58, (Color){ 180, 193, 184, 255 });
            DrawText(acoes[indiceAcao], posicaoX + 16, 648, 20, RAYWHITE);
        }
    }
    EndDrawing();
}

// Encerra a janela aberta pela interface.
void encerrarInterface(void)
{
    CloseWindow();
}
