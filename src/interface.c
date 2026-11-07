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

static Font fonteJogo;
static bool fonteCarregada = false;
static RenderTexture2D canvasVirtual;
static bool canvasCarregado = false;

// Converte coordenadas reais da janela/tela cheia para a resolução virtual 1280x720.
bool converterCoordenadaMouseVirtual(float mouseRealX, float mouseRealY,
                                     int larguraJanela, int alturaJanela,
                                     int *virtualX, int *virtualY)
{
    if (larguraJanela <= 0 || alturaJanela <= 0 || virtualX == NULL || virtualY == NULL) {
        return false;
    }
    *virtualX = (int)((mouseRealX * (float)LARGURA_VIRTUAL) / (float)larguraJanela);
    *virtualY = (int)((mouseRealY * (float)ALTURA_VIRTUAL) / (float)alturaJanela);
    if (*virtualX < 0) {
        *virtualX = 0;
    } else if (*virtualX >= LARGURA_VIRTUAL) {
        *virtualX = LARGURA_VIRTUAL - 1;
    }
    if (*virtualY < 0) {
        *virtualY = 0;
    } else if (*virtualY >= ALTURA_VIRTUAL) {
        *virtualY = ALTURA_VIRTUAL - 1;
    }
    return true;
}

// Obtém a posição atual do mouse já mapeada para a resolução virtual 1280x720.
static Vector2 obterMouseVirtual(void)
{
    Vector2 mouseReal = GetMousePosition();
    int virtualX = (int)mouseReal.x;
    int virtualY = (int)mouseReal.y;
    if (converterCoordenadaMouseVirtual(mouseReal.x, mouseReal.y,
                                        GetScreenWidth(), GetScreenHeight(),
                                        &virtualX, &virtualY)) {
        return (Vector2){ (float)virtualX, (float)virtualY };
    }
    return mouseReal;
}

// Inicia o desenho no framebuffer virtual 1280x720 para suportar maximizar e tela cheia.
static void iniciarQuadroVirtual(Color corFundo)
{
    if (canvasCarregado && canvasVirtual.id != 0) {
        BeginTextureMode(canvasVirtual);
        ClearBackground(corFundo);
    } else {
        BeginDrawing();
        ClearBackground(corFundo);
    }
}

// Finaliza o framebuffer virtual e o escala suavemente para o tamanho real da janela.
static void finalizarQuadroVirtual(void)
{
    if (canvasCarregado && canvasVirtual.id != 0) {
        Rectangle origem = { 0.0f, 0.0f,
                             (float)canvasVirtual.texture.width,
                             -(float)canvasVirtual.texture.height };
        Rectangle destino = { 0.0f, 0.0f,
                              (float)GetScreenWidth(),
                              (float)GetScreenHeight() };
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

// Renderiza texto nítido com filtro bilinear usando a fonte TrueType carregada.
static void desenharTexto(const char *texto, int posX, int posY, int tamanho, Color cor)
{
    if (texto == NULL) {
        return;
    }
    if (fonteCarregada && fonteJogo.texture.id != 0) {
        DrawTextEx(fonteJogo, texto, (Vector2){ (float)posX, (float)posY },
                   (float)(tamanho + 2), 1.0f, cor);
    } else {
        DrawText(texto, posX, posY, tamanho, cor);
    }
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
        return "Epico";
    }
    return "Lendario";
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

// Avança a fila de iniciativa até o próximo combatente vivo apto a agir.
static void avancarIniciativa(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
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
            pirata->aliado = true;
            pirata->em_descanso = false;
            pirata->atacado_no_descanso = false;
            pirata->turno_perdido = false;
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

// Cria inimigos ou o chefe e abre a Fase Pré-Batalha na tela 3x6.
static void iniciarEncontro(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                            Combatente piratas[], int totalPiratas)
{
    int indice;
    int linha;
    int coluna;
    int indices[4];
    int etapaAtual;
    if (interfaceJogo->rota_atual == NULL || *interfaceJogo->rota_atual == NULL) {
        return;
    }
    limparMapa(mapa);
    posicionarTripulacao(interfaceJogo, mapa, piratas, totalPiratas);
    etapaAtual = (*interfaceJogo->rota_atual)->etapa;
    if ((*interfaceJogo->rota_atual)->tipo == ROTA_CHEFE) {
        interfaceJogo->total_inimigos = 1;
        if (!criarChefeFinal(&interfaceJogo->inimigos[0])) {
            TraceLog(LOG_ERROR, "Nao foi possivel criar Barba Negra.");
            interfaceJogo->estado = TELA_FIM_DE_JOGO;
            return;
        }
        escalonarInimigoPorRota(&interfaceJogo->inimigos[0], ROTA_CHEFE,
                                 etapaAtual, contadorFugas > 0, false);
        interfaceJogo->inimigos[0].linha = 1;
        interfaceJogo->inimigos[0].coluna = 5;
        mapa->celulas[1][5] = &interfaceJogo->inimigos[0];
    } else {
        unsigned int semente = (unsigned int)(*interfaceJogo->rota_atual)->nivel_dificuldade;
        semente += (unsigned int)(etapaAtual * 31);
        semente += (unsigned int)(contadorSaques * 17 + contadorFugas * 13);
        interfaceJogo->total_inimigos =
            sortearInimigosPorEtapa((*interfaceJogo->rota_atual)->tipo, etapaAtual,
                                    semente, indices, 4);
        for (indice = 0; indice < interfaceJogo->total_inimigos; indice++) {
            Combatente *inimigo = &interfaceJogo->inimigos[indice];
            int posicao = indice;
            if (!criarInimigoInicial(indices[indice], inimigo)) {
                TraceLog(LOG_ERROR, "Nao foi possivel criar inimigo de indice %d.",
                         indices[indice]);
                interfaceJogo->estado = TELA_FIM_DE_JOGO;
                return;
            }
            escalonarInimigoPorRota(inimigo, (*interfaceJogo->rota_atual)->tipo,
                                     etapaAtual, contadorFugas > 0,
                                     contadorSaques > 0 ||
                                     interfaceJogo->saque_em_andamento ||
                                     (*interfaceJogo->rota_atual)->tipo == ROTA_LOJA);
            linha = posicao % LINHAS_TABULEIRO;
            coluna = posicao < LINHAS_TABULEIRO ? 5 : 4;
            if (mapa->celulas[linha][coluna] != NULL) {
                coluna--;
            }
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
                   "Posicione sua tripulacao nas Colunas 0 e 1 antes da batalha.");
    interfaceJogo->fase_pre_combate_ativa = true;
    interfaceJogo->estado = TELA_COMBATE_3X6;
}

// Aplica a recompensa da rota ou do saque da loja após vencer o combate.
static void concluirVitoria(InterfaceJogo *interfaceJogo, Tabuleiro3x6 *mapa,
                            Combatente piratas[], int totalPiratas)
{
    TipoNoRota tipoAtual = (*interfaceJogo->rota_atual)->tipo;
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
    if (tipoAtual == ROTA_CHEFE) {
        interfaceJogo->estado = TELA_VITORIA;
    } else {
        limparMapa(mapa);
        posicionarTripulacao(interfaceJogo, mapa, piratas, totalPiratas);
        interfaceJogo->estado = TELA_MAPA_ROTAS;
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
    if (inimigo == NULL || !inimigo->vivo || inimigo->aliado ||
        inimigo->turno_perdido || inimigo->em_descanso ||
        inimigo->acoes_basicas_restantes <= 0) {
        avancarIniciativa(interfaceJogo, mapa, piratas, totalPiratas);
        return;
    }
    interfaceJogo->linha_origem_acao = inimigo->linha;
    interfaceJogo->coluna_origem_acao = inimigo->coluna;
    if (!selecionarAcaoIA(mapa, inimigo, &escolha) ||
        escolha.tipo_acao == ACAO_IA_DESCANSAR) {
        iniciarDescanso(mapa, inimigo);
        inimigo->acoes_basicas_restantes = 0;
        interfaceJogo->linha_destaque_acao = inimigo->linha;
        interfaceJogo->coluna_destaque_acao = inimigo->coluna;
        interfaceJogo->timer_destaque_acao = 1.0f;
        (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                       "ACAO INIMIGA: %s entrou em Descanso em (%d,%d).",
                       inimigo->nome, inimigo->linha, inimigo->coluna);
    } else if (escolha.tipo_acao == ACAO_IA_HABILIDADE) {
        Combatente *alvo = mapa->celulas[escolha.linha_alvo][escolha.coluna_alvo];
        char nomeAlvo[48] = "area";
        int vidaAntes = 0;
        int danoCausado = 0;
        if (alvo != NULL) {
            (void)strncpy(nomeAlvo, alvo->nome, sizeof(nomeAlvo) - 1);
            nomeAlvo[sizeof(nomeAlvo) - 1] = '\0';
            vidaAntes = alvo->vida_atual;
        }
        executarAtaqueOuHabilidade(mapa, inimigo, escolha.indice_habilidade,
                                   escolha.linha_alvo, escolha.coluna_alvo);
        if (alvo != NULL && vidaAntes > alvo->vida_atual) {
            danoCausado = vidaAntes - alvo->vida_atual;
        }
        inimigo->acoes_basicas_restantes--;
        interfaceJogo->linha_destaque_acao = escolha.linha_alvo;
        interfaceJogo->coluna_destaque_acao = escolha.coluna_alvo;
        interfaceJogo->timer_destaque_acao = 1.0f;
        if (alvo != NULL && danoCausado > 0) {
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "ACAO INIMIGA: %s usou %s em %s (-%d PV)!",
                           inimigo->nome,
                           inimigo->habilidades[escolha.indice_habilidade].nome,
                           nomeAlvo, danoCausado);
        } else if (alvo != NULL &&
                   inimigo->habilidades[escolha.indice_habilidade].efeito ==
                   EFEITO_CURA_CONTINUA) {
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "ACAO INIMIGA: %s usou %s em %s (+15 PV/turno)!",
                           inimigo->nome,
                           inimigo->habilidades[escolha.indice_habilidade].nome,
                           nomeAlvo);
        } else {
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "ACAO INIMIGA: %s usou %s em (%d,%d)!",
                           inimigo->nome,
                           inimigo->habilidades[escolha.indice_habilidade].nome,
                           escolha.linha_alvo, escolha.coluna_alvo);
        }
    } else if (escolha.tipo_acao == ACAO_IA_MOVER) {
        int origLin = inimigo->linha;
        int origCol = inimigo->coluna;
        if (moverCombatente3x6(mapa, inimigo, escolha.linha_alvo,
                               escolha.coluna_alvo)) {
            inimigo->acoes_basicas_restantes--;
            interfaceJogo->linha_destaque_acao = escolha.linha_alvo;
            interfaceJogo->coluna_destaque_acao = escolha.coluna_alvo;
            interfaceJogo->timer_destaque_acao = 1.0f;
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "ACAO INIMIGA: %s moveu de (%d,%d) para (%d,%d).",
                           inimigo->nome, origLin, origCol,
                           escolha.linha_alvo, escolha.coluna_alvo);
        } else {
            iniciarDescanso(mapa, inimigo);
            inimigo->acoes_basicas_restantes = 0;
            interfaceJogo->linha_destaque_acao = inimigo->linha;
            interfaceJogo->coluna_destaque_acao = inimigo->coluna;
            interfaceJogo->timer_destaque_acao = 1.0f;
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "ACAO INIMIGA: %s encerrou o turno em (%d,%d).",
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
            NoItem *item;
            interfaceJogo->ator_atual->acoes_basicas_restantes = 2;
            for (item = interfaceJogo->ator_atual->build_passivos.inicio;
                 item != NULL; item = item->proximo) {
                if (item->item.bonus_acoes_basicas > 0) {
                    interfaceJogo->ator_atual->acoes_basicas_restantes +=
                        item->item.bonus_acoes_basicas;
                }
            }
            interfaceJogo->entidade_selecionada = interfaceJogo->ator_atual;
            interfaceJogo->timer_turno_inimigo = 0.0f;
            return;
        }
        interfaceJogo->ator_atual->acoes_basicas_restantes = 2;
        interfaceJogo->entidade_selecionada = interfaceJogo->ator_atual;
        interfaceJogo->linha_origem_acao = interfaceJogo->ator_atual->linha;
        interfaceJogo->coluna_origem_acao = interfaceJogo->ator_atual->coluna;
        interfaceJogo->linha_destaque_acao = interfaceJogo->ator_atual->linha;
        interfaceJogo->coluna_destaque_acao = interfaceJogo->ator_atual->coluna;
        interfaceJogo->timer_destaque_acao = 1.0f;
        interfaceJogo->timer_turno_inimigo = 1.0f;
        (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                       "VEZ DO INIMIGO: %s preparando acao...",
                       interfaceJogo->ator_atual->nome);
        return;
    }
    if (interfaceJogo->estado == TELA_COMBATE_3X6) {
        TraceLog(LOG_WARNING, "A fila de iniciativa excedeu o limite de seguranca.");
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
    *interfaceJogo->raiz_rotas =
        fugirDaAventura(*interfaceJogo->raiz_rotas, interfaceJogo->inventario,
                        interfaceJogo->nivel_dificuldade);
    *interfaceJogo->rota_atual = *interfaceJogo->raiz_rotas;
    interfaceJogo->total_sobreviventes_fuga = 0;
    for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
        int idPirata = interfaceJogo->piratas_selecionados_ids[indice];
        Combatente *pirata = pirataPorId(piratasDisponiveis, totalDisponiveis, idPirata);
        if (pirata != NULL && pirata->vivo) {
            pirata->vida_atual = pirata->vida_maxima;
            pirata->estamina_atual = pirata->estamina_maxima;
            pirata->turnos_sangramento = 0;
            pirata->turnos_veneno = 0;
            pirata->turnos_atordoado = 0;
            pirata->turnos_desmoralizado = 0;
            pirata->turnos_desnorteado = 0;
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
    interfaceJogo->fase_pre_combate_ativa = false;
    interfaceJogo->saque_em_andamento = false;
    interfaceJogo->loja_aberta_pendente = false;
    interfaceJogo->total_itens_loja =
        sortearItensLoja(1, (unsigned int)GetRandomValue(1, 100000),
                         interfaceJogo->itens_loja, interfaceJogo->estoques_loja, 3);
    interfaceJogo->estado = TELA_SELECAO_TRIPULACAO;
}

// Inicializa a janela redimensionável, o framebuffer virtual 1280x720 e a fonte TrueType.
void inicializarInterface(InterfaceJogo *interfaceJogo)
{
    static const char *caminhosFontes[3] = {
        "C:/Windows/Fonts/segoeuisb.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    int codepoints[224];
    int indice;
    if (interfaceJogo == NULL) {
        return;
    }
    (void)memset(interfaceJogo, 0, sizeof(*interfaceJogo));
    interfaceJogo->estado = TELA_MENU_PRINCIPAL;
    interfaceJogo->popup_ativo = POPUP_NENHUM;
    interfaceJogo->linha_selecionada = -1;
    interfaceJogo->coluna_selecionada = -1;
    interfaceJogo->linha_destaque_acao = -1;
    interfaceJogo->coluna_destaque_acao = -1;
    interfaceJogo->linha_origem_acao = -1;
    interfaceJogo->coluna_origem_acao = -1;
    interfaceJogo->largura_celula = 180;
    interfaceJogo->altura_celula = 148;
    interfaceJogo->margem_esquerda = 100;
    interfaceJogo->margem_superior = 130;
    interfaceJogo->pirata_posicionando_indice = -1;
    interfaceJogo->indice_iniciativa = -1;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(LARGURA_VIRTUAL, ALTURA_VIRTUAL, "Cronicas de Sangue e Polvora");
    SetExitKey(KEY_NULL);
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);
    canvasVirtual = LoadRenderTexture(LARGURA_VIRTUAL, ALTURA_VIRTUAL);
    if (canvasVirtual.id != 0) {
        SetTextureFilter(canvasVirtual.texture, TEXTURE_FILTER_BILINEAR);
        canvasCarregado = true;
    }
    for (indice = 0; indice < 224; indice++) {
        codepoints[indice] = 32 + indice;
    }
    fonteCarregada = false;
    for (indice = 0; indice < 3; indice++) {
        if (FileExists(caminhosFontes[indice])) {
            fonteJogo = LoadFontEx(caminhosFontes[indice], 32, codepoints, 224);
            if (fonteJogo.texture.id != 0) {
                SetTextureFilter(fonteJogo.texture, TEXTURE_FILTER_BILINEAR);
                fonteCarregada = true;
                break;
            }
        }
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
            bool clicouBotaoAtivador =
                (interfaceJogo->estado == TELA_MAPA_ROTAS &&
                 retanguloClicado((Rectangle){ 510, 630, 190, 55 }, mouse)) ||
                (interfaceJogo->estado == TELA_COMBATE_3X6 &&
                 retanguloClicado((Rectangle){ 975, 630, 130, 55 }, mouse));
            if (interfaceJogo->estado == TELA_SELECAO_TRIPULACAO) {
                int limite = totalDisponiveis < TOTAL_PIRATAS_PREDEFINIDOS ?
                             totalDisponiveis : TOTAL_PIRATAS_PREDEFINIDOS;
                for (indice = 0; indice < limite; indice++) {
                    int cardX = 60 + (indice % 3) * 390;
                    int cardY = 145 + (indice / 3) * 205;
                    if (retanguloClicado((Rectangle){ (float)(cardX + 230),
                                                      (float)(cardY + 138),
                                                      108.0f, 32.0f }, mouse)) {
                        clicouBotaoAtivador = true;
                    }
                }
            }
            if (clicouBotaoAtivador ||
                retanguloClicado((Rectangle){ 760, 68, 85, 38 }, mouse)) {
                interfaceJogo->popup_ativo = POPUP_NENHUM;
            }
            return;
        }
        if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_FUGA) {
            if (retanguloClicado((Rectangle){ 300, 630, 190, 55 }, mouse) ||
                retanguloClicado((Rectangle){ 530, 420, 150, 50 }, mouse)) {
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                return;
            }
            if (interfaceJogo->estado == TELA_MAPA_ROTAS &&
                retanguloClicado((Rectangle){ 350, 420, 150, 50 }, mouse)) {
                processarFugaETrocaTripulacao(interfaceJogo, mapa,
                                              piratasDisponiveis, totalDisponiveis);
            }
            return;
        }
        if (interfaceJogo->popup_ativo == POPUP_LOJA) {
            if (retanguloClicado((Rectangle){ 740, 72, 105, 38 }, mouse)) {
                interfaceJogo->loja_aberta_pendente = false;
                interfaceJogo->popup_ativo = POPUP_NENHUM;
                return;
            }
            for (indice = 0; indice < interfaceJogo->total_itens_loja; indice++) {
                Rectangle botaoCompra = { 190.0f, 175.0f + (float)indice * 92.0f,
                                          640.0f, 78.0f };
                if (retanguloClicado(botaoCompra, mouse)) {
                    (void)comprarItemNaLoja(interfaceJogo->itens_loja,
                                            interfaceJogo->estoques_loja,
                                            interfaceJogo->total_itens_loja,
                                            indice, inventario, moedas);
                    return;
                }
            }
            if (retanguloClicado((Rectangle){ 185, 495, 200, 55 }, mouse) ||
                retanguloClicado((Rectangle){ 90, 630, 190, 55 }, mouse)) {
                if ((interfaceJogo->entidade_selecionada == NULL ||
                     !interfaceJogo->entidade_selecionada->aliado ||
                     !interfaceJogo->entidade_selecionada->vivo) &&
                    interfaceJogo->total_selecionados > 0) {
                    interfaceJogo->entidade_selecionada =
                        pirataPorId(piratasDisponiveis, totalDisponiveis,
                                    interfaceJogo->piratas_selecionados_ids[0]);
                }
                interfaceJogo->loja_aberta_pendente = true;
                interfaceJogo->popup_ativo = POPUP_MOCHILA_BUILDS;
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
        }
        if (interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS) {
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
                Combatente *alvoUso = interfaceJogo->entidade_selecionada;
                if (alvoUso == NULL || !alvoUso->aliado || !alvoUso->vivo) {
                    alvoUso = interfaceJogo->ator_atual;
                }
                if ((alvoUso == NULL || !alvoUso->aliado || !alvoUso->vivo) &&
                    interfaceJogo->total_selecionados > 0) {
                    alvoUso = pirataPorId(piratasDisponiveis, totalDisponiveis,
                                          interfaceJogo->piratas_selecionados_ids[0]);
                    interfaceJogo->entidade_selecionada = alvoUso;
                }
                for (item = inventario->inicio; item != NULL; item = item->proximo) {
                    Rectangle linhaItem;
                    if (interfaceJogo->estado == TELA_COMBATE_3X6 &&
                        !interfaceJogo->fase_pre_combate_ativa &&
                        item->item.tipo != ITEM_CONSUMIVEL) {
                        continue;
                    }
                    if (itemIndice >= 6) {
                        break;
                    }
                    linhaItem = (Rectangle){ 180.0f, 182.0f + (float)itemIndice * 34.0f,
                                             660.0f, 30.0f };
                    if (retanguloClicado(linhaItem, mouse)) {
                        if (item->item.tipo == ITEM_CONSUMIVEL && alvoUso != NULL &&
                            alvoUso->aliado && alvoUso->vivo) {
                            Combatente *cozinheiroAtivo = interfaceJogo->ator_atual;
                            bool turnoCozinheiroGratis =
                                interfaceJogo->estado == TELA_COMBATE_3X6 &&
                                !interfaceJogo->fase_pre_combate_ativa &&
                                cozinheiroAtivo != NULL && cozinheiroAtivo->aliado &&
                                cozinheiroAtivo->classe == CLASSE_COZINHEIRO &&
                                !cozinheiroAtivo->usou_consumivel_gratis_turno;
                            bool aprimoradoCozinheiro =
                                cozinheiroAtivo != NULL && cozinheiroAtivo->aliado &&
                                cozinheiroAtivo->classe == CLASSE_COZINHEIRO &&
                                cozinheiroAtivo->passiva_aprimorada;
                            bool flagSalva = alvoUso->passiva_aprimorada;
                            ClassePirata classeSalva = alvoUso->classe;
                            if (aprimoradoCozinheiro) {
                                alvoUso->passiva_aprimorada = true;
                                alvoUso->classe = CLASSE_COZINHEIRO;
                            }
                            if (usarItemConsumivel(alvoUso, &item->item)) {
                                alvoUso->passiva_aprimorada = flagSalva;
                                alvoUso->classe = classeSalva;
                                if (alvoUso->nivel >= 4) {
                                    alvoUso->passiva_aprimorada = true;
                                }
                                limparConsumiveisZerados(inventario);
                                if (interfaceJogo->estado == TELA_COMBATE_3X6 &&
                                    !interfaceJogo->fase_pre_combate_ativa) {
                                    interfaceJogo->popup_ativo = POPUP_NENHUM;
                                    if (turnoCozinheiroGratis) {
                                        cozinheiroAtivo->usou_consumivel_gratis_turno = true;
                                    } else {
                                        concluirAcaoAliada(interfaceJogo, mapa,
                                                           piratasDisponiveis,
                                                           totalDisponiveis);
                                    }
                                }
                            } else {
                                alvoUso->passiva_aprimorada = flagSalva;
                                alvoUso->classe = classeSalva;
                            }
                        } else if ((interfaceJogo->estado == TELA_MAPA_ROTAS ||
                                    interfaceJogo->fase_pre_combate_ativa) &&
                                   item->item.tipo == ITEM_PASSIVO &&
                                   alvoUso != NULL &&
                                   alvoUso->aliado &&
                                   alvoUso->vivo) {
                            (void)aplicarItemPassivoNoPirata(
                                inventario, alvoUso, item->item.id);
                        }
                        return;
                    }
                    itemIndice++;
                }
                if (interfaceJogo->entidade_selecionada != NULL &&
                    interfaceJogo->entidade_selecionada->aliado) {
                    itemIndice = 0;
                    for (item = interfaceJogo->entidade_selecionada->build_passivos.inicio;
                         item != NULL; item = item->proximo) {
                        Rectangle linhaItem = { 180.0f, 424.0f + (float)itemIndice * 32.0f,
                                                660.0f, 28.0f };
                        if (retanguloClicado(linhaItem, mouse) &&
                            interfaceJogo->estado == TELA_MAPA_ROTAS) {
                            (void)desequiparItemPassivoBuild(
                                inventario, interfaceJogo->entidade_selecionada,
                                item->item.id);
                            return;
                        }
                        itemIndice++;
                    }
                }
            }
            return;
        }
        return;
    }
    if (interfaceJogo->estado == TELA_MENU_PRINCIPAL) {
        if (retanguloClicado((Rectangle){ 470, 350, 340, 70 }, mouse)) {
            interfaceJogo->estado = TELA_SELECAO_TRIPULACAO;
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
                interfaceJogo->popup_ativo = POPUP_DETALHES_PERSONAGEM;
            }
            return;
        }
        if (rotaAtual != NULL && *rotaAtual != NULL && (*rotaAtual)->etapa >= 3) {
            if (retanguloClicado((Rectangle){ 480, 340, 380, 100 }, mouse) &&
                (*rotaAtual)->caminho1 != NULL) {
                *rotaAtual = avancarParaRota(*rotaAtual, 1);
                iniciarEncontro(interfaceJogo, mapa, piratasDisponiveis, totalDisponiveis);
                return;
            }
        } else {
            if (retanguloClicado((Rectangle){ 440, 290, 260, 95 }, mouse) &&
                rotaAtual != NULL && *rotaAtual != NULL &&
                (*rotaAtual)->caminho1 != NULL) {
                *rotaAtual = avancarParaRota(*rotaAtual, 1);
                interfaceJogo->saque_em_andamento = false;
                iniciarEncontro(interfaceJogo, mapa, piratasDisponiveis, totalDisponiveis);
                return;
            }
            if (retanguloClicado((Rectangle){ 730, 290, 260, 95 }, mouse) &&
                rotaAtual != NULL && *rotaAtual != NULL &&
                (*rotaAtual)->caminho2 != NULL) {
                *rotaAtual = avancarParaRota(*rotaAtual, 2);
                interfaceJogo->saque_em_andamento = false;
                iniciarEncontro(interfaceJogo, mapa, piratasDisponiveis, totalDisponiveis);
                return;
            }
            if (retanguloClicado((Rectangle){ 585, 415, 260, 95 }, mouse) &&
                rotaAtual != NULL && *rotaAtual != NULL &&
                (*rotaAtual)->loja != NULL) {
                unsigned int sementeLoja;
                *rotaAtual = avancarParaRota(*rotaAtual, 3);
                sementeLoja = (unsigned int)((*rotaAtual)->etapa * 97 +
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
            interfaceJogo->popup_ativo = POPUP_MOCHILA_BUILDS;
            return;
        }
        if (interfaceJogo->entidade_selecionada != NULL &&
            retanguloClicado((Rectangle){ 975, 630, 130, 55 }, mouse)) {
            interfaceJogo->popup_ativo = POPUP_DETALHES_PERSONAGEM;
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
            } else if (clicado != NULL) {
                interfaceJogo->entidade_selecionada = clicado;
            }
        }
        return;
    }
    if (retanguloClicado((Rectangle){ 90, 630, 130, 55 }, mouse)) {
        interfaceJogo->acao_combate = ACAO_TELA_MOVER;
    } else if (retanguloClicado((Rectangle){ 235, 630, 130, 55 }, mouse)) {
        if (interfaceJogo->ator_atual != NULL) {
            (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                           "%s entrou em Descanso.", interfaceJogo->ator_atual->nome);
        }
        iniciarDescanso(mapa, interfaceJogo->ator_atual);
        concluirAcaoAliada(interfaceJogo, mapa, piratasDisponiveis, totalDisponiveis);
    } else if (retanguloClicado((Rectangle){ 380, 630, 130, 55 }, mouse)) {
        interfaceJogo->acao_combate = ACAO_TELA_HABILIDADE_0;
    } else if (retanguloClicado((Rectangle){ 525, 630, 130, 55 }, mouse)) {
        interfaceJogo->acao_combate = ACAO_TELA_HABILIDADE_1;
    } else if (retanguloClicado((Rectangle){ 670, 630, 130, 55 }, mouse)) {
        interfaceJogo->acao_combate = ACAO_TELA_HABILIDADE_2;
    } else if (retanguloClicado((Rectangle){ 815, 630, 150, 55 }, mouse)) {
        interfaceJogo->popup_ativo = POPUP_MOCHILA_BUILDS;
    } else if (interfaceJogo->entidade_selecionada != NULL &&
               retanguloClicado((Rectangle){ 975, 630, 130, 55 }, mouse)) {
        interfaceJogo->popup_ativo = POPUP_DETALHES_PERSONAGEM;
    } else if (obterCelulaClicada(interfaceJogo, mouse, &linha, &coluna)) {
        if (interfaceJogo->acao_combate == ACAO_TELA_MOVER &&
            interfaceJogo->ator_atual != NULL) {
            if (moverCombatente3x6(mapa, interfaceJogo->ator_atual, linha, coluna)) {
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
                (void)snprintf(interfaceJogo->log_combate, sizeof(interfaceJogo->log_combate),
                               "%s usou %s em (%d,%d)!",
                               interfaceJogo->ator_atual->nome,
                               interfaceJogo->ator_atual->habilidades[habilidade].nome,
                               linha, coluna);
                concluirAcaoAliada(interfaceJogo, mapa, piratasDisponiveis,
                                   totalDisponiveis);
            } else if (mapa->celulas[linha][coluna] != NULL) {
                interfaceJogo->entidade_selecionada = mapa->celulas[linha][coluna];
            }
        } else {
            interfaceJogo->linha_selecionada = linha;
            interfaceJogo->coluna_selecionada = coluna;
            interfaceJogo->entidade_selecionada = mapa->celulas[linha][coluna];
        }
    }
}

// Retorna o nome do pirata pelo seu ID fixo de classe (1 a 5).
static const char *nomePirataPorId(int idPirata)
{
    static const char *nomes[5] = {
        "Bucaneiro", "Timoneiro", "Cozinheiro", "Capitao", "Gavea"
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
    if (unidade->turnos_motivacao > 0 && total < 8) {
        textos[total] = TextFormat("+MOT %dt", unidade->turnos_motivacao);
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

// Retorna o texto da passiva de classe (Nv 1 ou Nv 4) para o painel de inspeção.
static const char *descricaoPassiva(ClassePirata classe, bool aprimorada)
{
    static const char *passivas[5][2] = {
        { "Nv1: Anda 2 quadrados por acao de Movimentar.",
          "Nv4: +40% de dano ao atacar inimigos pelas costas." },
        { "Nv1: +25% de dano quando HP abaixo de 60%.",
          "Nv4: Aliados em cruz ao redor recebem -40% de dano." },
        { "Nv1: Usa 1 item consumivel por turno sem gastar acao.",
          "Nv4: Consumiveis usados por ele sao +20% mais fortes." },
        { "Nv1: Aliados adjacentes nao sofrem penalidade no descanso.",
          "Nv4: Aliados adjacentes causam +20% de dano." },
        { "Nv1: +5% de precisao por quadrado mais perto do alvo.",
          "Nv4: +10% de dano por quadrado mais perto do alvo." }
    };
    if (classe < CLASSE_BUCANEIRO || classe > CLASSE_GAVEA) {
        return "Sem passiva definida.";
    }
    return passivas[classe][aprimorada ? 1 : 0];
}

// Retorna a descrição de comportamento e passiva de um inimigo ou do Barba Negra.
static const char *descricaoPassivaInimigo(const Combatente *unidade)
{
    if (unidade == NULL) {
        return "";
    }
    if (unidade->eh_boss) {
        return unidade->fase_boss == 2 ?
            "Fase 2 Ativa (<=50% HP): 3 turnos/rodada, invocou 2 Grunts e planta Mina!" :
            "Fase 1: Age 3 vezes por rodada (Vel 8, 6 e 4). Em 50% HP entra na 2a Fase.";
    }
    if (unidade->tipo_inimigo == INIMIGO_GRUNT_ESPADA) {
        return "Acertar 3 cortes no mesmo alvo aplica Sangramento (20 dano/3t).";
    }
    if (unidade->tipo_inimigo == INIMIGO_GRUNT_PISTOLA) {
        return "Dispara Tiro em linha (alcance 2) e Granada em cruz (fogo amigo).";
    }
    if (unidade->tipo_inimigo == INIMIGO_ATIRADOR_MAGRELO) {
        return "Sniper de longo alcance (4 casas em linha, 25-60 dano).";
    }
    return "Escopeta causa 50-70 no 1o quadrado e 25-40 no 2o; Bandagens cura aliado.";
}

// Retorna uma breve descrição do efeito de um item pelo seu ID.
static const char *descricaoItemCatalogo(int idItem)
{
    switch (idItem) {
    case 1: return "Cura 40 HP instantaneamente.";
    case 2: return "+20% de dano nos ataques por 2 turnos.";
    case 3: return "Recupera 100% da Estamina.";
    case 4: return "Concede +2 niveis permanentes ao pirata.";
    case 5: return "Reduz todo o dano recebido em 15%.";
    case 6: return "+20% de dano em ataques Melee.";
    case 7: return "+25% na chance de causar dano maximo (Precisao).";
    case 8: return "+1 quadrado ao usar a acao de Andar.";
    case 9: return "+35% de dano em ataques Ranged.";
    case 10: return "+2 cargas na Estamina maxima.";
    case 11: return "Realiza 3 acoes basicas por turno!";
    default: return "Item da expedicao.";
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

// Desenha a ficha completa de status, passivas, ataques e build da entidade inspecionada.
static void desenharDetalhes(const Combatente *unidade)
{
    int indice;
    if (unidade == NULL) {
        return;
    }
    DrawRectangle(160, 55, 700, 545, (Color){ 12, 19, 27, 252 });
    DrawRectangleLines(160, 55, 700, 545, GOLD);
    desenharTexto("FICHA DE INSPECAO DO PERSONAGEM", 250, 72, 22, GOLD);
    DrawRectangle(760, 68, 85, 38, (Color){ 60, 75, 87, 255 });
    desenharTexto("FECHAR", 772, 80, 14, RAYWHITE);
    desenharTexto(unidade->nome, 185, 112, 22, unidade->aliado ? SKYBLUE : ORANGE);
    if (unidade->eh_boss) {
        desenharTexto(TextFormat("PV %d/%d | Estamina %d/%d | Fase %d | Velocidades: 8, 6 e 4",
                                 unidade->vida_atual, unidade->vida_maxima,
                                 unidade->estamina_atual, unidade->estamina_maxima,
                                 unidade->fase_boss),
                      185, 144, 16, RAYWHITE);
    } else {
        desenharTexto(TextFormat("PV %d/%d | Estamina %d/%d | Nivel %d | Velocidade %d",
                                 unidade->vida_atual, unidade->vida_maxima,
                                 unidade->estamina_atual, unidade->estamina_maxima,
                                 unidade->nivel, unidade->velocidade),
                      185, 144, 16, RAYWHITE);
    }
    desenharTexto(TextFormat("Chance Dano Max (Precisao): Melee %d%% | Ranged %d%% | Bonus Nv +%d%%",
                             unidade->precisao_melee, unidade->precisao_ranged,
                             unidade->dano_base),
                  185, 170, 15, LIGHTGRAY);
    desenharTexto("EFEITOS ATIVOS (BUFFS / DEBUFFS):", 185, 196, 14, GOLD);
    desenharIndicadoresStatusUnidade(unidade, 435, 194, 410);
    desenharTexto("PASSIVAS E COMPORTAMENTO", 185, 226, 17, GOLD);
    if (unidade->aliado) {
        desenharTexto(descricaoPassiva(unidade->classe, false), 195, 250, 14, RAYWHITE);
        desenharTexto(descricaoPassiva(unidade->classe, true), 195, 272, 14,
                      unidade->passiva_aprimorada ? GREEN : GRAY);
    } else {
        desenharTexto(descricaoPassivaInimigo(unidade), 195, 250, 14, RAYWHITE);
    }
    desenharTexto("HABILIDADES (Dano Min ou Max definido pela Precisao)", 185, 304, 17, GOLD);
    for (indice = 0; indice < MAX_HABILIDADES; indice++) {
        const Habilidade *habilidade = &unidade->habilidades[indice];
        bool desbloqueada = indice < unidade->habilidades_desbloqueadas;
        if (habilidade->nome[0] == '\0') {
            continue;
        }
        desenharTexto(TextFormat("[%s] %s | Min %d / Max %d | Alc %d | %d Est | Max %d%%",
                                 desbloqueada ? "ATIVA" : "BLOQ Nv3/6",
                                 habilidade->nome, habilidade->dano_min, habilidade->dano_max,
                                 habilidade->alcance, habilidade->custo_estamina,
                                 habilidade->precisao),
                      195, 330 + indice * 26, 14, desbloqueada ? RAYWHITE : GRAY);
    }
    if (unidade->aliado) {
        desenharTexto(TextFormat("BUILD PASSIVA (%d/%d)", unidade->build_passivos.tamanho,
                                 MAX_ITENS_PASSIVOS), 185, 420, 17, GOLD);
        if (unidade->build_passivos.inicio == NULL) {
            desenharTexto("Nenhum item passivo equipado neste pirata.", 195, 448, 14, LIGHTGRAY);
        } else {
            NoItem *item;
            int posTextoY = 448;
            for (item = unidade->build_passivos.inicio; item != NULL; item = item->proximo) {
                Color corItem = corRaridadeItem(item->item.raridade);
                desenharTexto(TextFormat("- [%s] %s: %s",
                                         nomeRaridadeItem(item->item.raridade),
                                         item->item.nome,
                                         descricaoItemCatalogo(item->item.id)),
                              195, posTextoY, 14, corItem);
                posTextoY += 24;
            }
        }
    }
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
    iniciarQuadroVirtual((Color){ 18, 28, 39, 255 });
    desenharTexto(contadorFugas > 0 ?
                  "REPOSICAO DE TRIPULACAO POS-FUGA" :
                  "ESCOLHA SUA TRIPULACAO",
                  contadorFugas > 0 ? 310 : 370, 42, 30, GOLD);
    desenharTexto(contadorFugas > 0 ?
                  TextFormat("Selecione substitutos para os caidos (%d/3) - Mortos voltaram ao Nv 1 sem passivos",
                             interfaceJogo->total_selecionados) :
                  TextFormat("Selecione 3 piratas para a expedicao (%d/3)",
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
        desenharTexto(TextFormat("HP %d | Precisao %d%% | Vel %d",
                                 piratasDisponiveis[indice].vida_maxima,
                                 piratasDisponiveis[indice].precisao_melee,
                                 piratasDisponiveis[indice].velocidade),
                      cardX + 16, cardY + 52, 16, LIGHTGRAY);
        desenharTexto(TextFormat("Estamina %d | Nivel %d | Passivos %d",
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
        desenharDetalhes(interfaceJogo->entidade_selecionada);
    } else if (interfaceJogo->popup_ativo == POPUP_PAUSE) {
        desenharMenuPause();
    }
    finalizarQuadroVirtual();
}

// Desenha a mochila organizada com abas de pirata, cores por raridade e gestão de builds.
static void desenharMochila(const InterfaceJogo *interfaceJogo)
{
    NoItem *item;
    int indice = 0;
    const Combatente *selecionado = interfaceJogo->entidade_selecionada;
    bool somenteConsumiveis = interfaceJogo->estado == TELA_COMBATE_3X6 &&
                              !interfaceJogo->fase_pre_combate_ativa;
    DrawRectangle(160, 55, 700, 545, (Color){ 12, 19, 27, 252 });
    DrawRectangleLines(160, 55, 700, 545, GOLD);
    desenharTexto("MOCHILA DA TRIPULACAO E BUILDS PASSIVAS", 215, 70, 21, GOLD);
    DrawRectangle(740, 68, 105, 38, (Color){ 60, 75, 87, 255 });
    desenharTexto(interfaceJogo->loja_aberta_pendente ? "VOLTAR LOJA" : "FECHAR",
                  interfaceJogo->loja_aberta_pendente ? 746 : 765, 80, 13, RAYWHITE);
    for (indice = 0; indice < interfaceJogo->total_selecionados; indice++) {
        int posAbaX = 180 + indice * 220;
        int idPirata = interfaceJogo->piratas_selecionados_ids[indice];
        bool abaAtiva = selecionado != NULL && selecionado->id == idPirata;
        DrawRectangle(posAbaX, 112, 205, 36,
                      abaAtiva ? (Color){ 45, 95, 130, 255 } : (Color){ 28, 42, 54, 255 });
        DrawRectangleLines(posAbaX, 112, 205, 36, abaAtiva ? GOLD : GRAY);
        desenharTexto(nomePirataPorId(idPirata),
                      posAbaX + 24, 121, 15, abaAtiva ? GOLD : RAYWHITE);
    }
    desenharTexto(somenteConsumiveis ?
                  "ITENS NA MOCHILA (Em combate: apenas Consumiveis podem ser usados):" :
                  "ITENS NA MOCHILA (Clique para Usar Consumivel ou Equipar Passivo):",
                  180, 158, 14, LIGHTGRAY);
    indice = 0;
    for (item = interfaceJogo->inventario == NULL ? NULL :
                interfaceJogo->inventario->inicio;
         item != NULL; item = item->proximo) {
        if (somenteConsumiveis && item->item.tipo != ITEM_CONSUMIVEL) {
            continue;
        }
        if (indice < 6) {
            Color corRaridade = corRaridadeItem(item->item.raridade);
            int posLinhaY = 182 + indice * 34;
            DrawRectangle(180, posLinhaY, 660, 30, (Color){ 26, 40, 52, 255 });
            DrawRectangleLines(180, posLinhaY, 660, 30, corRaridade);
            desenharTexto(TextFormat("[%s | %s] %s (x%d) - %s",
                                     item->item.tipo == ITEM_CONSUMIVEL ? "USAR" : "EQUIPAR",
                                     nomeRaridadeItem(item->item.raridade),
                                     item->item.nome, item->item.quantidade,
                                     descricaoItemCatalogo(item->item.id)),
                          190, posLinhaY + 7, 13, corRaridade);
        }
        indice++;
    }
    if (indice == 0) {
        DrawRectangle(180, 195, 660, 55, (Color){ 22, 33, 44, 255 });
        desenharTexto("Mochila vazia! Conquiste itens vencendo rotas ou comprando na Loja.",
                      210, 214, 15, LIGHTGRAY);
    }
    desenharTexto(TextFormat("BUILD PASSIVA DE: %s (%d/5) %s",
                             (selecionado != NULL && selecionado->aliado) ?
                             selecionado->nome : "Selecione um pirata",
                             (selecionado != NULL && selecionado->aliado) ?
                             selecionado->build_passivos.tamanho : 0,
                             interfaceJogo->estado == TELA_MAPA_ROTAS ?
                             "- Clique no item para devolver a Mochila" :
                             "(Travada em combate)"),
                  180, 398, 15, GOLD);
    if (selecionado != NULL && selecionado->aliado) {
        NoItem *passivo;
        int slot = 0;
        for (passivo = selecionado->build_passivos.inicio;
             passivo != NULL && slot < MAX_ITENS_PASSIVOS; passivo = passivo->proximo) {
            Color corPassivo = corRaridadeItem(passivo->item.raridade);
            int posSlotY = 424 + slot * 32;
            DrawRectangle(180, posSlotY, 660, 28, (Color){ 28, 45, 42, 255 });
            DrawRectangleLines(180, posSlotY, 660, 28, corPassivo);
            desenharTexto(TextFormat("[Slot %d - %s] %s: %s",
                                     slot + 1,
                                     nomeRaridadeItem(passivo->item.raridade),
                                     passivo->item.nome,
                                     descricaoItemCatalogo(passivo->item.id)),
                          190, posSlotY + 6, 13, corPassivo);
            slot++;
        }
        if (slot == 0) {
            desenharTexto("Nenhum item passivo equipado neste pirata.",
                          190, 435, 14, LIGHTGRAY);
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
    desenharTexto(TextFormat("Moedas de Ouro: %d | Saques realizados: %d (Inflacao +%d%%)",
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
        desenharTexto(TextFormat("[%s] %s | Preco: %d ouros | Estoque: %d/2",
                                 nomeRaridadeItem(item->raridade),
                                 item->nome, preco, estoque),
                      206, posItemY + 12, 17, estoque > 0 ? corRaridade : GRAY);
        desenharTexto(descricaoItemCatalogo(item->id), 206, posItemY + 44, 14,
                      estoque > 0 ? corRaridade : DARKGRAY);
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
static void desenharConfirmacaoFuga(void)
{
    DrawRectangle(210, 200, 620, 295, (Color){ 12, 19, 27, 252 });
    DrawRectangleLines(210, 200, 620, 295, GOLD);
    desenharTexto("Fugir da Aventura (Retornar a Rota 1)?", 295, 235, 22, GOLD);
    desenharTexto("Piratas vivos mantem nivel e itens equipados nas builds.", 265, 282, 15, RAYWHITE);
    desenharTexto("Piratas mortos podem ser reescolhidos no Nv 1 (sem passivos).", 260, 312, 15, SKYBLUE);
    desenharTexto("Itens na mochila serao perdidos e inimigos terao +50% status!",
                  260, 342, 15, ORANGE);
    DrawRectangle(350, 420, 150, 50, (Color){ 140, 50, 40, 255 });
    DrawRectangle(530, 420, 150, 50, (Color){ 60, 75, 87, 255 });
    desenharTexto("FUGIR", 395, 436, 18, RAYWHITE);
    desenharTexto("CANCELAR", 554, 436, 18, RAYWHITE);
}

// Desenha a barra superior da fila de iniciativa, destacando as 3 entradas do Chefe.
static void desenharFila(const InterfaceJogo *interfaceJogo)
{
    int indice;
    DrawRectangle(0, 0, LARGURA_VIRTUAL, 44, (Color){ 10, 17, 24, 240 });
    desenharTexto("INICIATIVA", 12, 13, 14, GOLD);
    for (indice = 0; indice < interfaceJogo->total_iniciativa && indice < 10; indice++) {
        Combatente *ator = interfaceJogo->fila_iniciativa[indice].ator;
        int posCardX = 112 + indice * 115;
        bool turnoAtivo = indice == interfaceJogo->indice_iniciativa;
        if (ator == NULL || !ator->vivo) {
            continue;
        }
        DrawRectangle(posCardX, 6, 110, 32, ator->aliado ?
                      (Color){ 35, 75, 105, 255 } : (Color){ 115, 55, 40, 255 });
        if (turnoAtivo) {
            DrawRectangleLinesEx((Rectangle){ (float)posCardX, 6.0f, 110.0f, 32.0f },
                                 2.0f, GOLD);
        }
        desenharTexto(TextFormat("%.*s (%d)", 8, ator->nome,
                                 interfaceJogo->fila_iniciativa[indice].velocidade_efetiva),
                      posCardX + 6, 14, 12, RAYWHITE);
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
                desenharTexto(TextFormat("PV %d/%d", unidade->vida_atual,
                                         unidade->vida_maxima), posX + 8, posY + 30, 13, RAYWHITE);
                desenharTexto(TextFormat("EST %d/%d", unidade->estamina_atual,
                                         unidade->estamina_maxima), posX + 8, posY + 50, 13, GOLD);
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
            DrawRectangleLines(posX, posY, interfaceJogo->largura_celula - 4,
                               interfaceJogo->altura_celula - 4,
                               (Color){ 180, 193, 184, 255 });
        }
    }
}

// Desenha a tela de menu principal dentro do canvas virtual 1280x720.
static void desenharMenu(void)
{
    iniciarQuadroVirtual((Color){ 18, 28, 39, 255 });
    desenharTexto("CRONICAS DE SANGUE E POLVORA", 320, 190, 34, GOLD);
    desenharTexto("RPG Tatico Roguelike em Turnos (100% Point-and-Click)", 350, 250, 20, RAYWHITE);
    DrawRectangle(470, 350, 340, 70, (Color){ 45, 125, 70, 255 });
    desenharTexto("INICIAR AVENTURA", 525, 374, 22, RAYWHITE);
    desenharTexto("Controle pelo Mouse | ESC abre o menu de Pause.", 440, 470, 16, LIGHTGRAY);
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
    if (interfaceJogo->estado == TELA_MENU_PRINCIPAL) {
        desenharMenu();
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
    iniciarQuadroVirtual((Color){ 18, 28, 39, 255 });
    if (interfaceJogo->estado == TELA_MAPA_ROTAS) {
        int etapaAtual = (interfaceJogo->rota_atual != NULL &&
                          *interfaceJogo->rota_atual != NULL) ?
                         (*interfaceJogo->rota_atual)->etapa : 0;
        desenharTexto(TextFormat("MAPA DE ROTAS - ETAPA %d DE 3",
                                 etapaAtual < 3 ? etapaAtual + 1 : 3),
                      410, 55, 28, GOLD);
        desenharTexto(TextFormat("Moedas de Ouro: %d | Saques: %d | Desistencias: %d",
                                 interfaceJogo->moedas == NULL ? 0 : *interfaceJogo->moedas,
                                 contadorSaques, contadorFugas),
                      390, 95, 18, RAYWHITE);
        if (etapaAtual >= 3) {
            DrawRectangle(480, 340, 380, 100, (Color){ 145, 42, 38, 255 });
            DrawRectangleLines(480, 340, 380, 100, GOLD);
            desenharTexto("ENFRENTAR BARBA NEGRA", 515, 365, 22, GOLD);
            desenharTexto("Chefe Final (1500 HP | 2 Fases | 3 Turnos/Rodada)",
                          500, 400, 14, RAYWHITE);
        } else {
            DrawRectangle(440, 290, 260, 95, (Color){ 50, 88, 125, 255 });
            DrawRectangle(730, 290, 260, 95, (Color){ 70, 112, 75, 255 });
            DrawRectangle(585, 415, 260, 95, (Color){ 115, 77, 42, 255 });
            desenharTexto("CAMINHO 1 (4 Inimigos)", 458, 308, 18, RAYWHITE);
            desenharTexto("+2 Niveis | +300 Ouros", 465, 336, 14, GOLD);
            desenharTexto("+15% Cura | +1 Item", 465, 358, 14, LIGHTGRAY);
            desenharTexto("CAMINHO 2 (3 Inimigos)", 748, 308, 18, RAYWHITE);
            desenharTexto("+1 Nivel | +700 Ouros", 755, 336, 14, GOLD);
            desenharTexto("+50% Cura | +2 Itens", 755, 358, 14, LIGHTGRAY);
            desenharTexto("CAMINHO 3: LOJA", 625, 433, 19, GOLD);
            desenharTexto("3 Itens a venda (2 unid.)", 615, 461, 14, RAYWHITE);
            desenharTexto("Ou Saquear Loja (+100% status)", 600, 483, 13, LIGHTGRAY);
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
        desenharTexto("FUGIR AVENTURA", 320, 648, 16, RAYWHITE);
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
                    desenharTexto(TextFormat("PV %d/%d | Nivel %d | Passivos %d/5",
                                             pirata->vida_atual, pirata->vida_maxima,
                                             pirata->nivel, pirata->build_passivos.tamanho),
                                  72, posCardY + 34, 14, LIGHTGRAY);
                    desenharIndicadoresStatusUnidade(pirata, 72, posCardY + 58, 310);
                }
            }
        }
    } else if (interfaceJogo->estado == TELA_COMBATE_3X6 && mapa != NULL) {
        desenharFila(interfaceJogo);
        if (interfaceJogo->fase_pre_combate_ativa) {
            desenharTexto("FASE PRE-COMBATE: Clique nos piratas e nas Colunas 0 e 1 para posicionar/trocar!",
                          80, 62, 18, GOLD);
            desenharTexto("Inspecione qualquer unidade selecionando-a e clicando em DETALHES.",
                          80, 90, 15, LIGHTGRAY);
        } else {
            desenharTexto(TextFormat("Turno: %s | Acoes restantes: %d",
                                     interfaceJogo->ator_atual == NULL ?
                                     "aguardando" : interfaceJogo->ator_atual->nome,
                                     interfaceJogo->ator_atual == NULL ?
                                     0 : interfaceJogo->ator_atual->acoes_basicas_restantes),
                          80, 58, 20,
                          (interfaceJogo->ator_atual != NULL &&
                           !interfaceJogo->ator_atual->aliado) ? ORANGE : GOLD);
            if (interfaceJogo->log_combate[0] != '\0') {
                desenharTexto(interfaceJogo->log_combate, 80, 88, 16, RAYWHITE);
            }
        }
        desenharTabuleiro(interfaceJogo, mapa, alcanceAtivo);
        DrawRectangle(70, 610, 1170, 92, (Color){ 12, 20, 29, 235 });
        if (interfaceJogo->fase_pre_combate_ativa) {
            DrawRectangle(90, 630, 260, 55, (Color){ 45, 125, 70, 255 });
            desenharTexto("INICIAR COMBATE", 132, 648, 19, RAYWHITE);
            DrawRectangle(370, 630, 190, 55,
                          interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS ?
                          (Color){ 65, 115, 135, 255 } : (Color){ 48, 65, 78, 255 });
            desenharTexto("MOCHILA / ITENS", 390, 648, 16, RAYWHITE);
            DrawRectangle(975, 630, 130, 55,
                          interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM ?
                          (Color){ 65, 115, 135, 255 } : (Color){ 42, 63, 76, 255 });
            desenharTexto("DETALHES", 985, 648, 15, RAYWHITE);
        } else {
            int habDesbloq = interfaceJogo->ator_atual != NULL ?
                             interfaceJogo->ator_atual->habilidades_desbloqueadas : 1;
            DrawRectangle(90, 630, 130, 55,
                          interfaceJogo->acao_combate == ACAO_TELA_MOVER ?
                          (Color){ 65, 115, 135, 255 } : (Color){ 42, 63, 76, 255 });
            DrawRectangle(235, 630, 130, 55, (Color){ 42, 63, 76, 255 });
            DrawRectangle(380, 630, 130, 55,
                          interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_0 ?
                          (Color){ 65, 115, 135, 255 } : (Color){ 42, 63, 76, 255 });
            DrawRectangle(525, 630, 130, 55,
                          habDesbloq >= 2 ?
                          (interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_1 ?
                           (Color){ 65, 115, 135, 255 } : (Color){ 42, 63, 76, 255 }) :
                          (Color){ 30, 35, 40, 255 });
            DrawRectangle(670, 630, 130, 55,
                          habDesbloq >= 3 ?
                          (interfaceJogo->acao_combate == ACAO_TELA_HABILIDADE_2 ?
                           (Color){ 65, 115, 135, 255 } : (Color){ 42, 63, 76, 255 }) :
                          (Color){ 30, 35, 40, 255 });
            DrawRectangle(815, 630, 150, 55,
                          interfaceJogo->popup_ativo == POPUP_MOCHILA_BUILDS ?
                          (Color){ 65, 115, 135, 255 } : (Color){ 42, 63, 76, 255 });
            DrawRectangle(975, 630, 130, 55,
                          interfaceJogo->popup_ativo == POPUP_DETALHES_PERSONAGEM ?
                          (Color){ 65, 115, 135, 255 } : (Color){ 42, 63, 76, 255 });
            desenharTexto("MOVER (1)", 106, 648, 16, RAYWHITE);
            desenharTexto("DESCANSAR", 244, 648, 15, RAYWHITE);
            desenharTexto("ATAQUE 1", 398, 648, 16, RAYWHITE);
            desenharTexto(habDesbloq >= 2 ? "ATAQUE 2" : "ATQ 2 (Nv3)",
                          536, 648, 15, habDesbloq >= 2 ? RAYWHITE : GRAY);
            desenharTexto(habDesbloq >= 3 ? "ULTIMATE" : "ULT (Nv6)",
                          684, 648, 15, habDesbloq >= 3 ? RAYWHITE : GRAY);
            desenharTexto("USAR ITEM", 840, 648, 16, RAYWHITE);
            desenharTexto("DETALHES", 985, 648, 15, RAYWHITE);
        }
    } else if (interfaceJogo->estado == TELA_VITORIA) {
        desenharTexto("VITORIA! BARBA NEGRA FOI DERROTADO!", 265, 250, 32, GOLD);
        desenharTexto("A tripulacao conquistou os mares em Cronicas de Sangue e Polvora!",
                      280, 315, 20, RAYWHITE);
        DrawRectangle(470, 440, 340, 65, (Color){ 45, 125, 70, 255 });
        desenharTexto("NOVA AVENTURA", 540, 462, 22, RAYWHITE);
    } else if (interfaceJogo->estado == TELA_FIM_DE_JOGO) {
        desenharTexto("FIM DE JOGO - TRIPULACAO DERROTADA", 290, 250, 32, ORANGE);
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
            desenharDetalhes(interfaceJogo->entidade_selecionada);
        } else if (interfaceJogo->popup_ativo == POPUP_CONFIRMAR_FUGA) {
            desenharConfirmacaoFuga();
        } else if (interfaceJogo->popup_ativo == POPUP_PAUSE) {
            desenharMenuPause();
        }
    }
    finalizarQuadroVirtual();
    if (alcanceDinamico != NULL) {
        liberarEstruturasJogo(NULL, NULL, alcanceDinamico);
    }
}

// Libera a fonte TrueType, o framebuffer virtual e encerra a janela Raylib.
void encerrarInterface(void)
{
    if (fonteCarregada && fonteJogo.texture.id != 0) {
        UnloadFont(fonteJogo);
        fonteCarregada = false;
    }
    if (canvasCarregado && canvasVirtual.id != 0) {
        UnloadRenderTexture(canvasVirtual);
        canvasCarregado = false;
    }
    CloseWindow();
}
