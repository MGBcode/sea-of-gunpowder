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
- **w64devkit x64** e utilitário `make` no `PATH`. O projeto usa o GCC do w64devkit em `C:\w64devkit`, mesmo se outro MinGW estiver instalado.
- Biblioteca **Raylib 5.5** pré-compilada para MinGW-w64, com os headers em `C:\w64devkit\include` e `libraylib.a` em `C:\w64devkit\lib`.

### Passo 1 — Compilar o projeto
Abra o terminal (**PowerShell** ou **CMD**) na pasta raiz do projeto (`sea-of-gunpowder`) e execute:

```powershell
make
```

O comando compilará todos os arquivos de `src/` e gerará o executável em `build/jogo.exe`. Se você acabou de adicionar o w64devkit ao `PATH`, abra um terminal novo antes de executar `make`.

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