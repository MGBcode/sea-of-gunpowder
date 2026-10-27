#include "animacao.h"

// Reinicia uma animação com a duração indicada por quadro.
void iniciarAnimacao(Animacao *animacao, int totalQuadros, float duracaoQuadro)
{
    if (animacao == 0 || totalQuadros <= 0 || duracaoQuadro <= 0.0f) {
        return;
    }
    animacao->quadro = 0;
    animacao->total_quadros = totalQuadros;
    animacao->tempo_acumulado = 0.0f;
    animacao->duracao_quadro = duracaoQuadro;
    animacao->ativa = 1;
}

// Atualiza o quadro atual usando o tempo do último frame.
void atualizarAnimacao(Animacao *animacao, float tempoFrame)
{
    if (animacao == 0 || !animacao->ativa || tempoFrame < 0.0f) {
        return;
    }
    animacao->tempo_acumulado += tempoFrame;
    while (animacao->tempo_acumulado >= animacao->duracao_quadro) {
        animacao->tempo_acumulado -= animacao->duracao_quadro;
        animacao->quadro++;
        if (animacao->quadro >= animacao->total_quadros) {
            animacao->quadro = animacao->total_quadros - 1;
            animacao->ativa = 0;
            break;
        }
    }
}

