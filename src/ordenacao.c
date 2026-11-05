#include "ordenacao.h"

// Compara duas entradas de iniciativa segundo velocidade, jogável, menor vida e maior estamina.
static bool entradaTemPrioridade(const EntradaIniciativa *candidata,
                                 const EntradaIniciativa *referencia)
{
    if (candidata == NULL || candidata->ator == NULL) {
        return false;
    }
    if (referencia == NULL || referencia->ator == NULL) {
        return true;
    }
    if (candidata->velocidade_efetiva != referencia->velocidade_efetiva) {
        return candidata->velocidade_efetiva > referencia->velocidade_efetiva;
    }
    if (candidata->ator->aliado != referencia->ator->aliado) {
        return candidata->ator->aliado;
    }
    if (candidata->ator->vida_atual != referencia->ator->vida_atual) {
        return candidata->ator->vida_atual < referencia->ator->vida_atual;
    }
    if (candidata->ator->estamina_atual != referencia->ator->estamina_atual) {
        return candidata->ator->estamina_atual > referencia->ator->estamina_atual;
    }
    return false;
}

// Ordena entradas de iniciativa por velocidade efetiva e critérios de desempate.
void ordenarFilaPorVelocidade(EntradaIniciativa fila[], int total)
{
    int indice;
    if (fila == NULL || total < 2) {
        return;
    }
    for (indice = 1; indice < total; indice++) {
        EntradaIniciativa entrada = fila[indice];
        int posicao = indice;
        while (posicao > 0 && entradaTemPrioridade(&entrada, &fila[posicao - 1])) {
            fila[posicao] = fila[posicao - 1];
            posicao--;
        }
        fila[posicao] = entrada;
    }
}

// Ordena candidatos do Tactical Blackboard por pontuação decrescente.
void ordenarCandidatosIA(CandidatoAcao vetor[], int total)
{
    int indice;
    if (vetor == NULL || total < 2) {
        return;
    }
    for (indice = 1; indice < total; indice++) {
        CandidatoAcao candidato = vetor[indice];
        int posicao = indice;
        while (posicao > 0 && vetor[posicao - 1].pontuacao < candidato.pontuacao) {
            vetor[posicao] = vetor[posicao - 1];
            posicao--;
        }
        vetor[posicao] = candidato;
    }
}
