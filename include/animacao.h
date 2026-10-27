#ifndef ANIMACAO_H
#define ANIMACAO_H

typedef struct {
    int quadro;
    int total_quadros;
    float tempo_acumulado;
    float duracao_quadro;
    int ativa;
} Animacao;

// Reinicia uma animação com a duração indicada por quadro.
void iniciarAnimacao(Animacao *animacao, int totalQuadros, float duracaoQuadro);
// Atualiza o quadro atual usando o tempo do último frame.
void atualizarAnimacao(Animacao *animacao, float tempoFrame);

#endif

