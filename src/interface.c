#include "interface.h"

#include "combate.h"
#include "dados_jogo.h"
#include "ia_inimigos.h"
#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LARGURA_VIRTUAL 1280
#define ALTURA_VIRTUAL 720
#define LARGURA_RENDER_1080P 1920
#define ALTURA_RENDER_1080P 1080
#define ESCALA_1080P(v) (((float)(v)) * 1.5f)

static const InterfaceJogo *interfaceRenderizada;
static RenderTexture2D canvasVirtual;
static bool canvasCarregado = false;

extern void *glfwGetCurrentContext(void);
extern void glfwGetWindowSize(void *window, int *width, int *height);

// Obtém as dimensões reais da área cliente da janela para manter viewport e cliques sincronizados.
static void obterDimensoesClienteJanela(int *largura, int *altura)
{
    int larg = GetScreenWidth();
    int alt = GetScreenHeight();
    void *contexto = glfwGetCurrentContext();
    if (contexto != NULL) {
        int largGlfw = 0;
        int altGlfw = 0;
        glfwGetWindowSize(contexto, &largGlfw, &altGlfw);
        if (largGlfw > 0 && altGlfw > 0) {
            larg = largGlfw;
            alt = altGlfw;
            if (!IsWindowFullscreen() &&
                (GetScreenWidth() != largGlfw || GetScreenHeight() != altGlfw ||
                 GetRenderWidth() != largGlfw || GetRenderHeight() != altGlfw)) {
                SetWindowSize(largGlfw - 1, altGlfw);
                SetWindowSize(largGlfw, altGlfw);
            }
        }
    }
    if (largura != NULL) {
        *largura = larg;
    }
    if (altura != NULL) {
        *altura = alt;
    }
}

static void desenharRetangulo1080p(int posX, int posY, int largura, int altura, Color cor)
{
    DrawRectangleRec((Rectangle){ ESCALA_1080P(posX), ESCALA_1080P(posY),
                                  ESCALA_1080P(largura), ESCALA_1080P(altura) }, cor);
}

static void desenharBorda1080p(int posX, int posY, int largura, int altura, Color cor)
{
    DrawRectangleLinesEx((Rectangle){ ESCALA_1080P(posX), ESCALA_1080P(posY),
                                      ESCALA_1080P(largura), ESCALA_1080P(altura) },
                         1.5f, cor);
}

static void desenharBordaEx1080p(Rectangle rec, float espessura, Color cor)
{
    DrawRectangleLinesEx((Rectangle){ rec.x * 1.5f, rec.y * 1.5f,
                                      rec.width * 1.5f, rec.height * 1.5f },
                         espessura * 1.5f, cor);
}

static void desenharCirculo1080p(int centroX, int centroY, float raio, Color cor)
{
    DrawCircleV((Vector2){ ESCALA_1080P(centroX), ESCALA_1080P(centroY) },
                raio * 1.5f, cor);
}

#define DrawRectangle desenharRetangulo1080p
#define DrawRectangleLines desenharBorda1080p
#define DrawRectangleLinesEx desenharBordaEx1080p
#define DrawCircle desenharCirculo1080p

// Converte coordenadas reais da janela/tela cheia para a resolução virtual 1280x720.
bool converterCoordenadaMouseVirtual(float mouseRealX, float mouseRealY,
                                     int larguraJanela, int alturaJanela,
                                     int *virtualX, int *virtualY)
{
    if (larguraJanela <= 0 || alturaJanela <= 0 || virtualX == NULL || virtualY == NULL) {
        return false;
    }
    if (mouseRealX < 0.0f || mouseRealX >= (float)larguraJanela ||
        mouseRealY < 0.0f || mouseRealY >= (float)alturaJanela) {
        return false;
    }
    *virtualX = (int)((mouseRealX * (float)LARGURA_VIRTUAL) / (float)larguraJanela);
    *virtualY = (int)((mouseRealY * (float)ALTURA_VIRTUAL) / (float)alturaJanela);
    return true;
}

// Obtém a posição atual do mouse já mapeada para a resolução virtual 1280x720.
static Vector2 obterMouseVirtual(void)
{
    Vector2 mouseReal = GetMousePosition();
    int larguraJanela = 0;
    int alturaJanela = 0;
    int virtualX = (int)mouseReal.x;
    int virtualY = (int)mouseReal.y;
    obterDimensoesClienteJanela(&larguraJanela, &alturaJanela);
    if (converterCoordenadaMouseVirtual(mouseReal.x, mouseReal.y,
                                        larguraJanela, alturaJanela,
                                        &virtualX, &virtualY)) {
        return (Vector2){ (float)virtualX, (float)virtualY };
    }
    return mouseReal;
}

static void desenharFundoEstado(const InterfaceJogo *interfaceJogo)
{
    Texture2D textura = { 0 };
    Rectangle origem;
    Rectangle destino;
    if (interfaceJogo == NULL) {
        return;
    }
    if (interfaceJogo->estado == TELA_COMBATE_3X6) {
        if (interfaceJogo->bgBossAtivo) {
            textura = interfaceJogo->bgBoss;
        } else if (interfaceJogo->bgCombateAtualIdx >= 0 &&
                   interfaceJogo->bgCombateAtualIdx < interfaceJogo->qtdBgCombate &&
                   interfaceJogo->bgCombateAtualIdx < 5) {
            textura = interfaceJogo->bgCombatePool[interfaceJogo->bgCombateAtualIdx];
        }
    } else {
        textura = interfaceJogo->bgMar;
    }
    if (textura.id == 0) {
        return;
    }
    origem = (Rectangle){ 0.0f, 0.0f, (float)textura.width,
                          (float)textura.height };
    destino = canvasCarregado && canvasVirtual.id != 0 ?
              (Rectangle){ 0.0f, 0.0f, (float)canvasVirtual.texture.width,
                           (float)canvasVirtual.texture.height } :
              (Rectangle){ 0.0f, 0.0f, (float)GetScreenWidth(),
                           (float)GetScreenHeight() };
    DrawTexturePro(textura, origem, destino, (Vector2){ 0.0f, 0.0f },
                   0.0f, WHITE);
}

static Texture2D obterTexturaFundoCombate(const InterfaceJogo *interfaceJogo)
{
    if (interfaceJogo == NULL) {
        return (Texture2D){ 0 };
    }
    if (interfaceJogo->bgBossAtivo) {
        return interfaceJogo->bgBoss;
    }
    if (interfaceJogo->bgCombateAtualIdx >= 0 &&
        interfaceJogo->bgCombateAtualIdx < interfaceJogo->qtdBgCombate &&
        interfaceJogo->bgCombateAtualIdx < 5) {
        return interfaceJogo->bgCombatePool[interfaceJogo->bgCombateAtualIdx];
    }
    return (Texture2D){ 0 };
}

// Inicia o quadro e aplica o fundo apropriado ao estado da interface.
static void iniciarQuadroVirtual(const InterfaceJogo *interfaceJogo, Color corFundo)
{
    if (interfaceJogo != NULL && interfaceJogo->estado == TELA_COMBATE_3X6 &&
        obterTexturaFundoCombate(interfaceJogo).id == 0) {
        corFundo = DARKBLUE;
    }
    if (canvasCarregado && canvasVirtual.id != 0) {
        BeginTextureMode(canvasVirtual);
        ClearBackground(corFundo);
        desenharFundoEstado(interfaceJogo);
    } else {
        BeginDrawing();
        ClearBackground(corFundo);
        desenharFundoEstado(interfaceJogo);
    }
}

// Finaliza o framebuffer virtual e o escala suavemente para o tamanho real da janela.
static void finalizarQuadroVirtual(void)
{
    if (canvasCarregado && canvasVirtual.id != 0) {
        int larguraJanela = 0;
        int alturaJanela = 0;
        Rectangle origem = { 0.0f, 0.0f,
                             (float)canvasVirtual.texture.width,
                             -(float)canvasVirtual.texture.height };
        Rectangle destino;
        obterDimensoesClienteJanela(&larguraJanela, &alturaJanela);
        destino = (Rectangle){ 0.0f, 0.0f,
                               (float)larguraJanela,
                               (float)alturaJanela };
        EndTextureMode();
        BeginDrawing();
        ClearBackground((Color){ 10, 16, 22, 255 });
        DrawTexturePro(canvasVirtual.texture, origem, destino,
                       (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        EndDrawing();
    } else {
        EndDrawing();
    }
}

static Font obterFonteInterface(void)
{
    if (interfaceRenderizada != NULL && interfaceRenderizada->fonteJogo.texture.id != 0) {
        return interfaceRenderizada->fonteJogo;
    }
    return GetFontDefault();
}

static Rectangle converterBoundsVirtual(Rectangle bounds)
{
    return (Rectangle){ bounds.x * 1.5f, bounds.y * 1.5f,
                        bounds.width * 1.5f, bounds.height * 1.5f };
}

static void desenharTexturaBounds(Texture2D textura, Rectangle bounds)
{
    Rectangle origem;
    if (textura.id == 0 || textura.width <= 0 || textura.height <= 0) {
        return;
    }
    origem = (Rectangle){ 0.0f, 0.0f, (float)textura.width, (float)textura.height };
    DrawTexturePro(textura, origem, converterBoundsVirtual(bounds),
                   (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
}

void DrawUIButtonCustom(Rectangle bounds, const char *text, Texture2D texBotao, Font font)
{
    Rectangle boundsRender = converterBoundsVirtual(bounds);
    if (texBotao.id != 0) {
        desenharTexturaBounds(texBotao, bounds);
    } else {
        DrawRectangleRec(boundsRender, (Color){ 42, 63, 76, 255 });
        DrawRectangleLinesEx(bounds, 2.0f, (Color){ 125, 153, 170, 255 });
    }
    if (text != NULL) {
        Vector2 tamanhoTexto;
        float tamanhoFonte = bounds.height * 0.34f;
        if (font.texture.id == 0) {
            font = GetFontDefault();
        }
        tamanhoTexto = MeasureTextEx(font, text, tamanhoFonte, 0.0f);
        DrawTextEx(font, text,
                   (Vector2){ boundsRender.x + (boundsRender.width - tamanhoTexto.x * 1.5f) / 2.0f,
                              boundsRender.y + (boundsRender.height - tamanhoTexto.y * 1.5f) / 2.0f },
                   tamanhoFonte * 1.5f, 0.0f, RAYWHITE);
    }
}

void DrawUIPanelCustom(Rectangle bounds, Texture2D texPainel, Color colorFallback)
{
    Rectangle boundsRender = converterBoundsVirtual(bounds);
    if (texPainel.id != 0) {
        desenharTexturaBounds(texPainel, bounds);
    } else {
        DrawRectangleRec(boundsRender, colorFallback);
        DrawRectangleLinesEx(bounds, 1.5f, (Color){ 90, 110, 125, 210 });
    }
}

void DrawUIFrameCustom(Rectangle bounds, Texture2D texBorda)
{
    if (texBorda.id != 0) {
        desenharTexturaBounds(texBorda, bounds);
    } else {
        DrawRectangleLinesEx(bounds, 2.0f, GOLD);
    }
}

void desenharTextoGlobal(const char *texto, Vector2 pos, float tamanho, Color cor)
{
    if (texto != NULL) {
        DrawTextEx(obterFonteInterface(), texto,
                   (Vector2){ pos.x * 1.5f, pos.y * 1.5f },
                   tamanho * 1.5f, 1.5f, cor);
    }
}

static void desenharTexto(const char *texto, int posX, int posY, int tamanho, Color cor)
{
    if (texto != NULL) {
        desenharTextoGlobal(texto, (Vector2){ (float)posX, (float)posY },
                            (float)tamanho + 2.0f, cor);
    }
}

static int medirLarguraTexto(const char *texto, int tamanho);

static void desenharTextoCentralizado(const char *texto, int posY,
                                      int tamanho, Color cor)
{
    int largura = medirLarguraTexto(texto, tamanho);
    desenharTexto(texto, (LARGURA_VIRTUAL - largura) / 2, posY, tamanho, cor);
}

// Mede a largura lógica de uma string na fonte 1080p atual para quebra automática de linha.
static int medirLarguraTexto(const char *texto, int tamanho)
{
    if (texto == NULL || texto[0] == '\0') {
        return 0;
    }
    if (obterFonteInterface().texture.id != 0) {
        return (int)(MeasureTextEx(obterFonteInterface(), texto,
                                   (float)(tamanho + 2) * 1.5f, 1.5f).x / 1.5f);
    }
    return MeasureText(texto, tamanho);
}

// Desenha texto quebrando linhas dentro da largura máxima e respeitando limites verticais.
static int desenharTextoQuebrado(const char *texto, int posX, int posY, int larguraMax,
                                 int tamanho, int espacoLinha, Color cor,
                                 int minY, int maxY)
{
    char linhaAtual[384];
    char palavra[128];
    char candidata[512];
    int yAtual = posY;
    int idxTexto = 0;
    int tamLinha = 0;
    if (texto == NULL || texto[0] == '\0') {
        return 0;
    }
    linhaAtual[0] = '\0';
    while (texto[idxTexto] != '\0') {
        int tamPalavra = 0;
        while (texto[idxTexto] == ' ') {
            idxTexto++;
        }
        if (texto[idxTexto] == '\0') {
            break;
        }
        while (texto[idxTexto] != '\0' && texto[idxTexto] != ' ' &&
               tamPalavra < (int)sizeof(palavra) - 1) {
            palavra[tamPalavra++] = texto[idxTexto++];
        }
        palavra[tamPalavra] = '\0';
        if (tamLinha == 0) {
            (void)strncpy(linhaAtual, palavra, sizeof(linhaAtual) - 1);
            linhaAtual[sizeof(linhaAtual) - 1] = '\0';
            tamLinha = tamPalavra;
        } else {
            (void)snprintf(candidata, sizeof(candidata), "%s %s", linhaAtual, palavra);
            if (medirLarguraTexto(candidata, tamanho) <= larguraMax) {
                (void)strncpy(linhaAtual, candidata, sizeof(linhaAtual) - 1);
                linhaAtual[sizeof(linhaAtual) - 1] = '\0';
                tamLinha = (int)strlen(linhaAtual);
            } else {
                if ((minY <= 0 && maxY <= 0) || (yAtual >= minY && yAtual + tamanho <= maxY)) {
                    desenharTexto(linhaAtual, posX, yAtual, tamanho, cor);
                }
                yAtual += espacoLinha;
                (void)strncpy(linhaAtual, palavra, sizeof(linhaAtual) - 1);
                linhaAtual[sizeof(linhaAtual) - 1] = '\0';
                tamLinha = tamPalavra;
            }
        }
    }
    if (tamLinha > 0) {
        if ((minY <= 0 && maxY <= 0) || (yAtual >= minY && yAtual + tamanho <= maxY)) {
            desenharTexto(linhaAtual, posX, yAtual, tamanho, cor);
        }
        yAtual += espacoLinha;
    }
    return yAtual - posY;
}

// Retorna a cor correspondente à raridade do item (Verde, Azul, Roxo ou Laranja).
static Color corRaridadeItem(RaridadeItem raridade)
{
    if (raridade == RARIDADE_COMUM) {
        return (Color){ 80, 225, 100, 255 };
    }
    if (raridade == RARIDADE_RARO) {
        return (Color){ 75, 170, 255, 255 };
    }
    if (raridade == RARIDADE_EPICO) {
        return (Color){ 195, 105, 255, 255 };
    }
    return (Color){ 255, 155, 45, 255 };
}

// Retorna o nome textual da raridade do item.
static const char *nomeRaridadeItem(RaridadeItem raridade)
{
    if (raridade == RARIDADE_COMUM) {
        return "Comum";
    }
    if (raridade == RARIDADE_RARO) {
        return "Raro";
    }
    if (raridade == RARIDADE_EPICO) {
        return "Épico";
    }
    return "Lendário";
}

// Retorna verdadeiro quando o retângulo recebe um clique esquerdo do mouse.
static bool retanguloClicado(Rectangle retangulo, Vector2 mouse)
{
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
           CheckCollisionPointRec(mouse, retangulo);
}

// Converte o clique do mouse em uma célula válida do tabuleiro 3x6.
static bool obterCelulaClicada(const InterfaceJogo *interfaceJogo, Vector2 mouse,
                               int *linha, int *coluna)
{
    int offsetX;
    int offsetY;
    if (interfaceJogo->largura_celula <= 0 || interfaceJogo->altura_celula <= 0 ||
        mouse.x < interfaceJogo->margem_esquerda ||
        mouse.y < interfaceJogo->margem_superior) {
        return false;
    }
    offsetX = (int)mouse.x - interfaceJogo->margem_esquerda;
    offsetY = (int)mouse.y - interfaceJogo->margem_superior;
    *coluna = offsetX / interfaceJogo->largura_celula;
    *linha = offsetY / interfaceJogo->altura_celula;
    return *linha >= 0 && *linha < LINHAS_TABULEIRO &&
           *coluna >= 0 && *coluna < COLUNAS_TABULEIRO &&
           offsetX % interfaceJogo->largura_celula < interfaceJogo->largura_celula - 4 &&
           offsetY % interfaceJogo->altura_celula < interfaceJogo->altura_celula - 4;
}

// Retorna a posição do pirata selecionado pelo ID na lista da tripulação.
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

// Verifica se o pirata é um sobrevivente fixo de uma desistência anterior.
static bool ehSobreviventeFixoFuga(const InterfaceJogo *interfaceJogo, int idPirata)
{
    int indice;
    for (indice = 0; indice < interfaceJogo->total_sobreviventes_fuga; indice++) {
        if (interfaceJogo->ids_sobreviventes_fuga[indice] == idPirata) {
            return true;
        }
    }
    return false;
}

static int posicaoXSlotFila(int slot)
{
    return 10 + slot * 50;
}

// Busca o pirata disponível cujo ID corresponde ao solicitado.
static Combatente *pirataPorId(Combatente piratas[], int total, int id)
{
    int indice;
    for (indice = 0; indice < total; indice++) {
        if (piratas[indice].id == id) {
            return &piratas[indice];
        }
    }
    return NULL;
}

// Garante que a entidade selecionada na mochila pertença sempre à tripulação viva.
static Combatente *garantirPirataSelecionadoMochila(InterfaceJogo *interfaceJogo,
                                                    Combatente piratas[], int total)
{
    int indice;
    if (interfaceJogo == NULL || piratas == NULL) {
        return NULL;
    }
    if (interfaceJogo->entidade_selecionada != NULL &&
        interfaceJogo->entidade_selecionada->aliado &&
        interfaceJogo->entidade_selecionada->vivo &&
        indicePirataSelecionado(interfaceJogo,
                                interfaceJogo->entidade_selecionada->id) >= 0) {
        return interfaceJogo->entidade_selecionada;
    }
    if (interfaceJogo->ator_atual != NULL &&
        interfaceJogo->ator_atual->aliado &&
        interfaceJogo->ator_atual->vivo &&
        indicePirataSelecionado(interfaceJogo,
                                interfaceJogo->ator_atual->id) >= 0) {
        interfaceJogo->entidade_selecionada = interfaceJogo->ator_atual;
        return interfaceJogo->ator_atual;
    }
    for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
        Combatente *pirata = pirataPorId(piratas, total,
                                         interfaceJogo->piratas_selecionados_ids[indice]);
        if (pirata != NULL && pirata->vivo) {
            interfaceJogo->entidade_selecionada = pirata;
            return pirata;
        }
    }
    return interfaceJogo->entidade_selecionada;
}

// Calcula o scroll máximo da mochila para evitar ocultar itens visíveis e clicáveis.
static int calcularMaxScrollMochila(const InterfaceJogo *interfaceJogo)
{
    NoItem *item;
    int totalVisiveis = 0;
    bool somenteConsumiveis;
    if (interfaceJogo == NULL || interfaceJogo->inventario == NULL) {
        return 0;
    }
    somenteConsumiveis = interfaceJogo->estado == TELA_COMBATE_3X6 &&
                         !interfaceJogo->fase_pre_combate_ativa;
    for (item = interfaceJogo->inventario->inicio; item != NULL; item = item->proximo) {
        if (somenteConsumiveis && item->item.tipo != ITEM_CONSUMIVEL) {
            continue;
        }
        totalVisiveis++;
    }
    if (totalVisiveis <= 4) {
        return 0;
    }
    return (totalVisiveis - 4) * 48;
}

// Retorna o combatente correspondente ao card clicado na barra superior de ordem de turno.
static Combatente *obterAtorClicadoNaFila(const InterfaceJogo *interfaceJogo, Vector2 mouse)
{
    int indice;
    int slotVisivel = 0;
    if (interfaceJogo == NULL) {
        return NULL;
    }
    for (indice = 0; indice < interfaceJogo->total_iniciativa && slotVisivel < 9; indice++) {
        Combatente *ator = interfaceJogo->fila_iniciativa[indice].ator;
        int posCardX;
        if (ator == NULL || !ator->vivo) {
            continue;
        }
        posCardX = posicaoXSlotFila(slotVisivel);
        if (retanguloClicado((Rectangle){ (float)posCardX, 7.0f, 45.0f, 45.0f },
                             mouse)) {
            return ator;
        }
        slotVisivel++;
    }
    return NULL;
}

// Avança a fila de iniciativa até o próximo combatente vivo apto a agir.
static void avancarIniciativa(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                              Combatente piratas[], int totalPiratas);
static void concluirAcaoAliada(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                               Combatente piratas[], int totalPiratas);

// Limpa todas as células e minas do tabuleiro sem invalidar os combatentes.
static void limparMapa(Tabuleiro3x6 *mapa)
{
    int linha;
    int coluna;
    (void)memset(mapa, 0, sizeof(*mapa));
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            mapa->celulas[linha][coluna] = NULL;
        }
    }
}

// Posiciona os três piratas vivos selecionados na primeira coluna do tabuleiro.
static void posicionarTripulacao(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                                 Combatente piratas[], int totalPiratas)
{
    int indice;
    Combatente *primeiroVivo = NULL;
    for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
        Combatente *pirata = pirataPorId(piratas, totalPiratas,
                                         interfaceJogo->piratas_selecionados_ids[indice]);
        if (pirata != NULL && pirata->vivo) {
            limparEfeitosPosCombate(pirata);
            pirata->aliado = true;
            pirata->estamina_atual = pirata->estamina_maxima;
            (void)posicionarPirataPreBatalha(mapa, pirata, indice, 0);
            if (primeiroVivo == NULL) {
                primeiroVivo = pirata;
            }
        }
    }
    if (primeiroVivo != NULL &&
        (interfaceJogo->entidade_selecionada == NULL ||
         !interfaceJogo->entidade_selecionada->vivo ||
         !interfaceJogo->entidade_selecionada->aliado)) {
        interfaceJogo->entidade_selecionada = primeiroVivo;
    }
}

// Soma todo o dano registrado nas células do tabuleiro na última ação executada.
static int somarUltimoDanoTabuleiro(const Tabuleiro3x6 *mapa)
{
    int linha;
    int coluna;
    int soma = 0;
    if (mapa == NULL) {
        return 0;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            soma += mapa->ultimo_dano_celula[linha][coluna];
        }
    }
    return soma;
}

// Soma toda a cura registrada nas células do tabuleiro na última ação executada.
static int somarUltimaCuraTabuleiro(const Tabuleiro3x6 *mapa)
{
    int linha;
    int coluna;
    int soma = 0;
    if (mapa == NULL) {
        return 0;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            soma += mapa->ultima_cura_celula[linha][coluna];
        }
    }
    return soma;
}

// Remove nós de itens consumíveis cuja quantidade tenha chegado a zero.
static void limparConsumiveisZerados(ListaItens *inventario)
{
    NoItem **elo;
    if (inventario == NULL) {
        return;
    }
    elo = &inventario->inicio;
    while (*elo != NULL) {
        if ((*elo)->item.quantidade <= 0) {
            NoItem *removido = *elo;
            *elo = removido->proximo;
            free(removido);
            inventario->tamanho--;
        } else {
            elo = &(*elo)->proximo;
        }
    }
}

static bool usarConsumivelSelecionado(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                                     Combatente piratas[], int totalPiratas,
                                     ListaItens *inventario, Combatente *alvoUso)
{
    NoItem *item;
    Combatente *cozinheiroAtivo = interfaceJogo->ator_atual;
    bool turnoCozinheiroGratis;
    bool aprimoradoCozinheiro;
    bool flagSalva;
    ClassePirata classeSalva;
    if (interfaceJogo->item_id_selecionado <= 0 || alvoUso == NULL ||
        !alvoUso->aliado || !alvoUso->vivo || inventario == NULL) {
        return false;
    }
    for (item = inventario->inicio; item != NULL; item = item->proximo) {
        if (item->item.id == interfaceJogo->item_id_selecionado &&
            item->item.tipo == ITEM_CONSUMIVEL && item->item.quantidade > 0) {
            break;
        }
    }
    if (item == NULL) {
        interfaceJogo->item_id_selecionado = 0;
        TraceLog(LOG_WARNING, "O consumível selecionado não está mais disponível.");
        return false;
    }
    turnoCozinheiroGratis = interfaceJogo->estado == TELA_COMBATE_3X6 &&
                            !interfaceJogo->fase_pre_combate_ativa &&
                            cozinheiroAtivo != NULL && cozinheiroAtivo->aliado &&
                            cozinheiroAtivo->classe == CLASSE_COZINHEIRO &&
                            !cozinheiroAtivo->usou_consumivel_gratis_turno;
    aprimoradoCozinheiro = cozinheiroAtivo != NULL && cozinheiroAtivo->aliado &&
                            cozinheiroAtivo->classe == CLASSE_COZINHEIRO &&
                            cozinheiroAtivo->passiva_aprimorada;
    flagSalva = alvoUso->passiva_aprimorada;
    classeSalva = alvoUso->classe;
    if (aprimoradoCozinheiro) {
        alvoUso->passiva_aprimorada = true;
        alvoUso->classe = CLASSE_COZINHEIRO;
    }
    if (!usarItemConsumivel(alvoUso, &item->item)) {
        alvoUso->passiva_aprimorada = flagSalva;
        alvoUso->classe = classeSalva;
        TraceLog(LOG_WARNING, "O consumível não pôde ser usado neste personagem.");
        return false;
    }
    alvoUso->passiva_aprimorada = flagSalva;
    alvoUso->classe = classeSalva;
    if (alvoUso->nivel >= 4) {
        alvoUso->passiva_aprimorada = true;
    }
    interfaceJogo->item_id_selecionado = 0;
    limparConsumiveisZerados(inventario);
    if (interfaceJogo->scroll_mochila > calcularMaxScrollMochila(interfaceJogo)) {
        interfaceJogo->scroll_mochila = calcularMaxScrollMochila(interfaceJogo);
    }
    if (interfaceJogo->estado == TELA_COMBATE_3X6 &&
        !interfaceJogo->fase_pre_combate_ativa) {
        interfaceJogo->popup_ativo = POPUP_NENHUM;
        if (turnoCozinheiroGratis) {
            cozinheiroAtivo->usou_consumivel_gratis_turno = true;
        } else {
            concluirAcaoAliada(interfaceJogo, mapa, piratas, totalPiratas);
        }
    }
    return true;
}

// Cria inimigos ou o chefe em posições variadas na metade direita (colunas 3 a 5).
static void iniciarEncontro(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                            Combatente piratas[], int totalPiratas)
{
    int indice;
    int linha;
    int coluna;
    int indices[4];
    int celulasDireita[9][2];
    int totalCelulasDireita = 0;
    int etapaAtual;
    if (interfaceJogo->rota_atual == NULL || *interfaceJogo->rota_atual == NULL) {
        return;
    }
    interfaceJogo->bgBossAtivo = (*interfaceJogo->rota_atual)->tipo == ROTA_CHEFE;
    if (interfaceJogo->bgBossAtivo) {
        interfaceJogo->bgCombateAtualIdx = -1;
    } else if (interfaceJogo->qtdBgCombate > 0) {
        interfaceJogo->bgCombateAtualIdx =
            GetRandomValue(0, interfaceJogo->qtdBgCombate - 1);
    } else {
        interfaceJogo->bgCombateAtualIdx = -1;
    }
    limparMapa(mapa);
    posicionarTripulacao(interfaceJogo, mapa, piratas, totalPiratas);
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 3; coluna < COLUNAS_TABULEIRO; coluna++) {
            celulasDireita[totalCelulasDireita][0] = linha;
            celulasDireita[totalCelulasDireita][1] = coluna;
            totalCelulasDireita++;
        }
    }
    for (indice = totalCelulasDireita - 1; indice > 0; indice--) {
        int sorteio = GetRandomValue(0, indice);
        int tempLin = celulasDireita[indice][0];
        int tempCol = celulasDireita[indice][1];
        celulasDireita[indice][0] = celulasDireita[sorteio][0];
        celulasDireita[indice][1] = celulasDireita[sorteio][1];
        celulasDireita[sorteio][0] = tempLin;
        celulasDireita[sorteio][1] = tempCol;
    }
    etapaAtual = (*interfaceJogo->rota_atual)->etapa;
    if ((*interfaceJogo->rota_atual)->tipo == ROTA_CHEFE) {
        interfaceJogo->total_inimigos = 1;
        if (!criarChefeFinal(&interfaceJogo->inimigos[0])) {
            TraceLog(LOG_ERROR, "Não foi possível criar Barba Negra.");
            interfaceJogo->estado = TELA_FIM_DE_JOGO;
            return;
        }
        escalonarInimigoPorRota(&interfaceJogo->inimigos[0], ROTA_CHEFE,
                                 etapaAtual, contadorFugas, false);
        interfaceJogo->inimigos[0].etapa_rota = etapaAtual;
        interfaceJogo->inimigos[0].linha = 1;
        interfaceJogo->inimigos[0].coluna = COLUNAS_TABULEIRO - 1;
        mapa->celulas[1][COLUNAS_TABULEIRO - 1] = &interfaceJogo->inimigos[0];
    } else {
        unsigned int semente = (unsigned int)(*interfaceJogo->rota_atual)->nivel_dificuldade;
        semente += (unsigned int)(etapaAtual * 31);
        semente += (unsigned int)(contadorSaques * 17 + contadorFugas * 13);
        interfaceJogo->total_inimigos =
            sortearInimigosPorEtapa((*interfaceJogo->rota_atual)->tipo, etapaAtual,
                                    semente, indices, 4);
        for (indice = 0; indice < interfaceJogo->total_inimigos; indice++) {
            Combatente *inimigo = &interfaceJogo->inimigos[indice];
            if (!criarInimigoInicial(indices[indice], inimigo)) {
                TraceLog(LOG_ERROR, "Não foi possível criar inimigo de índice %d.",
                         indices[indice]);
                interfaceJogo->estado = TELA_FIM_DE_JOGO;
                return;
            }
            escalonarInimigoPorRota(inimigo, (*interfaceJogo->rota_atual)->tipo,
                                     etapaAtual, contadorFugas,
                                     contadorSaques > 0 ||
                                     interfaceJogo->saque_em_andamento ||
                                     (*interfaceJogo->rota_atual)->tipo == ROTA_LOJA);
            inimigo->etapa_rota = etapaAtual;
            if (indice == 0 && podeGerarEliteNaEtapa(etapaAtual, contadorFugas)) {
                aplicarBuffElite(inimigo);
            }
            linha = celulasDireita[indice][0];
            coluna = celulasDireita[indice][1];
            inimigo->linha = linha;
            inimigo->coluna = coluna;
            mapa->celulas[linha][coluna] = inimigo;
        }
    }
    interfaceJogo->total_iniciativa = montarFilaTurnos(mapa, interfaceJogo->fila_iniciativa);
    interfaceJogo->indice_iniciativa = -1;
    interfaceJogo->ator_atual = NULL;
    interfaceJogo->acao_combate = ACAO_TELA_NENHUMA;
    interfaceJogo->pirata_posicionando_indice = -1;
    interfaceJogo->timer_turno_inimigo = 0.0f;
    interfaceJogo->timer_destaque_acao = 0.0f;
    interfaceJogo->linha_destaque_acao = -1;
    interfaceJogo->coluna_destaque_acao = -1;
    interfaceJogo->linha_origem_acao = -1;
    interfaceJogo->coluna_origem_acao = -1;
    (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                   "Posicione sua tripulação nas Colunas 0 e 1 antes da batalha.");
    interfaceJogo->fase_pre_combate_ativa = true;
    interfaceJogo->estado = TELA_COMBATE_3X6;
}

static int quantidadeItemInventario(const ListaItens *inventario, int id)
{
    const NoItem *item;
    int quantidade = 0;
    if (inventario == NULL) {
        return 0;
    }
    for (item = inventario->inicio; item != NULL; item = item->proximo) {
        if (item->item.id == id) {
            quantidade += item->item.quantidade;
        }
    }
    return quantidade;
}

static void registrarItensRecompensa(InterfaceJogo *interfaceJogo,
                                     const int quantidadesAntes[TOTAL_ITENS_PREDEFINIDOS])
{
    Item catalogo[TOTAL_ITENS_PREDEFINIDOS];
    int indice;
    interfaceJogo->total_itens_recompensa = 0;
    (void)carregarItensIniciais(catalogo, TOTAL_ITENS_PREDEFINIDOS);
    for (indice = 0; indice < TOTAL_ITENS_PREDEFINIDOS &&
                      interfaceJogo->total_itens_recompensa < 6; indice++) {
        int quantidadeNova = quantidadeItemInventario(interfaceJogo->inventario,
                                                       catalogo[indice].id) -
                             quantidadesAntes[indice];
        if (quantidadeNova > 0) {
            Item *recompensa =
                &interfaceJogo->itens_recompensa[interfaceJogo->total_itens_recompensa++];
            *recompensa = catalogo[indice];
            recompensa->quantidade = quantidadeNova;
        }
    }
}

// Aplica a recompensa da rota ou do saque da loja após vencer o combate.
static void concluirVitoria(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                            Combatente piratas[], int totalPiratas)
{
    Item catalogo[TOTAL_ITENS_PREDEFINIDOS];
    int quantidadesAntes[TOTAL_ITENS_PREDEFINIDOS] = { 0 };
    int indice;
    TipoNoRota tipoAtual = (*interfaceJogo->rota_atual)->tipo;
    (void)carregarItensIniciais(catalogo, TOTAL_ITENS_PREDEFINIDOS);
    for (indice = 0; indice < TOTAL_ITENS_PREDEFINIDOS; indice++) {
        quantidadesAntes[indice] =
            quantidadeItemInventario(interfaceJogo->inventario, catalogo[indice].id);
    }
    for (indice = 0; indice < totalPiratas; indice++) {
        limparEfeitosPosCombate(&piratas[indice]);
    }
    if (tipoAtual == ROTA_LOJA && interfaceJogo->saque_em_andamento) {
        if (!saquearItensRestantesLoja(interfaceJogo->itens_loja,
                                       interfaceJogo->estoques_loja,
                                       interfaceJogo->total_itens_loja,
                                       interfaceJogo->inventario)) {
            TraceLog(LOG_ERROR, "Falha ao transferir itens saqueados da loja.");
        }
        interfaceJogo->saque_em_andamento = false;
    } else if (tipoAtual != ROTA_CHEFE) {
        if (!aplicarRecompensaRota(tipoAtual, piratas, totalPiratas,
                                   interfaceJogo->inventario, 0)) {
            TraceLog(LOG_ERROR, "Falha ao alocar recompensas da rota.");
            interfaceJogo->estado = TELA_FIM_DE_JOGO;
            return;
        }
    }
    if (tipoAtual != ROTA_CHEFE) {
        registrarItensRecompensa(interfaceJogo, quantidadesAntes);
    }
    interfaceJogo->timer_indicador_dano = 0.0f;
    if (tipoAtual == ROTA_CHEFE) {
        interfaceJogo->estado = TELA_VITORIA;
    } else {
        limparMapa(mapa);
        posicionarTripulacao(interfaceJogo, mapa, piratas, totalPiratas);
        interfaceJogo->estado = TELA_MAPA_ROTAS;
        interfaceJogo->popup_ativo = POPUP_RECOMPENSAS;
    }
}

// Verifica se a batalha terminou e encaminha para vitória ou fim de jogo.
static bool verificarResultadoBatalha(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                                      Combatente piratas[], int totalPiratas)
{
    bool vitoriaAliada = false;
    if (!verificarFimDeCombate(mapa, &vitoriaAliada)) {
        return false;
    }
    if (vitoriaAliada) {
        concluirVitoria(interfaceJogo, mapa, piratas, totalPiratas);
    } else {
        interfaceJogo->estado = TELA_FIM_DE_JOGO;
    }
    return true;
}

// Executa uma única ação visível do inimigo atual e atualiza o log e destaque visual.
static void executarPassoTurnoInimigo(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                                      Combatente piratas[], int totalPiratas)
{
    Combatente *inimigo = interfaceJogo->ator_atual;
    CandidatoAcao escolha;
    bool acaoSelecionada;
    if (inimigo == NULL || !inimigo->vivo || inimigo->aliado ||
        inimigo->turno_perdido || inimigo->em_descanso ||
        inimigo->acoes_basicas_restantes <= 0) {
        avancarIniciativa(interfaceJogo, mapa, piratas, totalPiratas);
        return;
    }
    interfaceJogo->linha_origem_acao = inimigo->linha;
    interfaceJogo->coluna_origem_acao = inimigo->coluna;
    acaoSelecionada = selecionarAcaoIA(mapa, inimigo, &escolha);
    if ((!acaoSelecionada || escolha.tipo_acao == ACAO_IA_DESCANSAR) &&
        inimigo->eh_boss && inimigo->estamina_atual > 0) {
        inimigo->acoes_basicas_restantes = 0;
        (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                       "Barba Negra não descansa enquanto ainda possui estamina.");
    } else if (!acaoSelecionada || escolha.tipo_acao == ACAO_IA_DESCANSAR) {
        iniciarDescanso(mapa, inimigo);
        inimigo->acoes_basicas_restantes = 0;
        interfaceJogo->linha_destaque_acao = inimigo->linha;
        interfaceJogo->coluna_destaque_acao = inimigo->coluna;
        interfaceJogo->timer_destaque_acao = 1.0f;
        (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                       "AÇÃO INIMIGA: %s entrou em Descanso em (%d,%d).",
                       inimigo->nome, inimigo->linha, inimigo->coluna);
    } else if (escolha.tipo_acao == ACAO_IA_HABILIDADE) {
        Combatente *alvo = mapa->celulas[escolha.linha_alvo][escolha.coluna_alvo];
        char nomeAlvo[48] = "área";
        int danoTotal;
        int curaTotal;
        if (alvo != NULL) {
            (void)strncpy(nomeAlvo, alvo->nome, sizeof(nomeAlvo) - 1);
            nomeAlvo[sizeof(nomeAlvo) - 1] = '\0';
        }
        executarAtaqueOuHabilidade(mapa, inimigo, escolha.indice_habilidade,
                                   escolha.linha_alvo, escolha.coluna_alvo);
        danoTotal = somarUltimoDanoTabuleiro(mapa);
        curaTotal = somarUltimaCuraTabuleiro(mapa);
        inimigo->acoes_basicas_restantes--;
        interfaceJogo->linha_destaque_acao = escolha.linha_alvo;
        interfaceJogo->coluna_destaque_acao = escolha.coluna_alvo;
        interfaceJogo->timer_destaque_acao = 1.0f;
        if (danoTotal > 0 || curaTotal > 0) {
            interfaceJogo->timer_indicador_dano = 1.8f;
        }
        if (danoTotal > 0) {
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "AÇÃO INIMIGA: %s usou %s em %s (-%d PV)!",
                           inimigo->nome,
                           inimigo->habilidades[escolha.indice_habilidade].nome,
                           nomeAlvo, danoTotal);
        } else if (alvo != NULL &&
                   inimigo->habilidades[escolha.indice_habilidade].efeito ==
                   EFEITO_CURA_CONTINUA) {
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "AÇÃO INIMIGA: %s usou %s em %s (+10%% PV Máx/turno por 3t)!",
                           inimigo->nome,
                           inimigo->habilidades[escolha.indice_habilidade].nome,
                           nomeAlvo);
        } else {
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "AÇÃO INIMIGA: %s usou %s em (%d,%d)!",
                           inimigo->nome,
                           inimigo->habilidades[escolha.indice_habilidade].nome,
                           escolha.linha_alvo, escolha.coluna_alvo);
        }
    } else if (escolha.tipo_acao == ACAO_IA_MOVER) {
        int origLin = inimigo->linha;
        int origCol = inimigo->coluna;
        if (moverCombatente3x6(mapa, inimigo, escolha.linha_alvo,
                               escolha.coluna_alvo)) {
            int danoMina = somarUltimoDanoTabuleiro(mapa);
            inimigo->acoes_basicas_restantes--;
            interfaceJogo->linha_destaque_acao = escolha.linha_alvo;
            interfaceJogo->coluna_destaque_acao = escolha.coluna_alvo;
            interfaceJogo->timer_destaque_acao = 1.0f;
            if (danoMina > 0) {
                interfaceJogo->timer_indicador_dano = 1.8f;
            }
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "AÇÃO INIMIGA: %s moveu de (%d,%d) para (%d,%d).",
                           inimigo->nome, origLin, origCol,
                           escolha.linha_alvo, escolha.coluna_alvo);
        } else {
            if (!inimigo->eh_boss || inimigo->estamina_atual <= 0) {
                iniciarDescanso(mapa, inimigo);
            }
            inimigo->acoes_basicas_restantes = 0;
            interfaceJogo->linha_destaque_acao = inimigo->linha;
            interfaceJogo->coluna_destaque_acao = inimigo->coluna;
            interfaceJogo->timer_destaque_acao = 1.0f;
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "AÇÃO INIMIGA: %s encerrou o turno em (%d,%d).",
                           inimigo->nome, inimigo->linha, inimigo->coluna);
        }
    }
    atualizarVelocidadesFilaTurnos(interfaceJogo->fila_iniciativa,
                                   interfaceJogo->total_iniciativa,
                                   interfaceJogo->indice_iniciativa);
    if (verificarResultadoBatalha(interfaceJogo, mapa, piratas, totalPiratas)) {
        return;
    }
    interfaceJogo->timer_turno_inimigo = 1.0f;
}

static void gerarMinasInicioRodada(Tabuleiro3x6 *mapa)
{
    int livres[LINHAS_TABULEIRO * COLUNAS_TABULEIRO][2];
    int totalLivres = 0;
    int linha;
    int coluna;
    int quantidade = 0;
    if (mapa == NULL || !mapa->boss_fase_2_ativa) {
        return;
    }
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            if (mapa->celulas[linha][coluna] == NULL &&
                mapa->mina_ativa[linha][coluna] == 0) {
                livres[totalLivres][0] = linha;
                livres[totalLivres][1] = coluna;
                totalLivres++;
            }
        }
    }
    while (quantidade < 3 && totalLivres > 0) {
        int escolhido = GetRandomValue(0, totalLivres - 1);
        int celulaLinha = livres[escolhido][0];
        int celulaColuna = livres[escolhido][1];
        mapa->mina_ativa[celulaLinha][celulaColuna] = 1;
        totalLivres--;
        livres[escolhido][0] = livres[totalLivres][0];
        livres[escolhido][1] = livres[totalLivres][1];
        quantidade++;
    }
}

// Avança a fila de iniciativa e prepara o turno do próximo aliado ou inimigo.
static void avancarIniciativa(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                              Combatente piratas[], int totalPiratas)
{
    int tentativas = 0;
    interfaceJogo->ator_atual = NULL;
    while (tentativas < MAX_ENTRADAS_INICIATIVA * 4) {
        if (verificarResultadoBatalha(interfaceJogo, mapa, piratas, totalPiratas)) {
            return;
        }
        if (interfaceJogo->indice_iniciativa + 1 >= interfaceJogo->total_iniciativa) {
            interfaceJogo->total_iniciativa =
                montarFilaTurnos(mapa, interfaceJogo->fila_iniciativa);
            interfaceJogo->indice_iniciativa = -1;
            mapa->boss_atordoado_nesta_rodada = false;
            gerarMinasInicioRodada(mapa);
            if (interfaceJogo->total_iniciativa <= 0) {
                interfaceJogo->estado = TELA_FIM_DE_JOGO;
                return;
            }
        }
        if (!avancarProximoTurno(interfaceJogo->fila_iniciativa,
                                 interfaceJogo->total_iniciativa,
                                 &interfaceJogo->indice_iniciativa, mapa)) {
            interfaceJogo->indice_iniciativa = interfaceJogo->total_iniciativa;
            continue;
        }
        atualizarVelocidadesFilaTurnos(interfaceJogo->fila_iniciativa,
                                       interfaceJogo->total_iniciativa,
                                       interfaceJogo->indice_iniciativa);
        interfaceJogo->ator_atual =
            interfaceJogo->fila_iniciativa[interfaceJogo->indice_iniciativa].ator;
        if (interfaceJogo->ator_atual->aliado) {
            interfaceJogo->ator_atual->acoes_basicas_restantes =
                calcularAcoesPorTurno(interfaceJogo->ator_atual);
            interfaceJogo->entidade_selecionada = interfaceJogo->ator_atual;
            interfaceJogo->timer_turno_inimigo = 0.0f;
            return;
        }
        interfaceJogo->ator_atual->acoes_basicas_restantes =
            calcularAcoesPorTurno(interfaceJogo->ator_atual);
        interfaceJogo->entidade_selecionada = interfaceJogo->ator_atual;
        interfaceJogo->linha_origem_acao = interfaceJogo->ator_atual->linha;
        interfaceJogo->coluna_origem_acao = interfaceJogo->ator_atual->coluna;
        interfaceJogo->linha_destaque_acao = interfaceJogo->ator_atual->linha;
        interfaceJogo->coluna_destaque_acao = interfaceJogo->ator_atual->coluna;
        interfaceJogo->timer_destaque_acao = 1.0f;
        interfaceJogo->timer_turno_inimigo = 1.0f;
        (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                       "VEZ DO INIMIGO: %s preparando ação...",
                       interfaceJogo->ator_atual->nome);
        return;
    }
    if (interfaceJogo->estado == TELA_COMBATE_3X6) {
        TraceLog(LOG_WARNING, "A fila de iniciativa excedeu o limite de segurança.");
        interfaceJogo->estado = TELA_FIM_DE_JOGO;
    }
}

// Consome uma ação básica do pirata e avança o turno apenas quando as ações acabam.
static void concluirAcaoAliada(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                               Combatente piratas[], int totalPiratas)
{
    atualizarVelocidadesFilaTurnos(interfaceJogo->fila_iniciativa,
                                   interfaceJogo->total_iniciativa,
                                   interfaceJogo->indice_iniciativa);
    if (interfaceJogo->ator_atual != NULL &&
        verificarResultadoBatalha(interfaceJogo, mapa, piratas, totalPiratas)) {
        return;
    }
    if (interfaceJogo->ator_atual != NULL &&
        interfaceJogo->ator_atual->vivo &&
        !interfaceJogo->ator_atual->em_descanso &&
        interfaceJogo->ator_atual->acoes_basicas_restantes > 1) {
        interfaceJogo->ator_atual->acoes_basicas_restantes--;
        interfaceJogo->acao_combate = ACAO_TELA_NENHUMA;
        return;
    }
    interfaceJogo->acao_combate = ACAO_TELA_NENHUMA;
    avancarIniciativa(interfaceJogo, mapa, piratas, totalPiratas);
}

// Processa a desistência, reseta piratas mortos ao nível 1 e solicita reposição.
void processarFugaETrocaTripulacao(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                                   Combatente piratasDisponiveis[], int totalDisponiveis)
{
    int indice;
    int novosSelecionados = 0;
    bool houvePirataMorto = false;
    if (interfaceJogo == NULL || mapa == NULL || piratasDisponiveis == NULL ||
        interfaceJogo->raiz_rotas == NULL || interfaceJogo->rota_atual == NULL ||
        interfaceJogo->nivel_dificuldade == NULL) {
        return;
    }
    if (contadorFugas >= 2) {
        return;
    }
    *interfaceJogo->raiz_rotas =
        fugirDaAventura(*interfaceJogo->raiz_rotas, interfaceJogo->inventario,
                        interfaceJogo->nivel_dificuldade);
    *interfaceJogo->rota_atual = *interfaceJogo->raiz_rotas;
    interfaceJogo->total_sobreviventes_fuga = 0;
    for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
        int idPirata = interfaceJogo->piratas_selecionados_ids[indice];
        Combatente *pirata = pirataPorId(piratasDisponiveis, totalDisponiveis, idPirata);
        if (pirata != NULL && pirata->vivo) {
            limparEfeitosPosCombate(pirata);
            pirata->vida_atual = pirata->vida_maxima;
            pirata->estamina_atual = pirata->estamina_maxima;
            interfaceJogo->piratas_selecionados_ids[novosSelecionados] = idPirata;
            interfaceJogo->ids_sobreviventes_fuga[novosSelecionados] = idPirata;
            novosSelecionados++;
        } else {
            houvePirataMorto = true;
        }
    }
    interfaceJogo->total_selecionados = novosSelecionados;
    interfaceJogo->total_sobreviventes_fuga = novosSelecionados;
    for (indice = 0; indice < totalDisponiveis; indice++) {
        if (!piratasDisponiveis[indice].vivo) {
            houvePirataMorto = true;
            liberarListaItensCombatente(&piratasDisponiveis[indice].build_passivos);
            (void)criarPirataInicial(indice, &piratasDisponiveis[indice]);
        }
    }
    limparMapa(mapa);
    interfaceJogo->popup_ativo = POPUP_NENHUM;
    interfaceJogo->fase_pre_combate_ativa = false;
    interfaceJogo->saque_em_andamento = false;
    interfaceJogo->loja_aberta_pendente = false;
    interfaceJogo->total_itens_loja =
        sortearItensLoja(1, (unsigned int)(contadorFugas * 131 + contadorSaques * 37 + 17),
                         interfaceJogo->itens_loja, interfaceJogo->estoques_loja, 3);
    interfaceJogo->ator_atual = NULL;
    interfaceJogo->acao_combate = ACAO_TELA_NENHUMA;
    if (*interfaceJogo->raiz_rotas == NULL) {
        interfaceJogo->estado = TELA_FIM_DE_JOGO;
    } else if (houvePirataMorto || interfaceJogo->total_selecionados < 3) {
        interfaceJogo->estado = TELA_SELECAO_TRIPULACAO;
    } else {
        posicionarTripulacao(interfaceJogo, mapa, piratasDisponiveis, totalDisponiveis);
        interfaceJogo->estado = TELA_MAPA_ROTAS;
    }
}

// Reinicia completamente a expedição com mochila vazia para uma nova partida.
static void reiniciarPartidaCompleta(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                                     Combatente piratasDisponiveis[], int totalDisponiveis,
                                     NoRota **raizRotas, NoRota **rotaAtual,
                                     ListaItens *inventario, int *moedas,
                                     int *nivelDificuldade)
{
    int indice;
    limparMapa(mapa);
    for (indice = 0; indice < totalDisponiveis; indice++) {
        liberarListaItensCombatente(&piratasDisponiveis[indice].build_passivos);
        (void)criarPirataInicial(indice, &piratasDisponiveis[indice]);
    }
    if (raizRotas != NULL) {
        liberarEstruturasJogo(*raizRotas, inventario, NULL);
        *raizRotas = construirArvoreRotas(1);
        if (rotaAtual != NULL) {
            *rotaAtual = *raizRotas;
        }
    }
    contadorFugas = 0;
    contadorSaques = 0;
    inflacaoLojasPercentual = 0;
    if (moedas != NULL) {
        *moedas = 300;
    }
    if (nivelDificuldade != NULL) {
        *nivelDificuldade = 1;
    }
    interfaceJogo->total_selecionados = 0;
    interfaceJogo->total_sobreviventes_fuga = 0;
    interfaceJogo->entidade_selecionada = NULL;
    interfaceJogo->ator_atual = NULL;
    interfaceJogo->popup_ativo = POPUP_NENHUM;
    interfaceJogo->acao_combate = ACAO_TELA_NENHUMA;
    interfaceJogo->escolha_rota_pendente = 0;
    interfaceJogo->indice_compra_pendente = -1;
    interfaceJogo->item_id_selecionado = 0;
    interfaceJogo->fase_pre_combate_ativa = false;
    interfaceJogo->bgBossAtivo = false;
    interfaceJogo->bgCombateAtualIdx = -1;
    interfaceJogo->saque_em_andamento = false;
    interfaceJogo->loja_aberta_pendente = false;
    interfaceJogo->total_itens_loja =
        sortearItensLoja(1, (unsigned int)GetRandomValue(1, 100000),
                         interfaceJogo->itens_loja, interfaceJogo->estoques_loja, 3);
    interfaceJogo->estado = TELA_SELECAO_TRIPULACAO;
}

static Texture2D carregarTexturaOpcional(const char *caminho)
{
    Texture2D textura = { 0 };
    if (caminho != NULL && FileExists(caminho)) {
        textura = LoadTexture(caminho);
        if (textura.id == 0) {
            TraceLog(LOG_WARNING, "Nao foi possivel carregar a textura opcional %s.",
                     caminho);
        } else {
            SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
        }
    }
    return textura;
}

// Inicializa a janela em 1920x1080 (1080p), o framebuffer 1080p e a fonte TrueType de alta nitidez.
void inicializarInterface(InterfaceJogo *interfaceJogo)
{
    int codepoints[224];
    int indice;
    int fundosCarregados = 0;
    const char *diretorioWindows;
    char caminhoFonte[512];
    char caminhoTextura[128];
    if (interfaceJogo == NULL) {
        return;
    }
    (void)memset(interfaceJogo, 0, sizeof(*interfaceJogo));
    interfaceJogo->estado = TELA_MENU_PRINCIPAL;
    interfaceJogo->popup_ativo = POPUP_NENHUM;
    interfaceJogo->linha_selecionada = -1;
    interfaceJogo->coluna_selecionada = -1;
    interfaceJogo->indice_compra_pendente = -1;
    interfaceJogo->linha_destaque_acao = -1;
    interfaceJogo->coluna_destaque_acao = -1;
    interfaceJogo->linha_origem_acao = -1;
    interfaceJogo->bgCombateAtualIdx = -1;
    interfaceJogo->coluna_origem_acao = -1;
    interfaceJogo->largura_celula = 180;
    interfaceJogo->altura_celula = 135;
    interfaceJogo->margem_esquerda = 100;
    interfaceJogo->margem_superior = 155;
    interfaceJogo->pirata_posicionando_indice = -1;
    interfaceJogo->indice_iniciativa = -1;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_VIRTUAL, ALTURA_VIRTUAL,
               "Sea Of Gunpowder - Crônicas de Sangue e Pólvora");
    MaximizeWindow();
    SetExitKey(KEY_NULL);
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);
    canvasVirtual = LoadRenderTexture(LARGURA_RENDER_1080P, ALTURA_RENDER_1080P);
    if (canvasVirtual.id != 0) {
        SetTextureFilter(canvasVirtual.texture, TEXTURE_FILTER_BILINEAR);
        canvasCarregado = true;
    }
    if (FileExists("assets/textures/bg_mar.png")) {
        interfaceJogo->bgMar = LoadTexture("assets/textures/bg_mar.png");
        if (interfaceJogo->bgMar.id != 0) {
            SetTextureFilter(interfaceJogo->bgMar, TEXTURE_FILTER_BILINEAR);
        } else {
            TraceLog(LOG_ERROR, "Nao foi possivel carregar assets/textures/bg_mar.png.");
        }
    } else {
        TraceLog(LOG_ERROR, "Arquivo assets/textures/bg_mar.png nao encontrado.");
    }
    for (indice = 0; indice < 224; indice++) {
        codepoints[indice] = 32 + indice;
    }
    if (FileExists("assets/textures/bg_navio.png")) {
        interfaceJogo->bgNavio = LoadTexture("assets/textures/bg_navio.png");
        if (interfaceJogo->bgNavio.id != 0) {
            SetTextureFilter(interfaceJogo->bgNavio, TEXTURE_FILTER_BILINEAR);
        } else {
            TraceLog(LOG_ERROR, "Nao foi possivel carregar assets/textures/bg_navio.png.");
        }
    } else {
        TraceLog(LOG_ERROR, "Arquivo assets/textures/bg_navio.png nao encontrado.");
    }
    for (indice = 0; indice < 5; indice++) {
        (void)snprintf(caminhoTextura, sizeof(caminhoTextura),
                       "assets/textures/bg_combate_%d.png", indice + 1);
        interfaceJogo->bgCombatePool[fundosCarregados] =
            carregarTexturaOpcional(caminhoTextura);
        if (interfaceJogo->bgCombatePool[fundosCarregados].id != 0) {
            fundosCarregados++;
        }
    }
    if (fundosCarregados == 0 && interfaceJogo->bgNavio.id != 0) {
        interfaceJogo->bgCombatePool[0] = interfaceJogo->bgNavio;
        fundosCarregados = 1;
    }
    interfaceJogo->qtdBgCombate = fundosCarregados;
    interfaceJogo->bgBoss = carregarTexturaOpcional("assets/textures/bg_boss.png");
    interfaceJogo->texBtnPadrao =
        carregarTexturaOpcional("assets/textures/tex_btn_padrao.png");
    interfaceJogo->texFrameRetrato =
        carregarTexturaOpcional("assets/textures/tex_frame_retrato.png");
    interfaceJogo->texPainelDetalhes =
        carregarTexturaOpcional("assets/textures/tex_painel_detalhes.png");
    interfaceJogo->texPainelMochila =
        carregarTexturaOpcional("assets/textures/tex_painel_mochila.png");
    diretorioWindows = getenv("WINDIR");
    if (diretorioWindows == NULL || diretorioWindows[0] == '\0') {
        diretorioWindows = getenv("SystemRoot");
    }
    if (diretorioWindows != NULL && diretorioWindows[0] != '\0' &&
        snprintf(caminhoFonte, sizeof(caminhoFonte),
                 "%s\\Fonts\\segoeui.ttf", diretorioWindows) > 0 &&
        FileExists(caminhoFonte)) {
        interfaceJogo->fonteJogo = LoadFontEx(caminhoFonte, 128, codepoints, 224);
        if (interfaceJogo->fonteJogo.texture.id != 0) {
            SetTextureFilter(interfaceJogo->fonteJogo.texture, TEXTURE_FILTER_BILINEAR);
        } else {
            TraceLog(LOG_WARNING,
                     "Nao foi possivel carregar Segoe UI; usando a fonte padrao do Raylib.");
        }
    } else {
        TraceLog(LOG_WARNING,
                 "Segoe UI nao encontrada; usando a fonte padrao do Raylib.");
    }
}

// Processa menus, menu de pause (ESC), pop-ups, rotas e ações de combate.
void atualizarInterface(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                        Combatente piratasDisponiveis[], int totalDisponiveis,
                        NoRota **raizRotas, NoRota **rotaAtual,
                        ListaItens *inventario, int *moedas, int *nivelDificuldade)
{
    Vector2 mouse;
    float deltaTempo;
    float rodaMouse;
    int linha;
    int coluna;
    int indice;
    if (interfaceJogo == NULL || mapa == NULL) {
        return;
    }
    interfaceJogo->raiz_rotas = raizRotas;
    interfaceJogo->rota_atual = rotaAtual;
    interfaceJogo->inventario = inventario;
    interfaceJogo->moedas = moedas;
    interfaceJogo->nivel_dificuldade = nivelDificuldade;
    for (indice = 0; indice < totalDisponiveis; indice++) {
        int idPirata = piratasDisponiveis[indice].id;
        if (idPirata >= 1 && idPirata <= 5) {
            interfaceJogo->piratas_mortos[idPirata - 1] =
                !piratasDisponiveis[indice].vivo;
        }
    }
    if (interfaceJogo->estado != TELA_MENU_PRINCIPAL && IsKeyPressed(KEY_ESCAPE)) {
        if (interfaceJogo->popup_ativo == POPUP_PAUSE) {
            interfaceJogo->popup_ativo = POPUP_NENHUM;
        } else {
            interfaceJogo->popup_ativo = POPUP_PAUSE;
        }
        return;
    }
    deltaTempo = GetFrameTime();
    if (deltaTempo <= 0.0f || deltaTempo > 0.25f) {
        deltaTempo = 0.016f;
    }
    if (interfaceJogo->timer_destaque_acao > 0.0f) {
        interfaceJogo->timer_destaque_acao -= deltaTempo;
    }
    if (interfaceJogo->timer_indicador_dano > 0.0f) {
        interfaceJogo->timer_indicador_dano -= deltaTempo;
        if (interfaceJogo->timer_indicador_dano < 0.0f) {
            interfaceJogo->timer_indicador_dano = 0.0f;
        }
    }
    rodaMouse = GetMouseWheelMove();
    if (rodaMouse != 0.0f) {
        if (interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM) {
            interfaceJogo->scroll_detalhes -= (int)(rodaMouse * 28.0f);
            if (interfaceJogo->scroll_detalhes < 0) {
                interfaceJogo->scroll_detalhes = 0;
            } else if (interfaceJogo->scroll_detalhes > 420) {
                interfaceJogo->scroll_detalhes = 420;
            }
        } else if (interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS) {
            int maxScrollMochila = calcularMaxScrollMochila(interfaceJogo);
            interfaceJogo->scroll_mochila -= (int)(rodaMouse * 48.0f);
            if (interfaceJogo->scroll_mochila < 0) {
                interfaceJogo->scroll_mochila = 0;
            } else if (interfaceJogo->scroll_mochila > maxScrollMochila) {
                interfaceJogo->scroll_mochila = maxScrollMochila;
            }
        }
    }
    if (interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS) {
        int maxScrollMochila = calcularMaxScrollMochila(interfaceJogo);
        if (interfaceJogo->scroll_mochila > maxScrollMochila) {
            interfaceJogo->scroll_mochila = maxScrollMochila;
        }
        if (interfaceJogo->scroll_mochila < 0) {
            interfaceJogo->scroll_mochila = 0;
        }
    }
    if (interfaceJogo->estado == TELA_COMBATE_3X6 &&
        !interfaceJogo->fase_pre_combate_ativa &&
        interfaceJogo->popup_ativo == POPUP_NENHUM &&
        interfaceJogo->ator_atual != NULL &&
        !interfaceJogo->ator_atual->aliado) {
        interfaceJogo->timer_turno_inimigo -= deltaTempo;
        if (interfaceJogo->timer_turno_inimigo <= 0.0f) {
            executarPassoTurnoInimigo(interfaceJogo, mapa, piratasDisponiveis,
                                      totalDisponiveis);
        }
        return;
    }
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }
    mouse = obterMouseVirtual();
    if (interfaceJogo->popup_ativo != POPUP_NENHUM) {
        if (interfaceJogo->popup_ativo == POPUP_RECOMPENSAS) {
            if (retanguloClicado((Rectangle){ 530, 470, 220, 55 }, mouse)) {
                interfaceJogo->popup_ativo = POPUP_NENHUM;
            }
            return;
        }
        if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_ROTA) {
            if (retanguloClicado((Rectangle){ 390, 390, 180, 55 }, mouse)) {
                int escolha = interfaceJogo->escolha_rota_pendente;
                interfaceJogo->escolha_rota_pendente = 0;
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                if (rotaAtual == NULL || *rotaAtual == NULL) {
                    TraceLog(LOG_ERROR, "Não foi possível confirmar a rota selecionada.");
                    return;
                }
                *rotaAtual = avancarParaRota(*rotaAtual, escolha);
                if (escolha == 3 && (*rotaAtual)->tipo == ROTA_LOJA) {
                    unsigned int sementeLoja =
                        (unsigned int)((*rotaAtual)->etapa * 97 +
                                       contadorFugas * 31 + contadorSaques * 19 +
                                       (moedas != NULL ? *moedas : 0) +
                                       GetRandomValue(1, 10000));
                    interfaceJogo->total_itens_loja =
                        sortearItensLoja((*rotaAtual)->etapa, sementeLoja,
                                         interfaceJogo->itens_loja,
                                         interfaceJogo->estoques_loja, 3);
                    interfaceJogo->saque_em_andamento = false;
                    interfaceJogo->loja_aberta_pendente = true;
                    interfaceJogo->popup_ativo = POPUP_LOJA;
                } else {
                    interfaceJogo->saque_em_andamento = false;
                    iniciarEncontro(interfaceJogo, mapa, piratasDisponiveis,
                                    totalDisponiveis);
                }
                return;
            }
            if (retanguloClicado((Rectangle){ 610, 390, 180, 55 }, mouse)) {
                interfaceJogo->escolha_rota_pendente = 0;
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                return;
            }
            return;
        }
        if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_COMPRA) {
            if (retanguloClicado((Rectangle){ 390, 390, 180, 55 }, mouse)) {
                int indiceCompra = interfaceJogo->indice_compra_pendente;
                interfaceJogo->popup_ativo = POPUP_LOJA;
                interfaceJogo->indice_compra_pendente = -1;
                if (!comprarItemNaLoja(interfaceJogo->itens_loja,
                                       interfaceJogo->estoques_loja,
                                       interfaceJogo->total_itens_loja,
                                       indiceCompra, inventario, moedas)) {
                    TraceLog(LOG_WARNING, "Compra não realizada: saldo, estoque ou mochila inválidos.");
                }
                return;
            }
            if (retanguloClicado((Rectangle){ 610, 390, 180, 55 }, mouse)) {
                interfaceJogo->indice_compra_pendente = -1;
                interfaceJogo->popup_ativo = POPUP_LOJA;
                return;
            }
            return;
        }
        if (interfaceJogo->popup_ativo == POPUP_PAUSE) {
            if (retanguloClicado((Rectangle){ 490, 300, 300, 58 }, mouse)) {
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                return;
            }
            if (retanguloClicado((Rectangle){ 490, 380, 300, 58 }, mouse)) {
                reiniciarPartidaCompleta(interfaceJogo, mapa, piratasDisponiveis,
                                         totalDisponiveis, raizRotas, rotaAtual,
                                         inventario, moedas, nivelDificuldade);
                interfaceJogo->estado = TELA_MENU_PRINCIPAL;
                return;
            }
            return;
        }
        if (interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM) {
            Rectangle areaPopupDetalhes = { 160.0f, 50.0f, 740.0f, 555.0f };
            bool clicouBotaoAtivador =
                (interfaceJogo->estado == TELA_MAPA_ROTAS &&
                 retanguloClicado((Rectangle){ 510, 630, 190, 55 }, mouse)) ||
                (interfaceJogo->estado == TELA_COMBATE_3X6 &&
                 retanguloClicado((Rectangle){ 975, 630, 130, 55 }, mouse));
            if (clicouBotaoAtivador ||
                retanguloClicado((Rectangle){ 760, 68, 85, 38 }, mouse)) {
                interfaceJogo->popup_ativo = interfaceJogo->loja_aberta_pendente ?
                                             POPUP_LOJA : POPUP_NENHUM;
                return;
            }
            if (CheckCollisionPointRec(mouse, areaPopupDetalhes)) {
                return;
            }
            if (interfaceJogo->estado == TELA_SELECAO_TRIPULACAO) {
                int limite = totalDisponiveis < TOTAL_PIRATAS_PREDEFINIDOS ?
                             totalDisponiveis : TOTAL_PIRATAS_PREDEFINIDOS;
                for (indice = 0; indice < limite; indice++) {
                    int cardX = 60 + (indice % 3) * 390;
                    int cardY = 145 + (indice / 3) * 205;
                    if (retanguloClicado((Rectangle){ (float)(cardX + 230),
                                                      (float)(cardY + 138),
                                                      108.0f, 32.0f }, mouse)) {
                        if (interfaceJogo->entidade_selecionada ==
                            &piratasDisponiveis[indice]) {
                            interfaceJogo->popup_ativo = POPUP_NENHUM;
                        } else {
                            interfaceJogo->entidade_selecionada =
                                &piratasDisponiveis[indice];
                            interfaceJogo->scroll_detalhes = 0;
                        }
                        return;
                    }
                }
            }
            interfaceJogo->popup_ativo = interfaceJogo->loja_aberta_pendente ?
                                         POPUP_LOJA : POPUP_NENHUM;
            if (interfaceJogo->popup_ativo != POPUP_NENHUM) {
                return;
            }
        } else if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_FUGA) {
            Rectangle areaPopupFuga = { 210.0f, 200.0f, 620.0f, 295.0f };
            if (retanguloClicado((Rectangle){ 300, 630, 190, 55 }, mouse) ||
                retanguloClicado((Rectangle){ 530, 420, 150, 50 }, mouse)) {
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                return;
            }
            if (interfaceJogo->estado == TELA_MAPA_ROTAS &&
                contadorFugas < 2 &&
                retanguloClicado((Rectangle){ 350, 420, 150, 50 }, mouse)) {
                processarFugaETrocaTripulacao(interfaceJogo, mapa,
                                              piratasDisponiveis, totalDisponiveis);
                return;
            }
            if (CheckCollisionPointRec(mouse, areaPopupFuga)) {
                return;
            }
            if (interfaceJogo->estado == TELA_MAPA_ROTAS &&
                (retanguloClicado((Rectangle){ 90, 630, 190, 55 }, mouse) ||
                 retanguloClicado((Rectangle){ 510, 630, 190, 55 }, mouse))) {
                interfaceJogo->popup_ativo = POPUP_NENHUM;
            } else {
                return;
            }
        } else if (interfaceJogo->popup_ativo == POPUP_LOJA) {
            if (retanguloClicado((Rectangle){ 740, 72, 105, 38 }, mouse)) {
                interfaceJogo->loja_aberta_pendente = false;
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                return;
            }
            for (indice = 0; indice < interfaceJogo->total_itens_loja; indice++) {
                Rectangle botaoCompra = { 190.0f, 175.0f + (float)indice * 92.0f,
                                          640.0f, 78.0f };
                if (retanguloClicado(botaoCompra, mouse)) {
                    interfaceJogo->indice_compra_pendente = indice;
                    interfaceJogo->popup_ativo = POPUP_CONFIRMAR_COMPRA;
                    return;
                }
            }
            if (retanguloClicado((Rectangle){ 185, 495, 200, 55 }, mouse) ||
                retanguloClicado((Rectangle){ 90, 630, 190, 55 }, mouse)) {
                (void)garantirPirataSelecionadoMochila(interfaceJogo, piratasDisponiveis,
                                                       totalDisponiveis);
                interfaceJogo->loja_aberta_pendente = true;
                interfaceJogo->scroll_mochila = 0;
                interfaceJogo->popup_ativo = POPUP_MOCHILA_BUILDS;
                return;
            }
            if (retanguloClicado((Rectangle){ 510, 630, 190, 55 }, mouse)) {
                (void)garantirPirataSelecionadoMochila(interfaceJogo, piratasDisponiveis,
                                                       totalDisponiveis);
                if (interfaceJogo->entidade_selecionada != NULL) {
                    interfaceJogo->loja_aberta_pendente = true;
                    interfaceJogo->scroll_detalhes = 0;
                    interfaceJogo->popup_ativo = POPUP_DETALHES_PERSONAGEM;
                }
                return;
            }
            if (retanguloClicado((Rectangle){ 400, 495, 215, 55 }, mouse)) {
                interfaceJogo->loja_aberta_pendente = false;
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                interfaceJogo->saque_em_andamento = true;
                iniciarEncontro(interfaceJogo, mapa, piratasDisponiveis, totalDisponiveis);
                return;
            }
            if (retanguloClicado((Rectangle){ 630, 495, 195, 55 }, mouse)) {
                interfaceJogo->loja_aberta_pendente = false;
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                return;
            }
            return;
        } else if (interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS) {
            Rectangle areaPopupMochila = { 160.0f, 50.0f, 700.0f, 555.0f };
            bool clicouBotaoMochila =
                (interfaceJogo->estado == TELA_MAPA_ROTAS &&
                 retanguloClicado((Rectangle){ 90, 630, 190, 55 }, mouse)) ||
                (interfaceJogo->estado == TELA_COMBATE_3X6 &&
                 interfaceJogo->fase_pre_combate_ativa &&
                 retanguloClicado((Rectangle){ 370, 630, 190, 55 }, mouse)) ||
                (interfaceJogo->estado == TELA_COMBATE_3X6 &&
                 !interfaceJogo->fase_pre_combate_ativa &&
                 retanguloClicado((Rectangle){ 815, 630, 150, 55 }, mouse));
            if (clicouBotaoMochila ||
                retanguloClicado((Rectangle){ 740, 68, 105, 38 }, mouse)) {
                interfaceJogo->item_id_selecionado = 0;
                interfaceJogo->popup_ativo = interfaceJogo->loja_aberta_pendente ?
                                             POPUP_LOJA : POPUP_NENHUM;
                return;
            }
            for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
                Combatente *pirataAba =
                    pirataPorId(piratasDisponiveis, totalDisponiveis,
                                interfaceJogo->piratas_selecionados_ids[indice]);
                Rectangle abaPirata = { 180.0f + (float)indice * 220.0f, 112.0f,
                                        205.0f, 36.0f };
                if (pirataAba != NULL && pirataAba->vivo &&
                    retanguloClicado(abaPirata, mouse)) {
                    interfaceJogo->entidade_selecionada = pirataAba;
                    return;
                }
            }
            if (interfaceJogo->inventario != NULL) {
                NoItem *item;
                int itemIndice = 0;
                Combatente *alvoUso =
                    garantirPirataSelecionadoMochila(interfaceJogo, piratasDisponiveis,
                                                     totalDisponiveis);
                if (retanguloClicado((Rectangle){ 690, 372, 135, 32 }, mouse)) {
                    (void)usarConsumivelSelecionado(interfaceJogo, mapa,
                                                    piratasDisponiveis,
                                                    totalDisponiveis, inventario,
                                                    alvoUso);
                    return;
                }
                for (item = inventario->inicio; item != NULL; item = item->proximo) {
                    Rectangle linhaItem;
                    float posYItem;
                    if (interfaceJogo->estado == TELA_COMBATE_3X6 &&
                        !interfaceJogo->fase_pre_combate_ativa &&
                        item->item.tipo != ITEM_CONSUMIVEL) {
                        continue;
                    }
                    posYItem = 178.0f + (float)itemIndice * 48.0f -
                               (float)interfaceJogo->scroll_mochila;
                    linhaItem = (Rectangle){ 180.0f, posYItem, 650.0f, 44.0f };
                    if (posYItem >= 172.0f && posYItem <= 325.0f &&
                        retanguloClicado(linhaItem, mouse)) {
                        if (item->item.tipo == ITEM_CONSUMIVEL) {
                            interfaceJogo->item_id_selecionado = item->item.id;
                        } else if ((interfaceJogo->estado == TELA_MAPA_ROTAS ||
                                    interfaceJogo->fase_pre_combate_ativa) &&
                                   item->item.tipo == ITEM_PASSIVO &&
                                   alvoUso != NULL &&
                                   alvoUso->aliado &&
                                   alvoUso->vivo) {
                            int maxScrollAtual;
                            interfaceJogo->item_id_selecionado = 0;
                            (void)aplicarItemPassivoNoPirata(
                                inventario, alvoUso, item->item.id);
                            maxScrollAtual = calcularMaxScrollMochila(interfaceJogo);
                            if (interfaceJogo->scroll_mochila > maxScrollAtual) {
                                interfaceJogo->scroll_mochila = maxScrollAtual;
                            }
                        }
                        return;
                    }
                    itemIndice++;
                }
                if (alvoUso != NULL && alvoUso->aliado) {
                    itemIndice = 0;
                    for (item = alvoUso->build_passivos.inicio;
                         item != NULL; item = item->proximo) {
                        Rectangle linhaItem = { 180.0f, 406.0f + (float)itemIndice * 36.0f,
                                                650.0f, 33.0f };
                        if (retanguloClicado(linhaItem, mouse) &&
                            (interfaceJogo->estado == TELA_MAPA_ROTAS ||
                             interfaceJogo->fase_pre_combate_ativa)) {
                            (void)desequiparItemPassivoBuild(
                                inventario, alvoUso, item->item.id);
                            return;
                        }
                        itemIndice++;
                    }
                }
            }
            if (CheckCollisionPointRec(mouse, areaPopupMochila)) {
                return;
            }
            if (!interfaceJogo->loja_aberta_pendente &&
                mouse.y >= 630.0f && mouse.y <= 685.0f) {
                interfaceJogo->popup_ativo = POPUP_NENHUM;
            } else {
                return;
            }
        } else {
            return;
        }
    }
    if (interfaceJogo->estado == TELA_MENU_PRINCIPAL) {
        if (retanguloClicado((Rectangle){ 470, 350, 340, 70 }, mouse)) {
            interfaceJogo->estado = TELA_SELECAO_TRIPULACAO;
        } else if (retanguloClicado((Rectangle){ 470, 445, 340, 60 }, mouse)) {
            interfaceJogo->sair_solicitado = true;
        }
        return;
    }
    if (interfaceJogo->estado == TELA_VITORIA ||
        interfaceJogo->estado == TELA_FIM_DE_JOGO) {
        if (retanguloClicado((Rectangle){ 470, 440, 340, 65 }, mouse)) {
            reiniciarPartidaCompleta(interfaceJogo, mapa, piratasDisponiveis,
                                     totalDisponiveis, raizRotas, rotaAtual,
                                     inventario, moedas, nivelDificuldade);
        }
        return;
    }
    if (interfaceJogo->estado == TELA_SELECAO_TRIPULACAO) {
        int limite = totalDisponiveis < TOTAL_PIRATAS_PREDEFINIDOS ?
                     totalDisponiveis : TOTAL_PIRATAS_PREDEFINIDOS;
        for (indice = 0; indice < limite; indice++) {
            int cardX = 60 + (indice % 3) * 390;
            int cardY = 145 + (indice / 3) * 205;
            if (retanguloClicado((Rectangle){ (float)(cardX + 230),
                                              (float)(cardY + 138),
                                              108.0f, 32.0f }, mouse)) {
                interfaceJogo->entidade_selecionada = &piratasDisponiveis[indice];
                interfaceJogo->scroll_detalhes = 0;
                interfaceJogo->popup_ativo = POPUP_DETALHES_PERSONAGEM;
                return;
            }
            if (retanguloClicado((Rectangle){ (float)cardX, (float)cardY,
                                              350.0f, 180.0f }, mouse)) {
                int id = piratasDisponiveis[indice].id;
                int slot = indicePirataSelecionado(interfaceJogo, id);
                if (ehSobreviventeFixoFuga(interfaceJogo, id)) {
                    return;
                }
                if (slot >= 0) {
                    for (; slot < interfaceJogo->total_selecionados - 1; slot++) {
                        interfaceJogo->piratas_selecionados_ids[slot] =
                            interfaceJogo->piratas_selecionados_ids[slot + 1];
                    }
                    interfaceJogo->total_selecionados--;
                } else if (interfaceJogo->total_selecionados < 3) {
                    interfaceJogo->piratas_selecionados_ids[
                        interfaceJogo->total_selecionados++] = id;
                }
                return;
            }
        }
        if (interfaceJogo->total_selecionados == 3 &&
            retanguloClicado((Rectangle){ 440, 590, 400, 60 }, mouse) &&
            rotaAtual != NULL && *rotaAtual != NULL) {
            limparMapa(mapa);
            for (indice = 0; indice < totalDisponiveis; indice++) {
                piratasDisponiveis[indice].aliado =
                    indicePirataSelecionado(interfaceJogo,
                                            piratasDisponiveis[indice].id) >= 0;
            }
            posicionarTripulacao(interfaceJogo, mapa, piratasDisponiveis,
                                 totalDisponiveis);
            interfaceJogo->estado = TELA_MAPA_ROTAS;
        }
        return;
    }
    if (interfaceJogo->estado == TELA_MAPA_ROTAS) {
        if (retanguloClicado((Rectangle){ 90, 630, 190, 55 }, mouse)) {
            (void)garantirPirataSelecionadoMochila(interfaceJogo, piratasDisponiveis,
                                                   totalDisponiveis);
            interfaceJogo->scroll_mochila = 0;
            interfaceJogo->popup_ativo = POPUP_MOCHILA_BUILDS;
            return;
        }
        if (retanguloClicado((Rectangle){ 300, 630, 190, 55 }, mouse)) {
            interfaceJogo->popup_ativo = POPUP_CONFIRMAR_FUGA;
            return;
        }
        if (retanguloClicado((Rectangle){ 510, 630, 190, 55 }, mouse)) {
            if (interfaceJogo->entidade_selecionada == NULL &&
                interfaceJogo->total_selecionados > 0) {
                interfaceJogo->entidade_selecionada =
                    pirataPorId(piratasDisponiveis, totalDisponiveis,
                                interfaceJogo->piratas_selecionados_ids[0]);
            }
            if (interfaceJogo->entidade_selecionada != NULL) {
                interfaceJogo->scroll_detalhes = 0;
                interfaceJogo->popup_ativo = POPUP_DETALHES_PERSONAGEM;
            }
            return;
        }
        if (rotaAtual != NULL && *rotaAtual != NULL && (*rotaAtual)->etapa >= 3) {
            if (retanguloClicado((Rectangle){ 480, 340, 380, 100 }, mouse) &&
                (*rotaAtual)->caminho1 != NULL) {
                interfaceJogo->escolha_rota_pendente = 1;
                interfaceJogo->popup_ativo = POPUP_CONFIRMAR_ROTA;
                return;
            }
        } else {
            if (retanguloClicado((Rectangle){ 440, 290, 260, 95 }, mouse) &&
                rotaAtual != NULL && *rotaAtual != NULL &&
                (*rotaAtual)->caminho1 != NULL) {
                interfaceJogo->escolha_rota_pendente = 1;
                interfaceJogo->popup_ativo = POPUP_CONFIRMAR_ROTA;
                return;
            }
            if (retanguloClicado((Rectangle){ 730, 290, 260, 95 }, mouse) &&
                rotaAtual != NULL && *rotaAtual != NULL &&
                (*rotaAtual)->caminho2 != NULL) {
                interfaceJogo->escolha_rota_pendente = 2;
                interfaceJogo->popup_ativo = POPUP_CONFIRMAR_ROTA;
                return;
            }
            if (retanguloClicado((Rectangle){ 585, 415, 260, 95 }, mouse) &&
                rotaAtual != NULL && *rotaAtual != NULL &&
                (*rotaAtual)->loja != NULL) {
                interfaceJogo->escolha_rota_pendente = 3;
                interfaceJogo->popup_ativo = POPUP_CONFIRMAR_ROTA;
                return;
            }
        }
        for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
            Combatente *pirata = pirataPorId(piratasDisponiveis, totalDisponiveis,
                                             interfaceJogo->piratas_selecionados_ids[indice]);
            Rectangle card = { 55.0f, 200.0f + (float)indice * 100.0f,
                               340.0f, 82.0f };
            if (pirata != NULL && pirata->vivo && retanguloClicado(card, mouse)) {
                interfaceJogo->entidade_selecionada = pirata;
                return;
            }
        }
        return;
    }
    if (interfaceJogo->estado != TELA_COMBATE_3X6) {
        return;
    }
    {
        Combatente *atorFila = obterAtorClicadoNaFila(interfaceJogo, mouse);
        if (atorFila != NULL) {
            interfaceJogo->entidade_selecionada = atorFila;
            return;
        }
    }
    if (interfaceJogo->fase_pre_combate_ativa) {
        if (retanguloClicado((Rectangle){ 90, 630, 260, 55 }, mouse)) {
            interfaceJogo->fase_pre_combate_ativa = false;
            interfaceJogo->pirata_posicionando_indice = -1;
            interfaceJogo->total_iniciativa =
                montarFilaTurnos(mapa, interfaceJogo->fila_iniciativa);
            interfaceJogo->indice_iniciativa = -1;
            avancarIniciativa(interfaceJogo, mapa, piratasDisponiveis, totalDisponiveis);
            return;
        }
        if (retanguloClicado((Rectangle){ 370, 630, 190, 55 }, mouse)) {
            (void)garantirPirataSelecionadoMochila(interfaceJogo, piratasDisponiveis,
                                                   totalDisponiveis);
            interfaceJogo->scroll_mochila = 0;
            interfaceJogo->popup_ativo = POPUP_MOCHILA_BUILDS;
            return;
        }
        if (retanguloClicado((Rectangle){ 975, 630, 130, 55 }, mouse)) {
            if (interfaceJogo->entidade_selecionada == NULL) {
                (void)garantirPirataSelecionadoMochila(interfaceJogo, piratasDisponiveis,
                                                       totalDisponiveis);
            }
            if (interfaceJogo->entidade_selecionada != NULL) {
                interfaceJogo->scroll_detalhes = 0;
                interfaceJogo->popup_ativo = POPUP_DETALHES_PERSONAGEM;
            }
            return;
        }
        if (obterCelulaClicada(interfaceJogo, mouse, &linha, &coluna)) {
            Combatente *clicado = mapa->celulas[linha][coluna];
            if (coluna <= 1) {
                if (interfaceJogo->pirata_posicionando_indice >= 0) {
                    Combatente *pirataMovendo =
                        pirataPorId(piratasDisponiveis, totalDisponiveis,
                                    interfaceJogo->pirata_posicionando_indice);
                    if (pirataMovendo != NULL &&
                        posicionarPirataPreBatalha(mapa, pirataMovendo, linha, coluna)) {
                        interfaceJogo->entidade_selecionada = pirataMovendo;
                        interfaceJogo->pirata_posicionando_indice = -1;
                        return;
                    }
                }
                if (clicado != NULL && clicado->aliado && clicado->vivo) {
                    interfaceJogo->pirata_posicionando_indice = clicado->id;
                    interfaceJogo->entidade_selecionada = clicado;
                }
            } else {
                interfaceJogo->pirata_posicionando_indice = -1;
                if (clicado != NULL) {
                    interfaceJogo->entidade_selecionada = clicado;
                }
            }
        }
        return;
    }
    {
        int habDesbloq = interfaceJogo->ator_atual != NULL ?
                         interfaceJogo->ator_atual->habilidades_desbloqueadas : 1;
        if (retanguloClicado((Rectangle){ 90, 630, 130, 55 }, mouse)) {
            interfaceJogo->acao_combate =
                interfaceJogo->acao_combate == ACAO_TELA_MOVER ?
                ACAO_TELA_NENHUMA : ACAO_TELA_MOVER;
        } else if (retanguloClicado((Rectangle){ 235, 630, 130, 55 }, mouse)) {
            if (interfaceJogo->ator_atual != NULL) {
                (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                               "%s entrou em Descanso.", interfaceJogo->ator_atual->nome);
            }
            iniciarDescanso(mapa, interfaceJogo->ator_atual);
            concluirAcaoAliada(interfaceJogo, mapa, piratasDisponiveis, totalDisponiveis);
        } else if (retanguloClicado((Rectangle){ 380, 630, 130, 55 }, mouse)) {
            interfaceJogo->acao_combate =
                interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_0 ?
                ACAO_TELA_NENHUMA : ACAO_TELA_HABILIDADE_0;
        } else if (retanguloClicado((Rectangle){ 525, 630, 130, 55 }, mouse)) {
            if (habDesbloq >= 2) {
                interfaceJogo->acao_combate =
                    interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_1 ?
                    ACAO_TELA_NENHUMA : ACAO_TELA_HABILIDADE_1;
            } else {
                interfaceJogo->acao_combate = ACAO_TELA_NENHUMA;
                (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                               "Ataque 2 bloqueado (requer Nível 3).");
            }
        } else if (retanguloClicado((Rectangle){ 670, 630, 130, 55 }, mouse)) {
            if (habDesbloq >= 3) {
                interfaceJogo->acao_combate =
                    interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_2 ?
                    ACAO_TELA_NENHUMA : ACAO_TELA_HABILIDADE_2;
            } else {
                interfaceJogo->acao_combate = ACAO_TELA_NENHUMA;
                (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                               "Ultimate bloqueada (requer Nível 6).");
            }
        } else if (retanguloClicado((Rectangle){ 815, 630, 150, 55 }, mouse)) {
            (void)garantirPirataSelecionadoMochila(interfaceJogo, piratasDisponiveis,
                                                   totalDisponiveis);
            interfaceJogo->scroll_mochila = 0;
            interfaceJogo->popup_ativo = POPUP_MOCHILA_BUILDS;
        } else if (retanguloClicado((Rectangle){ 975, 630, 130, 55 }, mouse)) {
            if (interfaceJogo->entidade_selecionada == NULL) {
                (void)garantirPirataSelecionadoMochila(interfaceJogo, piratasDisponiveis,
                                                       totalDisponiveis);
            }
            if (interfaceJogo->entidade_selecionada != NULL) {
                interfaceJogo->scroll_detalhes = 0;
                interfaceJogo->popup_ativo = POPUP_DETALHES_PERSONAGEM;
            }
        } else if (obterCelulaClicada(interfaceJogo, mouse, &linha, &coluna)) {
            if (interfaceJogo->acao_combate == ACAO_TELA_MOVER &&
                interfaceJogo->ator_atual != NULL) {
                if (moverCombatente3x6(mapa, interfaceJogo->ator_atual, linha, coluna)) {
                    int danoMina = somarUltimoDanoTabuleiro(mapa);
                    if (danoMina > 0) {
                        interfaceJogo->timer_indicador_dano = 1.8f;
                    }
                    (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                                   "%s moveu para (%d,%d).",
                                   interfaceJogo->ator_atual->nome, linha, coluna);
                    concluirAcaoAliada(interfaceJogo, mapa, piratasDisponiveis,
                                       totalDisponiveis);
                } else if (mapa->celulas[linha][coluna] != NULL) {
                    interfaceJogo->entidade_selecionada = mapa->celulas[linha][coluna];
                    interfaceJogo->acao_combate = ACAO_TELA_NENHUMA;
                }
            } else if (interfaceJogo->acao_combate >= ACAO_TELA_HABILIDADE_0 &&
                       interfaceJogo->acao_combate <= ACAO_TELA_HABILIDADE_2 &&
                       interfaceJogo->ator_atual != NULL) {
                int estaminaAntes = interfaceJogo->ator_atual->estamina_atual;
                int habilidade = interfaceJogo->acao_combate - ACAO_TELA_HABILIDADE_0;
                executarAtaqueOuHabilidade(mapa, interfaceJogo->ator_atual, habilidade,
                                           linha, coluna);
                if (interfaceJogo->ator_atual->estamina_atual < estaminaAntes) {
                    int danoTotal = somarUltimoDanoTabuleiro(mapa);
                    int curaTotal = somarUltimaCuraTabuleiro(mapa);
                    if (danoTotal > 0 || curaTotal > 0) {
                        interfaceJogo->timer_indicador_dano = 1.8f;
                    }
                    if (danoTotal > 0) {
                        (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                                       "%s usou %s em (%d,%d) causando %d de dano!",
                                       interfaceJogo->ator_atual->nome,
                                       interfaceJogo->ator_atual->habilidades[habilidade].nome,
                                       linha, coluna, danoTotal);
                    } else if (curaTotal > 0) {
                        (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                                       "%s usou %s curando %d PV!",
                                       interfaceJogo->ator_atual->nome,
                                       interfaceJogo->ator_atual->habilidades[habilidade].nome,
                                       curaTotal);
                    } else {
                        (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                                       "%s usou %s em (%d,%d)!",
                                       interfaceJogo->ator_atual->nome,
                                       interfaceJogo->ator_atual->habilidades[habilidade].nome,
                                       linha, coluna);
                    }
                    concluirAcaoAliada(interfaceJogo, mapa, piratasDisponiveis,
                                       totalDisponiveis);
                } else if (mapa->celulas[linha][coluna] != NULL) {
                    interfaceJogo->entidade_selecionada = mapa->celulas[linha][coluna];
                }
            } else {
                interfaceJogo->linha_selecionada = linha;
                interfaceJogo->coluna_selecionada = coluna;
                if (mapa->celulas[linha][coluna] != NULL) {
                    interfaceJogo->entidade_selecionada = mapa->celulas[linha][coluna];
                }
            }
        }
    }
}

// Retorna o nome do pirata pelo seu ID fixo de classe (1 a 5).
static const char *nomePirataPorId(int idPirata)
{
    static const char *nomes[5] = {
        "Bucaneiro", "Timoneiro", "Cozinheiro", "Capitão", "Gávea"
    };
    if (idPirata >= 1 && idPirata <= 5) {
        return nomes[idPirata - 1];
    }
    return "Pirata";
}

// Desenha selos visuais coloridos de buffs e debuffs ativos em uma unidade.
static void desenharIndicadoresStatusUnidade(const Combatente *unidade, int xBase, int yBase,
                                             int larguraMax)
{
    int posX = xBase;
    int posY = yBase;
    const char *textos[8];
    Color cores[8];
    int total = 0;
    int indice;
    if (unidade == NULL || !unidade->vivo) {
        return;
    }
    if ((unidade->turnos_motivacao > 0 || unidade->motivado_capitao) && total < 8) {
        int turnosMot = unidade->turnos_motivacao > 0 ? unidade->turnos_motivacao : 1;
        textos[total] = TextFormat("+MOT %dt", turnosMot);
        cores[total++] = (Color){ 45, 175, 95, 255 };
    }
    if (unidade->turnos_tonico > 0 && total < 8) {
        textos[total] = TextFormat("+ATK %dt", unidade->turnos_tonico);
        cores[total++] = (Color){ 55, 150, 235, 255 };
    }
    if (unidade->turnos_cura_hot > 0 && total < 8) {
        textos[total] = TextFormat("+HOT %dt", unidade->turnos_cura_hot);
        cores[total++] = (Color){ 45, 195, 125, 255 };
    }
    if (unidade->turnos_sangramento > 0 && total < 8) {
        textos[total] = TextFormat("SANGR %dt", unidade->turnos_sangramento);
        cores[total++] = (Color){ 210, 45, 45, 255 };
    } else if (unidade->golpes_corte_bucaneiro_recebidos > 0 && total < 8) {
        textos[total] = TextFormat("CORTE %d/2", unidade->golpes_corte_bucaneiro_recebidos);
        cores[total++] = (Color){ 195, 95, 45, 255 };
    } else if (unidade->golpes_corte_grunt_recebidos > 0 && total < 8) {
        textos[total] = TextFormat("CORTE %d/3", unidade->golpes_corte_grunt_recebidos);
        cores[total++] = (Color){ 195, 95, 45, 255 };
    }
    if (unidade->turnos_veneno > 0 && total < 8) {
        textos[total] = TextFormat("VEN %dt", unidade->turnos_veneno);
        cores[total++] = (Color){ 155, 65, 215, 255 };
    }
    if (unidade->turnos_atordoado > 0 && total < 8) {
        textos[total] = TextFormat("STUN %dt", unidade->turnos_atordoado);
        cores[total++] = (Color){ 225, 145, 30, 255 };
    }
    if (unidade->turnos_desnorteado > 0 && total < 8) {
        textos[total] = TextFormat("-VEL %dt", unidade->turnos_desnorteado);
        cores[total++] = (Color){ 205, 115, 40, 255 };
    }
    if (unidade->turnos_desmoralizado > 0 && total < 8) {
        textos[total] = TextFormat("-DESM %dt", unidade->turnos_desmoralizado);
        cores[total++] = (Color){ 185, 70, 90, 255 };
    }
    for (indice = 0; indice < total; indice++) {
        int largBadge = 66;
        if (posX + largBadge > xBase + larguraMax) {
            posX = xBase;
            posY += 20;
        }
        DrawRectangle(posX, posY, largBadge, 17, cores[indice]);
        DrawRectangleLines(posX, posY, largBadge, 17, RAYWHITE);
        desenharTexto(textos[indice], posX + 4, posY + 2, 11, RAYWHITE);
        posX += largBadge + 4;
    }
}

// Retorna um resumo temático e explicativo da função tática do personagem ou inimigo.
static const char *descricaoResumoPersonagem(const Combatente *unidade)
{
    if (unidade == NULL) {
        return "";
    }
    if (unidade->aliado) {
        switch (unidade->classe) {
        case CLASSE_BUCANEIRO:
            return "Duelista implacável das abordagens piratas. Com passos ágeis pelo convés, anda até 2 casas por movimento, cega fileiras inteiras com areia e retalha rivais de perto para drenar sua vitalidade.";
        case CLASSE_TIMONEIRO:
            return "Colosso do leme e muralha viva da embarcação. Suporta pesados bombardeios na linha de frente, esmaga adversários à queima-roupa para atordoá-los e gira o timão de carvalho para varrer quem se aproximar.";
        case CLASSE_COZINHEIRO:
            return "Mestre-cuca do porão e guardião dos mantimentos. Suas refeições restauram a tripulação com cura garantida (sem depender de precisão), enquanto seu bacamarte carregado de chumbo varre uma linha de 2 quadrados causando mais dano de perto.";
        case CLASSE_CAPITAO:
            return "Comandante supremo da nau pirata. Alterna golpes de sabre e tiros de garrucha em combate híbrido, inspira os marujos com discursos de abordagem (+40% precisão, +2 velocidade e +1 estamina máx por 3 turnos) e ordena baterias de canhões.";
        case CLASSE_GAVEA:
            return "Vigia de elite postado no cesto da gávea. Especialista em rifle de pederneira e zarabatana tática, elimina alvos alinhados à distância e torna seus disparos ainda mais certeiros e letais de perto.";
        default:
            return "Pirata veterano da tripulação.";
        }
    }
    if (unidade->eh_boss) {
        return "O lendário Terror dos Sete Mares! Comanda o convés com 3 turnos completos por rodada, alternando golpes de sabre, tiros de pistola e bombardeios navais devastadores.";
    }
    switch (unidade->tipo_inimigo) {
    case INIMIGO_GRUNT_ESPADA:
        return "Marujo corsário de linha de frente armado com cutelo serrilhado. Avança sem hesitar para cercar piratas feridos e abrir cortes profundos que causam Sangramento (-15% da Vida Máxima por 2 turnos) ao atingir 3 golpes no mesmo alvo.";
    case INIMIGO_GRUNT_PISTOLA:
        return "Atirador de convés armado com pistola pederneira de alcance de 2 quadrados e granadas de pólvora negra capazes de explodir formações em cruz.";
    case INIMIGO_ATIRADOR_MAGRELO:
        return "Franco-atirador esguio postado na retaguarda. Recua pelo convés para manter distância segura e disparar seu mosquete pesado a até 3 quadrados em linha reta.";
    default:
        return "Brutamontes encouraçado do navio rival. Dispara sua escopeta numa linha de 2 quadrados (causando mais dano de perto, igual ao Bacamarte) e aplica bandagens que curam 10% da Vida Máxima por 3 turnos.";
    }
}

// Retorna o texto detalhado e temático da passiva de classe (Nv 1 ou Nv 4).
static const char *descricaoPassiva(ClassePirata classe, bool aprimorada)
{
    static const char *passivas[5][2] = {
        { "Passiva Nv1 (Passos de Abordagem): Suas pernas acostumadas ao balanço do mar permitem deslocar-se até 2 quadrados ortogonais com uma única ação de Mover (custo: 1 Estamina).",
          "Passiva Nv4 (Frenesi de Abordagem): Seu domínio de combate concede +1 ação básica extra em todos os seus turnos (passa a ter 3 ações por turno, acumulando com Rum de Poseidon!)." },
        { "Passiva Nv1 (Fúria do Náufrago): Quando sua Vida cai abaixo de 60%, a fúria da batalha aumenta todo o dano causado pelos seus ataques em +25%.",
          "Passiva Nv4 (Escudo de Carvalho): Protege os companheiros ao redor; aliados posicionados nas 4 casas adjacentes em cruz recebem -40% de dano em qualquer ataque sofrido." },
        { "Passiva Nv1 (Ração de Reserva): Uma vez por turno, pode utilizar 1 item consumível da mochila instantaneamente sem gastar nenhuma de suas ações básicas.",
          "Passiva Nv4 (Tempero Secreto): Domina a alquimia culinária do navio, aumentando em +20% a eficácia de qualquer item consumível utilizado em seu turno." },
        { "Passiva Nv1 (Vigia do Convés): Mantém a guarda dos marujos próximos; aliados adjacentes em cruz não sofrem +25% de dano extra nem perdem estamina ao serem atacados em Descanso.",
          "Passiva Nv4 (Ordem de Abordagem): Sua presença inspira ferocidade na tripulação; aliados posicionados nas 4 casas adjacentes em cruz causam +20% de dano em todos os ataques." },
        { "Passiva Nv1 (Olho de Águia): Quanto mais perto o alvo estiver da mira do seu rifle, mais certeiro é o disparo: recebe +5% de Precisão para cada quadrado de aproximação (até +25% a 1 casa).",
          "Passiva Nv4 (Ponto Vital): Seus disparos causam +15% de dano a 1 casa, +10% a 2 casas e +5% a 3 casas." }
    };
    if (classe < CLASSE_BUCANEIRO || classe > CLASSE_GAVEA) {
        return "Sem passiva definida.";
    }
    return passivas[classe][aprimorada ? 1 : 0];
}

// Retorna a descrição detalhada do comportamento e passiva de um inimigo ou do Barba Negra.
static const char *descricaoPassivaInimigo(const Combatente *unidade)
{
    if (unidade == NULL) {
        return "";
    }
    if (unidade->eh_boss) {
        return unidade->fase_boss == 2 ?
            "Fase 2 Ativa (Fúria do Kraken - <=50% PV): Estamina máxima sobe para 30. Fica imune até os 2 Grunts com Espada invocados morrerem; 3 minas surgem em casas vazias a cada rodada e seus ataques aplicam Envenenamento (-10% da Vida Máx por 2 turnos)!" :
            "Fase 1 (Senhor do Convés): 3000 PV e 20 de Estamina; executa 3 turnos completos por rodada (Velocidades 8, 6 e 4). Ao cair para 50% de Vida ou menos, desperta a Fase 2!";
    }
    if (unidade->tipo_inimigo == INIMIGO_GRUNT_ESPADA) {
        return "Passiva (Lâmina Serrilhada): Ao acertar 3 vezes o ataque Corte no mesmo pirata durante o combate, abre uma hemorragia que aplica Sangramento (-15% da Vida Máxima por turno durante 2 turnos).";
    }
    if (unidade->tipo_inimigo == INIMIGO_GRUNT_PISTOLA) {
        return "Comportamento Tático: Dispara Tiro de pistola em linha reta (alcance de 2 quadrados) no primeiro alvo à frente ou lança Granada explosiva em cruz atingindo inimigos (100%) e aliados (50%).";
    }
    if (unidade->tipo_inimigo == INIMIGO_ATIRADOR_MAGRELO) {
        return "Comportamento Tático: Recua sempre que possível para manter distância segura e disparar seu mosquete pesado em linha reta a até 3 quadrados de alcance.";
    }
    return "Comportamento Tático: Dispara Escopeta numa linha de 2 quadrados (causando dano máximo a 1 casa e dano mínimo a 2 casas, igual ao Bacamarte) e aplica Bandagens que curam 10% da Vida Máxima por 3 turnos.";
}

// Retorna a explicação criativa, temática e fiel ao código atual de cada habilidade.
static const char *descricaoHabilidadeDetalhada(const Combatente *unidade, int indiceHab)
{
    static char bufferInimigo[512];
    const Habilidade *hab;
    if (unidade == NULL || indiceHab < 0 || indiceHab >= MAX_HABILIDADES) {
        return "";
    }
    hab = &unidade->habilidades[indiceHab];
    if (unidade->aliado) {
        if (unidade->classe == CLASSE_BUCANEIRO) {
            if (indiceHab == 0) {
                return "Desfere um golpe feroz de cutelo a 1 quadrado de alcance. Acertar 2 vezes o mesmo inimigo no combate abre um ferimento profundo que aplica Sangramento (-15% da Vida Máxima por turno durante 2 turnos).";
            }
            if (indiceHab == 1) {
                return "Arremessa um punhado de areia salgada do convés numa faixa perpendicular de 1x3 quadrados (alcance 1, sem empurrar). Cega e aplica Atordoamento por 1 turno em todas as unidades na área.";
            }
            return "Lança-se em disparada com a lâmina em punho por 2 quadrados em linha reta, atravessando quem estiver na frente (para na 2ª ou 3ª casa livre). Atinge todos na linha (aliados sofrem 50%) e converte 70% de todo o dano causado em Vida para o Bucaneiro!";
        }
        if (unidade->classe == CLASSE_TIMONEIRO) {
            if (indiceHab == 0) {
                return "Golpeia o adversário a 1 quadrado de alcance com o peso bruto dos punhos e correntes de ferro do convés, causando dano corpo a corpo direto conforme sua Precisão.";
            }
            if (indiceHab == 1) {
                return "Desaba um golpe demolidor de cima para baixo a 1 quadrado de alcance. Além do pesado dano físico, o impacto violento deixa o inimigo Atordoado por 1 turno inteiro.";
            }
            return "Arranca o pesado timão de carvalho e gira com força brutal numa faixa 1x3 adjacente. Atinge todos na área (aliados sofrem 50%), empurra 1 quadrado para trás e aplica Desnorteado (-2 Velocidade por 2 turnos).";
        }
        if (unidade->classe == CLASSE_COZINHEIRO) {
            if (indiceHab == 0) {
                return "Serve uma refeição a si ou a um aliado em linha reta (até 2 casas), curando 20% da vida máxima; contra inimigos, aplica Veneno por 2 turnos.";
            }
            if (indiceHab == 1) {
                return "Dispara uma rajada de chumbo grosso com seu bacamarte de cano largo numa linha reta de 2 quadrados (sem empurrar inimigos). Atinge todos na linha (aliados sofrem 50%), causando dano maior a 1 quadrado de distância e dano menor a 2 quadrados.";
            }
            return "Toca o sino do convés e serve um banquete revigorante numa área de 2x3 quadrados ao redor do Cozinheiro. Todos os aliados dentro da área recuperam garantidamente 70% de sua Vida Máxima total!";
        }
        if (unidade->classe == CLASSE_CAPITAO) {
            if (indiceHab == 0) {
                return "Ataque híbrido de abordagem em linha reta: a 1 quadrado golpeia com seu sabre real causando dano máximo (70% Precisão); a 2 quadrados saca sua garrucha dourada causando dano de tiro (60% Precisão).";
            }
            if (indiceHab == 1) {
                return "Brada ordens a até 2 quadrados em linha reta. Em aliado: concede Motivação durante 3 turnos (+40% de Precisão, +2 de Velocidade e +1 carga de Estamina Máxima). Em inimigo: aplica Desmoralizado por 2 turnos (-15% Precisão e +1 custo de Estamina).";
            }
            return "Ordena fogo cerrado da bateria lateral do navio sobre qualquer zona de 3x2 quadrados do tabuleiro! O bombardeio atinge todas as unidades nos 6 quadrados (aliados na área recebem 50% do dano).";
        }
        if (unidade->classe == CLASSE_GAVEA) {
            if (indiceHab == 0) {
                return "Firma a coronha do seu rifle de pederneira no alto da gávea e dispara um projétil de chumbo em linha reta a até 3 quadrados no primeiro inimigo à frente, ficando mais preciso e letal conforme se aproxima.";
            }
            if (indiceHab == 1) {
                return "Sopra um dardo silencioso a até 2 quadrados em linha reta. Em inimigo: inocula Envenenamento por 2 turnos (-10% da Vida Máxima por turno, -3 Velocidade e -15% dano). Em aliado: aplica ervas que curam 20% da Vida Máxima ao longo de 2 turnos (10% por turno).";
            }
            return "Carrega o rifle com pólvora dupla e dispara uma bala maciça que atravessa todas as 6 casas da linha reta! Perfura todos os inimigos alinhados causando dano devastador (aliados na linha de tiro recebem 50%).";
        }
    }
    if (unidade->eh_boss) {
        if (indiceHab == 0) {
            (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                           "Golpe brutal do sabre amaldiçoado de Barba Negra a %d quadrado de alcance (Custo: %d Est | Precisão: %d%%). Causa %d de dano mínimo ou %d de dano máximo. Na Fase 2 recebe +15%% de dano e aplica Envenenamento (-10%% da Vida Máx por 2 turnos).",
                           hab->alcance, hab->custo_estamina, hab->precisao, hab->dano_min, hab->dano_max);
            return bufferInimigo;
        }
        if (indiceHab == 1) {
            (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                           "Saca sua pistola de cano duplo e dispara em linha reta a até %d quadrados no primeiro pirata à frente (Custo: %d Est | Precisão: %d%%). Causa %d de dano mínimo ou %d de dano máximo (+20%% de dano na Fase 2).",
                           hab->alcance, hab->custo_estamina, hab->precisao, hab->dano_min, hab->dano_max);
            return bufferInimigo;
        }
        (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                       "Ordena uma salva devastadora de canhões numa zona 3x2 em qualquer parte do convés (Alcance: %d | Custo: %d Est | Precisão: %d%%). Causa %d de dano mínimo ou %d de dano máximo em todos na área (aliados dele sofrem 50%%; na Fase 2 aplica Envenenamento por 2t).",
                       hab->alcance, hab->custo_estamina, hab->precisao, hab->dano_min, hab->dano_max);
        return bufferInimigo;
    }
    if (unidade->tipo_inimigo == INIMIGO_GRUNT_ESPADA) {
        (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                       "Avança pelo convés e golpeia com seu cutelo serrilhado a %d quadrado de alcance (Custo: %d Est | Precisão: %d%%), causando %d de dano mínimo ou %d de dano máximo. Ao acertar 3 cortes no mesmo pirata, aplica Sangramento (-15%% da Vida Máxima por 2 turnos).",
                       hab->alcance, hab->custo_estamina, hab->precisao, hab->dano_min, hab->dano_max);
        return bufferInimigo;
    }
    if (unidade->tipo_inimigo == INIMIGO_GRUNT_PISTOLA) {
        if (indiceHab == 0) {
            (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                           "Aponta sua pistola pederneira e dispara em linha reta a até %d quadrados no primeiro pirata à frente (Custo: %d Est | Precisão: %d%%), causando %d de dano mínimo ou %d de dano máximo.",
                           hab->alcance, hab->custo_estamina, hab->precisao, hab->dano_min, hab->dano_max);
        } else {
            (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                           "Arremessa uma granada de pólvora negra a até %d quadrados que explode em cruz (5 casas | Custo: %d Est | Precisão: %d%%), causando %d de dano mínimo ou %d de dano máximo em todos na área (aliados dele recebem 50%%).",
                           hab->alcance, hab->custo_estamina, hab->precisao, hab->dano_min, hab->dano_max);
        }
        return bufferInimigo;
    }
    if (unidade->tipo_inimigo == INIMIGO_ATIRADOR_MAGRELO) {
        (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                       "Firma o mosquete longo no ombro e dispara em linha reta a até %d quadrados de alcance no primeiro pirata à frente (Custo: %d Est | Precisão: %d%%), causando %d de dano mínimo ou %d de dano máximo.",
                       hab->alcance, hab->custo_estamina, hab->precisao, hab->dano_min, hab->dano_max);
        return bufferInimigo;
    }
    if (indiceHab == 0) {
        (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                       "Dispara uma rajada de chumbo com sua escopeta numa linha reta de %d quadrados (Custo: %d Est), funcionando igual ao Bacamarte do Cozinheiro: atinge todos na linha (aliados sofrem 50%%), causando %d de dano a 1 quadrado e %d de dano a 2 quadrados!",
                       hab->alcance, hab->custo_estamina, hab->dano_max, hab->dano_min);
    } else {
        (void)snprintf(bufferInimigo, sizeof(bufferInimigo),
                       "Enfaixa um inimigo ferido adjacente (%d quadrado de alcance | Custo: %d Est), aplicando regeneração que cura 10%% da Vida Máxima total do alvo por turno durante 3 turnos (30%% no total).",
                       hab->alcance, hab->custo_estamina);
    }
    return bufferInimigo;
}

// Retorna uma descrição temática e completa do efeito de um item pelo seu ID.
static const char *descricaoItemCatalogo(int idItem)
{
    switch (idItem) {
    case 1:
        return "Consumível: Gaze limpa embebida em rum esterilizante. Estanca ferimentos e cura imediatamente 40% da Vida Máxima de quem utilizar (48% com Cozinheiro Nv 4).";
    case 2:
        return "Consumível: Elixir picante de pólvora e especiarias. Desperta a fúria pirata concedendo +20% de dano nos ataques por 2 turnos (+24% com Cozinheiro Nv 4).";
    case 3:
        return "Consumível: Travesseiro macio saqueado de um galeão real. Recupera o fôlego na hora, restaurando 100% da Estamina Máxima do pirata.";
    case 4:
        return "Consumível: Diário antigo com segredos de navegação e combate. Promove o pirata em +2 Níveis permanentes (+20% Vida e Dano por nível).";
    case 5:
        return "Passivo de Build: Gibão reforçado com couro curtido e placas leves. Amortece golpes e tiros reduzindo em -15% todo o dano recebido pelo pirata.";
    case 6:
        return "Passivo de Build: Pedra áspera de amolar lâminas no convés. Mantém sabres e cutelos afiados, concedendo +20% de dano em ataques corpo a corpo (Melee).";
    case 7:
        return "Passivo de Build: Lentes polidas de cristal náutico que apuram a mira. Aumentam em +25% a Precisão (chance de causar Dano Máximo) nos ataques.";
    case 8:
        return "Passivo de Build: Botas de couro flexível para correr pelas pranchas. Concedem +1 quadrado extra de alcance em cada ação de Mover do pirata.";
    case 9:
        return "Passivo de Build: Munição especial de chumbo temperado que estilhaça defesas. Aumenta em +35% o dano de todos os ataques à distância e em área (Ranged).";
    case 10:
        return "Passivo de Build: Amuleto entalhado em osso de baleia. Aumenta permanentemente em +2 pontos a Estamina Máxima do pirata enquanto estiver equipado.";
    case 11:
        return "Passivo de Build: Garrafa lendária abençoada pelo deus dos mares. Cada garrafa equipada concede +1 Ação Básica extra por turno (acumula até 5x na build!).";
    default:
        return "Relíquia pirata conquistada na expedição.";
    }
}

// Formata o cabeçalho técnico da habilidade sem exibir dano mínimo/máximo em curas.
static void formatarCabecalhoHabilidade(const Combatente *unidade, int indiceHab,
                                        bool modoCompacto, char *destino, size_t tamanho)
{
    const Habilidade *hab;
    const char *statusStr;
    if (unidade == NULL || destino == NULL || tamanho == 0 ||
        indiceHab < 0 || indiceHab >= MAX_HABILIDADES) {
        if (destino != NULL && tamanho > 0) {
            destino[0] = '\0';
        }
        return;
    }
    hab = &unidade->habilidades[indiceHab];
    statusStr = (indiceHab < unidade->habilidades_desbloqueadas) ?
                "ATIVA" : (indiceHab == 1 ? "BLOQ Nv3" : "BLOQ Nv6");
    if (hab->efeito == EFEITO_CURA) {
        if (modoCompacto) {
            (void)snprintf(destino, tamanho, "%s (%d Est | Cura: %d%% da Vida Máx)",
                           hab->nome, hab->custo_estamina, hab->dano_max);
        } else {
            (void)snprintf(destino, tamanho,
                           "[%s] %s | Cura: %d%% da Vida Máx | Alcance: %d | Custo: %d Est",
                           statusStr, hab->nome, hab->dano_max,
                           hab->alcance, hab->custo_estamina);
        }
    } else if (hab->efeito == EFEITO_CURA_CONTINUA) {
        int duracao = hab->duracao_efeito > 0 ? hab->duracao_efeito : 3;
        if (modoCompacto) {
            (void)snprintf(destino, tamanho, "%s (%d Est | Cura: %d%% da Vida Máx/t por %dt)",
                           hab->nome, hab->custo_estamina, hab->dano_max, duracao);
        } else {
            (void)snprintf(destino, tamanho,
                           "[%s] %s | Cura: %d%% da Vida Máx/turno (%dt, total %d%%) | Alcance: %d | Custo: %d Est",
                           statusStr, hab->nome, hab->dano_max, duracao,
                           hab->dano_max * duracao, hab->alcance, hab->custo_estamina);
        }
    } else if ((unidade->aliado && unidade->classe == CLASSE_COZINHEIRO && indiceHab == 1) ||
               (!unidade->aliado && !unidade->eh_boss &&
                unidade->tipo_inimigo == INIMIGO_MARUJO_GORDO && indiceHab == 0)) {
        if (modoCompacto) {
            (void)snprintf(destino, tamanho,
                           "%s (%d Est | Linha 2 casas: %d dano a 1 casa / %d a 2 casas)",
                           hab->nome, hab->custo_estamina, hab->dano_max, hab->dano_min);
        } else {
            (void)snprintf(destino, tamanho,
                           "[%s] %s | Dano: %d (a 1 casa) ou %d (a 2 casas) | Alcance: %d (Linha) | Custo: %d Est",
                           statusStr, hab->nome, hab->dano_max, hab->dano_min,
                           hab->alcance, hab->custo_estamina);
        }
    } else if (unidade->aliado && unidade->classe == CLASSE_CAPITAO && indiceHab == 0) {
        if (modoCompacto) {
            (void)snprintf(destino, tamanho,
                           "%s (%d Est | Dano: %d a 1 casa / %d a 2 casas)",
                           hab->nome, hab->custo_estamina, hab->dano_max, hab->dano_min);
        } else {
            (void)snprintf(destino, tamanho,
                           "[%s] %s | Dano: %d (1 casa, Prec 70%%) ou %d (2 casas, Prec 60%%) | Alcance: %d | Custo: %d Est",
                           statusStr, hab->nome, hab->dano_max, hab->dano_min,
                           hab->alcance, hab->custo_estamina);
        }
    } else if (hab->efeito == EFEITO_VENENO && hab->dano_max == 0) {
        if (modoCompacto) {
            (void)snprintf(destino, tamanho,
                           "%s (%d Est | Inimigo: Veneno -10%%/t (2t) | Aliado: Cura 20%% Vida Máx)",
                           hab->nome, hab->custo_estamina);
        } else {
            (void)snprintf(destino, tamanho,
                           "[%s] %s | Inimigo: Veneno -10%%/t (2t) | Aliado: Cura 20%% da Vida Máx (2t) | Alcance: %d | Custo: %d Est",
                           statusStr, hab->nome, hab->alcance, hab->custo_estamina);
        }
    } else if (hab->dano_max == 0) {
        if (modoCompacto) {
            (void)snprintf(destino, tamanho, "%s (%d Est | Habilidade Tática)",
                           hab->nome, hab->custo_estamina);
        } else {
            (void)snprintf(destino, tamanho,
                           "[%s] %s | Efeito Tático | Alcance: %d | Custo: %d Est",
                           statusStr, hab->nome, hab->alcance, hab->custo_estamina);
        }
    } else {
        if (modoCompacto) {
            (void)snprintf(destino, tamanho, "%s (%d Est | Dano: %d ou %d)",
                           hab->nome, hab->custo_estamina, hab->dano_min, hab->dano_max);
        } else {
            (void)snprintf(destino, tamanho,
                           "[%s] %s | Dano: %d ou %d | Alcance: %d | Custo: %d Est | Prec: %d%%",
                           statusStr, hab->nome, hab->dano_min, hab->dano_max,
                           hab->alcance, hab->custo_estamina, hab->precisao);
        }
    }
}

// Desenha o menu de pause acionado pela tecla ESC com opções Retornar e Voltar ao Menu.
static void desenharMenuPause(void)
{
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 6, 10, 15, 195 });
    DrawRectangle(420, 190, 440, 290, (Color){ 15, 24, 34, 252 });
    DrawRectangleLinesEx((Rectangle){ 420.0f, 190.0f, 440.0f, 290.0f }, 3.0f, GOLD);
    desenharTexto("JOGO PAUSADO", 535, 225, 28, GOLD);
    DrawRectangle(490, 300, 300, 58, (Color){ 45, 125, 70, 255 });
    DrawRectangleLines(490, 300, 300, 58, RAYWHITE);
    desenharTexto("RETORNAR", 582, 318, 20, RAYWHITE);
    DrawRectangle(490, 380, 300, 58, (Color){ 145, 48, 38, 255 });
    DrawRectangleLines(490, 380, 300, 58, RAYWHITE);
    desenharTexto("VOLTAR AO MENU", 545, 398, 20, RAYWHITE);
}

// Desenha a ficha completa de status, passivas, ataques e build com suporte ao scroll do mouse.
static void desenharDetalhes(const InterfaceJogo *interfaceJogo, const Combatente *unidade)
{
    int indice;
    int offsetY;
    int posY;
    int barraY;
    int minY = 112;
    int maxY = 588;
    if (unidade == NULL || interfaceJogo == NULL) {
        return;
    }
    offsetY = interfaceJogo->scroll_detalhes;
    DrawUIPanelCustom((Rectangle){ 160.0f, 50.0f, 740.0f, 555.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 252 });
    posY = 116 - offsetY;
    if (posY >= minY && posY + 24 <= maxY) {
        desenharTexto(unidade->nome, 185, posY, 23, unidade->aliado ? SKYBLUE : ORANGE);
    }
    posY += 28;
    posY += desenharTextoQuebrado(descricaoResumoPersonagem(unidade), 185, posY, 680,
                                  15, 19, GOLD, minY, maxY) + 8;
    if (posY >= minY && posY + 16 <= maxY) {
        if (unidade->eh_boss) {
            desenharTexto(TextFormat("PV %d/%d | Estamina %d/%d | Fase %d | Velocidades: 8, 6 e 4",
                                     unidade->vida_atual, unidade->vida_maxima,
                                     unidade->estamina_atual, unidade->estamina_maxima,
                                     unidade->fase_boss),
                          185, posY, 16, RAYWHITE);
        } else {
            desenharTexto(TextFormat("PV %d/%d | Estamina %d/%d | Nível %d | Velocidade %d | Ações/Turno: %d",
                                     unidade->vida_atual, unidade->vida_maxima,
                                     unidade->estamina_atual, unidade->estamina_maxima,
                                     unidade->nivel, unidade->velocidade,
                                     calcularAcoesPorTurno(unidade)),
                          185, posY, 15, RAYWHITE);
        }
    }
    posY += 22;
    if (posY >= minY && posY + 15 <= maxY) {
        if (unidade->aliado && unidade->classe == CLASSE_COZINHEIRO) {
            desenharTexto("Cura Garantida (sem erro) | Dano do Bacamarte varia conforme a distância",
                          185, posY, 14, LIGHTGRAY);
        } else {
            desenharTexto(TextFormat("Chance Dano Máx (Precisão): Melee %d%% | Ranged %d%% | Bônus Nv +%d%%",
                                     unidade->precisao_melee, unidade->precisao_ranged,
                                     unidade->dano_base),
                          185, posY, 14, LIGHTGRAY);
        }
    }
    posY += 24;
    if (posY >= minY && posY + 16 <= maxY) {
        desenharTexto("EFEITOS ATIVOS (BUFFS / DEBUFFS):", 185, posY, 15, GOLD);
        desenharIndicadoresStatusUnidade(unidade, 445, posY - 2, 410);
    }
    posY += 26;
    if (posY >= minY && posY + 16 <= maxY) {
        desenharTexto("PASSIVAS E COMPORTAMENTO", 185, posY, 16, GOLD);
    }
    posY += 22;
    if (unidade->aliado) {
        posY += desenharTextoQuebrado(descricaoPassiva(unidade->classe, false), 192, posY,
                                      675, 14, 18, RAYWHITE, minY, maxY) + 6;
        posY += desenharTextoQuebrado(descricaoPassiva(unidade->classe, true), 192, posY,
                                      675, 14, 18,
                                      unidade->passiva_aprimorada ? GREEN : GRAY,
                                      minY, maxY) + 10;
    } else {
        if (unidade->eh_elite) {
            posY += desenharTextoQuebrado(
                "ELITE: +25% de vida, +3 de estamina e 3 ações por turno.",
                192, posY, 675, 14, 18, GOLD, minY, maxY) + 6;
        }
        posY += desenharTextoQuebrado(descricaoPassivaInimigo(unidade), 192, posY,
                                      675, 14, 18, RAYWHITE, minY, maxY) + 10;
    }
    if (posY >= minY && posY + 16 <= maxY) {
        desenharTexto("HABILIDADES E AÇÕES", 185, posY, 16, GOLD);
    }
    posY += 22;
    for (indice = 0; indice < MAX_HABILIDADES; indice++) {
        const Habilidade *habilidade = &unidade->habilidades[indice];
        bool desbloqueada = indice < unidade->habilidades_desbloqueadas;
        char cabecalho[192];
        int alturaCard = 84;
        if (habilidade->nome[0] == '\0') {
            continue;
        }
        formatarCabecalhoHabilidade(unidade, indice, false, cabecalho, sizeof(cabecalho));
        if (posY + alturaCard >= minY && posY <= maxY) {
            DrawUIPanelCustom((Rectangle){ 185.0f, (float)posY, 680.0f,
                                           (float)alturaCard },
                              interfaceJogo->texPainelDetalhes,
                              (Color){ 20, 32, 44, 255 });
            if (!desbloqueada) {
                DrawRectangleLines(185, posY, 680, alturaCard, DARKGRAY);
            }
        }
        if (posY + 4 >= minY && posY + 18 <= maxY) {
            desenharTexto(cabecalho, 193, posY + 4, 13, desbloqueada ? RAYWHITE : GRAY);
        }
        (void)desenharTextoQuebrado(descricaoHabilidadeDetalhada(unidade, indice),
                                    193, posY + 22, 664, 13, 17,
                                    desbloqueada ? GOLD : GRAY, minY, maxY);
        posY += alturaCard + 8;
    }
    if (unidade->aliado) {
        posY += 4;
        if (posY >= minY && posY + 16 <= maxY) {
            desenharTexto(TextFormat("BUILD PASSIVA EQUIPADA (%d/%d)",
                                     unidade->build_passivos.tamanho,
                                     MAX_ITENS_PASSIVOS), 185, posY, 16, GOLD);
        }
        posY += 22;
        if (unidade->build_passivos.inicio == NULL) {
            if (posY >= minY && posY + 15 <= maxY) {
                desenharTexto("Nenhum item passivo equipado neste pirata.",
                              195, posY, 14, LIGHTGRAY);
            }
        } else {
            NoItem *item;
            int slot = 1;
            for (item = unidade->build_passivos.inicio; item != NULL; item = item->proximo) {
                Color corItem = corRaridadeItem(item->item.raridade);
                char textoSlot[384];
                (void)snprintf(textoSlot, sizeof(textoSlot), "[Slot %d - %s] %s: %s",
                               slot, nomeRaridadeItem(item->item.raridade),
                               item->item.nome, descricaoItemCatalogo(item->item.id));
                posY += desenharTextoQuebrado(textoSlot, 192, posY, 672, 14, 18,
                                              corItem, minY, maxY) + 6;
                slot++;
            }
        }
    }
    DrawRectangle(161, 51, 738, 60, (Color){ 12, 19, 27, 255 });
    DrawRectangle(161, 589, 738, 15, (Color){ 12, 19, 27, 255 });
    DrawUIFrameCustom((Rectangle){ 160.0f, 50.0f, 740.0f, 555.0f },
                      interfaceJogo->texFrameRetrato);
    desenharTexto("FICHA DE INSPEÇÃO DO PERSONAGEM", 185, 68, 21, GOLD);
    desenharTexto("Role com o Scroll do Mouse", 535, 74, 14, LIGHTGRAY);
    DrawUIButtonCustom((Rectangle){ 760.0f, 68.0f, 85.0f, 38.0f },
                       "FECHAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
    DrawRectangle(880, 115, 8, 472, (Color){ 28, 40, 52, 255 });
    barraY = 115 + (offsetY * 392) / 420;
    DrawRectangle(880, barraY, 8, 80, GOLD);
}

// Desenha os cards dos piratas e o botão para iniciar ou retomar a aventura.
void desenharTelaSelecaoGrupo(InterfaceJogo *interfaceJogo,
                              const Combatente piratasDisponiveis[], int totalDisponiveis)
{
    int limite;
    int indice;
    if (interfaceJogo == NULL || piratasDisponiveis == NULL) {
        return;
    }
    limite = totalDisponiveis < TOTAL_PIRATAS_PREDEFINIDOS ?
             totalDisponiveis : TOTAL_PIRATAS_PREDEFINIDOS;
    interfaceRenderizada = interfaceJogo;
    iniciarQuadroVirtual(interfaceJogo, (Color){ 18, 28, 39, 255 });
    desenharTexto(contadorFugas > 0 ?
                  "REPOSIÇÃO DE TRIPULAÇÃO PÓS-FUGA" :
                  "ESCOLHA SUA TRIPULAÇÃO",
                  contadorFugas > 0 ? 310 : 370, 42, 30, GOLD);
    desenharTexto(contadorFugas > 0 ?
                  TextFormat("Selecione substitutos para os caídos (%d/3) - Mortos voltaram ao Nv 1 sem passivos",
                             interfaceJogo->total_selecionados) :
                  TextFormat("Selecione 3 piratas para a expedição (%d/3)",
                             interfaceJogo->total_selecionados),
                  contadorFugas > 0 ? 220 : 420, 92, 18, RAYWHITE);
    for (indice = 0; indice < limite; indice++) {
        int cardX = 60 + (indice % 3) * 390;
        int cardY = 145 + (indice / 3) * 205;
        bool selecionado = indicePirataSelecionado(interfaceJogo,
                                                   piratasDisponiveis[indice].id) >= 0;
        bool sobreviventeFixo = ehSobreviventeFixoFuga(interfaceJogo,
                                                       piratasDisponiveis[indice].id);
        Color borda = selecionado ? (Color){ 90, 220, 120, 255 } :
                                    (Color){ 150, 165, 175, 255 };
        DrawRectangle(cardX, cardY, 350, 180, (Color){ 42, 63, 76, 255 });
        DrawRectangleLinesEx((Rectangle){ (float)cardX, (float)cardY, 350, 180 },
                             selecionado ? 4.0f : 2.0f, borda);
        desenharTexto(piratasDisponiveis[indice].nome, cardX + 16, cardY + 14, 20, RAYWHITE);
        if (piratasDisponiveis[indice].classe == CLASSE_COZINHEIRO) {
            desenharTexto(TextFormat("HP %d | Cura Garantida | Vel %d",
                                     piratasDisponiveis[indice].vida_maxima,
                                     piratasDisponiveis[indice].velocidade),
                          cardX + 16, cardY + 52, 16, LIGHTGRAY);
        } else {
            desenharTexto(TextFormat("HP %d | Precisão %d%% | Vel %d",
                                     piratasDisponiveis[indice].vida_maxima,
                                     piratasDisponiveis[indice].precisao_melee,
                                     piratasDisponiveis[indice].velocidade),
                          cardX + 16, cardY + 52, 16, LIGHTGRAY);
        }
        desenharTexto(TextFormat("Estamina %d | Nível %d | Passivos %d",
                                 piratasDisponiveis[indice].estamina_maxima,
                                 piratasDisponiveis[indice].nivel,
                                 piratasDisponiveis[indice].build_passivos.tamanho),
                      cardX + 16, cardY + 84, 16, LIGHTGRAY);
        desenharTexto(sobreviventeFixo ? "SOBREVIVENTE (MANTIDO)" :
                      (selecionado ? "SELECIONADO" : "Clique para selecionar"),
                      cardX + 16, cardY + 144, 15,
                      sobreviventeFixo ? SKYBLUE : (selecionado ? GOLD : GRAY));
        DrawRectangle(cardX + 230, cardY + 138, 108, 32, (Color){ 28, 42, 54, 255 });
        DrawRectangleLines(cardX + 230, cardY + 138, 108, 32, GOLD);
        desenharTexto("DETALHES", cardX + 244, cardY + 147, 13, GOLD);
    }
    DrawRectangle(440, 590, 400, 60, interfaceJogo->total_selecionados == 3 ?
                  (Color){ 45, 125, 70, 255 } : (Color){ 65, 70, 75, 255 });
    desenharTexto(contadorFugas > 0 ? "RETOMAR AVENTURA" : "INICIAR AVENTURA",
                  515, 608, 22, RAYWHITE);
    if (interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM) {
        desenharDetalhes(interfaceJogo, interfaceJogo->entidade_selecionada);
    } else if (interfaceJogo->popup_ativo == POPUP_PAUSE) {
        desenharMenuPause();
    }
    finalizarQuadroVirtual();
}

// Desenha a mochila organizada com abas de pirata, cores por raridade, builds e scroll do mouse.
static void desenharMochila(const InterfaceJogo *interfaceJogo)
{
    NoItem *item;
    int indice = 0;
    int barraY;
    const Combatente *selecionado = interfaceJogo->entidade_selecionada;
    bool somenteConsumiveis = interfaceJogo->estado == TELA_COMBATE_3X6 &&
                              !interfaceJogo->fase_pre_combate_ativa;
    DrawUIPanelCustom((Rectangle){ 160.0f, 50.0f, 700.0f, 555.0f },
                      interfaceJogo->texPainelMochila, (Color){ 12, 19, 27, 252 });
    DrawUIFrameCustom((Rectangle){ 160.0f, 50.0f, 700.0f, 555.0f },
                      interfaceJogo->texFrameRetrato);
    desenharTexto("MOCHILA DA TRIPULAÇÃO E BUILDS PASSIVAS", 180, 68, 20, GOLD);
    desenharTexto("Scroll: rolar itens", 595, 73, 13, LIGHTGRAY);
    DrawUIButtonCustom((Rectangle){ 740.0f, 68.0f, 105.0f, 38.0f },
                       interfaceJogo->loja_aberta_pendente ? "VOLTAR LOJA" : "FECHAR",
                       interfaceJogo->texBtnPadrao, obterFonteInterface());
    for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
        int posAbaX = 180 + indice * 220;
        int idPirata = interfaceJogo->piratas_selecionados_ids[indice];
        bool abaAtiva = selecionado != NULL && selecionado->id == idPirata;
        bool morto = idPirata >= 1 && idPirata <= 5 &&
                     interfaceJogo->piratas_mortos[idPirata - 1];
        DrawRectangle(posAbaX, 112, 205, 36,
                      abaAtiva ? (Color){ 45, 95, 130, 255 } : (Color){ 28, 42, 54, 255 });
        DrawRectangleLines(posAbaX, 112, 205, 36, abaAtiva ? GOLD : GRAY);
        desenharTexto(morto ? TextFormat("%s - MORTO", nomePirataPorId(idPirata)) :
                              nomePirataPorId(idPirata),
                      posAbaX + 24, 121, 15,
                      morto ? GRAY : (abaAtiva ? GOLD : RAYWHITE));
    }
    desenharTexto(somenteConsumiveis ?
                  "ITENS: selecione um consumível e clique em USAR." :
                  "ITENS: selecione consumível para USAR ou clique em passivo para equipar.",
                  180, 156, 13, LIGHTGRAY);
    indice = 0;
    for (item = interfaceJogo->inventario == NULL ? NULL :
                interfaceJogo->inventario->inicio;
         item != NULL; item = item->proximo) {
        Color corRaridade;
        int posLinhaY;
        if (somenteConsumiveis && item->item.tipo != ITEM_CONSUMIVEL) {
            continue;
        }
        corRaridade = corRaridadeItem(item->item.raridade);
        posLinhaY = 178 + indice * 48 - interfaceJogo->scroll_mochila;
        if (posLinhaY >= 172 && posLinhaY <= 325) {
            bool selecionadoItem = item->item.id == interfaceJogo->item_id_selecionado;
            DrawUIPanelCustom((Rectangle){ 180.0f, (float)posLinhaY, 650.0f, 44.0f },
                              interfaceJogo->texPainelMochila,
                              selecionadoItem ? (Color){ 48, 65, 78, 255 } :
                                                (Color){ 26, 40, 52, 255 });
            DrawRectangleLines(180, posLinhaY, 650, 44,
                               selecionadoItem ? GOLD : corRaridade);
            desenharTexto(TextFormat("[%s | %s] %s (x%d)",
                                     item->item.tipo == ITEM_CONSUMIVEL ?
                                     (selecionadoItem ? "SELECIONADO" : "CONSUMÍVEL") :
                                     "EQUIPAR",
                                     nomeRaridadeItem(item->item.raridade),
                                     item->item.nome, item->item.quantidade),
                          188, posLinhaY + 2, 13, corRaridade);
            (void)desenharTextoQuebrado(descricaoItemCatalogo(item->item.id),
                                        188, posLinhaY + 17, 634, 12, 14,
                                        corRaridade, 174, 368);
        }
        indice++;
    }
    if (indice == 0) {
        DrawRectangle(180, 195, 650, 55, (Color){ 22, 33, 44, 255 });
        desenharTexto("Mochila vazia! Conquiste itens vencendo rotas ou comprando na Loja.",
                      205, 214, 15, LIGHTGRAY);
    }
    DrawUIButtonCustom((Rectangle){ 690.0f, 372.0f, 135.0f, 32.0f },
                       "USAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
    if (interfaceJogo->item_id_selecionado > 0) {
        DrawRectangleLines(690, 372, 135, 32, GOLD);
    }
    {
        int maxScrollMochila = calcularMaxScrollMochila(interfaceJogo);
        int scrollAtual = interfaceJogo->scroll_mochila;
        if (scrollAtual > maxScrollMochila) {
            scrollAtual = maxScrollMochila;
        }
        if (scrollAtual < 0) {
            scrollAtual = 0;
        }
        DrawRectangle(838, 178, 6, 192, (Color){ 28, 40, 52, 255 });
        barraY = maxScrollMochila > 0 ?
                 178 + (scrollAtual * 142) / maxScrollMochila : 178;
        DrawRectangle(838, barraY, 6, 50, GOLD);
    }
    desenharTexto(TextFormat("BUILD PASSIVA DE: %s (%d/5) %s",
                             (selecionado != NULL && selecionado->aliado) ?
                             selecionado->nome : "Selecione um pirata",
                             (selecionado != NULL && selecionado->aliado) ?
                             selecionado->build_passivos.tamanho : 0,
                             (interfaceJogo->estado == TELA_MAPA_ROTAS ||
                              interfaceJogo->fase_pre_combate_ativa) ?
                             "- Clique em um slot ocupado para devolver à Mochila" :
                             "(Travada em combate)"),
                  180, 410, 14, GOLD);
    if (selecionado != NULL && selecionado->aliado) {
        NoItem *passivo = selecionado->build_passivos.inicio;
        int slot;
        for (slot = 0; slot < MAX_ITENS_PASSIVOS; slot++) {
            int posSlotY = 432 + slot * 34;
            if (passivo != NULL) {
                Color corPassivo = corRaridadeItem(passivo->item.raridade);
                DrawRectangle(180, posSlotY, 650, 31, (Color){ 28, 45, 42, 255 });
                DrawRectangleLines(180, posSlotY, 650, 31, corPassivo);
                desenharTexto(TextFormat("[Slot %d - %s] %s",
                                         slot + 1,
                                         nomeRaridadeItem(passivo->item.raridade),
                                         passivo->item.nome),
                              188, posSlotY + 2, 12, corPassivo);
                (void)desenharTextoQuebrado(descricaoItemCatalogo(passivo->item.id),
                                            188, posSlotY + 16, 634, 11, 13,
                                            corPassivo, posSlotY, posSlotY + 31);
                passivo = passivo->proximo;
            } else {
                DrawRectangle(180, posSlotY, 650, 31, (Color){ 18, 28, 36, 255 });
                DrawRectangleLines(180, posSlotY, 650, 31, DARKGRAY);
                desenharTexto(TextFormat("[Slot %d Vazio - Disponível para item passivo]",
                                         slot + 1),
                              188, posSlotY + 9, 12, GRAY);
            }
        }
    }
}

// Desenha o pop-up comercial com os 3 itens sorteados coloridos por raridade.
static void desenharLoja(const InterfaceJogo *interfaceJogo)
{
    int indice;
    DrawRectangle(160, 55, 700, 545, (Color){ 12, 19, 27, 252 });
    DrawRectangleLines(160, 55, 700, 545, GOLD);
    desenharTexto("CAMINHO COMERCIAL - LOJA DA ROTA", 275, 75, 22, GOLD);
    DrawRectangle(740, 72, 105, 38, (Color){ 60, 75, 87, 255 });
    desenharTexto("FECHAR [X]", 750, 84, 14, RAYWHITE);
    desenharTexto(TextFormat("Moedas de Ouro: %d | Saques realizados: %d (Inflação +%d%%)",
                             interfaceJogo->moedas == NULL ? 0 : *interfaceJogo->moedas,
                             contadorSaques, contadorSaques * 25),
                  190, 128, 16, RAYWHITE);
    for (indice = 0; indice < interfaceJogo->total_itens_loja; indice++) {
        const Item *item = &interfaceJogo->itens_loja[indice];
        int estoque = interfaceJogo->estoques_loja[indice];
        int preco = aplicarInflacaoLoja(item->preco);
        int posItemY = 175 + indice * 92;
        Color corRaridade = corRaridadeItem(item->raridade);
        Color corFundo = estoque > 0 ? (Color){ 28, 44, 58, 255 } :
                                       (Color){ 35, 35, 40, 255 };
        DrawRectangle(190, posItemY, 640, 78, corFundo);
        DrawRectangleLines(190, posItemY, 640, 78, estoque > 0 ? corRaridade : GRAY);
        desenharTexto(TextFormat("[%s] %s | Preço: %d ouros | Estoque: %d/2",
                                 nomeRaridadeItem(item->raridade),
                                 item->nome, preco, estoque),
                      206, posItemY + 6, 16, estoque > 0 ? corRaridade : GRAY);
        (void)desenharTextoQuebrado(descricaoItemCatalogo(item->id), 206, posItemY + 28,
                                    612, 13, 17, estoque > 0 ? corRaridade : DARKGRAY, 0, 0);
    }
    DrawRectangle(185, 495, 200, 55, (Color){ 45, 95, 130, 255 });
    DrawRectangleLines(185, 495, 200, 55, GOLD);
    desenharTexto("MOCHILA / USAR", 215, 514, 16, RAYWHITE);
    DrawRectangle(400, 495, 215, 55, (Color){ 145, 45, 38, 255 });
    desenharTexto("SAQUEAR LOJA", 442, 514, 16, RAYWHITE);
    DrawRectangle(630, 495, 195, 55, (Color){ 45, 115, 68, 255 });
    desenharTexto("SEGUIR VIAGEM", 658, 514, 16, RAYWHITE);
}

// Desenha o pop-up modal de confirmação de desistência ("Fugir da Aventura").
static void desenharConfirmacaoFuga(const InterfaceJogo *interfaceJogo)
{
    DrawRectangle(210, 200, 620, 295, (Color){ 12, 19, 27, 252 });
    DrawRectangleLines(210, 200, 620, 295, GOLD);
    desenharTexto("Fugir da Aventura (Retornar à Rota 1)?", 295, 235, 22, GOLD);
    desenharTexto(TextFormat("Fugas usadas: %d/2 | Ouro mantido: %d",
                             contadorFugas,
                             interfaceJogo->moedas == NULL ? 0 : *interfaceJogo->moedas),
                  300, 265, 15, GOLD);
    desenharTexto("Piratas vivos mantêm nível e itens equipados nas builds.", 265, 295, 15, RAYWHITE);
    desenharTexto("Piratas mortos podem ser reescolhidos no Nv 1 (sem passivos).", 260, 325, 15, SKYBLUE);
    desenharTexto(contadorFugas == 0 ?
                  "Itens da mochila serão perdidos; inimigos recebem +50% de status." :
                  "Itens da mochila serão perdidos; inimigos recebem +75% de status.",
                  260, 355, 15, ORANGE);
    DrawRectangle(350, 420, 150, 50, contadorFugas < 2 ?
                  (Color){ 140, 50, 40, 255 } : (Color){ 65, 70, 75, 255 });
    DrawRectangle(530, 420, 150, 50, (Color){ 60, 75, 87, 255 });
    desenharTexto(contadorFugas < 2 ? "FUGIR" : "LIMITE 2/2",
                  contadorFugas < 2 ? 395 : 364, 436, 18,
                  contadorFugas < 2 ? RAYWHITE : GRAY);
    desenharTexto("CANCELAR", 554, 436, 18, RAYWHITE);
}

static void desenharConfirmacaoEscolha(const InterfaceJogo *interfaceJogo,
                                       bool compra)
{
    const char *titulo;
    const char *mensagem;
    char texto[192];
    DrawRectangle(310, 215, 660, 270, (Color){ 12, 19, 27, 252 });
    DrawRectangleLines(310, 215, 660, 270, GOLD);
    if (compra) {
        int indice = interfaceJogo->indice_compra_pendente;
        titulo = "CONFIRMAR COMPRA";
        if (indice >= 0 && indice < interfaceJogo->total_itens_loja) {
            const Item *item = &interfaceJogo->itens_loja[indice];
            (void)snprintf(texto, sizeof(texto), "%s por %d ouros?",
                           item->nome, aplicarInflacaoLoja(item->preco));
        } else {
            (void)snprintf(texto, sizeof(texto), "Este item não está mais disponível.");
        }
        mensagem = texto;
    } else {
        titulo = "CONFIRMAR CAMINHO";
        if (interfaceJogo->escolha_rota_pendente == 3) {
            mensagem = "Entrar na loja desta rota?";
        } else if (interfaceJogo->escolha_rota_pendente == 2) {
            mensagem = "Escolher o Caminho 2? (3 inimigos)";
        } else if (interfaceJogo->escolha_rota_pendente == 1 &&
                   interfaceJogo->rota_atual != NULL &&
                   *interfaceJogo->rota_atual != NULL &&
                   (*interfaceJogo->rota_atual)->etapa >= 3) {
            mensagem = "Enfrentar Barba Negra?";
        } else {
            mensagem = "Escolher o Caminho 1? (4 inimigos)";
        }
    }
    desenharTexto(titulo, 495, 265, 26, GOLD);
    desenharTextoCentralizado(mensagem, 330, 20, RAYWHITE);
    DrawRectangle(390, 390, 180, 55, (Color){ 45, 125, 70, 255 });
    DrawRectangleLines(390, 390, 180, 55, GOLD);
    desenharTexto("CONFIRMAR", 414, 407, 17, RAYWHITE);
    DrawRectangle(610, 390, 180, 55, (Color){ 60, 75, 87, 255 });
    desenharTexto("CANCELAR", 648, 407, 17, RAYWHITE);
}

static void desenharRecompensas(const InterfaceJogo *interfaceJogo)
{
    int indice;
    DrawRectangle(320, 175, 640, 390, (Color){ 12, 19, 27, 252 });
    DrawRectangleLines(320, 175, 640, 390, GOLD);
    desenharTexto("RECOMPENSAS DO COMBATE", 465, 205, 24, GOLD);
    if (interfaceJogo->total_itens_recompensa == 0) {
        desenharTexto("Nenhum item foi obtido neste combate.", 430, 280, 17, LIGHTGRAY);
    }
    for (indice = 0; indice < interfaceJogo->total_itens_recompensa; indice++) {
        const Item *item = &interfaceJogo->itens_recompensa[indice];
        int posY = 260 + indice * 42;
        DrawRectangle(385, posY, 510, 34, (Color){ 26, 40, 52, 255 });
        DrawRectangleLines(385, posY, 510, 34, corRaridadeItem(item->raridade));
        desenharTexto(TextFormat("%s (x%d)", item->nome, item->quantidade),
                      405, posY + 8, 15, corRaridadeItem(item->raridade));
    }
    DrawRectangle(530, 470, 220, 55, (Color){ 45, 125, 70, 255 });
    DrawRectangleLines(530, 470, 220, 55, GOLD);
    desenharTexto("CONTINUAR", 580, 487, 18, RAYWHITE);
}

// Retorna um nome conciso e legível da unidade para exibição clara na barra de iniciativa.
static const char *nomeCurtoIniciativa(const Combatente *ator)
{
    if (ator == NULL) {
        return "";
    }
    if (ator->aliado) {
        return ator->nome;
    }
    if (ator->eh_boss) {
        return "Barba Negra";
    }
    switch (ator->tipo_inimigo) {
    case INIMIGO_GRUNT_ESPADA:
        return "Grunt Espada";
    case INIMIGO_GRUNT_PISTOLA:
        return "Grunt Pistola";
    case INIMIGO_ATIRADOR_MAGRELO:
        return "Atirador Magrelo";
    default:
        return "Marujo Gordo";
    }
}

// Desenha a barra superior da fila de iniciativa com ordem de turnos clara e sem números soltos.
static void desenharFila(const InterfaceJogo *interfaceJogo)
{
    int indice;
    int slotVisivel = 0;
    DrawRectangle(0, 0, LARGURA_VIRTUAL, 62, (Color){ 10, 17, 24, 230 });
    for (indice = 0; indice < interfaceJogo->total_iniciativa && slotVisivel < 9; indice++) {
        Combatente *ator = interfaceJogo->fila_iniciativa[indice].ator;
        int posCardX = posicaoXSlotFila(slotVisivel);
        bool turnoAtivo = indice == interfaceJogo->indice_iniciativa;
        if (ator == NULL || !ator->vivo) {
            continue;
        }
        DrawRectangle(posCardX, 7, 45, 45, ator->aliado ?
                      (Color){ 35, 75, 105, 255 } : (Color){ 115, 55, 40, 255 });
        if (turnoAtivo) {
            DrawRectangleLinesEx((Rectangle){ (float)posCardX, 7.0f, 45.0f, 45.0f },
                                 3.0f, GOLD);
        } else {
            DrawRectangleLines(posCardX, 7, 45, 45, (Color){ 90, 110, 125, 180 });
        }
        desenharTexto(TextFormat("%c", nomeCurtoIniciativa(ator)[0]),
                      posCardX + 16, 18, 19, turnoAtivo ? GOLD : RAYWHITE);
        slotVisivel++;
    }
}

static void desenharHUDAtorAtivo(const InterfaceJogo *interfaceJogo)
{
    const Combatente *ator;
    Rectangle retrato = { 928.0f, 8.0f, 88.0f, 88.0f };
    Rectangle barraHP = { 928.0f, 103.0f, 324.0f, 20.0f };
    Rectangle barraEstamina = { 928.0f, 128.0f, 324.0f, 20.0f };
    float proporcao;
    Color corGrupo;
    if (interfaceJogo == NULL) {
        return;
    }
    ator = interfaceJogo->ator_atual;
    if ((ator == NULL || !ator->vivo) && interfaceJogo->fase_pre_combate_ativa) {
        ator = interfaceJogo->entidade_selecionada;
    }
    DrawUIPanelCustom((Rectangle){ 920.0f, 4.0f, 350.0f, 150.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 245 });
    corGrupo = ator != NULL && !ator->aliado ?
               (Color){ 135, 45, 38, 255 } : (Color){ 35, 75, 105, 255 };
    DrawRectangleRec(converterBoundsVirtual(retrato), corGrupo);
    DrawUIFrameCustom(retrato, interfaceJogo->texFrameRetrato);
    if (ator != NULL) {
        desenharTexto(TextFormat("%c", ator->nome[0] == '\0' ? '?' : ator->nome[0]),
                      962, 34, 30, RAYWHITE);
        desenharTexto(ator->nome, 1028, 12, 16, ator->aliado ? SKYBLUE : ORANGE);
        DrawRectangleRec(converterBoundsVirtual(barraHP), (Color){ 35, 42, 45, 255 });
        proporcao = ator->vida_maxima > 0 ?
                    (float)ator->vida_atual / (float)ator->vida_maxima : 0.0f;
        if (proporcao < 0.0f) {
            proporcao = 0.0f;
        } else if (proporcao > 1.0f) {
            proporcao = 1.0f;
        }
        DrawRectangleRec(converterBoundsVirtual((Rectangle){
            barraHP.x, barraHP.y, barraHP.width * proporcao, barraHP.height
        }), ator->vida_atual > ator->vida_maxima / 3 ? GREEN : RED);
        DrawRectangleLinesEx(barraHP, 1.0f, RAYWHITE);
        DrawRectangleRec(converterBoundsVirtual((Rectangle){
            932.0f, 104.0f, 132.0f, 18.0f
        }), (Color){ 245, 245, 235, 225 });
        desenharTexto(TextFormat("HP: %d/%d", ator->vida_atual, ator->vida_maxima),
                      935, 105, 12, BLACK);
        DrawRectangleRec(converterBoundsVirtual(barraEstamina), (Color){ 35, 42, 45, 255 });
        proporcao = ator->estamina_maxima > 0 ?
                    (float)ator->estamina_atual / (float)ator->estamina_maxima : 0.0f;
        if (proporcao < 0.0f) {
            proporcao = 0.0f;
        } else if (proporcao > 1.0f) {
            proporcao = 1.0f;
        }
        DrawRectangleRec(converterBoundsVirtual((Rectangle){
            barraEstamina.x, barraEstamina.y,
            barraEstamina.width * proporcao, barraEstamina.height
        }), ator->aliado ? GOLD : SKYBLUE);
        DrawRectangleLinesEx(barraEstamina, 1.0f, RAYWHITE);
        DrawRectangleRec(converterBoundsVirtual((Rectangle){
            932.0f, 129.0f, 132.0f, 18.0f
        }), (Color){ 245, 245, 235, 225 });
        desenharTexto(TextFormat("EST: %d/%d",
                                 ator->estamina_atual, ator->estamina_maxima),
                      935, 130, 12, BLACK);
    } else {
        desenharTexto("PRE-COMBATE", 1028, 15, 16, GOLD);
    }
}

// Calcula o alcance máximo de movimento da unidade ativa considerando passivas e itens.
static int obterAlcanceMovimentoUnidade(const Combatente *unidade)
{
    int passoMaximo;
    NoItem *item;
    if (unidade == NULL) {
        return 1;
    }
    passoMaximo = (unidade->aliado && unidade->classe == CLASSE_BUCANEIRO) ? 2 : 1;
    for (item = unidade->build_passivos.inicio; item != NULL; item = item->proximo) {
        if (item->item.id == 8) {
            passoMaximo++;
        }
    }
    return passoMaximo;
}

// Desenha o tabuleiro 3x6 contínuo, zonas de preparação, alcance, minas e unidades.
static void desenharTabuleiro(const InterfaceJogo *interfaceJogo,
                              const Tabuleiro3x6 *mapa, const ListaAlcance *alcance)
{
    int linha;
    int coluna;
    int passoMovimento = obterAlcanceMovimentoUnidade(interfaceJogo->ator_atual);
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            int posX = interfaceJogo->margem_esquerda + coluna * interfaceJogo->largura_celula;
            int posY = interfaceJogo->margem_superior + linha * interfaceJogo->altura_celula;
            Combatente *unidade = mapa->celulas[linha][coluna];
            Color cor = (Color){ 42, 63, 76, 255 };
            const NoCelula *marcada;
            if (interfaceJogo->fase_pre_combate_ativa && coluna <= 1) {
                cor = (Color){ 38, 82, 95, 255 };
            }
            if (unidade == NULL || !unidade->vivo) {
                cor.a = 128;
            }
            DrawRectangle(posX, posY, interfaceJogo->largura_celula - 4,
                          interfaceJogo->altura_celula - 4, cor);
            if (!interfaceJogo->fase_pre_combate_ativa &&
                interfaceJogo->acao_combate == ACAO_TELA_MOVER &&
                interfaceJogo->ator_atual != NULL &&
                interfaceJogo->ator_atual->aliado &&
                unidade == NULL) {
                int distMov = abs(linha - interfaceJogo->ator_atual->linha) +
                              abs(coluna - interfaceJogo->ator_atual->coluna);
                if (distMov >= 1 && distMov <= passoMovimento) {
                    DrawRectangle(posX, posY, interfaceJogo->largura_celula - 4,
                                  interfaceJogo->altura_celula - 4,
                                  (Color){ 55, 165, 95, 120 });
                }
            }
            if (alcance != NULL) {
                for (marcada = alcance->inicio; marcada != NULL; marcada = marcada->proximo) {
                    if (marcada->linha == linha && marcada->coluna == coluna) {
                        DrawRectangle(posX, posY, interfaceJogo->largura_celula - 4,
                                      interfaceJogo->altura_celula - 4,
                                      marcada->perigo_fogo_amigo ?
                                      (Color){ 195, 45, 35, 165 } :
                                      (Color){ 210, 165, 35, 130 });
                    }
                }
            }
            if (interfaceJogo->timer_destaque_acao > 0.0f) {
                if (linha == interfaceJogo->linha_destaque_acao &&
                    coluna == interfaceJogo->coluna_destaque_acao) {
                    DrawRectangle(posX, posY, interfaceJogo->largura_celula - 4,
                                  interfaceJogo->altura_celula - 4,
                                  (Color){ 235, 85, 45, 135 });
                    DrawRectangleLinesEx((Rectangle){ (float)posX, (float)posY,
                                                       (float)interfaceJogo->largura_celula - 4,
                                                       (float)interfaceJogo->altura_celula - 4 },
                                         4.0f, RED);
                } else if (linha == interfaceJogo->linha_origem_acao &&
                           coluna == interfaceJogo->coluna_origem_acao) {
                    DrawRectangleLinesEx((Rectangle){ (float)posX, (float)posY,
                                                       (float)interfaceJogo->largura_celula - 4,
                                                       (float)interfaceJogo->altura_celula - 4 },
                                         3.0f, ORANGE);
                }
            }
            if (mapa->mina_ativa[linha][coluna] > 0) {
                DrawCircle(posX + 86, posY + 100, 11, RED);
                desenharTexto("MINA", posX + 68, posY + 114, 12, RAYWHITE);
            }
            if (unidade != NULL && unidade->vivo) {
                desenharTexto(unidade->nome, posX + 8, posY + 8, 14,
                              unidade->aliado ? SKYBLUE : ORANGE);
                if (unidade->eh_elite) {
                    DrawRectangle(posX + 124, posY + 6, 48, 20, (Color){ 145, 42, 38, 255 });
                    desenharTexto("ELITE", posX + 130, posY + 9, 12, GOLD);
                }
                desenharTexto(TextFormat("PV %d/%d", unidade->vida_atual,
                                         unidade->vida_maxima), posX + 8, posY + 30, 14, RAYWHITE);
                desenharTexto(TextFormat("EST %d/%d", unidade->estamina_atual,
                                         unidade->estamina_maxima), posX + 8, posY + 50, 14, GOLD);
                if (unidade->em_descanso) {
                    desenharTexto("DESCANSANDO", posX + 8, posY + 70, 11, YELLOW);
                }
                desenharIndicadoresStatusUnidade(unidade, posX + 8, posY + 90,
                                                 interfaceJogo->largura_celula - 16);
                if (unidade == interfaceJogo->ator_atual &&
                    !interfaceJogo->fase_pre_combate_ativa) {
                    DrawRectangleLinesEx((Rectangle){ (float)posX, (float)posY,
                                                       (float)interfaceJogo->largura_celula - 4,
                                                       (float)interfaceJogo->altura_celula - 4 },
                                         3.0f, unidade->aliado ? GREEN : ORANGE);
                }
                if (unidade == interfaceJogo->entidade_selecionada) {
                    DrawRectangleLinesEx((Rectangle){ (float)posX + 2, (float)posY + 2,
                                                       (float)interfaceJogo->largura_celula - 8,
                                                       (float)interfaceJogo->altura_celula - 8 },
                                         2.0f, GOLD);
                }
            } else {
                desenharTexto(TextFormat("%d,%d", linha, coluna), posX + 8, posY + 12, 13, LIGHTGRAY);
            }
            if (interfaceJogo->timer_indicador_dano > 0.0f) {
                int danoCelula = mapa->ultimo_dano_celula[linha][coluna];
                int curaCelula = mapa->ultima_cura_celula[linha][coluna];
                if (danoCelula > 0) {
                    DrawRectangle(posX + 84, posY + 28, 86, 26, (Color){ 195, 25, 25, 245 });
                    DrawRectangleLinesEx((Rectangle){ (float)(posX + 84), (float)(posY + 28),
                                                       86.0f, 26.0f }, 2.0f, GOLD);
                    desenharTexto(TextFormat("-%d DANO", danoCelula),
                                  posX + 90, posY + 33, 13, RAYWHITE);
                } else if (curaCelula > 0) {
                    DrawRectangle(posX + 84, posY + 28, 86, 26, (Color){ 30, 155, 70, 245 });
                    DrawRectangleLinesEx((Rectangle){ (float)(posX + 84), (float)(posY + 28),
                                                       86.0f, 26.0f }, 2.0f, RAYWHITE);
                    desenharTexto(TextFormat("+%d PV", curaCelula),
                                  posX + 94, posY + 33, 13, RAYWHITE);
                }
            }
            DrawRectangleLines(posX, posY, interfaceJogo->largura_celula - 4,
                               interfaceJogo->altura_celula - 4,
                               (Color){ 180, 193, 184, 255 });
        }
    }
}

// Desenha a tela de menu principal renderizada em alta nitidez 1080p.
static void desenharMenu(const InterfaceJogo *interfaceJogo)
{
    iniciarQuadroVirtual(interfaceJogo, (Color){ 18, 28, 39, 255 });
    desenharTextoCentralizado("SEA OF GUNPOWDER", 120, 80, GOLD);
    desenharTextoCentralizado("CLIQUE PARA COMEÇAR", 250, 40, RAYWHITE);
    DrawRectangle(470, 350, 340, 70, (Color){ 45, 125, 70, 255 });
    desenharTexto("INICIAR AVENTURA", 525, 374, 22, RAYWHITE);
    DrawRectangle(470, 445, 340, 60, (Color){ 135, 48, 38, 255 });
    DrawRectangleLines(470, 445, 340, 60, GOLD);
    desenharTexto("SAIR DO JOGO", 575, 464, 20, RAYWHITE);
    desenharTexto("Controle 100% pelo Mouse | ESC abre o menu de Pause.",
                  420, 535, 16, LIGHTGRAY);
    finalizarQuadroVirtual();
}

// Desenha o estado atual da interface, o tabuleiro 3x6 e o pop-up modal ativo.
void desenharInterface(const InterfaceJogo *interfaceJogo, const Tabuleiro3x6 *mapa,
                       const ListaAlcance *alcance)
{
    ListaAlcance *alcanceDinamico = NULL;
    const ListaAlcance *alcanceAtivo = alcance;
    if (interfaceJogo == NULL) {
        return;
    }
    interfaceRenderizada = interfaceJogo;
    if (interfaceJogo->estado == TELA_MENU_PRINCIPAL) {
        desenharMenu(interfaceJogo);
        return;
    }
    if (alcanceAtivo == NULL && interfaceJogo->estado == TELA_COMBATE_3X6 && mapa != NULL) {
        if (!interfaceJogo->fase_pre_combate_ativa &&
            interfaceJogo->ator_atual != NULL &&
            interfaceJogo->acao_combate >= ACAO_TELA_HABILIDADE_0 &&
            interfaceJogo->acao_combate <= ACAO_TELA_HABILIDADE_2) {
            int habIndice = interfaceJogo->acao_combate - ACAO_TELA_HABILIDADE_0;
            alcanceDinamico = calcularAreaAtuacao3x6((Tabuleiro3x6 *)mapa,
                                                     interfaceJogo->ator_atual, habIndice);
            alcanceAtivo = alcanceDinamico;
        } else if (interfaceJogo->entidade_selecionada != NULL &&
                   !interfaceJogo->entidade_selecionada->aliado &&
                   interfaceJogo->entidade_selecionada->vivo) {
            alcanceDinamico = calcularAreaAtuacao3x6((Tabuleiro3x6 *)mapa,
                                                     interfaceJogo->entidade_selecionada, 0);
            alcanceAtivo = alcanceDinamico;
        }
    }
    iniciarQuadroVirtual(interfaceJogo, (Color){ 18, 28, 39, 255 });
    if (interfaceJogo->estado == TELA_MAPA_ROTAS) {
        int etapaAtual = (interfaceJogo->rota_atual != NULL &&
                          *interfaceJogo->rota_atual != NULL) ?
                         (*interfaceJogo->rota_atual)->etapa : 0;
        desenharTexto(TextFormat("MAPA DE ROTAS - ETAPA %d DE 3",
                                 etapaAtual < 3 ? etapaAtual + 1 : 3),
                      410, 55, 28, GOLD);
        desenharTexto(TextFormat("Moedas de Ouro: %d | Saques: %d | Desistências: %d",
                                 interfaceJogo->moedas == NULL ? 0 : *interfaceJogo->moedas,
                                 contadorSaques, contadorFugas),
                      390, 95, 18, RAYWHITE);
        if (etapaAtual >= 3) {
            DrawRectangle(480, 340, 380, 100, (Color){ 145, 42, 38, 255 });
            DrawRectangleLines(480, 340, 380, 100, GOLD);
            desenharTexto("ENFRENTAR BARBA NEGRA", 515, 365, 22, GOLD);
            desenharTexto("Chefe Final (3000 HP | 2 Fases | 3 Turnos/Rodada)",
                          500, 400, 14, RAYWHITE);
        } else {
            DrawRectangle(440, 290, 260, 95, (Color){ 50, 88, 125, 255 });
            DrawRectangle(730, 290, 260, 95, (Color){ 70, 112, 75, 255 });
            DrawRectangle(585, 415, 260, 95, (Color){ 115, 77, 42, 255 });
            desenharTexto("CAMINHO 1 (4 Inimigos)", 458, 308, 19, RAYWHITE);
            desenharTexto("+2 Níveis | +700 Ouros", 465, 336, 15, GOLD);
            desenharTexto("+15% Cura | +1 Item", 465, 358, 15, LIGHTGRAY);
            desenharTexto("CAMINHO 2 (3 Inimigos)", 748, 308, 19, RAYWHITE);
            desenharTexto("+1 Nível | +300 Ouros", 755, 336, 15, GOLD);
            desenharTexto("+50% Cura | +2 Itens", 755, 358, 15, LIGHTGRAY);
            desenharTexto("CAMINHO 3: LOJA", 625, 433, 20, GOLD);
            desenharTexto("3 Itens à venda (2 unid.)", 615, 461, 15, RAYWHITE);
            desenharTexto("Saque: inimigos +100% status", 600, 483, 14, LIGHTGRAY);
        }
        DrawRectangle(90, 630, 190, 55,
                      interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS ?
                      (Color){ 65, 115, 135, 255 } : (Color){ 48, 65, 78, 255 });
        DrawRectangle(300, 630, 190, 55,
                      interfaceJogo->popup_ativo == POPUP_CONFIRMAR_FUGA ?
                      (Color){ 175, 65, 50, 255 } : (Color){ 135, 48, 38, 255 });
        DrawRectangle(510, 630, 190, 55,
                      interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM ?
                      (Color){ 65, 115, 135, 255 } : (Color){ 48, 65, 78, 255 });
        desenharTexto("MOCHILA / BUILDS", 104, 648, 16, RAYWHITE);
        desenharTexto(contadorFugas < 2 ? "FUGIR AVENTURA" : "FUGAS 2/2",
                      320, 648, 16, contadorFugas < 2 ? RAYWHITE : GRAY);
        desenharTexto("DETALHES PIRATA", 524, 648, 16, RAYWHITE);
        if (mapa != NULL) {
            int linha;
            for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
                Combatente *pirata = mapa->celulas[linha][0];
                int posCardY = 200 + linha * 100;
                if (pirata != NULL && pirata->vivo) {
                    DrawRectangle(55, posCardY, 340, 82, (Color){ 35, 75, 105, 255 });
                    if (pirata == interfaceJogo->entidade_selecionada) {
                        DrawRectangleLines(55, posCardY, 340, 82, GOLD);
                    }
                    desenharTexto(pirata->nome, 72, posCardY + 8, 18, RAYWHITE);
                    desenharTexto(TextFormat("PV %d/%d | Nível %d | Passivos %d/5",
                                             pirata->vida_atual, pirata->vida_maxima,
                                             pirata->nivel, pirata->build_passivos.tamanho),
                                  72, posCardY + 34, 14, LIGHTGRAY);
                    desenharIndicadoresStatusUnidade(pirata, 72, posCardY + 58, 310);
                }
            }
        }
    } else if (interfaceJogo->estado == TELA_COMBATE_3X6 && mapa != NULL) {
        desenharFila(interfaceJogo);
        desenharHUDAtorAtivo(interfaceJogo);
        if (interfaceJogo->fase_pre_combate_ativa) {
            desenharTexto("PRÉ-COMBATE | Posicione a tripulação nas colunas 0 e 1.",
                          480, 10, 14, GOLD);
            desenharTexto("Selecione uma unidade para ver seus detalhes.",
                          480, 34, 13, LIGHTGRAY);
        } else {
            desenharTexto(TextFormat("Turno: %s",
                                     interfaceJogo->ator_atual == NULL ?
                                     "aguardando" : interfaceJogo->ator_atual->nome),
                          480, 10, 14,
                          (interfaceJogo->ator_atual != NULL &&
                           !interfaceJogo->ator_atual->aliado) ? ORANGE : GOLD);
            DrawRectangleRec(converterBoundsVirtual((Rectangle){
                700.0f, 5.0f, 180.0f, 40.0f
            }), (Color){ 25, 38, 48, 245 });
            DrawRectangleLinesEx((Rectangle){ 700.0f, 5.0f, 180.0f, 40.0f },
                                 2.0f, GOLD);
            desenharTexto(TextFormat("AÇÕES: %d",
                                     interfaceJogo->ator_atual == NULL ?
                                     0 : interfaceJogo->ator_atual->acoes_basicas_restantes),
                          716, 12, 19, GOLD);
            if (interfaceJogo->log_combate[0] != '\0') {
                desenharTexto(interfaceJogo->log_combate, 90, 578, 12, RAYWHITE);
            }
            if (mapa->boss_fase_2_ativa) {
                int gruntsVivos = 0;
                int linha;
                int coluna;
                for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
                    for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                        Combatente *unidade = mapa->celulas[linha][coluna];
                        if (unidade != NULL && unidade->vivo &&
                            unidade->tipo_inimigo == INIMIGO_GRUNT_ESPADA &&
                            (unidade->id == 110 || unidade->id == 111)) {
                            gruntsVivos++;
                        }
                    }
                }
                if (gruntsVivos > 0) {
                    DrawRectangle(480, 70, 410, 30, (Color){ 145, 42, 38, 245 });
                    DrawRectangleLines(480, 70, 410, 30, GOLD);
                    desenharTexto(TextFormat("BARBA NEGRA IMUNE: derrote os Grunts! Restantes: %d",
                                             gruntsVivos),
                                  490, 77, 12, RAYWHITE);
                }
            }
        }
        desenharTabuleiro(interfaceJogo, mapa, alcanceAtivo);
        DrawRectangle(70, 600, 1170, 102, (Color){ 12, 20, 29, 235 });
        if (interfaceJogo->fase_pre_combate_ativa) {
            desenharTexto("Clique nos piratas para posicionar/trocar antes de iniciar.",
                          90, 578, 12, LIGHTGRAY);
            DrawUIButtonCustom((Rectangle){ 90.0f, 630.0f, 260.0f, 55.0f },
                               "INICIAR COMBATE", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            DrawUIButtonCustom((Rectangle){ 370.0f, 630.0f, 190.0f, 55.0f },
                               "MOCHILA / ITENS", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            DrawUIButtonCustom((Rectangle){ 975.0f, 630.0f, 130.0f, 55.0f },
                               "DETALHES", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            if (interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS) {
                DrawRectangleLines(370, 630, 190, 55, GOLD);
            }
            if (interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM) {
                DrawRectangleLines(975, 630, 130, 55, GOLD);
            }
        } else {
            int habDesbloq = interfaceJogo->ator_atual != NULL ?
                             interfaceJogo->ator_atual->habilidades_desbloqueadas : 1;
            if (interfaceJogo->ator_atual != NULL && interfaceJogo->ator_atual->aliado) {
                if (interfaceJogo->acao_combate == ACAO_TELA_MOVER) {
                    desenharTexto("MOVER: Clique em uma casa verde ortogonal vazia (custo: 1 Estamina).",
                                  90, 608, 14, SKYBLUE);
                } else if (interfaceJogo->acao_combate >= ACAO_TELA_HABILIDADE_0 &&
                           interfaceJogo->acao_combate <= ACAO_TELA_HABILIDADE_2) {
                    int idxHab = interfaceJogo->acao_combate - ACAO_TELA_HABILIDADE_0;
                    char cabecalhoCompacto[160];
                    char textoBarra[512];
                    formatarCabecalhoHabilidade(interfaceJogo->ator_atual, idxHab, true,
                                                cabecalhoCompacto, sizeof(cabecalhoCompacto));
                    (void)snprintf(textoBarra, sizeof(textoBarra), "%s: %s",
                                   cabecalhoCompacto,
                                   descricaoHabilidadeDetalhada(interfaceJogo->ator_atual, idxHab));
                    (void)desenharTextoQuebrado(textoBarra, 90, 601, 1135, 12, 14, GOLD, 600, 629);
                } else {
                    desenharTexto("Dica: Escolha uma ação abaixo ou clique em DETALHES para ver todas as descrições.",
                                  90, 608, 14, LIGHTGRAY);
                }
            }
            DrawUIButtonCustom((Rectangle){ 90.0f, 630.0f, 130.0f, 55.0f },
                               "MOVER (1)", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            DrawUIButtonCustom((Rectangle){ 235.0f, 630.0f, 130.0f, 55.0f },
                               "DESCANSAR", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            DrawUIButtonCustom((Rectangle){ 380.0f, 630.0f, 130.0f, 55.0f },
                               "HABILIDADE 1", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            DrawUIButtonCustom((Rectangle){ 525.0f, 630.0f, 130.0f, 55.0f },
                               habDesbloq >= 2 ? "HABILIDADE 2" : NULL,
                               interfaceJogo->texBtnPadrao, obterFonteInterface());
            DrawUIButtonCustom((Rectangle){ 670.0f, 630.0f, 130.0f, 55.0f },
                               habDesbloq >= 3 ? "ULTIMATE" : NULL,
                               interfaceJogo->texBtnPadrao, obterFonteInterface());
            DrawUIButtonCustom((Rectangle){ 815.0f, 630.0f, 150.0f, 55.0f },
                               "USAR ITEM", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            DrawUIButtonCustom((Rectangle){ 975.0f, 630.0f, 130.0f, 55.0f },
                               "DETALHES", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            if (habDesbloq < 2) {
                DrawRectangle(525, 630, 130, 55, (Color){ 10, 14, 18, 130 });
                desenharTexto("HAB 2 (Nv3)", 536, 648, 15, GRAY);
            }
            if (habDesbloq < 3) {
                DrawRectangle(670, 630, 130, 55, (Color){ 10, 14, 18, 130 });
                desenharTexto("ULT (Nv6)", 684, 648, 15, GRAY);
            }
            if (interfaceJogo->acao_combate == ACAO_TELA_MOVER) {
                DrawRectangleLines(90, 630, 130, 55, GOLD);
            } else if (interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_0) {
                DrawRectangleLines(380, 630, 130, 55, GOLD);
            } else if (interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_1) {
                DrawRectangleLines(525, 630, 130, 55, GOLD);
            } else if (interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_2) {
                DrawRectangleLines(670, 630, 130, 55, GOLD);
            }
            if (interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS) {
                DrawRectangleLines(815, 630, 150, 55, GOLD);
            }
            if (interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM) {
                DrawRectangleLines(975, 630, 130, 55, GOLD);
            }
        }
    } else if (interfaceJogo->estado == TELA_VITORIA) {
        desenharTexto("VITÓRIA! BARBA NEGRA FOI DERROTADO!", 265, 250, 32, GOLD);
        desenharTexto("A tripulação conquistou os mares em Sea Of Gunpowder!",
                      330, 315, 20, RAYWHITE);
        DrawRectangle(470, 440, 340, 65, (Color){ 45, 125, 70, 255 });
        desenharTexto("NOVA AVENTURA", 540, 462, 22, RAYWHITE);
    } else if (interfaceJogo->estado == TELA_FIM_DE_JOGO) {
        desenharTexto("FIM DE JOGO - TRIPULAÇÃO DERROTADA", 290, 250, 32, ORANGE);
        desenharTexto("Todos os seus piratas tombaram em combate.", 420, 315, 20, RAYWHITE);
        DrawRectangle(470, 440, 340, 65, (Color){ 45, 125, 70, 255 });
        desenharTexto("TENTAR NOVAMENTE", 518, 462, 22, RAYWHITE);
    }
    if (interfaceJogo->popup_ativo != POPUP_NENHUM) {
        if (interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS) {
            desenharMochila(interfaceJogo);
        } else if (interfaceJogo->popup_ativo == POPUP_LOJA) {
            desenharLoja(interfaceJogo);
        } else if (interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM) {
            desenharDetalhes(interfaceJogo, interfaceJogo->entidade_selecionada);
        } else if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_FUGA) {
            desenharConfirmacaoFuga(interfaceJogo);
        } else if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_ROTA) {
            desenharConfirmacaoEscolha(interfaceJogo, false);
        } else if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_COMPRA) {
            desenharConfirmacaoEscolha(interfaceJogo, true);
        } else if (interfaceJogo->popup_ativo == POPUP_RECOMPENSAS) {
            desenharRecompensas(interfaceJogo);
        } else if (interfaceJogo->popup_ativo == POPUP_PAUSE) {
            desenharMenuPause();
        }
    }
    finalizarQuadroVirtual();
    if (alcanceDinamico != NULL) {
        liberarEstruturasJogo(NULL, NULL, alcanceDinamico);
    }
}

// Libera os recursos visuais da interface e encerra a janela Raylib.
void descarregarInterface(InterfaceJogo *interfaceJogo)
{
    if (interfaceJogo != NULL) {
        int indice;
        if (interfaceJogo->bgMar.id != 0) {
            UnloadTexture(interfaceJogo->bgMar);
            interfaceJogo->bgMar = (Texture2D){ 0 };
        }
        for (indice = 0; indice < 5; indice++) {
            if (interfaceJogo->bgCombatePool[indice].id != 0 &&
                interfaceJogo->bgCombatePool[indice].id != interfaceJogo->bgNavio.id) {
                UnloadTexture(interfaceJogo->bgCombatePool[indice]);
            }
            interfaceJogo->bgCombatePool[indice] = (Texture2D){ 0 };
        }
        if (interfaceJogo->bgNavio.id != 0) {
            UnloadTexture(interfaceJogo->bgNavio);
            interfaceJogo->bgNavio = (Texture2D){ 0 };
        }
        if (interfaceJogo->bgBoss.id != 0) {
            UnloadTexture(interfaceJogo->bgBoss);
            interfaceJogo->bgBoss = (Texture2D){ 0 };
        }
        if (interfaceJogo->texBtnPadrao.id != 0) {
            UnloadTexture(interfaceJogo->texBtnPadrao);
            interfaceJogo->texBtnPadrao = (Texture2D){ 0 };
        }
        if (interfaceJogo->texFrameRetrato.id != 0) {
            UnloadTexture(interfaceJogo->texFrameRetrato);
            interfaceJogo->texFrameRetrato = (Texture2D){ 0 };
        }
        if (interfaceJogo->texPainelDetalhes.id != 0) {
            UnloadTexture(interfaceJogo->texPainelDetalhes);
            interfaceJogo->texPainelDetalhes = (Texture2D){ 0 };
        }
        if (interfaceJogo->texPainelMochila.id != 0) {
            UnloadTexture(interfaceJogo->texPainelMochila);
            interfaceJogo->texPainelMochila = (Texture2D){ 0 };
        }
        interfaceJogo->qtdBgCombate = 0;
        interfaceJogo->bgCombateAtualIdx = -1;
        interfaceJogo->bgBossAtivo = false;
        if (interfaceJogo->fonteJogo.texture.id != 0) {
            UnloadFont(interfaceJogo->fonteJogo);
            interfaceJogo->fonteJogo = (Font){ 0 };
        }
        if (interfaceRenderizada == interfaceJogo) {
            interfaceRenderizada = NULL;
        }
    }
    if (canvasCarregado && canvasVirtual.id != 0) {
        UnloadRenderTexture(canvasVirtual);
        canvasCarregado = false;
    }
    CloseWindow();
}
