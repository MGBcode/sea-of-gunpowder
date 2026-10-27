#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

#include <stdbool.h>
#include <stddef.h>

#define LINHAS_TABULEIRO 3
#define COLUNAS_TABULEIRO 6
#define MAX_HABILIDADES 3
#define MAX_ITENS_PASSIVOS 5

typedef enum {
    ROTA_RAIZ,
    ROTA_CAMINHO_1,
    ROTA_CAMINHO_2,
    ROTA_LOJA,
    ROTA_CHEFE
} TipoNoRota;

typedef enum {
    ITEM_CONSUMIVEL,
    ITEM_PASSIVO,
    ITEM_ATIVO
} TipoItem;

typedef enum {
    EFEITO_DANO,
    EFEITO_BOMBA,
    EFEITO_MOTIVACAO,
    EFEITO_ESTUNAR,
    EFEITO_SANGRAMENTO,
    EFEITO_VENENO
} TipoEfeito;

typedef struct {
    int id;
    char nome[48];
    TipoItem tipo;
    int quantidade;
    int preco;
    int bonus_vida;
    int bonus_velocidade;
    int bonus_dano;
} Item;

typedef struct NoItem {
    Item item;
    struct NoItem *proximo;
} NoItem;

typedef struct {
    NoItem *inicio;
    int tamanho;
} ListaItens;

typedef struct {
    char nome[48];
    int dano;
    int alcance;
    int custo_estamina;
    int chance_critico;
    int raio_area;
    TipoEfeito efeito;
    bool disparo;
} Habilidade;

typedef struct Combatente {
    int id;
    char nome[48];
    int vida_atual;
    int vida_maxima;
    int estamina_atual;
    int estamina_maxima;
    int velocidade;
    int defesa;
    int dano_base;
    int nivel;
    int experiencia;
    int linha;
    int coluna;
    bool aliado;
    bool vivo;
    bool em_descanso;
    bool atacado_no_descanso;
    bool turno_perdido;
    bool passiva_aprimorada;
    int ganho_estamina_nivel;
    int habilidades_desbloqueadas;
    Habilidade habilidades[MAX_HABILIDADES];
    int turnos_desde_uso[MAX_HABILIDADES];
    ListaItens build_passivos;
    int turnos_motivacao;
    int turnos_atordoado;
    int turnos_sangramento;
    int turnos_veneno;
} Combatente;

typedef struct {
    Combatente *celulas[LINHAS_TABULEIRO][COLUNAS_TABULEIRO];
} Tabuleiro3x6;

typedef struct NoCelula {
    int linha;
    int coluna;
    struct NoCelula *proximo;
} NoCelula;

typedef struct {
    NoCelula *inicio;
    int tamanho;
} ListaAlcance;

typedef struct NoRota {
    int nivel_dificuldade;
    int etapa;
    TipoNoRota tipo;
    bool chefe_final;
    struct NoRota *caminho1;
    struct NoRota *caminho2;
    struct NoRota *loja;
} NoRota;

// Constrói a árvore ternária de rotas até o confronto final.
NoRota *construirArvoreRotas(int nivelDificuldade);
// Retorna o próximo nó conforme a rota escolhida.
NoRota *avancarParaRota(NoRota *noAtual, int escolha);
// Encerra a aventura, libera suas rotas e reinicia a dificuldade.
NoRota *fugirDaAventura(NoRota *raiz, ListaItens *inventarioGeral, int *nivelDificuldade);
// Move um item passivo do inventário para a build do personagem.
bool equiparItemPassivoBuild(ListaItens *inventario, ListaItens *buildPirata, int idItem);
// Equipa um item passivo no combatente e aplica seus bônus.
bool aplicarItemPassivoNoPirata(ListaItens *inventario, Combatente *pirata, int idItem);
// Remove um item passivo do combatente e seus bônus.
bool desequiparItemPassivoBuild(ListaItens *inventario, Combatente *pirata, int idItem);
// Lista as células do tabuleiro dentro do alcance da habilidade escolhida.
ListaAlcance *calcularAreaAtuacao3x6(Tabuleiro3x6 *mapa, Combatente *origem, int indiceHabilidade);
// Libera a árvore de rotas, o inventário e a lista de alcance.
void liberarEstruturasJogo(NoRota *raiz, ListaItens *inventario, ListaAlcance *alcance);

#endif
