#ifndef INTERFACE_H
#define INTERFACE_H

#include "estruturas.h"

typedef enum {
    ESTADO_MENU,
    ESTADO_SELECAO_GRUPO,
    ESTADO_MAPA_ROTAS,
    ESTADO_PRE_COMBATE,
    ESTADO_COMBATE,
    ESTADO_LOJA,
    ESTADO_INVENTARIO
} EstadoJogo;

typedef struct {
    EstadoJogo estado;
    int linha_selecionada;
    int coluna_selecionada;
    int largura_celula;
    int altura_celula;
    int margem_esquerda;
    int margem_superior;
    int piratas_selecionados_ids[3];
    int total_selecionados;
    int pirata_posicionando_indice;
} InterfaceJogo;

// Inicializa o estado visual e abre a janela Raylib.
void inicializarInterface(InterfaceJogo *interfaceJogo);
// Processa as seleções de grupo e os cliques no tabuleiro.
void atualizarInterface(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                        Combatente piratasDisponiveis[], int totalDisponiveis);
// Desenha os cards dos piratas e o controle para iniciar a aventura.
void desenharTelaSelecaoGrupo(InterfaceJogo *interfaceJogo,
                              const Combatente piratasDisponiveis[], int totalDisponiveis);
// Desenha o estado atual e o tabuleiro interativo 3x6.
void desenharInterface(const InterfaceJogo *interfaceJogo, const Tabuleiro3x6 *mapa,
                       const ListaAlcance *alcance);
// Encerra a janela aberta pela interface.
void encerrarInterface(void);

#endif
