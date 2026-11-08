#ifndef ESTRUTURAS_H
#define ESTRUTURAS_H

#include <stdbool.h>
#include <stddef.h>

#define LINHAS_TABULEIRO 3
#define COLUNAS_TABULEIRO 6
#define MAX_HABILIDADES 3
#define MAX_ITENS_PASSIVOS 5
#define PCT_SANGRAMENTO_TURNO 15
#define PCT_VENENO_TURNO 10
#define DANO_SANGRAMENTO_TURNO PCT_SANGRAMENTO_TURNO
#define DANO_VENENO_TURNO PCT_VENENO_TURNO

typedef enum {
    ROTA_RAIZ,
    ROTA_CAMINHO_1,
    ROTA_CAMINHO_2,
    ROTA_LOJA,
    ROTA_CHEFE
} TipoNoRota;

typedef enum {
    ITEM_CONSUMIVEL,
    ITEM_PASSIVO
} TipoItem;

typedef enum {
    RARIDADE_COMUM,
    RARIDADE_RARO,
    RARIDADE_EPICO,
    RARIDADE_LENDARIO
} RaridadeItem;

typedef enum {
    CLASSE_BUCANEIRO,
    CLASSE_TIMONEIRO,
    CLASSE_COZINHEIRO,
    CLASSE_CAPITAO,
    CLASSE_GAVEA
} ClassePirata;

typedef enum {
    INIMIGO_GRUNT_ESPADA,
    INIMIGO_GRUNT_PISTOLA,
    INIMIGO_ATIRADOR_MAGRELO,
    INIMIGO_MARUJO_GORDO,
    INIMIGO_BOSS_BARBA_NEGRA
} TipoInimigo;

typedef enum {
    EFEITO_DANO,
    EFEITO_BOMBA,
    EFEITO_MOTIVACAO,
    EFEITO_ESTUNAR,
    EFEITO_SANGRAMENTO,
    EFEITO_VENENO,
    EFEITO_CURA,
    EFEITO_KNOCKBACK,
    EFEITO_DESNORTEAR,
    EFEITO_DESMORALIZAR,
    EFEITO_CURA_CONTINUA
} TipoEfeito;

typedef enum {
    CONSUMIVEL_SEM_EFEITO,
    CONSUMIVEL_CURAR_VIDA,
    CONSUMIVEL_RESTAURAR_ESTAMINA,
    CONSUMIVEL_REMOVER_STATUS,
    CONSUMIVEL_TONICO
} TipoEfeitoConsumivel;

typedef enum {
    ATAQUE_MELEE,
    ATAQUE_RANGED,
    ATAQUE_LINHA,
    ATAQUE_CONE,
    ATAQUE_CRUZ,
    ATAQUE_AREA_1X3,
    ATAQUE_AREA_2X3,
    ATAQUE_AREA_3X2,
    ATAQUE_DASH_CARDEAL,
    ATAQUE_HIBRIDO_SAQUEAR
} TipoAlcanceAtaque;

typedef struct {
    int id;
    char nome[48];
    TipoItem tipo;
    RaridadeItem raridade;
    int quantidade;
    int preco;
    int bonus_vida;
    int bonus_velocidade;
    int bonus_dano;
    int bonus_estamina;
    int bonus_precisao;
    int reducao_dano_percentual;
    int bonus_dano_melee_percentual;
    int bonus_dano_ranged_percentual;
    int bonus_acoes_basicas;
    TipoEfeitoConsumivel efeito_consumivel;
    int valor_efeito;
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
    int dano_min;
    int dano_max;
    int precisao;
    int alcance;
    int custo_estamina;
    TipoEfeito efeito;
    TipoAlcanceAtaque tipo_ataque;
    bool fogo_amigo;
    bool perfurante;
    int duracao_efeito;
} Habilidade;

typedef struct Combatente {
    int id;
    char nome[48];
    int vida_atual;
    int vida_maxima;
    int estamina_atual;
    int estamina_maxima;
    int estamina_por_nivel[6];
    int velocidade;
    int defesa;
    int dano_base;
    int precisao_melee;
    int precisao_ranged;
    int nivel;
    int experiencia;
    int linha;
    int coluna;
    int direcao_linha;
    int direcao_coluna;
    bool aliado;
    bool vivo;
    bool eh_boss;
    int fase_boss;
    ClassePirata classe;
    TipoInimigo tipo_inimigo;
    bool em_descanso;
    bool atacado_no_descanso;
    bool turno_perdido;
    bool passiva_aprimorada;
    bool usou_consumivel_gratis_turno;
    bool motivado_capitao;
    int bonus_estamina_proximo_turno;
    int acoes_basicas_restantes;
    int habilidades_desbloqueadas;
    int golpes_corte_bucaneiro_recebidos;
    int golpes_corte_grunt_recebidos;
    int turnos_desde_uso[MAX_HABILIDADES];
    int turnos_motivacao;
    int turnos_atordoado;
    int turnos_sangramento;
    int turnos_veneno;
    int turnos_desnorteado;
    int turnos_desmoralizado;
    int turnos_tonico;
    int bonus_tonico_percentual;
    int turnos_cura_hot;
    int valor_cura_hot;
    ListaItens build_passivos;
    Habilidade habilidades[MAX_HABILIDADES];
} Combatente;

typedef struct {
    Combatente *celulas[LINHAS_TABULEIRO][COLUNAS_TABULEIRO];
    int mina_ativa[LINHAS_TABULEIRO][COLUNAS_TABULEIRO];
    int ultimo_dano_celula[LINHAS_TABULEIRO][COLUNAS_TABULEIRO];
    int ultima_cura_celula[LINHAS_TABULEIRO][COLUNAS_TABULEIRO];
    bool boss_fase_2_ativa;
    int turnos_boss_fase_2;
} Tabuleiro3x6;

typedef struct NoCelula {
    int linha;
    int coluna;
    int perigo_fogo_amigo;
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
// Encerra a aventura, libera suas rotas e reconstrói as três rotas.
NoRota *fugirDaAventura(NoRota *raiz, ListaItens *inventarioGeral, int *nivelDificuldade);
// Move um item passivo do inventário para a build do personagem.
bool equiparItemPassivoBuild(ListaItens *inventario, ListaItens *buildPirata, int idItem);
// Equipa um item passivo no combatente e aplica seus bônus.
bool aplicarItemPassivoNoPirata(ListaItens *inventario, Combatente *pirata, int idItem);
// Remove um item passivo do combatente e seus bônus.
bool desequiparItemPassivoBuild(ListaItens *inventario, Combatente *pirata, int idItem);
// Lista as células de alcance e perigo da habilidade escolhida.
ListaAlcance *calcularAreaAtuacao3x6(Tabuleiro3x6 *mapa, Combatente *origem, int indiceHabilidade);
// Libera a árvore de rotas, inventário e lista de alcance.
void liberarEstruturasJogo(NoRota *raiz, ListaItens *inventario, ListaAlcance *alcance);

#endif
