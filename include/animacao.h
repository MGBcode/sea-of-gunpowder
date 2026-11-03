#ifndef ANIMACAO_H
#define ANIMACAO_H

#include "raylib.h"

#include <stdbool.h>

typedef struct {
    Texture2D textura;
    int total_quadros;
    int quadro_atual;
    float tempo_acumulado;
    float duracao_quadro;
    int largura_frame;
    int altura_frame;
    bool ativa;
    bool loop;
} SpriteAnimado;

// Carrega uma spritesheet horizontal e configura sua velocidade de reprodução.
bool carregarSpriteAnimado(SpriteAnimado *sprite, const char *caminhoPng, int totalQuadros,
                           float fps, bool loop);
// Avança os quadros da animação conforme o tempo transcorrido.
void atualizarSpriteAnimado(SpriteAnimado *sprite, float tempoFrame);
// Desenha o quadro atual da spritesheet na posição informada.
void desenharSpriteAnimado(const SpriteAnimado *sprite, Vector2 posicao, Color tint,
                           bool inverterHorizontal);
// Libera a textura e reinicia os dados da animação.
void descarregarSpriteAnimado(SpriteAnimado *sprite);
// Carrega uma spritesheet em loop para integração com o gerenciador de animações.
void registrarAnimacaoSprite(SpriteAnimado *sprite, const char *caminhoPng, int totalQuadros,
                             float fps);

#endif
