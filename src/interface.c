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
#define CHARACTER_VISUAL_SCALE 1.15f
#define CHARACTER_BASE_HEIGHT_RATIO 0.80f
#define CHARACTER_STATUS_WIDTH_RATIO 0.86f
#define CHARACTER_BASE_STATUS_HEIGHT 46.0f
#define CHARACTER_BASE_FONT_SIZE 9.0f
#define CHARACTER_BASE_NAME_HEIGHT 13.0f
#define CHARACTER_VERTICAL_OFFSET -15.0f
#define CHARACTER_NAME_GAP 4.0f
#define CHARACTER_STATUS_GAP 2.0f
#define BOTTOM_SCREEN_MARGIN 12.0f

enum {
    SPRITE_PIRATA_BASE,
    SPRITE_PIRATA_DESCANSO,
    SPRITE_PIRATA_ACAO_0,
    SPRITE_PIRATA_ACAO_1,
    SPRITE_PIRATA_ACAO_2,
    SPRITE_PIRATA_CAPITAO_TIRO,
    SPRITE_PIRATA_COZINHEIRO_COMENDO,
    TOTAL_POSES_SPRITES_PIRATA
};

static const InterfaceJogo *interfaceRenderizada;
static InterfaceJogo *gerenciadorFundoCombate;
static RenderTexture2D canvasVirtual;
static bool canvasCarregado = false;

static void registrarPoseAcaoUnidade(InterfaceJogo *interfaceJogo,
                                     Combatente *unidade, int indiceHabilidade,
                                     int linhaAlvo, int colunaAlvo);
static void desenharSpriteUnidade(Texture2D textura, Rectangle bounds);
static void desenharTextoCentralizadoContornado(const char *texto, Rectangle bounds,
                                                int tamanho, Color cor);
static void desenharBarraStatusCombatente(const InterfaceJogo *interfaceJogo,
                                          const Combatente *unidade,
                                          Rectangle bounds, int tamanhoTexto);

void SelectCombatBackground(bool isBoss)
{
    int indice;
    if (gerenciadorFundoCombate == NULL) {
        return;
    }
    gerenciadorFundoCombate->bgBossAtivo = isBoss;
    if (isBoss) {
        gerenciadorFundoCombate->bgCombateAtualIdx = -1;
        return;
    }
    if (gerenciadorFundoCombate->qtdBgCombate <= 0) {
        gerenciadorFundoCombate->bgCombateAtualIdx = -1;
        return;
    }
    indice = GetRandomValue(0, gerenciadorFundoCombate->qtdBgCombate - 1);
    if (gerenciadorFundoCombate->qtdBgCombate > 1 &&
        indice == gerenciadorFundoCombate->bgCombateAnteriorIdx) {
        indice = GetRandomValue(0, gerenciadorFundoCombate->qtdBgCombate - 2);
        if (indice >= gerenciadorFundoCombate->bgCombateAnteriorIdx) {
            indice++;
        }
    }
    gerenciadorFundoCombate->bgCombateAtualIdx = indice;
    gerenciadorFundoCombate->bgCombateAnteriorIdx = indice;
}

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
                   interfaceJogo->bgCombateAtualIdx < 6) {
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
        interfaceJogo->bgCombateAtualIdx < 6) {
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

static void desenharTexturaCapaTint(Texture2D textura, Rectangle bounds, Color tint)
{
    Rectangle origem;
    float propTex;
    float propBounds;
    if (textura.id == 0 || textura.width <= 0 || textura.height <= 0 ||
        bounds.width <= 0.0f || bounds.height <= 0.0f) {
        return;
    }
    propTex = (float)textura.width / (float)textura.height;
    propBounds = bounds.width / bounds.height;
    if (propTex > propBounds) {
        float largVisivel = (float)textura.height * propBounds;
        origem = (Rectangle){ ((float)textura.width - largVisivel) * 0.5f, 0.0f,
                              largVisivel, (float)textura.height };
    } else {
        float altVisivel = (float)textura.width / propBounds;
        origem = (Rectangle){ 0.0f, ((float)textura.height - altVisivel) * 0.5f,
                              (float)textura.width, altVisivel };
    }
    DrawTexturePro(textura, origem, converterBoundsVirtual(bounds),
                   (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

static NPatchInfo configurarNPatchRec(Texture2D textura, Rectangle recorte,
                                      int bordaX, int bordaY)
{
    NPatchInfo info = { 0 };
    if (textura.id == 0) {
        return info;
    }
    if (recorte.width <= 0.0f || recorte.height <= 0.0f) {
        recorte = (Rectangle){ 0.0f, 0.0f, (float)textura.width,
                               (float)textura.height };
    }
    info.source = recorte;
    info.left = bordaX < (int)(recorte.width * 0.45f) ?
                bordaX : (int)(recorte.width * 0.45f);
    info.right = info.left;
    info.top = bordaY < (int)(recorte.height * 0.45f) ?
               bordaY : (int)(recorte.height * 0.45f);
    info.bottom = info.top;
    info.layout = NPATCH_NINE_PATCH;
    return info;
}

static NPatchInfo obterInfoNPatch(Texture2D textura)
{
    if (interfaceRenderizada != NULL) {
        if (textura.id == interfaceRenderizada->texBtnPadrao.id) {
            return interfaceRenderizada->nPatchBotao;
        }
        if (textura.id == interfaceRenderizada->texFrameRetrato.id) {
            return interfaceRenderizada->nPatchFrameRetrato;
        }
        if (textura.id == interfaceRenderizada->texPainelDetalhes.id) {
            return interfaceRenderizada->nPatchPainelDescricao;
        }
    }
    return (NPatchInfo){ 0 };
}

static void desenharNPatch(Texture2D textura, NPatchInfo info,
                           Rectangle bounds, Color tint)
{
    Rectangle src = info.source;
    Rectangle dst = converterBoundsVirtual(bounds);
    float sL = (float)info.left;
    float sR = (float)info.right;
    float sT = (float)info.top;
    float sB = (float)info.bottom;
    float maxEscalaX;
    float maxEscalaY;
    float escala;
    float dL;
    float dR;
    float dT;
    float dB;
    float sx[4];
    float sy[4];
    float dx[4];
    float dy[4];
    int lin;
    int col;
    if (textura.id == 0 || bounds.width <= 0.0f || bounds.height <= 0.0f) {
        return;
    }
    if (src.width <= 0.0f || src.height <= 0.0f) {
        desenharTexturaBounds(textura, bounds);
        return;
    }
    maxEscalaX = (sL + sR) > 0.0f ? (dst.width * 0.44f) / (sL + sR) : 0.72f;
    maxEscalaY = (sT + sB) > 0.0f ? (dst.height * 0.44f) / (sT + sB) : 0.72f;
    escala = maxEscalaX < maxEscalaY ? maxEscalaX : maxEscalaY;
    if (escala > 0.72f) {
        escala = 0.72f;
    }
    dL = sL * escala;
    dR = sR * escala;
    dT = sT * escala;
    dB = sB * escala;
    sx[0] = src.x;
    sx[1] = src.x + sL;
    sx[2] = src.x + src.width - sR;
    sx[3] = src.x + src.width;
    sy[0] = src.y;
    sy[1] = src.y + sT;
    sy[2] = src.y + src.height - sB;
    sy[3] = src.y + src.height;
    dx[0] = dst.x;
    dx[1] = dst.x + dL;
    dx[2] = dst.x + dst.width - dR;
    dx[3] = dst.x + dst.width;
    dy[0] = dst.y;
    dy[1] = dst.y + dT;
    dy[2] = dst.y + dst.height - dB;
    dy[3] = dst.y + dst.height;
    for (lin = 0; lin < 3; lin++) {
        for (col = 0; col < 3; col++) {
            Rectangle recOrigem = {
                sx[col], sy[lin], sx[col + 1] - sx[col], sy[lin + 1] - sy[lin]
            };
            Rectangle recDestino = {
                dx[col], dy[lin], dx[col + 1] - dx[col], dy[lin + 1] - dy[lin]
            };
            if (recOrigem.width > 0.0f && recOrigem.height > 0.0f &&
                recDestino.width > 0.0f && recDestino.height > 0.0f) {
                DrawTexturePro(textura, recOrigem, recDestino,
                               (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
            }
        }
    }
}

static Color obterTintBotao(bool isHovered, bool isPressed)
{
    if (isPressed) {
        return (Color){ 205, 205, 205, 255 };
    }
    if (isHovered) {
        return (Color){ 255, 242, 205, 255 };
    }
    return WHITE;
}

void DrawCharacterPortrait(Texture2D portrait, Rectangle bounds)
{
    if (portrait.id != 0) {
        float margemX = bounds.width * 0.07f;
        float margemY = bounds.height * 0.07f;
        Rectangle areaSprite = {
            bounds.x + margemX,
            bounds.y + margemY,
            bounds.width - margemX * 2.0f,
            bounds.height - margemY * 2.0f
        };
        desenharSpriteUnidade(portrait, areaSprite);
    }
    if (interfaceRenderizada != NULL) {
        desenharNPatch(interfaceRenderizada->texFrameRetrato,
                       interfaceRenderizada->nPatchFrameRetrato, bounds, WHITE);
    }
}

void DrawCustomButton(Rectangle bounds, const char *label, bool isHovered, bool isPressed)
{
    Texture2D botao = interfaceRenderizada != NULL ?
                      interfaceRenderizada->texBtnPadrao : (Texture2D){ 0 };
    Font fonte = obterFonteInterface();
    float tamanhoFonte = bounds.height * 0.30f;
    Rectangle render = converterBoundsVirtual(bounds);
    if (tamanhoFonte > 20.0f) {
        tamanhoFonte = 20.0f;
    } else if (tamanhoFonte < 13.0f) {
        tamanhoFonte = 13.0f;
    }
    if (botao.id != 0) {
        desenharNPatch(botao, interfaceRenderizada->nPatchBotao, bounds,
                       obterTintBotao(isHovered, isPressed));
    } else {
        DrawRectangleRec(render, isPressed ? (Color){ 34, 49, 59, 255 } :
                         isHovered ? (Color){ 58, 84, 99, 255 } :
                         (Color){ 42, 63, 76, 255 });
        DrawRectangleLinesEx(bounds, 2.0f, (Color){ 125, 153, 170, 255 });
    }
    if (label != NULL) {
        Vector2 tamanhoTexto;
        Vector2 posTexto;
        float larguraMaxRender = render.width - 26.0f;
        if (fonte.texture.id == 0) {
            fonte = GetFontDefault();
        }
        tamanhoTexto = MeasureTextEx(fonte, label, tamanhoFonte * 1.5f, 1.5f);
        if (larguraMaxRender > 20.0f && tamanhoTexto.x > larguraMaxRender) {
            tamanhoFonte *= larguraMaxRender / tamanhoTexto.x;
            tamanhoTexto = MeasureTextEx(fonte, label, tamanhoFonte * 1.5f, 1.5f);
        }
        posTexto = (Vector2){
            render.x + (render.width - tamanhoTexto.x) / 2.0f,
            render.y + (render.height - tamanhoTexto.y) / 2.0f
        };
        DrawTextEx(fonte, label, (Vector2){ posTexto.x + 1.5f, posTexto.y + 1.5f },
                   tamanhoFonte * 1.5f, 1.5f, (Color){ 6, 10, 14, 220 });
        DrawTextEx(fonte, label, posTexto,
                   tamanhoFonte * 1.5f, 1.5f, isHovered ? GOLD : RAYWHITE);
    }
}

void DrawUIButtonCustom(Rectangle bounds, const char *text, Texture2D texBotao, Font font)
{
    Vector2 mouse = obterMouseVirtual();
    if (interfaceRenderizada != NULL &&
        texBotao.id == interfaceRenderizada->texBtnPadrao.id) {
        DrawCustomButton(bounds, text, CheckCollisionPointRec(mouse, bounds),
                         CheckCollisionPointRec(mouse, bounds) &&
                         IsMouseButtonDown(MOUSE_BUTTON_LEFT));
        return;
    }
    if (texBotao.id != 0) {
        desenharNPatch(texBotao, obterInfoNPatch(texBotao), bounds, WHITE);
    } else {
        DrawRectangleRec(converterBoundsVirtual(bounds), (Color){ 42, 63, 76, 255 });
        DrawRectangleLinesEx(bounds, 2.0f, (Color){ 125, 153, 170, 255 });
    }
    if (text != NULL) {
        Vector2 tamanhoTexto;
        Rectangle boundsRender = converterBoundsVirtual(bounds);
        float tamanhoFonte = bounds.height * 0.30f;
        if (tamanhoFonte > 20.0f) {
            tamanhoFonte = 20.0f;
        }
        if (font.texture.id == 0) {
            font = GetFontDefault();
        }
        tamanhoTexto = MeasureTextEx(font, text, tamanhoFonte * 1.5f, 1.5f);
        DrawTextEx(font, text,
                   (Vector2){ boundsRender.x + (boundsRender.width - tamanhoTexto.x) / 2.0f,
                              boundsRender.y + (boundsRender.height - tamanhoTexto.y) / 2.0f },
                   tamanhoFonte * 1.5f, 1.5f, RAYWHITE);
    }
}

void DrawUIPanelCustom(Rectangle bounds, Texture2D texPainel, Color colorFallback)
{
    Rectangle boundsRender = converterBoundsVirtual(bounds);
    if (texPainel.id != 0) {
        desenharNPatch(texPainel, obterInfoNPatch(texPainel), bounds, WHITE);
    } else {
        DrawRectangleRec(boundsRender, colorFallback);
        DrawRectangleLinesEx(bounds, 1.5f, (Color){ 90, 110, 125, 210 });
    }
}

void DrawUIFrameCustom(Rectangle bounds, Texture2D texBorda)
{
    if (texBorda.id != 0) {
        desenharNPatch(texBorda, obterInfoNPatch(texBorda), bounds, WHITE);
    } else {
        DrawRectangleLinesEx(bounds, 2.0f, GOLD);
    }
}

static float medirTextoVirtual(Font fonte, const char *texto, float tamanhoFonte)
{
    return MeasureTextEx(fonte, texto, tamanhoFonte * 1.5f, 1.5f).x / 1.5f;
}

static void copiarTextoLimitado(char *destino, size_t capacidade, const char *origem)
{
    size_t tamanho;
    if (destino == NULL || capacidade == 0) {
        return;
    }
    tamanho = origem == NULL ? 0 : strlen(origem);
    if (tamanho >= capacidade) {
        tamanho = capacidade - 1;
    }
    if (tamanho > 0) {
        (void)memcpy(destino, origem, tamanho);
    }
    destino[tamanho] = '\0';
}

static int montarLinhasTexto(const char *texto, Font fonte, float tamanhoFonte,
                             float larguraMaxima, char linhas[][512],
                             float larguras[], int capacidade)
{
    char linha[512] = "";
    size_t indice = 0;
    int total = 0;
    if (texto == NULL || texto[0] == '\0' || capacidade <= 0) {
        return 0;
    }
    while (texto[indice] != '\0' && total < capacidade) {
        char palavra[512];
        size_t tamanhoPalavra = 0;
        char candidata[1024];
        while (texto[indice] == ' ' || texto[indice] == '\t') {
            indice++;
        }
        if (texto[indice] == '\n') {
            if (linha[0] != '\0') {
                (void)snprintf(linhas[total], sizeof(linhas[total]), "%s", linha);
                larguras[total++] = medirTextoVirtual(fonte, linha, tamanhoFonte);
                linha[0] = '\0';
            }
            indice++;
            continue;
        }
        if (texto[indice] == '\0') {
            break;
        }
        while (texto[indice] != '\0' && texto[indice] != ' ' &&
               texto[indice] != '\t' && texto[indice] != '\n' &&
               tamanhoPalavra < sizeof(palavra) - 1) {
            palavra[tamanhoPalavra++] = texto[indice++];
        }
        palavra[tamanhoPalavra] = '\0';
        (void)snprintf(candidata, sizeof(candidata), "%s%s%s", linha,
                       linha[0] == '\0' ? "" : " ", palavra);
        if (linha[0] != '\0' &&
            medirTextoVirtual(fonte, candidata, tamanhoFonte) > larguraMaxima) {
            (void)snprintf(linhas[total], sizeof(linhas[total]), "%s", linha);
            larguras[total++] = medirTextoVirtual(fonte, linha, tamanhoFonte);
            linha[0] = '\0';
            (void)snprintf(candidata, sizeof(candidata), "%s", palavra);
        }
        if (medirTextoVirtual(fonte, candidata, tamanhoFonte) > larguraMaxima &&
            linha[0] == '\0') {
            size_t posicao = 0;
            char chunk[512] = "";
            while (posicao < tamanhoPalavra && total < capacidade) {
                unsigned char byte = (unsigned char)palavra[posicao];
                size_t bytesCodigo = byte < 0x80 ? 1 :
                                     (byte & 0xE0) == 0xC0 ? 2 :
                                     (byte & 0xF0) == 0xE0 ? 3 :
                                     (byte & 0xF8) == 0xF0 ? 4 : 1;
                char proximo[1024];
                if (posicao + bytesCodigo > tamanhoPalavra) {
                    bytesCodigo = 1;
                }
                (void)snprintf(proximo, sizeof(proximo), "%s%.*s",
                               chunk, (int)bytesCodigo, palavra + posicao);
                if (chunk[0] != '\0' &&
                    medirTextoVirtual(fonte, proximo, tamanhoFonte) > larguraMaxima) {
                    (void)snprintf(linhas[total], sizeof(linhas[total]), "%s", chunk);
                    larguras[total++] = medirTextoVirtual(fonte, chunk, tamanhoFonte);
                    chunk[0] = '\0';
                    (void)snprintf(proximo, sizeof(proximo), "%.*s",
                                   (int)bytesCodigo, palavra + posicao);
                }
                copiarTextoLimitado(chunk, sizeof(chunk), proximo);
                posicao += bytesCodigo;
            }
            if (chunk[0] != '\0' && total < capacidade) {
                (void)snprintf(linhas[total], sizeof(linhas[total]), "%s", chunk);
                larguras[total++] = medirTextoVirtual(fonte, chunk, tamanhoFonte);
            }
        } else {
            copiarTextoLimitado(linha, sizeof(linha), candidata);
        }
    }
    if (linha[0] != '\0' && total < capacidade) {
        (void)snprintf(linhas[total], sizeof(linhas[total]), "%s", linha);
        larguras[total++] = medirTextoVirtual(fonte, linha, tamanhoFonte);
    }
    return total;
}

void DrawDynamicDescriptionBox(Vector2 pos, float maxWidth,
                               const char *title, const char *body)
{
    char linhasTitulo[64][512] = { { 0 } };
    char linhasCorpo[64][512] = { { 0 } };
    float largurasTitulo[64] = { 0.0f };
    float largurasCorpo[64] = { 0.0f };
    Font fonte = obterFonteInterface();
    const float padding = 9.0f;
    const float tamanhoTitulo = 18.0f;
    const float tamanhoCorpo = 14.0f;
    const float alturaTitulo = 22.0f;
    const float alturaCorpo = 18.0f;
    float larguraTextoMaxima;
    float larguraConteudo = 0.0f;
    float alturaConteudo = padding * 2.0f;
    float larguraPainel;
    int totalTitulo;
    int totalCorpo;
    int indice;
    float y;
    if (maxWidth <= padding * 2.0f) {
        return;
    }
    larguraTextoMaxima = maxWidth - padding * 2.0f;
    totalTitulo = montarLinhasTexto(title, fonte, tamanhoTitulo, larguraTextoMaxima,
                                    linhasTitulo, largurasTitulo, 64);
    totalCorpo = montarLinhasTexto(body, fonte, tamanhoCorpo, larguraTextoMaxima,
                                   linhasCorpo, largurasCorpo, 64);
    for (indice = 0; indice < totalTitulo; indice++) {
        if (largurasTitulo[indice] > larguraConteudo) {
            larguraConteudo = largurasTitulo[indice];
        }
    }
    for (indice = 0; indice < totalCorpo; indice++) {
        if (largurasCorpo[indice] > larguraConteudo) {
            larguraConteudo = largurasCorpo[indice];
        }
    }
    alturaConteudo += (float)totalTitulo * alturaTitulo;
    if (totalTitulo > 0 && totalCorpo > 0) {
        alturaConteudo += 3.0f;
    }
    alturaConteudo += (float)totalCorpo * alturaCorpo;
    larguraPainel = larguraConteudo + padding * 2.0f;
    if (larguraPainel < 2.0f * padding) {
        larguraPainel = 2.0f * padding;
    }
    DrawUIPanelCustom((Rectangle){ pos.x, pos.y, larguraPainel, alturaConteudo },
                      interfaceRenderizada == NULL ? (Texture2D){ 0 } :
                      interfaceRenderizada->texPainelDetalhes,
                      (Color){ 12, 19, 27, 245 });
    y = pos.y + padding;
    for (indice = 0; indice < totalTitulo; indice++) {
        desenharTextoGlobal(linhasTitulo[indice],
                            (Vector2){ pos.x + padding, y }, tamanhoTitulo, GOLD);
        y += alturaTitulo;
    }
    if (totalTitulo > 0 && totalCorpo > 0) {
        y += 3.0f;
    }
    for (indice = 0; indice < totalCorpo; indice++) {
        desenharTextoGlobal(linhasCorpo[indice],
                            (Vector2){ pos.x + padding, y }, tamanhoCorpo, RAYWHITE);
        y += alturaCorpo;
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

static int posicaoYSlotBuild(int indice)
{
    return 432 + indice * 34;
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
    SelectCombatBackground((*interfaceJogo->rota_atual)->tipo == ROTA_CHEFE);
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

static bool prepararLojaOpcional(InterfaceJogo *interfaceJogo, NoRota *rotaAtual)
{
    NoRota *noLoja;
    unsigned int semente;
    if (interfaceJogo == NULL || rotaAtual == NULL || rotaAtual->loja == NULL ||
        rotaAtual->loja->tipo != ROTA_LOJA) {
        return false;
    }
    noLoja = rotaAtual->loja;
    if (interfaceJogo->etapa_loja_atual == noLoja->etapa &&
        interfaceJogo->total_itens_loja == 3) {
        return true;
    }
    semente = (unsigned int)(noLoja->etapa * 97 +
                             contadorFugas * 31 + contadorSaques * 19 +
                             (interfaceJogo->moedas != NULL ?
                              *interfaceJogo->moedas : 0) +
                             GetRandomValue(1, 10000));
    interfaceJogo->total_itens_loja =
        sortearItensLoja(noLoja->etapa, semente, interfaceJogo->itens_loja,
                         interfaceJogo->estoques_loja, 3);
    if (interfaceJogo->total_itens_loja != 3) {
        TraceLog(LOG_ERROR, "Não foi possível gerar o estoque da loja opcional.");
        return false;
    }
    interfaceJogo->etapa_loja_atual = noLoja->etapa;
    return true;
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
            interfaceJogo->estado = TELA_FIM_DE_JOGO;
            return;
        }
        interfaceJogo->etapa_loja_atual = 0;
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
        registrarPoseAcaoUnidade(interfaceJogo, inimigo, escolha.indice_habilidade,
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
    interfaceJogo->etapa_loja_atual = 1;
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
    interfaceJogo->etapa_loja_atual = 1;
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

static Texture2D carregarTexturaBackground(const char *nomeArquivo)
{
    char caminho[160];
    if (nomeArquivo == NULL) {
        return (Texture2D){ 0 };
    }
    (void)snprintf(caminho, sizeof(caminho), "assets/backgrounds/%s", nomeArquivo);
    if (!FileExists(caminho)) {
        (void)snprintf(caminho, sizeof(caminho), "assets/textures/%s", nomeArquivo);
    }
    return carregarTexturaOpcional(caminho);
}

// Seleciona a pose visual do pirata ou Grunt conforme seu estado atual.
static Texture2D obterSpriteUnidade(const InterfaceJogo *interfaceJogo,
                                   const Combatente *unidade)
{
    int indiceSprite;
    int pose = SPRITE_PIRATA_BASE;
    Texture2D textura;
    if (interfaceJogo == NULL || unidade == NULL) {
        return (Texture2D){ 0 };
    }
    if (unidade->aliado) {
        if (unidade->classe < CLASSE_BUCANEIRO || unidade->classe > CLASSE_GAVEA) {
            return (Texture2D){ 0 };
        }
        indiceSprite = (int)unidade->classe;
        if (unidade->em_descanso) {
            pose = SPRITE_PIRATA_DESCANSO;
        } else if (interfaceJogo->unidade_em_animacao == unidade &&
                   interfaceJogo->timer_pose_acao_unidade > 0.0f &&
                   interfaceJogo->pose_acao_sprite_unidade >= SPRITE_PIRATA_ACAO_0 &&
                   interfaceJogo->pose_acao_sprite_unidade < TOTAL_POSES_SPRITES_PIRATA) {
            pose = interfaceJogo->pose_acao_sprite_unidade;
        }
        textura = interfaceJogo->spritesPiratas[indiceSprite][pose];
        if (textura.id == 0 && pose != SPRITE_PIRATA_BASE) {
            textura = interfaceJogo->spritesPiratas[indiceSprite][SPRITE_PIRATA_BASE];
        }
        return textura;
    }

    if (unidade->tipo_inimigo < INIMIGO_GRUNT_ESPADA ||
        unidade->tipo_inimigo > INIMIGO_MARUJO_GORDO) {
        return (Texture2D){ 0 };
    }
    indiceSprite = (int)unidade->tipo_inimigo;
    if (unidade->em_descanso) {
        pose = SPRITE_PIRATA_DESCANSO;
    } else if (interfaceJogo->unidade_em_animacao == unidade &&
               interfaceJogo->timer_pose_acao_unidade > 0.0f &&
               interfaceJogo->pose_acao_sprite_unidade >= SPRITE_PIRATA_ACAO_0 &&
               interfaceJogo->pose_acao_sprite_unidade <= SPRITE_PIRATA_ACAO_1) {
        pose = interfaceJogo->pose_acao_sprite_unidade;
    }
    textura = interfaceJogo->spritesInimigos[indiceSprite][pose];
    if (textura.id == 0 && pose != SPRITE_PIRATA_BASE) {
        textura = interfaceJogo->spritesInimigos[indiceSprite][SPRITE_PIRATA_BASE];
    }
    return textura;
}

// Os cards da seleção sempre representam piratas, mesmo quando aliado está falso no estado da run.
static Texture2D obterSpriteBasePirata(const InterfaceJogo *interfaceJogo,
                                       const Combatente *pirata)
{
    if (interfaceJogo == NULL || pirata == NULL ||
        pirata->classe < CLASSE_BUCANEIRO || pirata->classe > CLASSE_GAVEA) {
        return (Texture2D){ 0 };
    }
    return interfaceJogo->spritesPiratas[(int)pirata->classe][SPRITE_PIRATA_BASE];
}

// Mantém a pose da habilidade no tabuleiro por um segundo.
static void registrarPoseAcaoUnidade(InterfaceJogo *interfaceJogo,
                                     Combatente *unidade, int indiceHabilidade,
                                     int linhaAlvo, int colunaAlvo)
{
    int pose;
    if (interfaceJogo == NULL || unidade == NULL || indiceHabilidade < 0) {
        return;
    }
    if (unidade->aliado) {
        if (unidade->classe < CLASSE_BUCANEIRO || unidade->classe > CLASSE_GAVEA ||
            indiceHabilidade >= 3) {
            return;
        }
        pose = SPRITE_PIRATA_ACAO_0 + indiceHabilidade;
        if (unidade->classe == CLASSE_CAPITAO && indiceHabilidade == 0 &&
            abs(unidade->linha - linhaAlvo) + abs(unidade->coluna - colunaAlvo) == 2) {
            pose = SPRITE_PIRATA_CAPITAO_TIRO;
        } else if (unidade->classe == CLASSE_COZINHEIRO && indiceHabilidade == 0 &&
                   unidade->linha == linhaAlvo && unidade->coluna == colunaAlvo) {
            pose = SPRITE_PIRATA_COZINHEIRO_COMENDO;
        }
    } else {
        switch (unidade->tipo_inimigo) {
        case INIMIGO_GRUNT_ESPADA:
        case INIMIGO_ATIRADOR_MAGRELO:
            if (indiceHabilidade != 0) {
                return;
            }
            pose = SPRITE_PIRATA_ACAO_0;
            break;
        case INIMIGO_GRUNT_PISTOLA:
        case INIMIGO_MARUJO_GORDO:
            if (indiceHabilidade > 1) {
                return;
            }
            pose = SPRITE_PIRATA_ACAO_0 + indiceHabilidade;
            break;
        default:
            return;
        }
    }
    interfaceJogo->unidade_em_animacao = unidade;
    interfaceJogo->pose_acao_sprite_unidade = pose;
    interfaceJogo->timer_pose_acao_unidade = 1.0f;
}

// Desenha a ilustração em coordenadas virtuais sem distorcer sua proporção.
static void desenharSpriteUnidade(Texture2D textura, Rectangle bounds)
{
    float proporcaoTextura;
    float proporcaoBounds;
    Rectangle destino = bounds;
    if (textura.id == 0 || textura.width <= 0 || textura.height <= 0 ||
        bounds.width <= 0.0f || bounds.height <= 0.0f) {
        return;
    }
    proporcaoTextura = (float)textura.width / (float)textura.height;
    proporcaoBounds = bounds.width / bounds.height;
    if (proporcaoTextura > proporcaoBounds) {
        destino.height = bounds.width / proporcaoTextura;
        destino.y += (bounds.height - destino.height) * 0.5f;
    } else {
        destino.width = bounds.height * proporcaoTextura;
        destino.x += (bounds.width - destino.width) * 0.5f;
    }
    desenharTexturaBounds(textura, destino);
}

// Inicializa a janela em 1920x1080 (1080p), o framebuffer 1080p e a fonte TrueType de alta nitidez.
void inicializarInterface(InterfaceJogo *interfaceJogo)
{
    static const char *arquivosSpritesInimigos[4][4] = {
        { "gruntEspadaBase.png", "gruntEspadaDescanso.png",
          "gruntEspadaCorte.png", NULL },
        { "gruntPistolaBase.png", "gruntPistolaDescanso.png",
          "gruntPistolaTiro.png", "gruntPistolaGranada.png" },
        { "AtiradorMagreloBase.png", "AtiradorMagreloDescanso.png",
          "AtiradorMagreloTiro.png", NULL },
        { "MarujoGordoBase.png", "MarujoGordoDescanso.png",
          "MarujoGordoTiro.png", "MarujoGordoCura.png" }
    };
    static const char *arquivosSpritesPiratas[5][TOTAL_POSES_SPRITES_PIRATA] = {
        { "bucaneiroBase.png", "bucaneiroDescanso.png", "bucaneiroCorte.png",
          "bucaneiroPocketSand.png", "bucaneiroUlt.png", NULL, NULL },
        { "Timoneiro.png", "TimoneiroDescanso.png", "TimoneiroSocao.png",
          "TimoneiroSocao.png", "TimoneiroTimonada.png", NULL, NULL },
        { "cozinheiroBase.png", "cozinheiroDescanso.png",
          "cozinheiroArremessando.png", "cozinheiroTiro.png", "cozinheiroUlt.png",
          NULL, "cozinheiroComendo.png" },
        { "capitaoBase.png", "capitaoDescanso.png", "capitaoCorte.png",
          "capitaoMotivar.png", "capitaoUlt.png", "capitaoTiro.png", NULL },
        { "gaveaBase.png", "gaveaDescanso.png", "gaveaTiro.png",
          "gaveaSoprada.png", "gaveaUlt.png", NULL, NULL }
    };
    int codepoints[224];
    int indice;
    int pose;
    int fundosCarregados = 0;
    static const char *arquivosBackgroundCombate[6] = {
        "bg_navio.png",
        "bg_praia.png",
        "bg_floresta.png",
        "bg_ruinas.png",
        "bg_ilha_sombria.png",
        "bg_cidade_pirata.png"
    };
    const char *diretorioWindows;
    char caminhoFonte[512];
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
    interfaceJogo->bgCombateAnteriorIdx = -1;
    interfaceJogo->coluna_origem_acao = -1;
    interfaceJogo->largura_celula = 180;
    interfaceJogo->altura_celula = 135;
    interfaceJogo->margem_esquerda = 100;
    interfaceJogo->margem_superior = 155;
    interfaceJogo->pirata_posicionando_indice = -1;
    interfaceJogo->indice_iniciativa = -1;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_VIRTUAL, ALTURA_VIRTUAL, "Sea Of Gunpowder");
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
    gerenciadorFundoCombate = interfaceJogo;
    for (indice = 0; indice < 5; indice++) {
        for (pose = 0; pose < TOTAL_POSES_SPRITES_PIRATA; pose++) {
            char caminhoSprite[160];
            if (arquivosSpritesPiratas[indice][pose] == NULL) {
                interfaceJogo->spritesPiratas[indice][pose] = (Texture2D){ 0 };
                continue;
            }
            (void)snprintf(caminhoSprite, sizeof(caminhoSprite), "assets/sprites/%s",
                           arquivosSpritesPiratas[indice][pose]);
            interfaceJogo->spritesPiratas[indice][pose] =
                carregarTexturaOpcional(caminhoSprite);
        }
    }
    for (indice = 0; indice < 4; indice++) {
        for (pose = 0; pose < 4; pose++) {
            char caminhoSprite[160];
            if (arquivosSpritesInimigos[indice][pose] == NULL) {
                interfaceJogo->spritesInimigos[indice][pose] = (Texture2D){ 0 };
                continue;
            }
            (void)snprintf(caminhoSprite, sizeof(caminhoSprite), "assets/sprites/%s",
                           arquivosSpritesInimigos[indice][pose]);
            interfaceJogo->spritesInimigos[indice][pose] =
                carregarTexturaOpcional(caminhoSprite);
        }
    }
    for (indice = 0; indice < 224; indice++) {
        codepoints[indice] = 32 + indice;
    }
    for (indice = 0; indice < 6; indice++) {
        interfaceJogo->bgCombatePool[fundosCarregados] =
            carregarTexturaBackground(arquivosBackgroundCombate[indice]);
        if (interfaceJogo->bgCombatePool[fundosCarregados].id != 0) {
            fundosCarregados++;
        }
    }
    interfaceJogo->qtdBgCombate = fundosCarregados;
    interfaceJogo->bgBoss = carregarTexturaBackground("bg_boss_navio.png");
    interfaceJogo->texBtnPadrao =
        carregarTexturaOpcional("assets/ui/botao_ui.png");
    interfaceJogo->texFrameRetrato =
        carregarTexturaOpcional("assets/ui/frame_personagem.png");
    interfaceJogo->texPainelDetalhes =
        carregarTexturaOpcional("assets/ui/painel_descricao.png");
    interfaceJogo->texBarraStatusUnificada =
        carregarTexturaOpcional("assets/ui/barra_status_unificada.png");
    if (interfaceJogo->texBarraStatusUnificada.id == 0) {
        TraceLog(LOG_ERROR,
                 "Nao foi possivel carregar assets/ui/barra_status_unificada.png.");
    }
    interfaceJogo->nPatchBotao =
        configurarNPatchRec(interfaceJogo->texBtnPadrao,
                            (Rectangle){ 10.0f, 8.0f, 614.0f, 128.0f }, 52, 48);
    interfaceJogo->nPatchFrameRetrato =
        configurarNPatchRec(interfaceJogo->texFrameRetrato,
                            (Rectangle){ 10.0f, 8.0f, 474.0f, 482.0f }, 82, 82);
    interfaceJogo->nPatchPainelDescricao =
        configurarNPatchRec(interfaceJogo->texPainelDetalhes,
                            (Rectangle){ 10.0f, 6.0f, 326.0f, 526.0f }, 48, 48);
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
    if (interfaceJogo->timer_pose_acao_unidade > 0.0f) {
        interfaceJogo->timer_pose_acao_unidade -= deltaTempo;
        if (interfaceJogo->timer_pose_acao_unidade <= 0.0f) {
            interfaceJogo->timer_pose_acao_unidade = 0.0f;
            interfaceJogo->unidade_em_animacao = NULL;
            interfaceJogo->pose_acao_sprite_unidade = -1;
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
                {
                    NoRota *proximaRota = avancarParaRota(*rotaAtual, escolha);
                    if (proximaRota == NULL) {
                        TraceLog(LOG_ERROR, "A rota escolhida não possui uma próxima etapa.");
                        return;
                    }
                    *rotaAtual = proximaRota;
                }
                interfaceJogo->saque_em_andamento = false;
                iniciarEncontro(interfaceJogo, mapa, piratasDisponiveis,
                                totalDisponiveis);
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
                if (rotaAtual == NULL || *rotaAtual == NULL ||
                    (*rotaAtual)->loja == NULL ||
                    (*rotaAtual)->loja->tipo != ROTA_LOJA) {
                    TraceLog(LOG_WARNING, "Não há uma rota de loja para saquear nesta etapa.");
                    return;
                }
                *rotaAtual = (*rotaAtual)->loja;
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
                        int posSlotY = posicaoYSlotBuild(itemIndice);
                        Rectangle linhaItem = { 180.0f, (float)posSlotY,
                                                650.0f, 31.0f };
                        if (retanguloClicado(linhaItem, mouse) &&
                            (interfaceJogo->estado == TELA_MAPA_ROTAS ||
                             interfaceJogo->fase_pre_combate_ativa)) {
                            if (!desequiparItemPassivoBuild(
                                    inventario, alvoUso, item->item.id)) {
                                TraceLog(LOG_WARNING,
                                         "Não foi possível devolver o passivo %s à Mochila.",
                                         item->item.nome);
                            }
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
            if ((*rotaAtual)->etapa == 3 &&
                retanguloClicado((Rectangle){ 510, 470, 300, 80 }, mouse) &&
                prepararLojaOpcional(interfaceJogo, *rotaAtual)) {
                interfaceJogo->loja_aberta_pendente = true;
                interfaceJogo->popup_ativo = POPUP_LOJA;
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
                rotaAtual != NULL && *rotaAtual != NULL) {
                if (prepararLojaOpcional(interfaceJogo, *rotaAtual)) {
                    interfaceJogo->loja_aberta_pendente = true;
                    interfaceJogo->popup_ativo = POPUP_LOJA;
                }
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
                    registrarPoseAcaoUnidade(interfaceJogo, interfaceJogo->ator_atual,
                                             habilidade, linha, coluna);
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
    Texture2D texPainel = interfaceRenderizada != NULL ?
                          interfaceRenderizada->texPainelDetalhes : (Texture2D){ 0 };
    Texture2D texBotao = interfaceRenderizada != NULL ?
                         interfaceRenderizada->texBtnPadrao : (Texture2D){ 0 };
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 6, 10, 15, 200 });
    DrawUIPanelCustom((Rectangle){ 420.0f, 190.0f, 440.0f, 290.0f },
                      texPainel, (Color){ 15, 24, 34, 252 });
    desenharTextoCentralizadoContornado("JOGO PAUSADO",
                                        (Rectangle){ 420.0f, 218.0f, 440.0f, 36.0f },
                                        26, GOLD);
    desenharTextoCentralizado("Pressione ESC para retornar à partida", 260, 14, LIGHTGRAY);
    DrawUIButtonCustom((Rectangle){ 490.0f, 300.0f, 300.0f, 58.0f },
                       "RETORNAR", texBotao, obterFonteInterface());
    DrawRectangleLines(490, 300, 300, 58, (Color){ 80, 210, 115, 220 });
    DrawUIButtonCustom((Rectangle){ 490.0f, 380.0f, 300.0f, 58.0f },
                       "VOLTAR AO MENU", texBotao, obterFonteInterface());
    DrawRectangleLines(490, 380, 300, 58, (Color){ 215, 80, 65, 220 });
}

// Desenha a ficha completa de status, passivas, ataques e build com suporte ao scroll do mouse.
static void desenharDetalhes(const InterfaceJogo *interfaceJogo, const Combatente *unidade)
{
    int indice;
    int offsetY;
    int posY;
    int barraY;
    int minY = 114;
    int maxY = 584;
    if (unidade == NULL || interfaceJogo == NULL) {
        return;
    }
    offsetY = interfaceJogo->scroll_detalhes;
    DrawUIPanelCustom((Rectangle){ 160.0f, 50.0f, 740.0f, 555.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 252 });
    DrawRectangle(176, 112, 696, 474, (Color){ 12, 20, 28, 190 });
    posY = 120 - offsetY;
    if (posY >= minY && posY + 24 <= maxY) {
        desenharTexto(unidade->nome, 188, posY, 22, unidade->aliado ? SKYBLUE : ORANGE);
    }
    posY += 28;
    posY += desenharTextoQuebrado(descricaoResumoPersonagem(unidade), 188, posY, 674,
                                  14, 18, GOLD, minY, maxY) + 8;
    if (posY >= minY && posY + 16 <= maxY) {
        if (unidade->eh_boss) {
            desenharTexto(TextFormat("PV %d/%d | Estamina %d/%d | Fase %d | Velocidades: 8, 6 e 4",
                                     unidade->vida_atual, unidade->vida_maxima,
                                     unidade->estamina_atual, unidade->estamina_maxima,
                                     unidade->fase_boss),
                          188, posY, 15, RAYWHITE);
        } else {
            desenharTexto(TextFormat("PV %d/%d | Estamina %d/%d | Nível %d | Velocidade %d | Ações/Turno: %d",
                                     unidade->vida_atual, unidade->vida_maxima,
                                     unidade->estamina_atual, unidade->estamina_maxima,
                                     unidade->nivel, unidade->velocidade,
                                     calcularAcoesPorTurno(unidade)),
                          188, posY, 15, RAYWHITE);
        }
    }
    posY += 22;
    if (posY >= minY && posY + 15 <= maxY) {
        if (unidade->aliado && unidade->classe == CLASSE_COZINHEIRO) {
            desenharTexto("Cura Garantida (sem erro) | Dano do Bacamarte varia conforme a distância",
                          188, posY, 14, LIGHTGRAY);
        } else {
            desenharTexto(TextFormat("Chance Dano Máx (Precisão): Melee %d%% | Ranged %d%% | Bônus Nv +%d%%",
                                     unidade->precisao_melee, unidade->precisao_ranged,
                                     unidade->dano_base),
                          188, posY, 14, LIGHTGRAY);
        }
    }
    posY += 24;
    if (posY >= minY && posY + 16 <= maxY) {
        desenharTexto("EFEITOS ATIVOS (BUFFS / DEBUFFS):", 188, posY, 15, GOLD);
        desenharIndicadoresStatusUnidade(unidade, 448, posY - 2, 410);
    }
    posY += 26;
    if (posY >= minY && posY + 16 <= maxY) {
        desenharTexto("PASSIVAS E COMPORTAMENTO", 188, posY, 16, GOLD);
    }
    posY += 22;
    if (unidade->aliado) {
        posY += desenharTextoQuebrado(descricaoPassiva(unidade->classe, false), 192, posY,
                                      670, 14, 18, RAYWHITE, minY, maxY) + 6;
        posY += desenharTextoQuebrado(descricaoPassiva(unidade->classe, true), 192, posY,
                                      670, 14, 18,
                                      unidade->passiva_aprimorada ? GREEN : GRAY,
                                      minY, maxY) + 10;
    } else {
        if (unidade->eh_elite) {
            posY += desenharTextoQuebrado(
                "ELITE: +25% de vida, +3 de estamina e 3 ações por turno.",
                192, posY, 670, 14, 18, GOLD, minY, maxY) + 6;
        }
        posY += desenharTextoQuebrado(descricaoPassivaInimigo(unidade), 192, posY,
                                      670, 14, 18, RAYWHITE, minY, maxY) + 10;
    }
    if (posY >= minY && posY + 16 <= maxY) {
        desenharTexto("HABILIDADES E AÇÕES", 188, posY, 16, GOLD);
    }
    posY += 22;
    for (indice = 0; indice < MAX_HABILIDADES; indice++) {
        const Habilidade *habilidade = &unidade->habilidades[indice];
        bool desbloqueada = indice < unidade->habilidades_desbloqueadas;
        char cabecalho[192];
        int alturaCard = 84;
        int topoVisivel;
        int baseVisivel;
        if (habilidade->nome[0] == '\0') {
            continue;
        }
        formatarCabecalhoHabilidade(unidade, indice, false, cabecalho, sizeof(cabecalho));
        topoVisivel = posY < minY ? minY : posY;
        baseVisivel = (posY + alturaCard) > maxY ? maxY : (posY + alturaCard);
        if (baseVisivel > topoVisivel) {
            DrawRectangle(185, topoVisivel, 678, baseVisivel - topoVisivel,
                          desbloqueada ? (Color){ 22, 36, 48, 245 } :
                                         (Color){ 18, 24, 30, 235 });
            DrawRectangle(185, topoVisivel, 4, baseVisivel - topoVisivel,
                          desbloqueada ? GOLD : DARKGRAY);
            DrawRectangleLines(185, topoVisivel, 678, baseVisivel - topoVisivel,
                               desbloqueada ? (Color){ 185, 155, 85, 220 } : DARKGRAY);
        }
        if (posY + 4 >= minY && posY + 18 <= maxY) {
            desenharTexto(cabecalho, 195, posY + 4, 13, desbloqueada ? GOLD : GRAY);
        }
        (void)desenharTextoQuebrado(descricaoHabilidadeDetalhada(unidade, indice),
                                    195, posY + 23, 660, 13, 17,
                                    desbloqueada ? RAYWHITE : GRAY, minY, maxY);
        posY += alturaCard + 8;
    }
    if (unidade->aliado) {
        posY += 4;
        if (posY >= minY && posY + 16 <= maxY) {
            desenharTexto(TextFormat("BUILD PASSIVA EQUIPADA (%d/%d)",
                                     unidade->build_passivos.tamanho,
                                     MAX_ITENS_PASSIVOS), 188, posY, 16, GOLD);
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
                posY += desenharTextoQuebrado(textoSlot, 192, posY, 668, 14, 18,
                                              corItem, minY, maxY) + 6;
                slot++;
            }
        }
    }
    DrawRectangle(175, 62, 710, 50, (Color){ 18, 30, 42, 255 });
    DrawRectangle(175, 111, 710, 2, GOLD);
    desenharTexto("FICHA DE INSPEÇÃO DO PERSONAGEM", 185, 72, 20, GOLD);
    desenharTexto("Scroll: rolar ficha", 575, 78, 13, LIGHTGRAY);
    DrawUIButtonCustom((Rectangle){ 760.0f, 68.0f, 85.0f, 38.0f },
                       "FECHAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
    DrawRectangle(876, 116, 8, 468, (Color){ 24, 36, 48, 255 });
    barraY = 116 + (offsetY * 388) / 420;
    DrawRectangle(876, barraY, 8, 80, GOLD);
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
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 8, 16, 26, 110 });
    DrawUIPanelCustom((Rectangle){ 170.0f, 18.0f, 940.0f, 108.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 14, 24, 34, 245 });
    desenharTextoCentralizadoContornado(
        contadorFugas > 0 ? "REPOSIÇÃO DE TRIPULAÇÃO PÓS-FUGA" :
                            "ESCOLHA SUA TRIPULAÇÃO PIRATA",
        (Rectangle){ 170.0f, 30.0f, 940.0f, 38.0f }, 28, GOLD);
    desenharTextoCentralizado(
        contadorFugas > 0 ?
        TextFormat("Selecione substitutos para os caídos (%d/3) — Mortos voltaram ao Nv 1 sem passivos",
                   interfaceJogo->total_selecionados) :
        TextFormat("Selecione 3 piratas para embarcar na expedição (%d/3 selecionados)",
                   interfaceJogo->total_selecionados),
        80, 17, RAYWHITE);
    for (indice = 0; indice < limite; indice++) {
        int cardX = 60 + (indice % 3) * 390;
        int cardY = 145 + (indice / 3) * 205;
        bool selecionado = indicePirataSelecionado(interfaceJogo,
                                                   piratasDisponiveis[indice].id) >= 0;
        bool sobreviventeFixo = ehSobreviventeFixoFuga(interfaceJogo,
                                                       piratasDisponiveis[indice].id);
        Rectangle retratoRect = { (float)(cardX + 14), (float)(cardY + 18), 76.0f, 112.0f };
        DrawUIPanelCustom((Rectangle){ (float)cardX, (float)cardY, 350.0f, 180.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 24, 40, 52, 248 });
        if (selecionado) {
            DrawRectangle(cardX + 6, cardY + 6, 338, 168, (Color){ 45, 115, 75, 45 });
            DrawRectangleLinesEx((Rectangle){ (float)cardX + 2.0f, (float)cardY + 2.0f,
                                              346.0f, 176.0f },
                                 3.0f, sobreviventeFixo ? SKYBLUE : (Color){ 95, 230, 130, 255 });
        }
        DrawRectangle((int)retratoRect.x + 2, (int)retratoRect.y + 2,
                      (int)retratoRect.width - 4, (int)retratoRect.height - 4,
                      selecionado ? (Color){ 32, 68, 92, 255 } : (Color){ 20, 32, 44, 255 });
        DrawCharacterPortrait(obterSpriteBasePirata(interfaceJogo,
                                                    &piratasDisponiveis[indice]),
                              retratoRect);
        desenharTexto(piratasDisponiveis[indice].nome, cardX + 100, cardY + 15, 20,
                      selecionado ? GOLD : RAYWHITE);
        if (piratasDisponiveis[indice].classe == CLASSE_COZINHEIRO) {
            desenharTexto(TextFormat("HP %d | Cura Garantida | Vel %d",
                                     piratasDisponiveis[indice].vida_maxima,
                                     piratasDisponiveis[indice].velocidade),
                          cardX + 100, cardY + 45, 13, RAYWHITE);
        } else {
            desenharTexto(TextFormat("HP %d | Precisão %d%% | Vel %d",
                                     piratasDisponiveis[indice].vida_maxima,
                                     piratasDisponiveis[indice].precisao_melee,
                                     piratasDisponiveis[indice].velocidade),
                          cardX + 100, cardY + 45, 13, RAYWHITE);
        }
        desenharTexto(TextFormat("Estamina %d | Nível %d | Passivos %d/5",
                                 piratasDisponiveis[indice].estamina_maxima,
                                 piratasDisponiveis[indice].nivel,
                                 piratasDisponiveis[indice].build_passivos.tamanho),
                      cardX + 100, cardY + 66, 13, LIGHTGRAY);
        desenharBarraStatusCombatente(interfaceJogo, &piratasDisponiveis[indice],
                                      (Rectangle){ (float)(cardX + 100),
                                                   (float)(cardY + 89),
                                                   234.0f, 40.0f }, 10);
        DrawRectangle(cardX + 14, cardY + 138, 206, 32,
                      selecionado ? (Color){ 28, 62, 46, 225 } : (Color){ 18, 28, 38, 215 });
        DrawRectangleLines(cardX + 14, cardY + 138, 206, 32,
                           sobreviventeFixo ? SKYBLUE :
                           (selecionado ? (Color){ 95, 230, 130, 255 } : DARKGRAY));
        desenharTexto(sobreviventeFixo ? "SOBREVIVENTE MANTIDO" :
                      (selecionado ? "SELECIONADO NA TRIPULAÇÃO" : "Clique para selecionar"),
                      cardX + 22, cardY + 146, 12,
                      sobreviventeFixo ? SKYBLUE : (selecionado ? GOLD : LIGHTGRAY));
        DrawUIButtonCustom((Rectangle){ (float)(cardX + 230), (float)(cardY + 138),
                                        108.0f, 32.0f },
                           "DETALHES", interfaceJogo->texBtnPadrao, obterFonteInterface());
    }
    DrawUIButtonCustom((Rectangle){ 440.0f, 590.0f, 400.0f, 60.0f },
                       contadorFugas > 0 ? "RETOMAR AVENTURA" : "INICIAR AVENTURA",
                       interfaceJogo->texBtnPadrao, obterFonteInterface());
    if (interfaceJogo->total_selecionados == 3) {
        DrawRectangleLinesEx((Rectangle){ 440.0f, 590.0f, 400.0f, 60.0f },
                             3.0f, (Color){ 95, 230, 130, 255 });
    } else {
        DrawRectangle(440, 590, 400, 60, (Color){ 10, 14, 20, 135 });
    }
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
    DrawUIPanelCustom((Rectangle){ 160.0f, 50.0f, 700.0f, 565.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 252 });
    desenharTexto("MOCHILA DA TRIPULAÇÃO E BUILDS PASSIVAS", 180, 68, 19, GOLD);
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
                      abaAtiva ? (Color){ 42, 88, 122, 255 } : (Color){ 22, 34, 46, 255 });
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
            DrawRectangle(180, posLinhaY, 650, 44,
                          selecionadoItem ? (Color){ 38, 58, 74, 255 } :
                                            (Color){ 20, 32, 44, 250 });
            DrawRectangle(180, posLinhaY, 5, 44, selecionadoItem ? GOLD : corRaridade);
            DrawRectangleLines(180, posLinhaY, 650, 44,
                               selecionadoItem ? GOLD : corRaridade);
            desenharTexto(TextFormat("[%s | %s] %s (x%d)",
                                     item->item.tipo == ITEM_CONSUMIVEL ?
                                     (selecionadoItem ? "SELECIONADO" : "CONSUMÍVEL") :
                                     "EQUIPAR",
                                     nomeRaridadeItem(item->item.raridade),
                                     item->item.nome, item->item.quantidade),
                          192, posLinhaY + 3, 13, corRaridade);
            (void)desenharTextoQuebrado(descricaoItemCatalogo(item->item.id),
                                        192, posLinhaY + 18, 630, 12, 14,
                                        RAYWHITE, 174, 368);
        }
        indice++;
    }
    if (indice == 0) {
        DrawRectangle(180, 195, 650, 55, (Color){ 20, 32, 44, 245 });
        DrawRectangleLines(180, 195, 650, 55, GRAY);
        desenharTexto("Mochila vazia! Conquiste itens vencendo rotas ou comprando na Loja.",
                      205, 214, 15, LIGHTGRAY);
    }
    desenharTexto(interfaceJogo->item_id_selecionado > 0 ?
                  "Consumível selecionado pronto para uso no pirata da aba ativa:" :
                  "Clique em um consumível da lista acima para habilitar o botão USAR:",
                  180, 380, 13,
                  interfaceJogo->item_id_selecionado > 0 ? GOLD : LIGHTGRAY);
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
            int posSlotY = posicaoYSlotBuild(slot);
            if (passivo != NULL) {
                Color corPassivo = corRaridadeItem(passivo->item.raridade);
                DrawRectangle(180, posSlotY, 650, 31, (Color){ 22, 38, 42, 255 });
                DrawRectangle(180, posSlotY, 5, 31, corPassivo);
                DrawRectangleLines(180, posSlotY, 650, 31, corPassivo);
                desenharTexto(TextFormat("[Slot %d - %s] %s",
                                         slot + 1,
                                         nomeRaridadeItem(passivo->item.raridade),
                                         passivo->item.nome),
                              192, posSlotY + 2, 12, corPassivo);
                (void)desenharTextoQuebrado(descricaoItemCatalogo(passivo->item.id),
                                            192, posSlotY + 16, 630, 11, 13,
                                            RAYWHITE, posSlotY, posSlotY + 31);
                passivo = passivo->proximo;
            } else {
                DrawRectangle(180, posSlotY, 650, 31, (Color){ 16, 26, 34, 255 });
                DrawRectangleLines(180, posSlotY, 650, 31, DARKGRAY);
                desenharTexto(TextFormat("[Slot %d Vazio - Disponível para item passivo]",
                                         slot + 1),
                              192, posSlotY + 9, 12, GRAY);
            }
        }
    }
}

// Desenha o pop-up comercial com os 3 itens sorteados coloridos por raridade.
static void desenharLoja(const InterfaceJogo *interfaceJogo)
{
    int indice;
    DrawUIPanelCustom((Rectangle){ 160.0f, 55.0f, 700.0f, 545.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 252 });
    desenharTexto("LOJA DO PORTO — Compras não avançam a rota", 185, 76, 19, GOLD);
    DrawUIButtonCustom((Rectangle){ 740.0f, 72.0f, 105.0f, 38.0f },
                       "FECHAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
    DrawRectangle(185, 120, 650, 36, (Color){ 18, 30, 42, 235 });
    DrawRectangleLines(185, 120, 650, 36, (Color){ 165, 135, 75, 200 });
    desenharTexto(TextFormat("Moedas de Ouro: %d   |   Saques realizados: %d (Inflação +%d%%)",
                             interfaceJogo->moedas == NULL ? 0 : *interfaceJogo->moedas,
                             contadorSaques, contadorSaques * 25),
                  200, 130, 15, RAYWHITE);
    for (indice = 0; indice < interfaceJogo->total_itens_loja; indice++) {
        const Item *item = &interfaceJogo->itens_loja[indice];
        int estoque = interfaceJogo->estoques_loja[indice];
        int preco = aplicarInflacaoLoja(item->preco);
        int posItemY = 175 + indice * 92;
        Color corRaridade = corRaridadeItem(item->raridade);
        Color corFundo = estoque > 0 ? (Color){ 22, 36, 48, 252 } :
                                       (Color){ 28, 30, 34, 240 };
        DrawRectangle(190, posItemY, 640, 78, corFundo);
        DrawRectangle(190, posItemY, 6, 78, estoque > 0 ? corRaridade : GRAY);
        DrawRectangleLines(190, posItemY, 640, 78, estoque > 0 ? corRaridade : GRAY);
        desenharTexto(TextFormat("[%s] %s   |   Preço: %d ouros   |   Estoque: %d/2",
                                 nomeRaridadeItem(item->raridade),
                                 item->nome, preco, estoque),
                      206, posItemY + 8, 16, estoque > 0 ? corRaridade : GRAY);
        (void)desenharTextoQuebrado(descricaoItemCatalogo(item->id), 206, posItemY + 32,
                                    612, 13, 17, estoque > 0 ? RAYWHITE : DARKGRAY, 0, 0);
    }
    DrawUIButtonCustom((Rectangle){ 185.0f, 495.0f, 200.0f, 55.0f },
                       "MOCHILA / USAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
    DrawUIButtonCustom((Rectangle){ 400.0f, 495.0f, 215.0f, 55.0f },
                       "SAQUEAR LOJA", interfaceJogo->texBtnPadrao, obterFonteInterface());
    DrawRectangleLines(400, 495, 215, 55, (Color){ 215, 75, 60, 225 });
    DrawUIButtonCustom((Rectangle){ 630.0f, 495.0f, 195.0f, 55.0f },
                       "SEGUIR VIAGEM", interfaceJogo->texBtnPadrao, obterFonteInterface());
    DrawRectangleLines(630, 495, 195, 55, (Color){ 85, 210, 115, 225 });
}

// Desenha o pop-up modal de confirmação de desistência ("Fugir da Aventura").
static void desenharConfirmacaoFuga(const InterfaceJogo *interfaceJogo)
{
    DrawUIPanelCustom((Rectangle){ 210.0f, 200.0f, 620.0f, 295.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 252 });
    desenharTexto("Fugir da Aventura (Retornar à Rota 1)?", 295, 228, 22, GOLD);
    desenharTexto(TextFormat("Fugas usadas: %d/2 | Ouro mantido: %d",
                             contadorFugas,
                             interfaceJogo->moedas == NULL ? 0 : *interfaceJogo->moedas),
                  300, 262, 15, GOLD);
    desenharTexto("Piratas vivos mantêm nível e itens equipados nas builds.", 255, 294, 15, RAYWHITE);
    desenharTexto("Piratas mortos podem ser reescolhidos no Nv 1 (sem passivos).", 255, 322, 15, SKYBLUE);
    desenharTexto(contadorFugas == 0 ?
                  "Itens da mochila serão perdidos; inimigos recebem +50% de status." :
                  "Itens da mochila serão perdidos; inimigos recebem +75% de status.",
                  255, 350, 15, ORANGE);
    DrawUIButtonCustom((Rectangle){ 350.0f, 420.0f, 150.0f, 50.0f },
                       contadorFugas < 2 ? "FUGIR" : "LIMITE 2/2",
                       interfaceJogo->texBtnPadrao, obterFonteInterface());
    if (contadorFugas >= 2) {
        DrawRectangle(350, 420, 150, 50, (Color){ 10, 14, 18, 150 });
    } else {
        DrawRectangleLines(350, 420, 150, 50, (Color){ 215, 75, 60, 225 });
    }
    DrawUIButtonCustom((Rectangle){ 530.0f, 420.0f, 150.0f, 50.0f },
                       "CANCELAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
}

static void desenharConfirmacaoEscolha(const InterfaceJogo *interfaceJogo,
                                       bool compra)
{
    const char *titulo;
    const char *mensagem;
    char texto[192];
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 6, 10, 15, 165 });
    DrawUIPanelCustom((Rectangle){ 310.0f, 215.0f, 660.0f, 270.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 252 });
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
        if (interfaceJogo->escolha_rota_pendente == 2) {
            mensagem = "Escolher o Caminho 2? (3 inimigos | +50% Cura | +2 Itens)";
        } else if (interfaceJogo->escolha_rota_pendente == 1 &&
                   interfaceJogo->rota_atual != NULL &&
                   *interfaceJogo->rota_atual != NULL &&
                   (*interfaceJogo->rota_atual)->etapa >= 3) {
            mensagem = "Enfrentar o temível Barba Negra no Combate Final?";
        } else {
            mensagem = "Escolher o Caminho 1? (4 inimigos | +2 Níveis | +700 Ouros)";
        }
    }
    desenharTextoCentralizadoContornado(
        titulo, (Rectangle){ 310.0f, 245.0f, 660.0f, 36.0f }, 25, GOLD);
    desenharTextoCentralizado(mensagem, 320, 18, RAYWHITE);
    DrawUIButtonCustom((Rectangle){ 390.0f, 390.0f, 180.0f, 55.0f },
                       "CONFIRMAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
    DrawRectangleLines(390, 390, 180, 55, (Color){ 85, 215, 120, 230 });
    DrawUIButtonCustom((Rectangle){ 610.0f, 390.0f, 180.0f, 55.0f },
                       "CANCELAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
}

static void desenharRecompensas(const InterfaceJogo *interfaceJogo)
{
    int indice;
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 6, 10, 15, 165 });
    DrawUIPanelCustom((Rectangle){ 320.0f, 175.0f, 640.0f, 390.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 252 });
    desenharTextoCentralizadoContornado(
        "RECOMPENSAS DO COMBATE", (Rectangle){ 320.0f, 200.0f, 640.0f, 34.0f }, 24, GOLD);
    if (interfaceJogo->total_itens_recompensa == 0) {
        desenharTexto("Nenhum item adicional foi obtido neste combate.", 420, 280, 17, LIGHTGRAY);
    }
    for (indice = 0; indice < interfaceJogo->total_itens_recompensa; indice++) {
        const Item *item = &interfaceJogo->itens_recompensa[indice];
        int posY = 252 + indice * 36;
        Color corRar = corRaridadeItem(item->raridade);
        DrawRectangle(385, posY, 510, 32, (Color){ 22, 36, 48, 255 });
        DrawRectangle(385, posY, 5, 32, corRar);
        DrawRectangleLines(385, posY, 510, 32, corRar);
        desenharTexto(TextFormat("[%s] %s (x%d)",
                                 nomeRaridadeItem(item->raridade),
                                 item->nome, item->quantidade),
                      402, posY + 7, 15, corRar);
    }
    DrawUIButtonCustom((Rectangle){ 530.0f, 470.0f, 220.0f, 55.0f },
                       "CONTINUAR", interfaceJogo->texBtnPadrao, obterFonteInterface());
    DrawRectangleLines(530, 470, 220, 55, GOLD);
}

// Desenha a barra superior da fila de iniciativa com ordem de turnos clara e sem números soltos.
static void desenharFila(const InterfaceJogo *interfaceJogo)
{
    int indice;
    int slotVisivel = 0;
    DrawUIPanelCustom((Rectangle){ 4.0f, 4.0f, 462.0f, 54.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 10, 17, 24, 240 });
    for (indice = 0; indice < interfaceJogo->total_iniciativa && slotVisivel < 9; indice++) {
        Combatente *ator = interfaceJogo->fila_iniciativa[indice].ator;
        int posCardX = posicaoXSlotFila(slotVisivel);
        bool turnoAtivo = indice == interfaceJogo->indice_iniciativa;
        if (ator == NULL || !ator->vivo) {
            continue;
        }
        DrawRectangle(posCardX + 2, 9, 41, 41,
                      ator->aliado ? (Color){ 28, 64, 92, 245 } :
                                     (Color){ 105, 38, 32, 245 });
        {
            Texture2D retrato = obterSpriteUnidade(interfaceJogo, ator);
            DrawCharacterPortrait(retrato,
                              (Rectangle){ (float)posCardX, 7.0f, 45.0f, 45.0f });
            if (retrato.id == 0) {
                desenharTexto(TextFormat("%c", ator->nome[0] == '\0' ? '?' : ator->nome[0]),
                              posCardX + 16, 18, 19, RAYWHITE);
            }
        }
        DrawRectangle(posCardX + 2, 8, 16, 13, (Color){ 10, 16, 22, 220 });
        desenharTexto(TextFormat("%d", slotVisivel + 1), posCardX + 5, 8, 10,
                      turnoAtivo ? GOLD : LIGHTGRAY);
        if (turnoAtivo) {
            DrawRectangleLinesEx((Rectangle){ (float)posCardX, 7.0f, 45.0f, 45.0f },
                                 2.5f, GOLD);
        } else if (ator == interfaceJogo->entidade_selecionada) {
            DrawRectangleLinesEx((Rectangle){ (float)posCardX, 7.0f, 45.0f, 45.0f },
                                 2.0f, SKYBLUE);
        }
        slotVisivel++;
    }
}

static void desenharHUDAtorAtivo(const InterfaceJogo *interfaceJogo)
{
    const Combatente *ator;
    Rectangle retrato = { 930.0f, 15.0f, 94.0f, 114.0f };
    Rectangle barraStatus = { 1034.0f, 66.0f, 224.0f, 64.0f };
    Rectangle nomeAtor = { 1034.0f, 14.0f, 224.0f, 24.0f };
    Rectangle infoAtor = { 1034.0f, 39.0f, 224.0f, 20.0f };
    Color corGrupo;
    if (interfaceJogo == NULL) {
        return;
    }
    ator = interfaceJogo->ator_atual;
    if ((ator == NULL || !ator->vivo) && interfaceJogo->fase_pre_combate_ativa) {
        ator = interfaceJogo->entidade_selecionada;
    }
    DrawUIPanelCustom((Rectangle){ 916.0f, 4.0f, 356.0f, 140.0f },
                      interfaceJogo->texPainelDetalhes, (Color){ 12, 19, 27, 245 });
    corGrupo = ator != NULL && !ator->aliado ?
               (Color){ 115, 38, 32, 255 } : (Color){ 28, 64, 92, 255 };
    DrawRectangle((int)retrato.x + 3, (int)retrato.y + 3,
                  (int)retrato.width - 6, (int)retrato.height - 6, corGrupo);
    {
        Texture2D texturaRetrato = ator == NULL ? (Texture2D){ 0 } :
                                   obterSpriteUnidade(interfaceJogo, ator);
        DrawCharacterPortrait(texturaRetrato, retrato);
        if (ator != NULL && texturaRetrato.id == 0) {
            desenharTexto(TextFormat("%c", ator->nome[0] == '\0' ? '?' : ator->nome[0]),
                          965, 52, 30, RAYWHITE);
        }
    }
    if (ator != NULL) {
        desenharTextoCentralizadoContornado(ator->nome, nomeAtor, 15,
                                            ator->aliado ? SKYBLUE : ORANGE);
        desenharTextoCentralizadoContornado(
            ator->eh_boss ? TextFormat("Chefe Fase %d | Vel %d", ator->fase_boss, ator->velocidade) :
                            TextFormat("Nível %d | Vel %d | %s",
                                       ator->nivel, ator->velocidade,
                                       ator->aliado ? "Aliado" : "Inimigo"),
            infoAtor, 12, GOLD);
        desenharBarraStatusCombatente(interfaceJogo, ator, barraStatus, 11);
    } else {
        desenharTexto("PRÉ-COMBATE", 1034, 24, 16, GOLD);
        desenharTexto("Selecione uma unidade", 1034, 52, 13, LIGHTGRAY);
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

static void desenharTextoCentralizadoContornado(const char *texto, Rectangle bounds,
                                                int tamanho, Color cor)
{
    int larguraTexto;
    int posX;
    int posY;
    if (texto == NULL || texto[0] == '\0') {
        return;
    }
    larguraTexto = medirLarguraTexto(texto, tamanho);
    posX = (int)(bounds.x + (bounds.width - (float)larguraTexto) * 0.5f);
    posY = (int)(bounds.y + (bounds.height - (float)(tamanho + 2)) * 0.5f);
    desenharTexto(texto, posX - 1, posY, tamanho, BLACK);
    desenharTexto(texto, posX + 1, posY, tamanho, BLACK);
    desenharTexto(texto, posX, posY - 1, tamanho, BLACK);
    desenharTexto(texto, posX, posY + 1, tamanho, BLACK);
    desenharTexto(texto, posX, posY, tamanho, cor);
}

static float calcularProporcaoStatus(int atual, int maximo)
{
    if (maximo <= 0 || atual <= 0) {
        return 0.0f;
    }
    if (atual >= maximo) {
        return 1.0f;
    }
    return (float)atual / (float)maximo;
}

static void desenharPreenchimentoStatus(Rectangle area, float proporcao,
                                       Color corPreenchida,
                                       bool desenharPartePreenchida)
{
    if (proporcao < 1.0f) {
        Rectangle vazio = {
            area.x + area.width * proporcao,
            area.y,
            area.width * (1.0f - proporcao),
            area.height
        };
        DrawRectangle((int)vazio.x, (int)vazio.y, (int)vazio.width,
                      (int)vazio.height,
                      (Color){ 10, 12, 14, 205 });
    }
    if (desenharPartePreenchida) {
        DrawRectangle((int)area.x, (int)area.y, (int)(area.width * proporcao),
                      (int)area.height, corPreenchida);
    }
}

static void desenharBarraStatusCombatente(const InterfaceJogo *interfaceJogo,
                                          const Combatente *unidade,
                                          Rectangle bounds, int tamanhoTexto)
{
    const float margemX = 0.068f;
    const float larguraPreenchimento = 0.866f;
    float proporcaoVida = calcularProporcaoStatus(unidade->vida_atual,
                                                   unidade->vida_maxima);
    float proporcaoEstamina = calcularProporcaoStatus(unidade->estamina_atual,
                                                      unidade->estamina_maxima);
    Rectangle areaVida = {
        bounds.x + bounds.width * margemX,
        bounds.y + bounds.height * 0.190f,
        bounds.width * larguraPreenchimento,
        bounds.height * 0.248f
    };
    Rectangle areaEstamina = {
        bounds.x + bounds.width * margemX,
        bounds.y + bounds.height * 0.565f,
        bounds.width * larguraPreenchimento,
        bounds.height * 0.248f
    };
    if (interfaceJogo->texBarraStatusUnificada.id != 0) {
        desenharTexturaBounds(interfaceJogo->texBarraStatusUnificada, bounds);
        desenharPreenchimentoStatus(areaVida, proporcaoVida, RED, false);
        desenharPreenchimentoStatus(areaEstamina, proporcaoEstamina, GREEN, false);
    } else {
        DrawRectangle((int)bounds.x, (int)bounds.y, (int)bounds.width,
                      (int)bounds.height, (Color){ 35, 40, 42, 255 });
        DrawRectangle((int)areaVida.x, (int)areaVida.y, (int)areaVida.width,
                      (int)areaVida.height, (Color){ 70, 20, 20, 255 });
        DrawRectangle((int)areaEstamina.x, (int)areaEstamina.y,
                      (int)areaEstamina.width, (int)areaEstamina.height,
                      (Color){ 15, 55, 25, 255 });
        desenharPreenchimentoStatus(areaVida, proporcaoVida, RED, true);
        desenharPreenchimentoStatus(areaEstamina, proporcaoEstamina, GREEN, true);
        DrawRectangleLinesEx(bounds, 2.0f, GOLD);
    }
    desenharTextoCentralizadoContornado(
        TextFormat("%d/%d", unidade->vida_atual, unidade->vida_maxima),
        areaVida, tamanhoTexto, RAYWHITE);
    desenharTextoCentralizadoContornado(
        TextFormat("%d/%d", unidade->estamina_atual, unidade->estamina_maxima),
        areaEstamina, tamanhoTexto, RAYWHITE);
}

typedef struct {
    Combatente *unidade;
    Rectangle squareBounds;
} UnidadeOrdenadaTabuleiro;

static void desenharUnidadeTabuleiro(const InterfaceJogo *interfaceJogo,
                                     Combatente *unidade, Rectangle quadrado)
{
    Rectangle spriteBounds;
    Rectangle nomeBounds;
    Rectangle barraStatus;
    Texture2D sprite = obterSpriteUnidade(interfaceJogo, unidade);
    float alturaSprite = 72.0f;
    float larguraSprite = sprite.width > 0 && sprite.height > 0 ?
                          alturaSprite * (float)sprite.width / (float)sprite.height :
                          54.0f;
    float larguraBarra = quadrado.width - 28.0f;
    float alturaBarra = 30.0f;
    if (larguraSprite > quadrado.width - 28.0f) {
        larguraSprite = quadrado.width - 28.0f;
    }
    spriteBounds = (Rectangle){
        quadrado.x + (quadrado.width - larguraSprite) * 0.5f,
        quadrado.y + 21.0f,
        larguraSprite, alturaSprite
    };
    desenharSpriteUnidade(sprite, spriteBounds);
    if (sprite.id == 0) {
        DrawRectangle((int)quadrado.x + 56, (int)quadrado.y + 26, 64, 60,
                      unidade->aliado ? (Color){ 35, 85, 125, 230 } :
                                        (Color){ 135, 45, 38, 230 });
        DrawRectangleLines((int)quadrado.x + 56, (int)quadrado.y + 26, 64, 60, GOLD);
        desenharTextoCentralizadoContornado(
            unidade->eh_boss ? "BN" : TextFormat("%c", unidade->nome[0]),
            (Rectangle){ quadrado.x + 56.0f, quadrado.y + 38.0f, 64.0f, 36.0f },
            22, GOLD);
    }
    nomeBounds = (Rectangle){
        quadrado.x + 14.0f,
        quadrado.y + 5.0f,
        quadrado.width - 28.0f,
        15.0f
    };
    DrawRectangle((int)nomeBounds.x, (int)nomeBounds.y,
                  (int)nomeBounds.width, (int)nomeBounds.height,
                  (Color){ 8, 14, 22, 205 });
    if (unidade->eh_elite) {
        DrawCircle((int)nomeBounds.x + 8, (int)nomeBounds.y + 7, 4.0f, GOLD);
    }
    desenharTextoCentralizadoContornado(unidade->nome, nomeBounds, 11,
                                        unidade->aliado ? SKYBLUE : ORANGE);
    barraStatus = (Rectangle){
        quadrado.x + (quadrado.width - larguraBarra) * 0.5f,
        quadrado.y + quadrado.height - alturaBarra - 5.0f,
        larguraBarra, alturaBarra
    };
    desenharBarraStatusCombatente(interfaceJogo, unidade, barraStatus, 9);
    desenharIndicadoresStatusUnidade(unidade, (int)quadrado.x + 8,
                                     (int)quadrado.y + 22, 66);
}

// Desenha o tabuleiro 3x6 contínuo, zonas de preparação, alcance, minas e unidades.
static void desenharTabuleiro(const InterfaceJogo *interfaceJogo,
                              const Tabuleiro3x6 *mapa, const ListaAlcance *alcance)
{
    int linha;
    int coluna;
    int passoMovimento = obterAlcanceMovimentoUnidade(interfaceJogo->ator_atual);
    UnidadeOrdenadaTabuleiro unidades[LINHAS_TABULEIRO * COLUNAS_TABULEIRO];
    int totalUnidades = 0;
    Combatente *unidadeHovered = NULL;
    float hoveredY = -1.0f;
    Vector2 mouse = obterMouseVirtual();
    Rectangle molduraTabuleiro = {
        (float)(interfaceJogo->margem_esquerda - 8),
        (float)(interfaceJogo->margem_superior - 6),
        (float)(COLUNAS_TABULEIRO * interfaceJogo->largura_celula + 12),
        (float)(LINHAS_TABULEIRO * interfaceJogo->altura_celula + 8)
    };
    DrawRectangle((int)molduraTabuleiro.x, (int)molduraTabuleiro.y,
                  (int)molduraTabuleiro.width, (int)molduraTabuleiro.height,
                  (Color){ 8, 14, 22, 130 });
    DrawUIFrameCustom(molduraTabuleiro, interfaceJogo->texFrameRetrato);
    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            int posX = interfaceJogo->margem_esquerda + coluna * interfaceJogo->largura_celula;
            int posY = interfaceJogo->margem_superior + linha * interfaceJogo->altura_celula;
            int largCel = interfaceJogo->largura_celula - 4;
            int altCel = interfaceJogo->altura_celula - 4;
            Rectangle rectCelula = { (float)posX, (float)posY, (float)largCel, (float)altCel };
            Combatente *unidade = mapa->celulas[linha][coluna];
            Color tintPainel = (Color){ 215, 225, 235, 195 };
            Color tintMoldura = (Color){ 200, 185, 150, 215 };
            const NoCelula *marcada;
            if (interfaceJogo->fase_pre_combate_ativa && coluna <= 1) {
                tintPainel = (Color){ 155, 225, 245, 220 };
                tintMoldura = SKYBLUE;
            } else if (unidade == NULL || !unidade->vivo) {
                tintPainel = (Color){ 185, 200, 215, 155 };
                tintMoldura = (Color){ 175, 165, 140, 175 };
            }
            if (interfaceJogo->texPainelDetalhes.id != 0) {
                desenharNPatch(interfaceJogo->texPainelDetalhes,
                               interfaceJogo->nPatchPainelDescricao,
                               rectCelula, tintPainel);
            } else {
                DrawRectangle(posX, posY, largCel, altCel, (Color){ 24, 38, 50, 165 });
            }
            if (!interfaceJogo->fase_pre_combate_ativa &&
                interfaceJogo->acao_combate == ACAO_TELA_MOVER &&
                interfaceJogo->ator_atual != NULL &&
                interfaceJogo->ator_atual->aliado &&
                unidade == NULL) {
                int distMov = abs(linha - interfaceJogo->ator_atual->linha) +
                              abs(coluna - interfaceJogo->ator_atual->coluna);
                if (distMov >= 1 && distMov <= passoMovimento) {
                    DrawRectangle(posX + 5, posY + 5, largCel - 10, altCel - 10,
                                  (Color){ 45, 175, 90, 130 });
                    tintMoldura = (Color){ 95, 240, 130, 255 };
                }
            }
            if (alcance != NULL) {
                for (marcada = alcance->inicio; marcada != NULL; marcada = marcada->proximo) {
                    if (marcada->linha == linha && marcada->coluna == coluna) {
                        DrawRectangle(posX + 5, posY + 5, largCel - 10, altCel - 10,
                                      marcada->perigo_fogo_amigo ?
                                      (Color){ 195, 45, 35, 150 } :
                                      (Color){ 215, 168, 35, 130 });
                        tintMoldura = marcada->perigo_fogo_amigo ? RED : GOLD;
                    }
                }
            }
            if (unidade != NULL && unidade->vivo) {
                if (unidade == interfaceJogo->ator_atual &&
                    !interfaceJogo->fase_pre_combate_ativa) {
                    tintMoldura = unidade->aliado ? (Color){ 95, 240, 130, 255 } : ORANGE;
                } else if (unidade == interfaceJogo->entidade_selecionada) {
                    tintMoldura = GOLD;
                }
            }
            if (interfaceJogo->texFrameRetrato.id != 0) {
                desenharNPatch(interfaceJogo->texFrameRetrato,
                               interfaceJogo->nPatchFrameRetrato,
                               rectCelula, tintMoldura);
            } else {
                DrawRectangleLines(posX, posY, largCel, altCel, tintMoldura);
            }
            if (unidade != NULL && unidade->vivo &&
                unidade == interfaceJogo->ator_atual &&
                !interfaceJogo->fase_pre_combate_ativa) {
                DrawRectangleLinesEx((Rectangle){ (float)posX + 2.0f, (float)posY + 2.0f,
                                                   (float)largCel - 4.0f, (float)altCel - 4.0f },
                                     2.5f, unidade->aliado ? GREEN : ORANGE);
            } else if (unidade != NULL && unidade->vivo &&
                       unidade == interfaceJogo->entidade_selecionada) {
                DrawRectangleLinesEx((Rectangle){ (float)posX + 2.0f, (float)posY + 2.0f,
                                                   (float)largCel - 4.0f, (float)altCel - 4.0f },
                                     2.0f, GOLD);
            }
            if (interfaceJogo->timer_destaque_acao > 0.0f) {
                if (linha == interfaceJogo->linha_destaque_acao &&
                    coluna == interfaceJogo->coluna_destaque_acao) {
                    DrawRectangle(posX + 5, posY + 5, largCel - 10, altCel - 10,
                                  (Color){ 235, 85, 45, 125 });
                    DrawRectangleLinesEx(rectCelula, 3.5f, RED);
                } else if (linha == interfaceJogo->linha_origem_acao &&
                           coluna == interfaceJogo->coluna_origem_acao) {
                    DrawRectangleLinesEx(rectCelula, 3.0f, ORANGE);
                }
            }
            if (mapa->mina_ativa[linha][coluna] > 0) {
                DrawCircle(posX + 88, posY + 64, 12, (Color){ 195, 30, 30, 230 });
                DrawCircle(posX + 88, posY + 64, 6, GOLD);
                desenharTexto("MINA", posX + 72, posY + 80, 10, ORANGE);
            }
        }
    }

    for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
        for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
            int posX = interfaceJogo->margem_esquerda + coluna * interfaceJogo->largura_celula;
            int posY = interfaceJogo->margem_superior + linha * interfaceJogo->altura_celula;
            Combatente *unidade = mapa->celulas[linha][coluna];
            Rectangle quadrado;
            if (unidade == NULL || !unidade->vivo) {
                continue;
            }
            quadrado = (Rectangle){
                (float)posX, (float)posY,
                (float)interfaceJogo->largura_celula - 4.0f,
                (float)interfaceJogo->altura_celula - 4.0f
            };
            unidades[totalUnidades++] = (UnidadeOrdenadaTabuleiro){ unidade, quadrado };
        }
    }

    for (linha = 0; linha < totalUnidades; linha++) {
        Combatente *unidade = unidades[linha].unidade;
        if (CheckCollisionPointRec(mouse, unidades[linha].squareBounds) &&
            unidades[linha].squareBounds.y >= hoveredY) {
            unidadeHovered = unidade;
            hoveredY = unidades[linha].squareBounds.y;
        }
    }

    for (linha = 0; linha < totalUnidades; linha++) {
        Combatente *unidade = unidades[linha].unidade;
        if (unidade != unidadeHovered) {
            desenharUnidadeTabuleiro(interfaceJogo, unidade,
                                     unidades[linha].squareBounds);
        }
    }
    if (unidadeHovered != NULL) {
        for (linha = 0; linha < totalUnidades; linha++) {
            if (unidades[linha].unidade == unidadeHovered) {
                desenharUnidadeTabuleiro(interfaceJogo, unidadeHovered,
                                         unidades[linha].squareBounds);
                break;
            }
        }
    }

    if (interfaceJogo->timer_indicador_dano > 0.0f) {
        for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
            for (coluna = 0; coluna < COLUNAS_TABULEIRO; coluna++) {
                int posX = interfaceJogo->margem_esquerda + coluna * interfaceJogo->largura_celula;
                int posY = interfaceJogo->margem_superior + linha * interfaceJogo->altura_celula;
                int danoCelula = mapa->ultimo_dano_celula[linha][coluna];
                int curaCelula = mapa->ultima_cura_celula[linha][coluna];
                if (danoCelula > 0) {
                    DrawRectangle(posX + 45, posY + 38, 86, 26, (Color){ 195, 25, 25, 245 });
                    DrawRectangleLinesEx((Rectangle){ (float)(posX + 45), (float)(posY + 38),
                                                       86.0f, 26.0f }, 2.0f, GOLD);
                    desenharTexto(TextFormat("-%d DANO", danoCelula),
                                  posX + 51, posY + 43, 13, RAYWHITE);
                } else if (curaCelula > 0) {
                    DrawRectangle(posX + 45, posY + 38, 86, 26, (Color){ 30, 155, 70, 245 });
                    DrawRectangleLinesEx((Rectangle){ (float)(posX + 45), (float)(posY + 38),
                                                       86.0f, 26.0f }, 2.0f, RAYWHITE);
                    desenharTexto(TextFormat("+%d PV", curaCelula),
                                  posX + 55, posY + 43, 13, RAYWHITE);
                }
            }
        }
    }
}

// Desenha a tela de menu principal renderizada em alta nitidez 1080p.
static void desenharMenu(const InterfaceJogo *interfaceJogo)
{
    Rectangle painelTitulo = { 330.0f, 135.0f, 620.0f, 120.0f };
    iniciarQuadroVirtual(interfaceJogo, (Color){ 18, 28, 39, 255 });
    DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 8, 16, 26, 75 });
    DrawUIPanelCustom(painelTitulo, interfaceJogo->texPainelDetalhes,
                      (Color){ 12, 20, 30, 235 });
    DrawUIFrameCustom(painelTitulo, interfaceJogo->texFrameRetrato);
    desenharTextoCentralizadoContornado("SEA OF GUNPOWDER",
                                        painelTitulo, 46, GOLD);
    DrawUIButtonCustom((Rectangle){ 470.0f, 350.0f, 340.0f, 70.0f },
                       "INICIAR AVENTURA", interfaceJogo->texBtnPadrao,
                       obterFonteInterface());
    DrawUIButtonCustom((Rectangle){ 470.0f, 445.0f, 340.0f, 60.0f },
                       "SAIR DO JOGO", interfaceJogo->texBtnPadrao,
                       obterFonteInterface());
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
        Texture2D texRota1 = interfaceJogo->qtdBgCombate > 0 ?
                             interfaceJogo->bgCombatePool[etapaAtual % interfaceJogo->qtdBgCombate] :
                             (Texture2D){ 0 };
        Texture2D texRota2 = interfaceJogo->qtdBgCombate > 1 ?
                             interfaceJogo->bgCombatePool[(etapaAtual + 2) % interfaceJogo->qtdBgCombate] :
                             texRota1;
        Texture2D texLoja = interfaceJogo->qtdBgCombate > 0 ?
                            interfaceJogo->bgCombatePool[interfaceJogo->qtdBgCombate - 1] :
                            (Texture2D){ 0 };
        DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 8, 16, 26, 110 });
        DrawUIPanelCustom((Rectangle){ 200.0f, 18.0f, 880.0f, 104.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 14, 24, 34, 245 });
        desenharTextoCentralizadoContornado(
            TextFormat("MAPA DE ROTAS — ETAPA %d DE 3",
                       etapaAtual < 3 ? etapaAtual + 1 : 3),
            (Rectangle){ 200.0f, 30.0f, 880.0f, 38.0f }, 28, GOLD);
        desenharTextoCentralizado(
            TextFormat("Moedas de Ouro: %d   |   Saques realizados: %d   |   Desistências: %d/2",
                       interfaceJogo->moedas == NULL ? 0 : *interfaceJogo->moedas,
                       contadorSaques, contadorFugas),
            78, 17, RAYWHITE);

        DrawUIPanelCustom((Rectangle){ 40.0f, 140.0f, 370.0f, 455.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 14, 24, 34, 240 });
        desenharTextoCentralizadoContornado(
            "TRIPULAÇÃO ATIVA",
            (Rectangle){ 40.0f, 156.0f, 370.0f, 28.0f }, 18, GOLD);
        if (mapa != NULL) {
            int linha;
            for (linha = 0; linha < LINHAS_TABULEIRO; linha++) {
                Combatente *pirata = mapa->celulas[linha][0];
                int posCardY = 200 + linha * 100;
                if (pirata != NULL && pirata->vivo) {
                    bool selecionado = pirata == interfaceJogo->entidade_selecionada;
                    Rectangle retratoCard = { 62.0f, (float)(posCardY + 9), 64.0f, 64.0f };
                    DrawRectangle(55, posCardY, 340, 82,
                                  selecionado ? (Color){ 34, 62, 84, 250 } :
                                                (Color){ 22, 38, 52, 245 });
                    DrawRectangleLinesEx((Rectangle){ 55.0f, (float)posCardY, 340.0f, 82.0f },
                                         selecionado ? 2.5f : 1.5f,
                                         selecionado ? GOLD : (Color){ 95, 120, 138, 220 });
                    DrawRectangle((int)retratoCard.x + 2, (int)retratoCard.y + 2,
                                  (int)retratoCard.width - 4, (int)retratoCard.height - 4,
                                  (Color){ 28, 54, 76, 255 });
                    DrawCharacterPortrait(obterSpriteBasePirata(interfaceJogo, pirata),
                                          retratoCard);
                    desenharTexto(TextFormat("%s (Nv %d)", pirata->nome, pirata->nivel),
                                  136, posCardY + 7, 16, selecionado ? GOLD : RAYWHITE);
                    desenharTexto(TextFormat("Passivos %d/5", pirata->build_passivos.tamanho),
                                  292, posCardY + 9, 13, LIGHTGRAY);
                    desenharBarraStatusCombatente(interfaceJogo, pirata,
                                                  (Rectangle){ 136.0f, (float)(posCardY + 30),
                                                               248.0f, 44.0f }, 10);
                }
            }
        }
        desenharTexto("Clique em um pirata para inspecionar sua ficha.",
                      62, 508, 13, LIGHTGRAY);

        DrawUIPanelCustom((Rectangle){ 420.0f, 140.0f, 820.0f, 455.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 14, 24, 34, 240 });
        desenharTextoCentralizadoContornado(
            etapaAtual >= 3 ? "CONFRONTO DECISIVO NOS SETE MARES" :
                              "ESCOLHA O PRÓXIMO RUMO DA EXPEDIÇÃO",
            (Rectangle){ 420.0f, 158.0f, 820.0f, 30.0f }, 20, GOLD);
        {
            int passo;
            for (passo = 0; passo < 4; passo++) {
                int nodoX = 505 + passo * 170;
                bool concluido = passo < etapaAtual;
                bool atual = (passo == etapaAtual) || (etapaAtual >= 3 && passo == 3);
                if (passo < 3) {
                    DrawRectangle(nodoX + 70, 214, 100, 4,
                                  concluido ? GOLD : (Color){ 55, 75, 90, 255 });
                }
                DrawRectangle(nodoX - 50, 200, 120, 32,
                              atual ? (Color){ 52, 95, 125, 255 } :
                              (concluido ? (Color){ 35, 85, 55, 255 } :
                                           (Color){ 24, 36, 48, 255 }));
                DrawRectangleLines(nodoX - 50, 200, 120, 32,
                                   atual ? GOLD : (concluido ? GREEN : GRAY));
                desenharTextoCentralizadoContornado(
                    passo < 3 ? TextFormat("ETAPA %d", passo + 1) : "CHEFE FINAL",
                    (Rectangle){ (float)(nodoX - 50), 200.0f, 120.0f, 32.0f },
                    13, atual ? GOLD : RAYWHITE);
            }
        }
        if (etapaAtual >= 3) {
            Rectangle cardBoss = { 480.0f, 340.0f, 380.0f, 100.0f };
            DrawRectangle(480, 340, 380, 100, (Color){ 55, 18, 18, 255 });
            desenharTexturaCapaTint(interfaceJogo->bgBoss, cardBoss,
                                    (Color){ 210, 140, 140, 130 });
            DrawRectangle(480, 340, 380, 100, (Color){ 22, 10, 12, 155 });
            DrawUIFrameCustom(cardBoss, interfaceJogo->texFrameRetrato);
            desenharTextoCentralizadoContornado("ENFRENTAR BARBA NEGRA",
                                                (Rectangle){ 480.0f, 355.0f, 380.0f, 32.0f },
                                                22, GOLD);
            desenharTextoCentralizadoContornado(
                "Chefe Final (3000 HP | 2 Fases | 3 Turnos/Rodada)",
                (Rectangle){ 480.0f, 395.0f, 380.0f, 24.0f }, 14, RAYWHITE);
            if (etapaAtual == 3) {
                Rectangle cardLojaBoss = { 510.0f, 470.0f, 300.0f, 80.0f };
                DrawRectangle(510, 470, 300, 80, (Color){ 52, 36, 22, 255 });
                desenharTexturaCapaTint(texLoja, cardLojaBoss,
                                        (Color){ 220, 195, 150, 120 });
                DrawRectangle(510, 470, 300, 80, (Color){ 16, 20, 26, 165 });
                DrawUIFrameCustom(cardLojaBoss, interfaceJogo->texFrameRetrato);
                desenharTextoCentralizadoContornado("LOJA OPCIONAL",
                                                    (Rectangle){ 510.0f, 478.0f, 300.0f, 24.0f },
                                                    18, GOLD);
                desenharTextoCentralizado("Comprar sem avançar rota | Saque: combate",
                                          508, 13, RAYWHITE);
            }
            DrawRectangle(885, 290, 325, 260, (Color){ 16, 28, 38, 230 });
            DrawRectangleLines(885, 290, 325, 260, GOLD);
            desenharTexto("ALERTA DO CHEFE FINAL", 935, 306, 16, GOLD);
            (void)desenharTextoQuebrado(
                "Barba Negra possui 3000 PV e age 3 vezes por rodada. Ao cair para 50% de PV, desperta a Fase 2: convoca 2 Grunts com Espada (ficando imune até serem derrotados) e espalha minas pelo convés!",
                900, 338, 295, 14, 19, RAYWHITE, 0, 0);
        } else {
            Rectangle cardCam1 = { 440.0f, 290.0f, 260.0f, 95.0f };
            Rectangle cardCam2 = { 730.0f, 290.0f, 260.0f, 95.0f };
            Rectangle cardLoja = { 585.0f, 415.0f, 260.0f, 95.0f };
            DrawRectangle(440, 290, 260, 95, (Color){ 28, 56, 82, 255 });
            desenharTexturaCapaTint(texRota1, cardCam1, (Color){ 190, 215, 240, 125 });
            DrawRectangle(440, 290, 260, 95, (Color){ 12, 22, 34, 165 });
            DrawUIFrameCustom(cardCam1, interfaceJogo->texFrameRetrato);
            desenharTextoCentralizadoContornado("CAMINHO 1 (4 Inimigos)",
                                                (Rectangle){ 440.0f, 302.0f, 260.0f, 24.0f },
                                                17, RAYWHITE);
            desenharTextoCentralizadoContornado("+2 Níveis | +700 Ouros",
                                                (Rectangle){ 440.0f, 330.0f, 260.0f, 22.0f },
                                                15, GOLD);
            desenharTextoCentralizadoContornado("+15% Cura | +1 Item",
                                                (Rectangle){ 440.0f, 354.0f, 260.0f, 20.0f },
                                                14, SKYBLUE);

            DrawRectangle(730, 290, 260, 95, (Color){ 32, 68, 46, 255 });
            desenharTexturaCapaTint(texRota2, cardCam2, (Color){ 190, 235, 200, 125 });
            DrawRectangle(730, 290, 260, 95, (Color){ 12, 24, 18, 165 });
            DrawUIFrameCustom(cardCam2, interfaceJogo->texFrameRetrato);
            desenharTextoCentralizadoContornado("CAMINHO 2 (3 Inimigos)",
                                                (Rectangle){ 730.0f, 302.0f, 260.0f, 24.0f },
                                                17, RAYWHITE);
            desenharTextoCentralizadoContornado("+1 Nível | +300 Ouros",
                                                (Rectangle){ 730.0f, 330.0f, 260.0f, 22.0f },
                                                15, GOLD);
            desenharTextoCentralizadoContornado("+50% Cura | +2 Itens",
                                                (Rectangle){ 730.0f, 354.0f, 260.0f, 20.0f },
                                                14, GREEN);

            DrawRectangle(585, 415, 260, 95, (Color){ 68, 46, 26, 255 });
            desenharTexturaCapaTint(texLoja, cardLoja, (Color){ 235, 205, 160, 125 });
            DrawRectangle(585, 415, 260, 95, (Color){ 20, 16, 12, 165 });
            DrawUIFrameCustom(cardLoja, interfaceJogo->texFrameRetrato);
            desenharTextoCentralizadoContornado("LOJA OPCIONAL",
                                                (Rectangle){ 585.0f, 427.0f, 260.0f, 24.0f },
                                                18, GOLD);
            desenharTextoCentralizadoContornado("Comprar sem avançar a rota",
                                                (Rectangle){ 585.0f, 455.0f, 260.0f, 20.0f },
                                                14, RAYWHITE);
            desenharTextoCentralizadoContornado("Saquear: combate e avança rota",
                                                (Rectangle){ 585.0f, 479.0f, 260.0f, 20.0f },
                                                13, ORANGE);

            DrawRectangle(1010, 275, 208, 285, (Color){ 16, 26, 36, 230 });
            DrawRectangleLines(1010, 275, 208, 285, (Color){ 165, 138, 78, 210 });
            desenharTexto("DIÁRIO DE NAVEGAÇÃO", 1030, 290, 14, GOLD);
            (void)desenharTextoQuebrado(
                "• Caminho 1: rota agressiva com maior evolução (+2 Níveis e +700 Ouros).\n• Caminho 2: rota segura com maior recuperação (+50% PV e +2 Itens).\n• Loja: compre suprimentos sem gastar a etapa ou saqueie todo o estoque!",
                1022, 318, 184, 13, 17, LIGHTGRAY, 0, 0);
        }

        DrawUIPanelCustom((Rectangle){ 70.0f, 616.0f, 650.0f, 82.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 12, 20, 29, 240 });
        DrawUIButtonCustom((Rectangle){ 90.0f, 630.0f, 190.0f, 55.0f },
                           "MOCHILA / BUILDS", interfaceJogo->texBtnPadrao,
                           obterFonteInterface());
        DrawUIButtonCustom((Rectangle){ 300.0f, 630.0f, 190.0f, 55.0f },
                           contadorFugas < 2 ? "FUGIR AVENTURA" : "FUGAS 2/2",
                           interfaceJogo->texBtnPadrao, obterFonteInterface());
        DrawUIButtonCustom((Rectangle){ 510.0f, 630.0f, 190.0f, 55.0f },
                           "DETALHES PIRATA", interfaceJogo->texBtnPadrao,
                           obterFonteInterface());
        if (interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS) {
            DrawRectangleLines(90, 630, 190, 55, GOLD);
        } else if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_FUGA) {
            DrawRectangleLines(300, 630, 190, 55, GOLD);
        } else if (interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM) {
            DrawRectangleLines(510, 630, 190, 55, GOLD);
        }
    } else if (interfaceJogo->estado == TELA_COMBATE_3X6 && mapa != NULL) {
        desenharFila(interfaceJogo);
        desenharHUDAtorAtivo(interfaceJogo);
        DrawUIPanelCustom((Rectangle){ 4.0f, 62.0f, 462.0f, 82.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 12, 20, 29, 235 });
        DrawUIPanelCustom((Rectangle){ 472.0f, 4.0f, 438.0f, 140.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 12, 20, 29, 240 });
        if (interfaceJogo->fase_pre_combate_ativa) {
            desenharTexto("FASE DE PREPARAÇÃO", 22, 74, 15, GOLD);
            desenharTexto("Colunas 0 e 1: Zona de posicionamento aliado", 22, 96, 13, SKYBLUE);
            desenharTexto("Clique nos piratas para trocar posições antes da luta.", 22, 116, 12, LIGHTGRAY);
            desenharTexto("COMANDO DO CONVÉS — PRÉ-COMBATE", 492, 18, 16, GOLD);
            (void)desenharTextoQuebrado(
                "Organize sua formação nas duas primeiras colunas, equipe builds passivas na Mochila e inspecione os inimigos clicando neles.",
                492, 48, 400, 13, 18, RAYWHITE, 0, 0);
        } else {
            int gruntsVivos = 0;
            if (mapa->boss_fase_2_ativa) {
                int linB;
                int colB;
                for (linB = 0; linB < LINHAS_TABULEIRO; linB++) {
                    for (colB = 0; colB < COLUNAS_TABULEIRO; colB++) {
                        Combatente *unidB = mapa->celulas[linB][colB];
                        if (unidB != NULL && unidB->vivo &&
                            unidB->tipo_inimigo == INIMIGO_GRUNT_ESPADA &&
                            (unidB->id == 110 || unidB->id == 111)) {
                            gruntsVivos++;
                        }
                    }
                }
            }
            if (gruntsVivos > 0) {
                DrawRectangle(16, 72, 438, 34, (Color){ 145, 38, 34, 245 });
                DrawRectangleLines(16, 72, 438, 34, GOLD);
                desenharTexto(TextFormat("BARBA NEGRA IMUNE: derrote os Grunts! (%d restantes)",
                                         gruntsVivos),
                              26, 82, 13, RAYWHITE);
                desenharTexto("Verde: Mover | Amarelo: Alcance | Vermelho: Fogo Amigo",
                              22, 114, 12, LIGHTGRAY);
            } else {
                desenharTexto("LEGENDA TÁTICA DO TABULEIRO:", 22, 74, 14, GOLD);
                desenharTexto("Verde: Casa de Movimento   |   Ouro: Alcance da Habilidade",
                              22, 96, 12, RAYWHITE);
                desenharTexto("Vermelho: Alerta de Fogo Amigo (50% dano em aliados)",
                              22, 116, 12, ORANGE);
            }
            desenharTexto(TextFormat("TURNO: %s",
                                     interfaceJogo->ator_atual == NULL ?
                                     "Aguardando" : interfaceJogo->ator_atual->nome),
                          492, 18, 17,
                          (interfaceJogo->ator_atual != NULL &&
                           !interfaceJogo->ator_atual->aliado) ? ORANGE : GOLD);
            DrawRectangle(745, 13, 148, 30, (Color){ 24, 40, 52, 250 });
            DrawRectangleLines(745, 13, 148, 30, GOLD);
            desenharTexto(TextFormat("AÇÕES: %d",
                                     interfaceJogo->ator_atual == NULL ?
                                     0 : interfaceJogo->ator_atual->acoes_basicas_restantes),
                          772, 19, 16, GOLD);
            DrawRectangle(488, 48, 406, 82, (Color){ 14, 24, 34, 215 });
            DrawRectangleLines(488, 48, 406, 82, (Color){ 85, 110, 128, 190 });
            (void)desenharTextoQuebrado(
                interfaceJogo->log_combate[0] != '\0' ?
                interfaceJogo->log_combate :
                "Selecione uma ação na barra inferior para comandar sua tripulação.",
                498, 56, 386, 14, 19, RAYWHITE, 48, 128);
        }
        desenharTabuleiro(interfaceJogo, mapa, alcanceAtivo);
        DrawUIPanelCustom((Rectangle){ 70.0f, 568.0f, 1140.0f, 134.0f },
                          interfaceJogo->texPainelDetalhes,
                          (Color){ 12, 20, 29, 242 });
        DrawRectangle(88, 578, 1104, 46, (Color){ 14, 24, 34, 225 });
        DrawRectangleLines(88, 578, 1104, 46, (Color){ 120, 105, 65, 190 });
        if (interfaceJogo->fase_pre_combate_ativa) {
            desenharTexto("PREPARAÇÃO PRÉ-COMBATE:", 98, 583, 14, GOLD);
            desenharTexto("Clique em um pirata e depois em qualquer casa das Colunas 0 ou 1 para posicionar ou trocar de lugar.",
                          98, 602, 13, RAYWHITE);
            DrawUIButtonCustom((Rectangle){ 90.0f, 630.0f, 260.0f, 55.0f },
                               "INICIAR COMBATE", interfaceJogo->texBtnPadrao,
                               obterFonteInterface());
            DrawRectangleLines(90, 630, 260, 55, (Color){ 85, 220, 120, 230 });
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
                    desenharTexto("AÇÃO SELECIONADA — MOVER (Custo: 1 Estamina)",
                                  98, 582, 14, GOLD);
                    desenharTexto("Clique em uma casa destacada em verde para deslocar o pirata.",
                                  98, 601, 13, RAYWHITE);
                } else if (interfaceJogo->acao_combate >= ACAO_TELA_HABILIDADE_0 &&
                           interfaceJogo->acao_combate <= ACAO_TELA_HABILIDADE_2) {
                    int idxHab = interfaceJogo->acao_combate - ACAO_TELA_HABILIDADE_0;
                    char cabecalhoCompacto[160];
                    formatarCabecalhoHabilidade(interfaceJogo->ator_atual, idxHab, true,
                                                cabecalhoCompacto, sizeof(cabecalhoCompacto));
                    desenharTexto(cabecalhoCompacto, 98, 581, 14, GOLD);
                    (void)desenharTextoQuebrado(
                        descricaoHabilidadeDetalhada(interfaceJogo->ator_atual, idxHab),
                        98, 598, 1084, 12, 13, RAYWHITE, 596, 623);
                } else {
                    desenharTexto("COMANDO DE TURNO:", 98, 582, 14, GOLD);
                    desenharTexto("Escolha Mover, Descansar, uma Habilidade ou Usar Item abaixo, ou clique em DETALHES para ver a ficha.",
                                  98, 601, 13, LIGHTGRAY);
                }
            } else {
                desenharTexto("TURNO DO INIMIGO EM ANDAMENTO...", 98, 582, 14, ORANGE);
                desenharTexto("Observe a movimentação e os ataques da tripulação adversária no convés.",
                              98, 601, 13, LIGHTGRAY);
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
                DrawRectangle(525, 630, 130, 55, (Color){ 10, 14, 18, 155 });
                desenharTextoCentralizadoContornado(
                    "HAB 2 (Nv3)", (Rectangle){ 525.0f, 630.0f, 130.0f, 55.0f }, 14, GRAY);
            }
            if (habDesbloq < 3) {
                DrawRectangle(670, 630, 130, 55, (Color){ 10, 14, 18, 155 });
                desenharTextoCentralizadoContornado(
                    "ULT (Nv6)", (Rectangle){ 670.0f, 630.0f, 130.0f, 55.0f }, 14, GRAY);
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
        DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 8, 16, 26, 125 });
        DrawUIPanelCustom((Rectangle){ 220.0f, 180.0f, 840.0f, 360.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 14, 24, 34, 248 });
        desenharTextoCentralizadoContornado(
            "VITÓRIA! BARBA NEGRA FOI DERROTADO!",
            (Rectangle){ 220.0f, 235.0f, 840.0f, 42.0f }, 32, GOLD);
        desenharTextoCentralizado(
            "Sua tripulação conquistou os Sete Mares em Sea Of Gunpowder!",
            310, 20, RAYWHITE);
        DrawUIButtonCustom((Rectangle){ 470.0f, 440.0f, 340.0f, 65.0f },
                           "NOVA AVENTURA", interfaceJogo->texBtnPadrao,
                           obterFonteInterface());
        DrawRectangleLines(470, 440, 340, 65, GOLD);
    } else if (interfaceJogo->estado == TELA_FIM_DE_JOGO) {
        DrawRectangle(0, 0, LARGURA_VIRTUAL, ALTURA_VIRTUAL, (Color){ 14, 8, 10, 145 });
        DrawUIPanelCustom((Rectangle){ 220.0f, 180.0f, 840.0f, 360.0f },
                          interfaceJogo->texPainelDetalhes, (Color){ 18, 16, 22, 248 });
        desenharTextoCentralizadoContornado(
            "FIM DE JOGO — TRIPULAÇÃO DERROTADA",
            (Rectangle){ 220.0f, 235.0f, 840.0f, 42.0f }, 32, ORANGE);
        desenharTextoCentralizado(
            "Todos os seus piratas tombaram em combate.",
            310, 20, RAYWHITE);
        DrawUIButtonCustom((Rectangle){ 470.0f, 440.0f, 340.0f, 65.0f },
                           "TENTAR NOVAMENTE", interfaceJogo->texBtnPadrao,
                           obterFonteInterface());
        DrawRectangleLines(470, 440, 340, 65, GOLD);
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
        for (indice = 0; indice < 5; indice++) {
            int pose;
            for (pose = 0; pose < TOTAL_POSES_SPRITES_PIRATA; pose++) {
                if (interfaceJogo->spritesPiratas[indice][pose].id != 0) {
                    UnloadTexture(interfaceJogo->spritesPiratas[indice][pose]);
                    interfaceJogo->spritesPiratas[indice][pose] = (Texture2D){ 0 };
                }
            }
        }
        for (indice = 0; indice < 4; indice++) {
            int pose;
            for (pose = 0; pose < 4; pose++) {
                if (interfaceJogo->spritesInimigos[indice][pose].id != 0) {
                    UnloadTexture(interfaceJogo->spritesInimigos[indice][pose]);
                    interfaceJogo->spritesInimigos[indice][pose] = (Texture2D){ 0 };
                }
            }
        }
        if (interfaceJogo->bgMar.id != 0) {
            UnloadTexture(interfaceJogo->bgMar);
            interfaceJogo->bgMar = (Texture2D){ 0 };
        }
        for (indice = 0; indice < 6; indice++) {
            if (interfaceJogo->bgCombatePool[indice].id != 0) {
                UnloadTexture(interfaceJogo->bgCombatePool[indice]);
            }
            interfaceJogo->bgCombatePool[indice] = (Texture2D){ 0 };
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
        if (interfaceJogo->texBarraStatusUnificada.id != 0) {
            UnloadTexture(interfaceJogo->texBarraStatusUnificada);
            interfaceJogo->texBarraStatusUnificada = (Texture2D){ 0 };
        }
        interfaceJogo->qtdBgCombate = 0;
        interfaceJogo->bgCombateAtualIdx = -1;
        interfaceJogo->bgCombateAnteriorIdx = -1;
        interfaceJogo->bgBossAtivo = false;
        if (interfaceJogo->fonteJogo.texture.id != 0) {
            UnloadFont(interfaceJogo->fonteJogo);
            interfaceJogo->fonteJogo = (Font){ 0 };
        }
        if (interfaceRenderizada == interfaceJogo) {
            interfaceRenderizada = NULL;
        }
        if (gerenciadorFundoCombate == interfaceJogo) {
            gerenciadorFundoCombate = NULL;
        }
    }
    if (canvasCarregado && canvasVirtual.id != 0) {
        UnloadRenderTexture(canvasVirtual);
        canvasCarregado = false;
    }
    CloseWindow();
}
