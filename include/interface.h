#ifndef INTERFACE_H
#define INTERFACE_H

#include "raylib.h"
#include "ordenacao.h"

typedef enum {
    TELA_MENU_PRINCIPAL,
    TELA_SELECAO_TRIPULACAO,
    TELA_MAPA_ROTAS,
    TELA_COMBATE_3X6,
    TELA_VITORIA,
    TELA_FIM_DE_JOGO
} EstadoTelaPrincipal;

typedef enum {
    POPUP_NENHUM,
    POPUP_MOCHILA_BUILDS,
    POPUP_LOJA,
    POPUP_DETALHES_PERSONAGEM,
    POPUP_CONFIRMAR_FUGA,
    POPUP_RECOMPENSAS,
    POPUP_CONFIRMAR_ROTA,
    POPUP_CONFIRMAR_COMPRA,
    POPUP_PAUSE
} TipoPopupModal;

typedef enum {
    ACAO_TELA_NENHUMA,
    ACAO_TELA_MOVER,
    ACAO_TELA_DESCANSAR,
    ACAO_TELA_HABILIDADE_0,
    ACAO_TELA_HABILIDADE_1,
    ACAO_TELA_HABILIDADE_2
} AcaoTelaCombate;

typedef struct {
    Font fonteJogo;
    Texture2D bgMar;
    Texture2D bgNavio;
    EstadoTelaPrincipal estado;
    TipoPopupModal popup_ativo;
    AcaoTelaCombate acao_combate;
    int linha_selecionada;
    int coluna_selecionada;
    int largura_celula;
    int altura_celula;
    int margem_esquerda;
    int margem_superior;
    int piratas_selecionados_ids[3];
    int total_selecionados;
    int ids_sobreviventes_fuga[3];
    int total_sobreviventes_fuga;
    bool piratas_mortos[5];
    int pirata_posicionando_indice;
    int indice_item_selecionado;
    int total_inimigos;
    int total_iniciativa;
    int indice_iniciativa;
    bool fase_pre_combate_ativa;
    bool saque_em_andamento;
    bool loja_aberta_pendente;
    bool sair_solicitado;
    int escolha_rota_pendente;
    int indice_compra_pendente;
    int item_id_selecionado;
    int total_itens_loja;
    Item itens_loja[3];
    int estoques_loja[3];
    Item itens_recompensa[6];
    int total_itens_recompensa;
    float timer_turno_inimigo;
    float timer_destaque_acao;
    float timer_indicador_dano;
    int scroll_detalhes;
    int scroll_mochila;
    int linha_destaque_acao;
    int coluna_destaque_acao;
    int linha_origem_acao;
    int coluna_origem_acao;
    char log_combate[256];
    Combatente *entidade_selecionada;
    Combatente *ator_atual;
    Combatente inimigos[4];
    EntradaIniciativa fila_iniciativa[MAX_ENTRADAS_INICIATIVA];
    NoRota **rota_atual;
    NoRota **raiz_rotas;
    ListaItens *inventario;
    int *moedas;
    int *nivel_dificuldade;
} InterfaceJogo;

// Inicializa o estado visual e abre a janela Raylib.
void inicializarInterface(InterfaceJogo *interfaceJogo);
// Desenha texto em coordenadas virtuais usando a fonte da interface atual.
void desenharTextoGlobal(const char *texto, Vector2 pos, float tamanho, Color cor);
// Processa menus, pop-ups, rota, seleção de unidade e cliques de combate.
void atualizarInterface(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                        Combatente piratasDisponiveis[], int totalDisponiveis,
                        NoRota **raizRotas, NoRota **rotaAtual,
                        ListaItens *inventario, int *moedas,
                        int *nivelDificuldade);
// Desenha os cards dos piratas e o controle para iniciar a aventura.
void desenharTelaSelecaoGrupo(InterfaceJogo *interfaceJogo,
                              const Combatente piratasDisponiveis[], int totalDisponiveis);
// Desenha o estado atual, tabuleiro interativo e pop-up ativo.
void desenharInterface(const InterfaceJogo *interfaceJogo, const Tabuleiro3x6 *mapa,
                       const ListaAlcance *alcance);
// Converte coordenadas reais da janela/tela cheia para a resolução virtual 1280x720.
bool converterCoordenadaMouseVirtual(float mouseRealX, float mouseRealY,
                                     int larguraJanela, int alturaJanela,
                                     int *virtualX, int *virtualY);
// Processa a desistência, reseta piratas mortos ao nível 1 e solicita reposição.
void processarFugaETrocaTripulacao(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                                   Combatente piratasDisponiveis[], int totalDisponiveis);
// Libera os recursos visuais e encerra a janela Raylib.
void descarregarInterface(InterfaceJogo *interfaceJogo);

#endif
