# Sea Of Gunpowder

Projeto desenvolvido para a disciplina de **Algoritmos e Estruturas de Dados (AED — Módulo 02 / 2026.2)**, seguindo o tema **Ode ao Cinema**.

---

## 1. Especificação do Jogo (Roteiro AED)

### a) Nome do Jogo
**sea of gunpowder**

### b) Equipe
- **Matheus Britto** — Dados base do jogo, catálogo de itens, atributos de piratas/inimigos, escalonamento de rotas e sistema de loja/saque (`dados_jogo.c`).
- **Igor Guimarães** — Sistema de combate em turnos no tabuleiro 3x6, cálculo de dano/precisão, efeitos de status (buffs/debuffs) e regras do chefe (`combate.c`).
- **Luiz Higino** — Estruturas de dados dinâmicas (árvore ternária de rotas, listas encadeadas de mochila/builds e área de alcance) e algoritmos de ordenação (`estruturas.c`, `ordenacao.c`).
- **Matheus Becker** — Inteligência artificial dos inimigos e interface gráfica com Raylib (`ia_inimigos.c`, `interface.c`, `main.c`).

### c) Gênero
RPG Tático em Turnos com elementos de *Roguelike*.

### d) Referências do Cinema
O jogo foi inspirado principalmente na franquia **Piratas do Caribe** (em especial *A Maldição do Pérola Negra* e *Navegando em Águas Misteriosas*), trazendo o **Barba Negra** como antagonista final, além de referências clássicas de filmes de pirataria na construção das classes da tripulação, armas de pólvora (bacamarte, pistolas, canhões) e saques.

### e) Descrição Geral
No jogo, o jogador escolhe 3 piratas dentre 5 classes diferentes (**Bucaneiro**, **Timoneiro**, **Cozinheiro**, **Capitão** e **Gávea**) para formar sua tripulação e navegar por um mapa dividido em 3 etapas antes do confronto final contra o Barba Negra.

Em cada etapa do mapa, o grupo escolhe entre três opções de rota:
1. **Caminho 1**: combate contra 4 inimigos, focado em subir o nível da tripulação (+2 níveis, +300 moedas, +15% de cura e 1 item).
2. **Caminho 2**: combate contra 3 inimigos, focado em recursos (+1 nível, +700 moedas, +50% de cura e 2 itens).
3. **Caminho 3 (Loja)**: permite gastar as moedas acumuladas para comprar itens consumíveis ou passivos (que mudam a cada visita). Na própria loja, o jogador pode abrir a mochila para usar ou equipar o que comprou e, se quiser arriscar, pode **Saquear a Loja**, enfrentando um combate mais pesado para levar todo o estoque restante de graça (o que gera inflação nas próximas lojas).

### f) Objetivo do Jogo
Sobreviver às 3 etapas da expedição, evoluindo o nível e a build de itens dos piratas, e derrotar o chefe final **Barba Negra** (que possui 1500 de vida, 2 fases de batalha e realiza 3 turnos por rodada) na etapa final.

### g) Regras Básicas e Mecânicas
- **Tabuleiro 3x6**: as batalhas acontecem em uma grade de 3 linhas por 6 colunas. Antes do combate começar (Fase Pré-Combate), o jogador pode posicionar ou trocar seus piratas livremente nas duas primeiras colunas (colunas 0 e 1).
- **Ordem dos Turnos (Iniciativa)**: a fila de turnos é ordenada pela velocidade de cada unidade. Em caso de empate, a prioridade segue a ordem:
  1. Maior velocidade;
  2. Personagem jogável (aliado tem preferência sobre inimigo);
  3. Personagem com menos vida atual;
  4. Personagem com mais estamina atual.
- **Ações e Estamina**: cada unidade realiza até 2 ações básicas por turno (podendo chegar a 3 com o item lendário *Rum de Poseidon*). Mover ou usar habilidades consome estamina. Se a estamina acabar, o personagem pode usar a ação de **Descansar** para recuperar estamina no próximo turno (ficando vulnerável a receber +25% de dano até a sua próxima vez, a menos que esteja protegido pela passiva do Capitão).
- **Sistema de Precisão**: nenhum ataque erra. A porcentagem de precisão define a chance do ataque causar o **dano máximo** da habilidade; caso contrário, ele causa o **dano mínimo** (sem valores intermediários).
- **Alcance e Ataques Perfurantes**: ataques normais são feitos nas direções cardeais (sem ataques na diagonal) e param no primeiro inimigo da linha. Apenas as ultimates do **Gávea** (*Bala Perfurante Especial*) e do **Bucaneiro** (*Avanço Impetuoso*), além das habilidades em área fora de linha reta, conseguem atingir/atravessar múltiplos alvos.
- **Mochila e Builds Passivas**: os itens só podem ser obtidos vencendo combates ou na loja. Cada item tem uma cor de acordo com sua raridade (Comum = verde, Raro = azul, Épico = roxo, Lendário = laranja). Cada pirata pode equipar até 5 itens passivos, que podem ser trocados entre os personagens quando o jogador está na seleção de rotas ou na loja.
- **Fuga da Aventura e Morte de Personagens**: na tela de rotas, o jogador pode desistir da tentativa atual e fugir de volta para a etapa 1. Os piratas vivos mantêm seu nível e seus itens passivos equipados, mas os itens soltos na mochila são perdidos e os inimigos ficam mais fortes. Caso algum pirata tenha morrido antes da fuga, o jogador volta para a tela de seleção para escolher substitutos (piratas mortos voltam a ficar disponíveis, porém resetados no nível 1 e sem itens).

### h) Controles
- **Botão Esquerdo do Mouse**: controla todas as ações do jogo (seleção de personagens, escolha de rotas, compra e uso de itens na loja/mochila, posicionamento pré-combate, movimentação e ataques no tabuleiro). Clicar novamente no botão de uma janela pop-up (como Mochila ou Detalhes) fecha a janela.
- **Tecla ESC**: abre e fecha o menu de Pause a qualquer momento durante a partida, permitindo **Retornar** ao jogo ou **Voltar ao Menu** inicial.
- **Botão Maximizar / Tela Cheia da Janela**: a janela pode ser redimensionada ou maximizada normalmente sem distorcer a interface nem a posição dos cliques.

### i) Estruturas de Dados e Algoritmos Utilizados
- **Matriz Estática 3x6 (`Tabuleiro3x6`)**: armazena os ponteiros para os combatentes posicionados em cada célula e controla as minas terrestres plantadas pelo Barba Negra na 2ª fase.
- **Árvore Ternária (`NoRota`)**: representa a progressão das etapas da expedição, onde cada nó possui três filhos (`caminho1`, `caminho2` e `loja`), convergindo para o nó do chefe na última etapa.
- **Listas Simplesmente Encadeadas (`ListaItens` e `ListaAlcance`)**:
  - `ListaItens`: usada tanto para a mochila compartilhada da tripulação quanto para a lista de até 5 itens passivos equipados em cada pirata.
  - `ListaAlcance`: criada dinamicamente para armazenar as células válidas de alcance de cada habilidade e sinalizar perigo de fogo amigo no tabuleiro.
- **Algoritmo de Ordenação (`Insertion Sort`)**: implementado em `ordenacao.c` para ordenar a fila de iniciativa por velocidade (com os 4 critérios de desempate) e para ordenar os candidatos de ação da inteligência artificial dos inimigos.

### j) Ferramentas e Bibliotecas
- **Linguagem**: C (padrão C99)
- **Compilador e Build**: GCC + Make (via **w64devkit** no Windows)
- **Biblioteca Gráfica**: **Raylib 5.5**

---

## 2. Como Compilar e Executar o Jogo

### Pré-requisitos no Windows
Para compilar sem dor de cabeça no Windows, o projeto usa o **w64devkit** (que já traz o `gcc` e o `make` portáteis) junto com a biblioteca **Raylib 5.5**.

1. Baixe o **w64devkit** (`w64devkit-x64-2.0.0.exe` ou `.zip`) em:  
   https://github.com/skeeto/w64devkit/releases  
   Extraia na raiz do disco para ficar em `C:\w64devkit`.
2. Baixe o pacote MinGW da **Raylib 5.5** (`raylib-5.5_win64_mingw-w64.zip`) em:  
   https://github.com/raysan5/raylib/releases/tag/5.5  
   Copie o conteúdo da pasta `include` da Raylib para dentro de `C:\w64devkit\x86_64-w64-mingw32\include` e o conteúdo da pasta `lib` para `C:\w64devkit\x86_64-w64-mingw32\lib`.

---

### Passo a Passo para Compilar (Windows)

Abra o **PowerShell** ou o terminal do VS Code na pasta raiz do projeto (`sea-of-gunpowder`).

#### Opção 1: Se o `C:\w64devkit\bin` já estiver no PATH do Windows
Basta rodar:
```powershell
make
```

#### Opção 2: Comando direto (funciona mesmo sem configurar o PATH)
Se o comando `make` não for reconhecido no seu terminal, execute diretamente pelo caminho do `w64devkit`:
```powershell
C:\w64devkit\bin\make.exe CC=C:/w64devkit/bin/gcc.exe
```

Isso vai compilar todos os arquivos da pasta `src/` e gerar o executável em `build/jogo.exe`.

---

### Como Executar o Jogo

Depois de compilar, inicie o jogo com:
```powershell
.\build\jogo.exe
```

Se quiser rodar apenas os testes automatizados no terminal (que validam os atributos dos piratas, combate, loja, alcance e estruturas de dados):
```powershell
.\build\jogo.exe --test
```

---

### Como Compilar e Executar no Linux
1. Instale o `gcc`, o `make` e os pacotes de desenvolvimento da `raylib` na sua distribuição.
2. No terminal, dentro da pasta do projeto, execute:
```bash
make
./build/jogo
```

---

### Limpeza dos Arquivos Compilados
Para apagar a pasta `build/` (arquivos `.o` e o executável `.exe`) antes de compactar o projeto para entrega ou recompilar do zero:
```powershell
make clean
```
*(ou `C:\w64devkit\bin\make.exe clean` caso não tenha o `make` no PATH).*
