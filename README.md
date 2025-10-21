# Laboratorio de Manipulacao de Arquivos

Este repositorio contem a implementacao em C do sistema de votacao descrito no enunciado "Laboratorio de Manipulacao de Arquivos". O programa lida com arquivos texto, valida dados e controla a escolha do melhor TC da IES.

## Estrutura do projeto

```
├── src/                 # Codigo fonte (main.c)
├── dados/               # Arquivos de entrada exigidos pelo sistema
├── saida/               # Relatorios gerados (parcial.txt, resultado.txt)
├── docs/                # Explicacoes em texto e anexos de referencia
├── bin/                 # Executaveis compilados (votacao.exe)
└── .vscode/             # Configuracoes de build/debug para VS Code
```

## Como compilar e executar

- **GCC (MinGW ou MSYS2)**  
  ```
  gcc src/main.c -o bin/votacao.exe -Wall -Wextra -std=c11
  bin\votacao.exe
  ```

- **VS Code**  
  Use `Ctrl+Shift+B` para acionar a task `build votacao`. O debug `Debug votacao` (F5) ja compila e executa `bin/votacao.exe`, com o diretorio de trabalho ajustado para a raiz do projeto.

- **Visual Studio (IDE completa)**  
  Crie um projeto "Empty Project" em C/C++, adicione `src/main.c`, defina o diretório de trabalho como a raiz do projeto e compile (`Ctrl+Shift+B`). Execute (`Ctrl+F5`) para iniciar o menu da votacao.

## Arquivos de entrada (pasta `dados/`)

- `professores.txt`: primeira linha com a quantidade; demais no formato `codigo depto idade nome`.  
- `alunos.txt`: primeira linha com a quantidade; demais `matricula ano depto idade nome` (ano sempre 3).  
- `TC_<SIGLA>.txt`: um arquivo por departamento (BCC, BSI, ADS, GIT, BEC) contendo `codigoTC matriculaAutor codigoOrientador titulo`.  
- `comissao.txt`: quantidade inicial seguida de CPFs no formato `xxx.xxx.xxx-yy`.

Todos os arquivos devem terminar com quebra de linha. Se algum estiver ausente ou com dados inconsistentes, o programa encerra informando o erro.

## Fluxo do programa

1. O executavel detecta o diretorio raiz, garante as pastas `dados/` e `saida/` e carrega todos os arquivos.  
2. Mostra o MENU1:  
   - `a` Iniciar nova votacao  
   - `b` Continuar votacao gravada  
3. MENU2 (apos iniciar ou retomar uma parcial):  
   - `a` Entrar com voto – valida CPF, verifica se o eleitor ja votou e obriga informar um codigo de TC existente.  
   - `b` Suspender votacao – grava `saida/parcial.txt` com CPFs e TCs votados e encerra.  
   - `c` Concluir votacao – gera `saida/resultado.txt` com TCs vencedores, dados de aluno e orientador, eleitores que votaram e lista dos que nao votaram.

## Regras e validacoes principais

- Departamentos aceitos: BCC, BSI, ADS, GIT, BEC.  
- CPFs sao checados pelo formato `xxx.xxx.xxx-yy` e pelo algoritmo de digitos verificadores.  
- Alunos e professores nao podem ter codigos repetidos; cada TC referencia um aluno do proprio departamento e um orientador valido.  
- Os arquivos sao lidos apenas uma vez; qualquer problema encerra a execucao com mensagem clara.  
- Os votos sao armazenados por TC e cada eleitor e marcado com o codigo escolhido, impedindo votos duplicados.

## Documentacao adicional

- `docs/explicacao.txt`: resumo em texto da atividade para apresentacao.  
- `docs/explicacao_codigo.txt`: guia rapido sobre o codigo e seu uso.  
- `docs/Votacao_ProfaEliane_Trabalho.pdf`: enunciado original.  
- `docs/imagens/page_*.png`: capturas das paginas do PDF para consulta rapida.

Mantenha os arquivos de entrada atualizados em `dados/` antes de rodar o programa e verifique `saida/` para os relatorios gerados.
