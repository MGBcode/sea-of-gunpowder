#include "animacao.h"

#include <string.h>

// Carrega uma spritesheet horizontal e configura sua velocidade de reprodução.
bool carregarSpriteAnimado(SpriteAnimado *sprite, const char *caminhoPng, int totalQuadros,
                           float fps, bool loop)
{
    Texture2D textura;

    if (sprite == NULL || caminhoPng == NULL || totalQuadros <= 0 || fps <= 0.0f) {
        TraceLog(LOG_ERROR, "Parametros invalidos para carregar animacao de sprite.");
        return false;
    }

    textura = LoadTexture(caminhoPng);
    if (textura.id == 0 || textura.width <= 0 || textura.height <= 0 ||
        textura.width % totalQuadros != 0) {
        if (textura.id != 0) {
            UnloadTexture(textura);
        }
        TraceLog(LOG_ERROR, "Nao foi possivel carregar spritesheet valida: %s", caminhoPng);
        return false;
    }

    sprite->textura = textura;
    sprite->total_quadros = totalQuadros;
    sprite->quadro_atual = 0;
    sprite->tempo_acumulado = 0.0f;
    sprite->duracao_quadro = 1.0f / fps;
    sprite->largura_frame = textura.width / totalQuadros;
    sprite->altura_frame = textura.height;
    sprite->ativa = true;
    sprite->loop = loop;
    return true;
}

// Avança os quadros da animação conforme o tempo transcorrido.
void atualizarSpriteAnimado(SpriteAnimado *sprite, float tempoFrame)
{
    if (sprite == NULL || !sprite->ativa || sprite->duracao_quadro <= 0.0f ||
        tempoFrame < 0.0f) {
        return;
    }

    sprite->tempo_acumulado += tempoFrame;
    while (sprite->tempo_acumulado >= sprite->duracao_quadro && sprite->ativa) {
        sprite->tempo_acumulado -= sprite->duracao_quadro;
        sprite->quadro_atual++;
        if (sprite->quadro_atual >= sprite->total_quadros) {
            if (sprite->loop) {
                sprite->quadro_atual = 0;
            } else {
                sprite->quadro_atual = sprite->total_quadros - 1;
                sprite->ativa = false;
            }
        }
    }
}

// Desenha o quadro atual da spritesheet na posição informada.
void desenharSpriteAnimado(const SpriteAnimado *sprite, Vector2 posicao, Color tint,
                           bool inverterHorizontal)
{
    Rectangle origem;
    Rectangle destino;

    if (sprite == NULL || sprite->textura.id == 0 || sprite->largura_frame <= 0 ||
        sprite->altura_frame <= 0 || sprite->quadro_atual < 0 ||
        sprite->quadro_atual >= sprite->total_quadros) {
        return;
    }

    origem.x = (float)(sprite->quadro_atual * sprite->largura_frame);
    origem.y = 0.0f;
    origem.width = (float)sprite->largura_frame;
    origem.height = (float)sprite->altura_frame;
    if (inverterHorizontal) {
        origem.x += origem.width;
        origem.width = -origem.width;
    }
    destino.x = posicao.x;
    destino.y = posicao.y;
    destino.width = (float)sprite->largura_frame;
    destino.height = (float)sprite->altura_frame;
    DrawTexturePro(sprite->textura, origem, destino, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

// Libera a textura e reinicia os dados da animação.
void descarregarSpriteAnimado(SpriteAnimado *sprite)
{
    if (sprite == NULL) {
        return;
    }
    if (sprite->textura.id != 0) {
        UnloadTexture(sprite->textura);
    }
    memset(sprite, 0, sizeof(*sprite));
}

// Carrega uma spritesheet em loop para integração com o gerenciador de animações.
void registrarAnimacaoSprite(SpriteAnimado *sprite, const char *caminhoPng, int totalQuadros,
                             float fps)
{
    (void)carregarSpriteAnimado(sprite, caminhoPng, totalQuadros, fps, true);
}
