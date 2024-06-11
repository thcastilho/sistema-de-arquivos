#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    struct tm *creation_time;
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
    //inicializa os setores reservados (boot, sistema de arquivos e raiz)
    for (i = 0; i < RESERVED_SECTORS; i++)
    {
        bitmap[i] = 1;
    }

    //incialia o resto dos setores
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

    // incializa os subdiretorios
    for (i = 0; i < TOTAL_SECTORS; i++)
    {
        root.subdirectories[i] = NULL;
    }

    // arquivos
    root.head = NULL;

    printf("root name: %s\n", root.direname);
    printf("root sector: %d\n\n", root.sector);
}

void makeDir(char *path)
{
    int i, j;
    int is_root = 1;

    int depth = 0;
    for (i = 0; i < strlen(path); i++)
    {
        if (path[i] == '\\')
        {
            depth += 1;	//calcula a profundidade do dir
        }
    }

    i = 0;
    char *arglist[depth + 1]; // numero de argumentos (profundidade na arvore)

    char *token;
    token = strtok(path, "\\");
    while (token != NULL)
    {
        arglist[i] = token;		//preenche arglist com os subdirs
        token = strtok(NULL, "\\");
        i++;
    }

    directory *pwd = &root;

    for (i = 0; i <= depth; i++)
    {
        int found = 0;

        //procura o subdir arglist[i] em pwd
        for (j = 0; j < pwd->qtd_subdir; j++)
        {
            if (strcmp(pwd->subdirectories[j]->direname, arglist[i]) == 0)
            {
                pwd = pwd->subdirectories[j];
                found = 1;
                break;
            }
        }

        if (!found)
        {
            // se estamos no último nível do caminho e o diretório não foi encontrado, cria o diretório
            if (i == depth)
            {
                for (j = RESERVED_SECTORS; j < TOTAL_SECTORS; j++)
                {
                    if (bitmap[j] == 0)
                    {
                        bitmap[j] = 1;	//marca o setor como ocupado
                        break;
                    }
                }

                if (j == TOTAL_SECTORS)
                {
                    fprintf(stderr, "ERRO: O sistema está cheio!\n");
                    return;
                }

                directory *new_dir = (directory *)malloc(sizeof(directory));
                if (new_dir == NULL)
                {
                    printf("Erro ao alocar memória para new_dir\n");
                    exit(1); // ou retorne um erro, dependendo do seu fluxo de código
                }

                //inicializa o novo subdir
                strcpy(new_dir->direname, arglist[i]);
                new_dir->sector = j;
                new_dir->qtd_subdir = 0;
                for (int k = 0; k < TOTAL_SECTORS; k++)
                {
                    new_dir->subdirectories[k] = NULL;
                }
                new_dir->head = NULL;

                // Obtém o tempo atual
                time_t segundos;
                time(&segundos);
                new_dir->creation_time = localtime(&segundos);

                pwd->subdirectories[pwd->qtd_subdir] = new_dir;	//insere no dir atual
                pwd->qtd_subdir += 1;
                pwd = new_dir;

                printf("Diretório %s criado no setor %d\n", new_dir->direname, new_dir->sector);
            }
            else
            {
                // se não estamos no último nível do caminho e o diretório não foi encontrado, cancela a operação
                printf("ERRO: O diretório %s não existe!\n", arglist[i]);
                return;
            }
        }
    }
}

directory *findDirectory(char *path)
{
    if (strcmp(path, "") == 0)
    {
        return &root;
    }

    int i, j;
    int depth = 0;

    for (i = 0; i < strlen(path); i++)
    {
        if (path[i] == '\\')
        {
            depth++;	//calcula a profundidade do dir
        }
    }

    i = 0;
    char *arglist[depth + 1];

    char *token;
    token = strtok(path, "\\");
    while (token != NULL)
    {
        arglist[i] = token;			//preenche arglist com os subdirs do path
        token = strtok(NULL, "\\");
        i++;
    }

    directory *pwd = &root;

    for (i = 0; i <= depth; i++)
    {
        int found = 0;

        //procura o subdir arglist[i] no pwd
        for (j = 0; j < pwd->qtd_subdir; j++)
        {
            if (strcmp(pwd->subdirectories[j]->direname, arglist[i]) == 0)
            {
                pwd = pwd->subdirectories[j];
                found = 1;
                break;
            }
        }

        if (!found)
        {
            printf("ERRO: O diretório %s não existe!\n", arglist[i]);
            return NULL;
        }
    }

    return pwd;
}

void createFile(char *full_path)
{
    char path[MAX_NAME_LENGTH] = "";
    char filename[MAX_NAME_LENGTH];

    // Extrai o nome do arquivo e o caminho do diretório
    char *last_backslash = strrchr(full_path, '\\');
    if (last_backslash == NULL)
    {
        strcpy(filename, full_path);
    }
    else
    {
        strncpy(path, full_path, last_backslash - full_path);
        path[last_backslash - full_path] = '\0';
        strcpy(filename, last_backslash + 1);
    }

    // Encontra o diretório especificado
    directory *dir = findDirectory(path);
    if (dir == NULL)
    {
        return;
    }

    // Verifica se um arquivo com o mesmo nome já existe no diretório
    file_pointer *fp = dir->head;
    while (fp != NULL)
    {
        if (strcmp(fp->fp->filename, filename) == 0)
        {
            printf("ERRO: O arquivo %s já existe!\n", filename);
            return;
        }
        fp = fp->next;
    }

    // Pergunta ao usuário o tamanho do arquivo
    int filesize;
    printf("Digite o tamanho do arquivo em bytes: ");
    scanf("%d", &filesize);

    int total_disk_size = TOTAL_SECTORS * SECTOR_SIZE;
    for (int i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++)
    {
        if (bitmap[i] == 1)
        {
            total_disk_size -= SECTOR_SIZE;	//calcula a quantidade livre do disco
        }
    }
    int free_space = total_disk_size;

    if (filesize > free_space){
        printf("ERRO: O arquivo é maior do que o espaço disponível no sistema!\n");
        return;
    }

    // Cria um novo arquivo e aloca memória para ele
    file *new_file = (file *)malloc(sizeof(file));
    if (new_file == NULL)
    {
        printf("Erro ao alocar memória para new_file\n");
        exit(1);
    }

    strcpy(new_file->filename, filename);
    new_file->filesize = filesize;

    // Obtém o tempo atual
    time_t segundos;
    time(&segundos);
    new_file->creation_time = localtime(&segundos);

    // Calcula a quantidade de setores necessários para armazenar o arquivo
    int sectors_needed = (filesize + SECTOR_SIZE - 1) / SECTOR_SIZE;
    FAT *current_fat = NULL;

    for (int i = 0; i < sectors_needed; i++)
    {
        // int sector_found = 0;
        for (int j = RESERVED_SECTORS; j < TOTAL_SECTORS; j++)
        {
            if (bitmap[j] == 0)	//checa se o setor está livre
            {
                bitmap[j] = 1;
                FAT *new_fat = (FAT *)malloc(sizeof(FAT));
                if (new_fat == NULL)
                {
                    printf("Erro ao alocar memória para new_fat\n");
                    exit(1);
                }
                new_fat->index = j;
                new_fat->next = NULL;

                if (current_fat == NULL)
                {
                    new_file->list = new_fat;
                }
                else
                {
                    current_fat->next = new_fat;
                }
                current_fat = new_fat;

                // sector_found = 1;
                break;
            }
        }

        // if (!sector_found)
        // {
        //     printf("ERRO: O sistema está cheio!\n");
        //     free(new_file);
        //     return;
        // }
    }

    // Adiciona o arquivo à lista de arquivos do diretório
    file_pointer *new_fp = (file_pointer *)malloc(sizeof(file_pointer));
    if (new_fp == NULL)
    {
        printf("Erro ao alocar memória para new_fp\n");
        exit(1);
    }

    new_fp->fp = new_file;
    new_fp->next = dir->head;
    dir->head = new_fp;

    printf("Criou arquivo %s de tamanho %d bytes\n", new_file->filename, new_file->filesize);
}

void removeDir(char *path)
{
    int i, j;
    int depth = 0;

    for (i = 0; i < strlen(path); i++)
    {
        if (path[i] == '\\')
        {
            depth += 1;	//calcula a profundidade do dir
        }
    }

    char *arglist[depth + 1]; // número de argumentos (profundidade na árvore)
    i = 0;
    char *token = strtok(path, "\\");
    while (token != NULL)
    {
        arglist[i] = token;		//preenche arglist com cada subdir de path
        token = strtok(NULL, "\\");
        i++;
    }

    directory *pwd = &root;
    directory *parent = NULL;

    for (i = 0; i <= depth; i++)
    {
        int found = 0;

        //itera pelos subdirs de pwd procurando por arglist[i]
        for (j = 0; j < pwd->qtd_subdir; j++)
        {
            if (strcmp(pwd->subdirectories[j]->direname, arglist[i]) == 0)
            {
                parent = pwd;
                pwd = pwd->subdirectories[j];	//vai pro subdir do arglist[i]
                found = 1;
                break;
            }
        }

        if (!found)
        {
            printf("ERRO: O diretório %s não existe!\n", arglist[i]);
            return;
        }
    }

    if (pwd->qtd_subdir != 0 || pwd->head != NULL)
    {
        printf("ERRO: Diretório não está vazio!\n");
        return;
    }

    // libera o bitmap do diretório
    bitmap[pwd->sector] = 0;

    // remove o diretório do parent
    for (i = 0; i < parent->qtd_subdir; i++)
    {
        if (parent->subdirectories[i] == pwd)
        {
            for (j = i; j < parent->qtd_subdir - 1; j++)
            {
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

void deleteFile(char *full_path)
{
    char path[MAX_NAME_LENGTH] = "";
    char filename[MAX_NAME_LENGTH];

    //determina a posição da última barra com essa função
    char *last_backslash = strrchr(full_path, '\\');
    if (last_backslash == NULL)
    {
        strcpy(filename, full_path);
    }
    else
    {
        strncpy(path, full_path, last_backslash - full_path);	//copia o path sem o /nome_do_arquivo
        path[last_backslash - full_path] = '\0';
        strcpy(filename, last_backslash + 1);	//copia o nome do arquivo
    }

    directory *dir = findDirectory(path);
    if (dir == NULL)
    {
        return;
    }

    file_pointer *prev_fp = NULL;
    file_pointer *fp = dir->head;

    //itera pela lista e procura o arquivo do argumento
    while (fp != NULL)
    {
        if (strcmp(fp->fp->filename, filename) == 0)
        {
            if (prev_fp == NULL)
            {
                dir->head = fp->next;	//deleta o arq no começo da lista
            }
            else
            {
                prev_fp->next = fp->next;	//deleta em outra posição
            }

            FAT *current_fat = fp->fp->list;
            while (current_fat != NULL)
            {
                bitmap[current_fat->index] = 0;	//muda pra 0 o bit do setor atual
                FAT *temp_fat = current_fat;	
                current_fat = current_fat->next;	//vai pro próximo bloco da tabela
                free(temp_fat);	//libera o bloco anterior
            }

            free(fp->fp);
            free(fp);	//libera o arquivo

            printf("Arquivo %s excluído\n", filename);
            return;
        }

        prev_fp = fp;
        fp = fp->next;
    }

    printf("ERRO: O arquivo %s não foi encontrado!\n", filename);
}

void seeDirectory(char *path)
{
    directory *dir;
    if (path == NULL){
        dir = &root;
    } else {
        dir = findDirectory(path);
        if (dir == NULL)
        {
            /* ERRO! */
            return;
        }
    }

    int total_files = 0;
    int total_dirs = 0;
    int total_file_size = 0;

    // printf("Conteúdo de %s:\n", path);
    //printa os subdiretorios
    for (int i = 0; i < dir->qtd_subdir; i++)
    {
        directory *subdir = dir->subdirectories[i];
        printf("%s\t<DIR>\t%d/%d/%d %d:%d\n",
               subdir->direname,
               subdir->creation_time->tm_mday,
               subdir->creation_time->tm_mon + 1,
               subdir->creation_time->tm_year + 1900,
               subdir->creation_time->tm_hour,
               subdir->creation_time->tm_min);
        total_dirs++;
    }

    //printa os arquivos
    file_pointer *fp = dir->head;
    while (fp != NULL)
    {
        printf("%s\t%d\t%d/%d/%d %d:%d\n",
               fp->fp->filename,
               fp->fp->filesize,
               fp->fp->creation_time->tm_mday,
               fp->fp->creation_time->tm_mon + 1,
               fp->fp->creation_time->tm_year + 1900,
               fp->fp->creation_time->tm_hour,
               fp->fp->creation_time->tm_min);
        total_files++;
        total_file_size += fp->fp->filesize;

        fp = fp->next;
    }

    int total_disk_size = TOTAL_SECTORS * SECTOR_SIZE;
    for (int i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++)
    {
        if (bitmap[i] == 1)
        {
            total_disk_size -= SECTOR_SIZE;	//calcula a quantidade livre do disco
        }
    }
    int free_space = total_disk_size;

    printf("\n%d diretório(s)\t%d bytes\n", total_dirs, total_file_size);
    printf("%d arquivo(s)\t%d bytes livres\n", total_files, free_space);
}

void seeSectors(char *full_path)
{
    char path[MAX_NAME_LENGTH] = "";
    char filename[MAX_NAME_LENGTH];

    //determina a posição da última barra com essa função
    char *last_backslash = strrchr(full_path, '\\');
    if (last_backslash == NULL)
    {
        strcpy(filename, full_path);
    }
    else
    {
        strncpy(path, full_path, last_backslash - full_path);	//copia o path sem o /nome_do_arquivo
        path[last_backslash - full_path] = '\0';
        strcpy(filename, last_backslash + 1);	//copia o nome do arquivo
    }

    directory *dir = findDirectory(path);
    if (dir == NULL)
    {
        return;
    }

    //itera pela lista e procura o arquivo do argumento
    file_pointer *fp = dir->head; //inicializa fp com o primeiro arq do diretorio 
    while (fp != NULL)
    {
        if (strcmp(fp->fp->filename, filename) == 0) //compara o nome do arquivo atual com o arg
        {
            printf("Setores ocupados pelo arquivo %s:\n", filename);

            FAT *current_fat = fp->fp->list;	//inicializa current_fat com o primeiro setor do arq
            for (int i = 0; i < RESERVED_SECTORS; i++)
            {
                printf("░ ");	//reservado
            }

            //itera por todos os setores não reservados
            for (int i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++)
            {
                bool is_occupied_by_file = false;
                FAT *temp_fat = current_fat;

                //pra cada setor do sistema de arquivos, itera pela tabela FAT do
                //arquivo e checa se o bloco atual é igual ao setor i
                while (temp_fat != NULL)
                {
                    if (temp_fat->index == i)
                    {
                        is_occupied_by_file = true;	//se for, significa q o arq ocupa esse setor
                        break;
                    }
                    temp_fat = temp_fat->next;
                }

                if (is_occupied_by_file)
                {
                    printf("▇ ");	//ocupado
                }
                else
                {
                    printf("▢ ");	//nao ocupado
                }

                if (i % 64 == 63)
                {
                    printf("\n");	//quebra a linha
                }
            }
            printf("\n");

            return;
        }
        fp = fp->next;	//vai pro próximo elemento
    }

    printf("ERRO: O arquivo %s não existe!\n", filename);
}

void showMap()
{ // working
    for (int i = 0; i < RESERVED_SECTORS; i++)
    {
        printf("░ ");	//reservado
    }
    for (int i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++)
    {
        if (bitmap[i] == 1)
        {
            printf("▇ ");	//ocupado
        }
        else
        {
            printf("▢ ");	//nao ocupado
        }

        if (i % 64 == 63)
        {
            printf("\n");	//quebra a linha
        }
    }
    printf("\n");
}

void printTree(directory *dir, int depth)
{ // working for directories
    for (int i = 0; i < depth; i++)
    {
        printf("  ");
    }
    printf("%s\n", dir->direname);

    for (int i = 0; i < dir->qtd_subdir; i++)
    {
        printTree(dir->subdirectories[i], depth + 1);
    }
}

void showHelp()
{
    printf("\n\t\t************** FAEX - Ajuda **************\n");
    printf("\t\t------------------------------------------\n");
    printf("criad [path\\]diretório\t\t-> cria diretório\n");
    printf("criaa [path\\]arquivo\t\t-> cria arquivo\n");
    printf("deletad [path\\]diretório\t-> deleta diretório\n");
    printf("deletaa [path\\]arquivo\t\t-> deleta arquivo\n");
    printf("verd [path]\t\t\t-> mostrar diretório\n");
    printf("verset [path\\]arquivo\t\t-> mostra setores ocupados pelo arquivo\n");
    printf("mapa\t\t\t\t-> mostra tabela de setores\n");
    printf("arvore\t\t\t\t-> árvore de diretórios\n");
    printf("ajuda\t\t\t\t-> mostra os comandos disponíveis do sistema\n");
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

    initialize_filesystem();

    while (1)
    {
        printf("#");
        char *arg = NULL;
        char instruction[1024];

        if (scanf(" %[^\n]%*c", instruction) == -1)
        {
            printf("exit\n");
            exit(0);
        }

        //tokeniza a string de entrada, separando o comando e o argumento
        token = strtok(instruction, " ");
        program = token;
        if ((token = strtok(NULL, " ")) != NULL)
        {
            arg = token;
        }

        //retorna um int pra cada comando possível
        switch (hash(program))
        {
        case CRIAD:
            makeDir(arg);
            break;

        case CRIAA:
            createFile(arg);
            break;

        case REMOVED:
            removeDir(arg);
            break;

        case REMOVEA:
            deleteFile(arg);
            break;

        case VERD:
            seeDirectory(arg);
            break;

        case VERSET:
            seeSectors(arg);
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
            printf("Comando inválido, tente 'ajuda' para ver os comandos disponíveis\n");
            break;
        }
    }

    return 0;
}
