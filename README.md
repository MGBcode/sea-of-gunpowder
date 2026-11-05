# Crônicas de Sangue e Pólvora

Jogo no estilo *Roguelike Tático em Turnos* desenvolvido em **Linguagem C (padrão C99)** com **Raylib 5.5** para a disciplina de Algoritmos e Estruturas de Dados (AED — 2026.2, tema *Ode ao Cinema*).

Toda a navegação e interação do jogo ocorre **100% via Mouse (Point-and-Click)**: selecione 3 entre os 5 piratas da tripulação, escolha rotas na Árvore Ternária (Caminho 1, Caminho 2 ou Loja/Saque), gerencie a Mochila e as Builds de até 5 itens passivos por pirata, posicione sua equipe nas Colunas 0 e 1 na Fase Pré-Combate e enfrente os inimigos e o Chefe Final **Barba Negra** no tabuleiro contínuo 3×6.

## Como Compilar e Executar

### No Windows (com w64devkit + Raylib 5.5)
1. Certifique-se de ter o **w64devkit** e a **Raylib 5.5** configurados conforme o guia rápido em [REQUIREMENTS.md](./REQUIREMENTS.md).
2. Abra o terminal na pasta raiz do projeto e compile com:
   ```powershell
   make
   ```
3. Execute o jogo:
   ```powershell
   .\build\jogo.exe
   ```
4. Para rodar apenas a bateria de testes sanitários automatizados no terminal:
   ```powershell
   .\build\jogo.exe --test
   ```

### No Linux / macOS
1. Instale `gcc`, `make` e a biblioteca `raylib` do seu sistema.
2. Compile e execute:
   ```bash
   make
   ./build/jogo
   ```

## Limpeza antes da Entrega (.zip)
Antes de compactar o pacote final de entrega (que não deve conter o executável nem arquivos objeto), execute:
```powershell
make clean
```
