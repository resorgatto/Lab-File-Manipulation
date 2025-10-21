#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#endif

#define max 50
#define maxNome 60
#define maxSigla 10

typedef struct {
    char nome[maxNome];
    int idade;
} Pessoa;

typedef struct {
    Pessoa pes;
    int codigo;
    char depto[maxSigla];
} Professor;

typedef struct {
    Pessoa pes;
    int matricula;
    int ano;
    char depto[maxSigla];
} Aluno;

typedef struct {
    int codigo;
    int autor;
    int orientador;
    char titulo[maxNome];
    int qtdeVotos;
} TC;

typedef struct {
    char cpf[15];
    bool votou;
    int codigoTC;
} Eleitor;

Professor docentes[max];
int qtdeDocentes = 0;

Aluno formandos[max];
int qtdeFormandos = 0;

TC listaTCs[max];
int qtdeTCs = 0;

Eleitor comissao[max];
int qtdeEleitores = 0;

const char *const DEPARTAMENTOS[] = {"BCC", "BSI", "ADS", "GIT", "BEC"};
const size_t QTD_DEPTOS = sizeof(DEPARTAMENTOS) / sizeof(DEPARTAMENTOS[0]);
static const char *const DATA_DIR = "dados";
static const char *const OUTPUT_DIR = "saida";

static void garantirDiretorio(const char *dir) {
#ifdef _WIN32
    CreateDirectoryA(dir, NULL);
#else
    mkdir(dir, 0775);
#endif
}

static void ajustarDiretorioParaExecutavel(void) {
#ifdef _WIN32
    char caminho[MAX_PATH];
    DWORD tamanho = GetModuleFileNameA(NULL, caminho, (DWORD)sizeof(caminho));
    if (tamanho == 0 || tamanho >= (DWORD)sizeof(caminho)) {
        return;
    }
    char *barra = strrchr(caminho, '\\');
    if (!barra) {
        return;
    }
    *barra = '\0';
    SetCurrentDirectoryA(caminho);
    SetCurrentDirectoryA("..");
#else
    char caminho[PATH_MAX];
    ssize_t tamanho = readlink("/proc/self/exe", caminho, sizeof(caminho) - 1);
    if (tamanho <= 0 || tamanho >= (ssize_t)sizeof(caminho)) {
        return;
    }
    caminho[tamanho] = '\0';
    char *barra = strrchr(caminho, '/');
    if (!barra) {
        return;
    }
    *barra = '\0';
    chdir(caminho);
    chdir("..");
#endif
    garantirDiretorio(DATA_DIR);
    garantirDiretorio(OUTPUT_DIR);
}

static bool montarCaminho(char *destino, size_t tamanho, const char *diretorio, const char *arquivo) {
    int escrito = snprintf(destino, tamanho, "%s/%s", diretorio, arquivo);
    return escrito > 0 && (size_t)escrito < tamanho;
}

static void retirarFimLinha(char *str) {
    if (!str) {
        return;
    }
    size_t len = strcspn(str, "\r\n");
    str[len] = '\0';
}

static bool departamentoValido(const char *sigla) {
    for (size_t i = 0; i < QTD_DEPTOS; ++i) {
        if (strcmp(sigla, DEPARTAMENTOS[i]) == 0) {
            return true;
        }
    }
    return false;
}

static Professor *buscarProfessor(int codigo) {
    for (int i = 0; i < qtdeDocentes; ++i) {
        if (docentes[i].codigo == codigo) {
            return &docentes[i];
        }
    }
    return NULL;
}

static Aluno *buscarAluno(int matricula) {
    for (int i = 0; i < qtdeFormandos; ++i) {
        if (formandos[i].matricula == matricula) {
            return &formandos[i];
        }
    }
    return NULL;
}

static TC *buscarTC(int codigo) {
    for (int i = 0; i < qtdeTCs; ++i) {
        if (listaTCs[i].codigo == codigo) {
            return &listaTCs[i];
        }
    }
    return NULL;
}

static Eleitor *buscarEleitor(const char *cpf) {
    for (int i = 0; i < qtdeEleitores; ++i) {
        if (strcmp(comissao[i].cpf, cpf) == 0) {
            return &comissao[i];
        }
    }
    return NULL;
}

static bool cpfFormatoValido(const char *cpf) {
    if (strlen(cpf) != 14) {
        return false;
    }
    for (int i = 0; i < 14; ++i) {
        if (i == 3 || i == 7) {
            if (cpf[i] != '.') {
                return false;
            }
        } else if (i == 11) {
            if (cpf[i] != '-') {
                return false;
            }
        } else {
            if (!isdigit((unsigned char)cpf[i])) {
                return false;
            }
        }
    }
    return true;
}

static bool cpfDigitosValidos(const char *cpf) {
    int numeros[11];
    int idx = 0;
    for (int i = 0; cpf[i] != '\0'; ++i) {
        if (isdigit((unsigned char)cpf[i])) {
            if (idx >= 11) {
                return false;
            }
            numeros[idx++] = cpf[i] - '0';
        }
    }
    if (idx != 11) {
        return false;
    }

    bool todosIguais = true;
    for (int i = 1; i < 11; ++i) {
        if (numeros[i] != numeros[0]) {
            todosIguais = false;
            break;
        }
    }
    if (todosIguais) {
        return false;
    }

    int soma = 0;
    for (int i = 0; i < 9; ++i) {
        soma += numeros[i] * (10 - i);
    }
    int resto = (soma * 10) % 11;
    if (resto == 10) {
        resto = 0;
    }
    if (resto != numeros[9]) {
        return false;
    }

    soma = 0;
    for (int i = 0; i < 10; ++i) {
        soma += numeros[i] * (11 - i);
    }
    resto = (soma * 10) % 11;
    if (resto == 10) {
        resto = 0;
    }
    if (resto != numeros[10]) {
        return false;
    }
    return true;
}

static bool validarCPF(const char *cpf) {
    return cpfFormatoValido(cpf) && cpfDigitosValidos(cpf);
}

static bool carregarProfessores(const char *arquivo) {
    char caminho[256];
    if (!montarCaminho(caminho, sizeof(caminho), DATA_DIR, arquivo)) {
        printf("Caminho muito longo para %s.\n", arquivo);
        return false;
    }
    FILE *fp = fopen(caminho, "r");
    if (!fp) {
        printf("Nao foi possivel abrir %s.\n", caminho);
        return false;
    }

    int quantidade = 0;
    if (fscanf(fp, "%d\n", &quantidade) != 1 || quantidade < 0) {
        printf("Formato invalido em %s.\n", caminho);
        fclose(fp);
        return false;
    }
    if (quantidade > max) {
        printf("Quantidade de professores excede o limite.\n");
        fclose(fp);
        return false;
    }

    char linha[256];
    for (int i = 0; i < quantidade; ++i) {
        if (!fgets(linha, sizeof(linha), fp)) {
            printf("Dados insuficientes em %s.\n", caminho);
            fclose(fp);
            return false;
        }
        Professor prof;
        if (sscanf(linha, "%d %9s %d %[^\n]", &prof.codigo, prof.depto, &prof.pes.idade, prof.pes.nome) != 4) {
            printf("Linha invalida em %s: %s", caminho, linha);
            fclose(fp);
            return false;
        }
        if (!departamentoValido(prof.depto)) {
            printf("Departamento invalido para professor %d.\n", prof.codigo);
            fclose(fp);
            return false;
        }
        if (buscarProfessor(prof.codigo)) {
            printf("Codigo de professor duplicado: %d.\n", prof.codigo);
            fclose(fp);
            return false;
        }
        docentes[qtdeDocentes++] = prof;
    }

    fclose(fp);
    return true;
}

static bool carregarAlunos(const char *arquivo) {
    char caminho[256];
    if (!montarCaminho(caminho, sizeof(caminho), DATA_DIR, arquivo)) {
        printf("Caminho muito longo para %s.\n", arquivo);
        return false;
    }
    FILE *fp = fopen(caminho, "r");
    if (!fp) {
        printf("Nao foi possivel abrir %s.\n", caminho);
        return false;
    }

    int quantidade = 0;
    if (fscanf(fp, "%d\n", &quantidade) != 1 || quantidade < 0) {
        printf("Formato invalido em %s.\n", caminho);
        fclose(fp);
        return false;
    }
    if (quantidade > max) {
        printf("Quantidade de alunos excede o limite.\n");
        fclose(fp);
        return false;
    }

    char linha[256];
    for (int i = 0; i < quantidade; ++i) {
        if (!fgets(linha, sizeof(linha), fp)) {
            printf("Dados insuficientes em %s.\n", caminho);
            fclose(fp);
            return false;
        }
        Aluno aluno;
        if (sscanf(linha, "%d %d %9s %d %[^\n]", &aluno.matricula, &aluno.ano, aluno.depto, &aluno.pes.idade,
                   aluno.pes.nome) != 5) {
            printf("Linha invalida em %s: %s", caminho, linha);
            fclose(fp);
            return false;
        }
        if (!departamentoValido(aluno.depto)) {
            printf("Departamento invalido para aluno %d.\n", aluno.matricula);
            fclose(fp);
            return false;
        }
        if (aluno.ano != 3) {
            printf("Ano invalido para aluno %d.\n", aluno.matricula);
            fclose(fp);
            return false;
        }
        if (buscarAluno(aluno.matricula)) {
            printf("Matricula de aluno duplicada: %d.\n", aluno.matricula);
            fclose(fp);
            return false;
        }
        formandos[qtdeFormandos++] = aluno;
    }

    fclose(fp);
    return true;
}

static void resetarVotos(void) {
    for (int i = 0; i < qtdeTCs; ++i) {
        listaTCs[i].qtdeVotos = 0;
    }
    for (int i = 0; i < qtdeEleitores; ++i) {
        comissao[i].votou = false;
        comissao[i].codigoTC = -1;
    }
}

static bool carregarTCDepartamento(const char *sigla) {
    char nomeArquivo[32];
    snprintf(nomeArquivo, sizeof(nomeArquivo), "TC_%s.txt", sigla);
    char caminho[256];
    if (!montarCaminho(caminho, sizeof(caminho), DATA_DIR, nomeArquivo)) {
        printf("Caminho muito longo para %s.\n", nomeArquivo);
        return false;
    }
    FILE *fp = fopen(caminho, "r");
    if (!fp) {
        printf("Nao foi possivel abrir %s.\n", caminho);
        return false;
    }

    int quantidade = 0;
    if (fscanf(fp, "%d\n", &quantidade) != 1 || quantidade < 0) {
        printf("Formato invalido em %s.\n", caminho);
        fclose(fp);
        return false;
    }
    if (qtdeTCs + quantidade > max) {
        printf("Quantidade total de TCs excede o limite.\n");
        fclose(fp);
        return false;
    }

    char linha[256];
    for (int i = 0; i < quantidade; ++i) {
        if (!fgets(linha, sizeof(linha), fp)) {
            printf("Dados insuficientes em %s.\n", caminho);
            fclose(fp);
            return false;
        }
        TC tc;
        if (sscanf(linha, "%d %d %d %[^\n]", &tc.codigo, &tc.autor, &tc.orientador, tc.titulo) != 4) {
            printf("Linha invalida em %s: %s", caminho, linha);
            fclose(fp);
            return false;
        }
        if (buscarTC(tc.codigo)) {
            printf("Codigo de TC duplicado: %d.\n", tc.codigo);
            fclose(fp);
            return false;
        }
        Aluno *autor = buscarAluno(tc.autor);
        if (!autor) {
            printf("Autor %d nao encontrado para TC %d.\n", tc.autor, tc.codigo);
            fclose(fp);
            return false;
        }
        if (strcmp(autor->depto, sigla) != 0) {
            printf("Autor %d nao pertence ao departamento %s.\n", tc.autor, sigla);
            fclose(fp);
            return false;
        }
        Professor *orientador = buscarProfessor(tc.orientador);
        if (!orientador) {
            printf("Orientador %d nao encontrado para TC %d.\n", tc.orientador, tc.codigo);
            fclose(fp);
            return false;
        }
        tc.qtdeVotos = 0;
        listaTCs[qtdeTCs++] = tc;
    }

    fclose(fp);
    return true;
}

static bool carregarComissao(const char *arquivo) {
    char caminho[256];
    if (!montarCaminho(caminho, sizeof(caminho), DATA_DIR, arquivo)) {
        printf("Caminho muito longo para %s.\n", arquivo);
        return false;
    }
    FILE *fp = fopen(caminho, "r");
    if (!fp) {
        printf("Nao foi possivel abrir %s.\n", caminho);
        return false;
    }

    int quantidade = 0;
    if (fscanf(fp, "%d\n", &quantidade) != 1 || quantidade < 0) {
        printf("Formato invalido em %s.\n", caminho);
        fclose(fp);
        return false;
    }
    if (quantidade > max) {
        printf("Quantidade de eleitores excede o limite.\n");
        fclose(fp);
        return false;
    }

    char linha[64];
    for (int i = 0; i < quantidade; ++i) {
        if (!fgets(linha, sizeof(linha), fp)) {
            printf("Dados insuficientes em %s.\n", caminho);
            fclose(fp);
            return false;
        }
        retirarFimLinha(linha);
        if (!validarCPF(linha)) {
            printf("CPF invalido encontrado: %s.\n", linha);
            fclose(fp);
            return false;
        }
        if (buscarEleitor(linha)) {
            printf("CPF duplicado encontrado: %s.\n", linha);
            fclose(fp);
            return false;
        }
        Eleitor el;
        strncpy(el.cpf, linha, sizeof(el.cpf));
        el.cpf[sizeof(el.cpf) - 1] = '\0';
        el.votou = false;
        el.codigoTC = -1;
        comissao[qtdeEleitores++] = el;
    }

    fclose(fp);
    return true;
}

static bool carregarDadosIniciais(void) {
    if (!carregarProfessores("professores.txt")) {
        return false;
    }
    if (!carregarAlunos("alunos.txt")) {
        return false;
    }
    for (size_t i = 0; i < QTD_DEPTOS; ++i) {
        if (!carregarTCDepartamento(DEPARTAMENTOS[i])) {
            return false;
        }
    }
    if (!carregarComissao("comissao.txt")) {
        return false;
    }
    return true;
}

static char lerOpcao(const char *prompt, const char *validos) {
    char buffer[32];
    while (true) {
        printf("%s", prompt);
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            return '\0';
        }
        if (buffer[0] == '\n' || buffer[0] == '\0') {
            continue;
        }
        char c = (char)tolower((unsigned char)buffer[0]);
        if (strchr(validos, c)) {
            return c;
        }
        printf("Opcao invalida. Tente novamente.\n");
    }
}

static void lerEntrada(const char *prompt, char *destino, size_t tamanho) {
    while (true) {
        printf("%s", prompt);
        if (!fgets(destino, (int)tamanho, stdin)) {
            continue;
        }
        retirarFimLinha(destino);
        if (destino[0] == '\0') {
            printf("Entrada vazia. Tente novamente.\n");
            continue;
        }
        return;
    }
}

static void registrarVoto(Eleitor *eleitor, TC *tc) {
    eleitor->votou = true;
    eleitor->codigoTC = tc->codigo;
    tc->qtdeVotos += 1;
}

static void tratarEntradaVoto(void) {
    Eleitor *eleitor = NULL;
    while (true) {
        char cpf[64];
        lerEntrada("CPF do eleitor: ", cpf, sizeof(cpf));
        if (!validarCPF(cpf)) {
            printf("CPF invalido. Digite novamente.\n");
            continue;
        }
        eleitor = buscarEleitor(cpf);
        if (!eleitor) {
            printf("CPF nao pertence a comissao. Digite novamente.\n");
            continue;
        }
        if (eleitor->votou) {
            printf("Esse eleitor ja votou. Informe outro CPF.\n");
            continue;
        }
        break;
    }

    while (true) {
        char buffer[64];
        lerEntrada("Codigo do TC: ", buffer, sizeof(buffer));
        int codigoTC = atoi(buffer);
        TC *tc = buscarTC(codigoTC);
        if (!tc) {
            printf("TC nao encontrado. Informe outro codigo.\n");
            continue;
        }
        registrarVoto(eleitor, tc);
        printf("Voto registrado com sucesso.\n");
        break;
    }
}

static bool salvarParcial(const char *arquivo) {
    char caminho[256];
    if (!montarCaminho(caminho, sizeof(caminho), OUTPUT_DIR, arquivo)) {
        printf("Caminho muito longo para %s.\n", arquivo);
        return false;
    }
    FILE *fp = fopen(caminho, "w");
    if (!fp) {
        printf("Nao foi possivel criar %s.\n", caminho);
        return false;
    }

    int total = 0;
    for (int i = 0; i < qtdeEleitores; ++i) {
        if (comissao[i].votou) {
            total++;
        }
    }
    fprintf(fp, "%d\n", total);
    for (int i = 0; i < qtdeEleitores; ++i) {
        if (comissao[i].votou) {
            fprintf(fp, "%s %d\n", comissao[i].cpf, comissao[i].codigoTC);
        }
    }

    fclose(fp);
    printf("Dados salvos em %s.\n", caminho);
    return true;
}

static void obterInfoTC(const TC *tc, const Aluno **aluno, const Professor **orientador) {
    *aluno = buscarAluno(tc->autor);
    *orientador = buscarProfessor(tc->orientador);
}

static bool salvarResultado(const char *arquivo) {
    char caminho[256];
    if (!montarCaminho(caminho, sizeof(caminho), OUTPUT_DIR, arquivo)) {
        printf("Caminho muito longo para %s.\n", arquivo);
        return false;
    }
    FILE *fp = fopen(caminho, "w");
    if (!fp) {
        printf("Nao foi possivel criar %s.\n", caminho);
        return false;
    }

    int maior = 0;
    for (int i = 0; i < qtdeTCs; ++i) {
        if (listaTCs[i].qtdeVotos > maior) {
            maior = listaTCs[i].qtdeVotos;
        }
    }

    if (maior == 0) {
        fprintf(fp, "Nenhum voto computado.\n");
    } else {
        fprintf(fp, "TC vencedor\n");
        for (int i = 0; i < qtdeTCs; ++i) {
            if (listaTCs[i].qtdeVotos == maior) {
                const Aluno *aluno = NULL;
                const Professor *orientador = NULL;
                obterInfoTC(&listaTCs[i], &aluno, &orientador);
                fprintf(fp, "Codigo: %d\n", listaTCs[i].codigo);
                fprintf(fp, "Titulo: %s\n", listaTCs[i].titulo);
                if (aluno) {
                    fprintf(fp, "Aluno: %s\n", aluno->pes.nome);
                    fprintf(fp, "Depto aluno: %s\n", aluno->depto);
                }
                if (orientador) {
                    fprintf(fp, "Orientador: %s\n", orientador->pes.nome);
                    fprintf(fp, "Depto orientador: %s\n", orientador->depto);
                }
                fprintf(fp, "\n");
            }
        }
    }

    fprintf(fp, "Eleitores que votaram\n");
    for (int i = 0; i < qtdeEleitores; ++i) {
        if (comissao[i].votou) {
            fprintf(fp, "%s %d\n", comissao[i].cpf, comissao[i].codigoTC);
        }
    }
    fprintf(fp, "\nEleitores que nao votaram\n");
    for (int i = 0; i < qtdeEleitores; ++i) {
        if (!comissao[i].votou) {
            fprintf(fp, "%s\n", comissao[i].cpf);
        }
    }

    fclose(fp);
    printf("Resultado salvo em %s.\n", caminho);
    return true;
}

static bool carregarParcial(const char *arquivo) {
    char caminho[256];
    if (!montarCaminho(caminho, sizeof(caminho), OUTPUT_DIR, arquivo)) {
        printf("Caminho muito longo para %s.\n", arquivo);
        return false;
    }
    FILE *fp = fopen(caminho, "r");
    if (!fp) {
        printf("Arquivo %s nao encontrado.\n", caminho);
        return false;
    }

    resetarVotos();

    int quantidade = 0;
    if (fscanf(fp, "%d\n", &quantidade) != 1 || quantidade < 0) {
        printf("Formato invalido em %s.\n", caminho);
        fclose(fp);
        resetarVotos();
        return false;
    }
    if (quantidade > qtdeEleitores) {
        printf("Quantidade invalida em %s.\n", arquivo);
        fclose(fp);
        resetarVotos();
        return false;
    }

    char linha[64];
    for (int i = 0; i < quantidade; ++i) {
        if (!fgets(linha, sizeof(linha), fp)) {
            printf("Dados insuficientes em %s.\n", caminho);
            fclose(fp);
            resetarVotos();
            return false;
        }
        char cpf[32];
        int codigoTC = 0;
        if (sscanf(linha, "%14s %d", cpf, &codigoTC) != 2) {
            printf("Linha invalida em %s: %s", caminho, linha);
            fclose(fp);
            resetarVotos();
            return false;
        }
        Eleitor *eleitor = buscarEleitor(cpf);
        TC *tc = buscarTC(codigoTC);
        if (!eleitor || !tc) {
            printf("Registro inconsistente em %s: %s", caminho, linha);
            fclose(fp);
            resetarVotos();
            return false;
        }
        if (eleitor->votou) {
            printf("Eleitor duplicado em %s: %s\n", caminho, cpf);
            fclose(fp);
            resetarVotos();
            return false;
        }
        registrarVoto(eleitor, tc);
    }

    fclose(fp);
    return true;
}

static void executarMenuVotacao(void) {
    while (true) {
        printf("\nMENU2\n");
        printf("a) Entrar com voto\n");
        printf("b) Suspender votacao\n");
        printf("c) Concluir votacao\n");
        char opcao = lerOpcao("Escolha uma opcao: ", "abc");
        if (opcao == 'a') {
            tratarEntradaVoto();
        } else if (opcao == 'b') {
            if (salvarParcial("parcial.txt")) {
                printf("Votacao suspensa.\n");
            }
            return;
        } else if (opcao == 'c') {
            salvarResultado("resultado.txt");
            printf("Votacao concluida.\n");
            return;
        } else {
            printf("Opcao invalida.\n");
        }
    }
}

int main(void) {
    ajustarDiretorioParaExecutavel();
    if (!carregarDadosIniciais()) {
        return EXIT_FAILURE;
    }

    while (true) {
        printf("\nMENU1\n");
        printf("a) Iniciar nova votacao\n");
        printf("b) Continuar votacao gravada\n");
        char opcao = lerOpcao("Escolha uma opcao: ", "ab");
        if (opcao == 'a') {
            resetarVotos();
            executarMenuVotacao();
            break;
        } else if (opcao == 'b') {
            if (carregarParcial("parcial.txt")) {
                executarMenuVotacao();
                break;
            }
        } else {
            printf("Opcao invalida.\n");
        }
    }

    return EXIT_SUCCESS;
}
