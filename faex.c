#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define MAX_NAME_LENGTH 64

#define CRIAD 0
#define CRIAA 1
#define REMOVED 2
#define REMOVEA 3
#define VERD 4
#define VERSET 5
#define MAPA 6
#define ARVORE 7
#define AJUDA 8
#define SAIR 9

#define TOTAL_SECTORS 256
#define SECTOR_SIZE 512 // bytes
#define RESERVED_SECTORS 10

typedef struct s_fat
{
    int index;
    struct s_fat *next;
} FAT;
// FAT. Esssa é a nossa tabela de alocação de blocos. Aponta para todos os blocos que possuem os dados DESTE arquivo.

typedef struct
{
    char filename[MAX_NAME_LENGTH];
    int filesize;
    FAT *list;
    struct tm *creation_time;
} file;

typedef struct s_file_pointer
{
    file *fp;
    struct s_file_pointer *next;
} file_pointer;

typedef struct dir
{
    char direname[MAX_NAME_LENGTH];
    int sector;
    int qtd_subdir;
    struct dir *subdirectories[TOTAL_SECTORS];
    file_pointer *head;
} directory;

int bitmap[TOTAL_SECTORS];

directory root;

void initialize_filesystem()
{
    /* INICIALIZAR OS SETORES */

    int i;
    for (i = 0; i < RESERVED_SECTORS; i++)
    {
        bitmap[i] = 1;
    }
    
    for (i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++)
    {
        bitmap[i] = 0;
    }

    // define o nome do diretorio raiz
    strcpy(root.direname, "\\");

    // define o bloco do diretorio raiz
    srand(time(NULL));
    root.sector = rand() % 10;

    // define a qtd de subdiretorios
    root.qtd_subdir = 0;

    // subdiretorios
    for (i = 0; i < TOTAL_SECTORS; i++)
    {
        root.subdirectories[i] = NULL;
    }

    // arquivos
    root.head = NULL;

    printf("root name: %s\n", root.direname);
    printf("root sector: %d\n\n", root.sector);
}

void makeDir(char *path) {
    int i = 0;
    int j;
    int is_root = 1;

    int depth = 0;
    for (i = 0; i < strlen(path); i++) {
        if (path[i] == '\\') {
            depth += 1;
        }
    }

    i = 0;
    char *arglist[depth + 1]; // numero de argumentos (profundidade na arvore)

    char *token;
    token = strtok(path, "\\");
    while (token != NULL) {
        arglist[i] = token;
        token = strtok(NULL, "\\");
        i++;
    }

    directory *pwd = &root;

    for (i = 0; i <= depth; i++) {
        int found = 0;

        for (j = 0; j < pwd->qtd_subdir; j++) {
            if (strcmp(pwd->subdirectories[j]->direname, arglist[i]) == 0) {
                pwd = pwd->subdirectories[j];
                found = 1;
                break;
            }
        }

        if (!found) {
            // se estamos no último nível do caminho e o diretório não foi encontrado, cria o diretório
            if (i == depth) {
                for (j = RESERVED_SECTORS; j < TOTAL_SECTORS; j++) {
                    if (bitmap[j] == 0) {
                        bitmap[j] = 1;
                        break;
                    }
                }

                if (j == TOTAL_SECTORS) {
                    fprintf(stderr, "ERRO: O sistema está cheio!\n");
                    return;
                }

                directory *new_dir = (directory *)malloc(sizeof(directory));
                if (new_dir == NULL) {
                    printf("Erro ao alocar memória para new_dir\n");
                    exit(1); // ou retorne um erro, dependendo do seu fluxo de código
                }

                strcpy(new_dir->direname, arglist[i]);
                new_dir->sector = j;
                new_dir->qtd_subdir = 0;
                for (int k = 0; k < TOTAL_SECTORS; k++) {
                    new_dir->subdirectories[k] = NULL;
                }
                new_dir->head = NULL;

                pwd->subdirectories[pwd->qtd_subdir] = new_dir;
                pwd->qtd_subdir += 1;
                pwd = new_dir;

                printf("criou diretorio %s no setor %d\n", new_dir->direname, new_dir->sector);
            } else {
                // se não estamos no último nível do caminho e o diretório não foi encontrado, cancela a operação
                printf("ERRO: O diretório %s não existe!\n", arglist[i]);
                return;
            }
        }
    }
}

void makeFile()
{
    printf("cria arquivo\n");
}

void removeDir(char *path) {
    int i = 0, j;
    int depth = 0;

    for (i = 0; i < strlen(path); i++) {
        if (path[i] == '\\') {
            depth += 1;
        }
    }

    char *arglist[depth + 1]; // número de argumentos (profundidade na árvore)
    i = 0;
    char *token = strtok(path, "\\");
    while (token != NULL) {
        arglist[i] = token;
        token = strtok(NULL, "\\");
        i++;
    }

    directory *pwd = &root;
    directory *parent = NULL;

    for (i = 0; i <= depth; i++) {
        int found = 0;

        for (j = 0; j < pwd->qtd_subdir; j++) {
            if (strcmp(pwd->subdirectories[j]->direname, arglist[i]) == 0) {
                parent = pwd;
                pwd = pwd->subdirectories[j];
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("ERRO: O diretório %s não existe!\n", arglist[i]);
            return;
        }
    }

    if (pwd->qtd_subdir != 0 || pwd->head != NULL) {
        printf("ERRO: Diretório não está vazio!\n");
        return;
    }

    // libera o bitmap do diretório
    bitmap[pwd->sector] = 0;

    // remove o diretório do parent
    for (i = 0; i < parent->qtd_subdir; i++) {
        if (parent->subdirectories[i] == pwd) {
            for (j = i; j < parent->qtd_subdir - 1; j++) {
                parent->subdirectories[j] = parent->subdirectories[j + 1];
            }
            parent->subdirectories[parent->qtd_subdir - 1] = NULL;
            parent->qtd_subdir -= 1;
            break;
        }
    }

    free(pwd);
    printf("Diretório removido com sucesso!\n");
}

void removeFile()
{
    printf("remove arquivo\n");
}

void seeDirectory(char *path)
{
    // int i = 0;
    // char *arglist[TOTAL_SECTORS];
    // char *token;
    // while (strtok(path, "\\") != NULL)
    // {
    //     arglist[++i] = strtok(path, "\\");
    //     printf("argumento %d \n", i);
    // }

    // directory pwd = root;
    // int path_length = i;
    // int current_arg = 0;

    // while (current_arg < path_length)
    // {
    //     // busca o diretorio no indice do current_arg dentro do root
    //     for (i = 0; i < pwd.qtd_subdir; i++)
    //     {
    //         // se achar o diretorio troca o ponteiro para para ele
    //         if (strcmp(pwd.subdirectories[i]->direname, arglist[current_arg]) == 0)
    //         {
    //             // se existir então percorre seus subdiretorios e verifica se algum bate com o proximo path do arglist
    //             pwd = *(pwd.subdirectories[i]);
    //             current_arg++;
    //             // break; sair só do for, posso usar flag mas é só pra n passar por 256 possiveis dirs (pior caso)
    //         }
    //     }
    //     if (i == pwd.qtd_subdir)
    //     {
    //         fprintf(stderr, "ERRO: Diretório %s não existe!\n", arglist[current_arg]);
    //         return;
    //     }
    // }

    // // printa a primeira string e coloca 50 "espaços" antes da segunda string
    // printf("%-50s %s\n\n", "Arquivos:", "Tamanho:");
    // for (i = 0; i < pwd.qtd_subdir; i++)
    // {
    //     // printar algo que indique que são diretórios
    //     //  printa o diretório e coloca 50 "espaços" antes do tamanho
    //     printf("%-50s", pwd.subdirectories[i]);
    //     printf("%d bytes", SECTOR_SIZE);
    //     printf("\n");
    // }

    // while (pwd.head != -1)
    // {
    //     printf("%-50s", pwd.head->fp->filename);
    //     printf("%s bytes", pwd.head->fp->filesize);
    //     // printar metadados (horario e data de criação)
    //     printf("\n");
    //     pwd.head = pwd.head->next;
    // }

    // // O simulador deve oferecer um comando verd que exibe os arquivos e subdiretórios do diretório
    // // indicado como parâmetro. Na listagem, os diretórios devem conter uma (indicação de que são diretórios) e os
    // // arquivos devem conter (tamanho), além de (data e hora de criação). No final da listagem deve aparecer o (total
    // // de arquivos e diretórios), o (total do tamanho dos arquivos) e a (quantidade de espaço livre no disco). Lembre-se
    // // que o espaço vazio dos setores utilizados não pode ser utilizado por outros arquivos ou diretórios, portanto
    // // não devem entrar na conta do espaço livre. Também deve ser indicado o (tamanho total alocado para
    // // arquivos e diretórios), incluindo aí o espaço perdido com fragmentação (espaço não usado em setores ocupados).
    // int free_sectors = 0;
    // int free_disk_space;
    // for (i = 0; i < TOTAL_SECTORS; i++)
    // {
    //     if (bitmap[i] == 0)
    //     {
    //         free_sectors++;
    //     }
    // }
    // free_disk_space = free_sectors * SECTOR_SIZE;
    // printf("\nEspaço livre em disco: %d bytes", free_disk_space);

    // printf("ve diretorio\n");
}

void seeSectors()
{
    printf("ve setores\n");
}

void showMap() { //working
	for (int i = 0; i < RESERVED_SECTORS; i++) {
		printf ("░ ");
	}
    for (int i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++) {
        if (bitmap[i] == 1) {
            printf("▇ ");
        } else {
            printf("▢ "); 
        }

        if (i % 64 == 63) {
            printf("\n");
        }
    }
	printf("\n");
}

void printTree(directory *dir, int depth) { //working for directories
    int i;
	for (i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("%s\n", dir->direname);

    for (i = 0; i < dir->qtd_subdir; i++) {
        printTree(dir->subdirectories[i], depth + 1);
    }
}

void showHelp()
{
    printf("\n\t\t************** FAEX - Ajuda **************\n");
    printf("\t\t------------------------------------------\n");
    printf("criad [path\\]diretorio\t\t-> cria diretorio\n");
    printf("criaa [path\\]arquivo\t\t-> cria arquivo\n");
    printf("deletad [path\\]diretorio\t-> deleta diretorio\n");
    printf("deletaa [path\\]arquivo\t\t-> deleta arquivo\n");
    printf("verd [path]\t\t\t-> mostrar diretorio\n");
    printf("verset [path\\]arquivo\t\t-> mostra setores ocupados pelo arquivo\n");
    printf("arvore\t\t\t\t-> arvore de diretorios\n");
    printf("mapa\t\t\t\t-> mostra tabela de setores\n");
    printf("ajuda\t\t\t\t-> mostra os comandos disponiveis do sistema\n");
    printf("sair\t\t\t\t-> encerra o programa\n\n");
}

int hash(char *program)
{
    if (strncmp(program, "criad", 5) == 0)
    {
        return CRIAD;
    }
    else if (strncmp(program, "criaa", 5) == 0)
    {
        return CRIAA;
    }
    else if (strncmp(program, "removed", 7) == 0)
    {
        return REMOVED;
    }
    else if (strncmp(program, "removea", 7) == 0)
    {
        return REMOVEA;
    }
    else if (strncmp(program, "verd", 4) == 0)
    {
        return VERD;
    }
    else if (strncmp(program, "verset", 6) == 0)
    {
        return VERSET;
    }
    else if (strncmp(program, "mapa", 4) == 0)
    {
        return MAPA;
    }
    else if (strncmp(program, "arvore", 6) == 0)
    {
        return ARVORE;
    }
    else if (strncmp(program, "ajuda", 5) == 0)
    {
        return AJUDA;
    }
    else if (strncmp(program, "sair", 4) == 0)
    {
        return SAIR;
    }
    else
        return -1;
}

int main(void)
{

    char *token;
    char *program;
    char *arg = NULL;

    initialize_filesystem();

    while (1)
    {
        printf("#");
        char instruction[1024];
        if (scanf(" %[^\n]%*c", instruction) == -1)
        {
            printf("exit\n");
            exit(0);
        }

        token = strtok(instruction, " ");
        program = token;
        if ((token = strtok(NULL, " ")) != NULL)
        {
            arg = token;
        }

        switch (hash(program))
        {
        case CRIAD:
            makeDir(arg);
            break;

        case CRIAA:
            makeFile();
            break;

        case REMOVED:
            removeDir(arg);
            break;

        case REMOVEA:
            removeFile();
            break;

        case VERD:
            seeDirectory(arg);
            break;

        case VERSET:
            seeSectors();
            break;

        case MAPA:
            showMap();
            break;

        case ARVORE:
            printTree(&root, 0);
            break;

        case AJUDA:
            showHelp();
            break;

        case SAIR:
            exit(0);

        default:
            break;
        }
    }
}