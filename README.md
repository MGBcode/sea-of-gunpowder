# Sea Of Gunpowder

**CESAR School — Algoritmos e Estruturas de Dados (AED — 2026.2)**  
**Módulo 02 — Atividade Prática — Tema: Ode ao Cinema**

## Equipe
- **Matheus Britto**
- **Igor Moura**
- **Luiz Higino**
- **Matheus Becker**


## Como Compilar e Executar o Jogo

### Pré-requisitos (Windows)
- Compilador C compatível com C99 (**GCC / MinGW-w64** ou **w64devkit**) e utilitário `make`.
- Biblioteca **Raylib 5.5** instalada em `C:/raylib/raylib` (ou apontada pela variável `RAYLIB_DIR`).

### Passo 1 — Compilar o projeto
Abra o terminal (**PowerShell** ou **CMD**) na pasta raiz do projeto (`sea-of-gunpowder`) e execute:

```powershell
make
```

*(Caso utilize o `w64devkit` sem adicioná-lo ao `PATH` do Windows, execute:)*

```powershell
C:\w64devkit\bin\make.exe CC=C:/w64devkit/bin/gcc.exe
```

O comando compilará todos os arquivos de `src/` e gerará o executável em `build/jogo.exe`.

### Passo 2 — Executar o jogo
Para abrir o jogo com a interface gráfica:

```powershell
.\build\jogo.exe
```

### Passo 3 — Rodar os testes automatizados (opcional)
Para validar todas as mecânicas, estruturas de dados, IA e ordenação no terminal sem abrir a janela gráfica:

```powershell
.\build\jogo.exe --test
```

### Passo 4 — Limpar arquivos de compilação (opcional)
Antes de compactar o projeto em `.zip` para entrega (sem o executável):

```powershell
make clean
```

---

## Controles Rápidos
- **Botão Esquerdo do Mouse**: Seleciona piratas, escolhe rotas, posiciona unidades nas colunas 0 e 1 na fase pré-combate, usa habilidades e gerencia itens na Loja e na Mochila.
- **Scroll do Mouse (Roda do Mouse)**: Rola os textos de descrição na janela de **Detalhes** do personagem e a lista de itens na **Mochila**.
- **Tecla `ESC`**: Abre ou fecha o menu de **Pause** (*Retornar* ou *Voltar ao Menu*).
