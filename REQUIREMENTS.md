# Requisitos de compilação no Windows

## Dependências

- **w64devkit x64**, que inclui GCC/MinGW-w64, GNU Make e o shell necessários:
  [releases oficiais](https://github.com/skeeto/w64devkit/releases).
- **Raylib 5.5 pré-compilada para MinGW-w64**:
  [raylib-5.5_win64_mingw-w64.zip](https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip).
- **VS Code** é opcional. O projeto configura o caminho do w64devkit nas tarefas
  de build do editor.

## Instalação rápida

1. Baixe o pacote x64 mais recente do w64devkit na página oficial de releases e
   extraia-o em `C:\`. O diretório resultante deve ser `C:\w64devkit`, contendo
   `bin\gcc.exe` e `bin\make.exe`.
2. Baixe e extraia o ZIP oficial da Raylib 5.5. Da pasta
   `raylib-5.5_win64_mingw-w64` extraída, copie todos os `.h` de `include` para
   `C:\w64devkit\include` e `lib\libraylib.a` para `C:\w64devkit\lib`.
3. Para usar `make` diretamente em um terminal, coloque
   `C:\w64devkit\bin` no início do `PATH` do usuário, antes de outros GCC/MinGW,
   e abra um terminal novo. As tarefas do VS Code já acrescentam esse diretório
   ao `PATH` automaticamente.
4. No terminal, entre na pasta do projeto, compile com `make` e execute
   `.\build\jogo.exe`. Antes de compactar a entrega, execute `make clean`.

O `Makefile` procura headers e bibliotecas tanto nos diretórios `include` e
`lib` do w64devkit quanto nos diretórios `x86_64-w64-mingw32` correspondentes.
