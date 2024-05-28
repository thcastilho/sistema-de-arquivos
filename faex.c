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

typedef struct s_fat {
    int index;
    struct s_fat *next;
}FAT;
// FAT. Esssa é a nossa tabela de alocação de blocos. Aponta para todos os blocos que possuem os dados DESTE arquivo.

typedef struct {
	char filename[MAX_NAME_LENGTH];
	int filesize;
	FAT *list;
	struct tm *creation_time;
}file;

typedef struct s_file_pointer{
	file* fp;
	struct s_file_pointer *next;
}file_pointer;

typedef struct dir {
	char direname[MAX_NAME_LENGTH];
	int sector;
	int qtd_subdir;
	struct dir *subdirectories[TOTAL_SECTORS];
	file_pointer* head; 
}directory;

int bitmap[TOTAL_SECTORS];

directory root;

void initialize_filesystem() {
    /* INICIALIZAR OS SETORES */
	
	// Mark reserved sectors as used
	int i;
    for (i = 0; i < RESERVED_SECTORS; i++) {
        bitmap[i] = 1;
    }
    // Mark data sectors as free
    for (i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++) {
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
	for (i = 0; i < TOTAL_SECTORS; i++) {
		root.subdirectories[i] = NULL;
	}

	// arquivos
	root.head = NULL;

	printf("name: %s\n", root.direname);
	printf("sector: %d\n", root.sector);
}

void makeDir(char* path) {
	printf("cria diretorio\n");

	int i = 0;
	int is_root = 1;

	char* arglist[TOTAL_SECTORS];
	char *token;
	token = strtok(path, "\\");
	arglist[i] = token;
	printf("%s\n", arglist[i]);

	if ((token = strtok(NULL, "\\")) != NULL) {
		is_root = 0;
		arglist[++i] = token;
		printf("%s\n", arglist[i]);
	}

	directory pwd = root;

	if (is_root) {
		if (pwd.qtd_subdir == 0) { // se nao existe nenhum diretorio, entao passe livre para criar
			// se nao existe, entao...
			for (i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++) {
				if (bitmap[i] == 0) {
					bitmap[i] = 1;
					break;
				}
			}

			if (i == TOTAL_SECTORS) {
				fprintf(stderr, "ERRO: O sistema esta cheio!\n");
				return;
			}

			directory *new_dir;
			strcpy(new_dir->direname, arglist[0]);
			new_dir->sector = i;

			// define a qtd de subdiretorios
			new_dir->qtd_subdir = 0;

			// subdiretorios
			for (i = 0; i < TOTAL_SECTORS; i++) {
				new_dir->subdirectories[i] = NULL;
			}

			// arquivos
			new_dir->head = NULL;

			root.subdirectories[i] = new_dir;
			root.qtd_subdir += 1;
		} else {
			// busca no path atual se nao existe diretorio com este nome
			for (i = 0; i < pwd.qtd_subdir; i++) {
				if(strcmp(pwd.subdirectories[i]->direname, arglist[0]) == 0) { // se o diretorio for o mesmo a ser inserido 
					fprintf(stderr, "ERRO: Diretorio ja existente!\n");
					return;
				}
			}

			// se nao existe, entao...
			for (i = RESERVED_SECTORS; i < TOTAL_SECTORS; i++) {
				if (bitmap[i] == 0) {
					bitmap[i] = 1;
					break;
				}
			}

			if (i == TOTAL_SECTORS) {
				fprintf(stderr, "ERRO: O sistema esta cheio!\n");
				return;
			}

			directory *new_dir;
			strcpy(new_dir->direname, arglist[0]);
			new_dir->sector = i;

			// define a qtd de subdiretorios
			new_dir->qtd_subdir = 0;

			// subdiretorios
			for (i = 0; i < TOTAL_SECTORS; i++) {
				new_dir->subdirectories[i] = NULL;
			}

			// arquivos
			new_dir->head = NULL;

			root.subdirectories[i] = new_dir;
			root.qtd_subdir += 1;

		}
	// } else {
	// 	// while()
	}

	printf("criou diretorio\n");
}

void makeFile() {
	printf("cria arquivo\n");
}

void removeDir() {
	printf("remove diretorio\n");
}

void removeFile() {
    printf("remove arquivo\n");
}

void seeDirectory() {
	printf("ve diretorio\n");
}

void seeSectors() {
	printf("ve setores\n");
}

void showMap() {
	printf("ve mapa\n");
}

void showTree() {
	printf("ve arvore\n");
}

void showHelp() {
	printf("\n\t\t************** FAEX - Ajuda **************\n");
	printf("\t\t------------------------------------------\n");
	printf("criad [path\\]diretorio\t\t-> cria diretorio\n");
	printf("criaa [path\\]arquivo\t\t-> cria arquivo\n");
	printf("deletad [path\\]diretorio\t-> deleta diretorio\n");
	printf("deletaa [path\\]arquivo\t\t-> deleta arquivo\n");
	printf("verd [path]\t\t\t-> mostrar diretorio\n");
	printf("verset [path\\]arquivo\t\t-> mostra granulos ocupados pelo arquivo\n");
	printf("arvore\t\t\t\t-> arvore de diretorios\n");
	printf("mapa\t\t\t\t-> mostra tabela de granulos\n");
	printf("ajuda\t\t\t\t-> mostra os comandos disponiveis do sistema\n");
	printf("sair\t\t\t\t-> encerra o programa\n\n");
}

int hash(char* program) {
	if (strncmp(program, "criad", 5) == 0) {
		return CRIAD;
	} else if (strncmp(program, "criaa", 5) == 0) {
		return CRIAA;
	} else if (strncmp(program, "removed", 7) == 0) {
		return REMOVED;
	} else if (strncmp(program, "removea", 7) == 0) {
		return REMOVEA;
	} else if (strncmp(program, "verd", 4) == 0) {
		return VERD;
	} else if (strncmp(program, "verset", 6) == 0) {
		return VERSET;
	} else if (strncmp(program, "mapa", 4) == 0) {
		return MAPA;
	} else if (strncmp(program, "arvore", 6) == 0) {
		return ARVORE;
	} else if (strncmp(program, "ajuda", 5) == 0) {
		return AJUDA;
	} else if (strncmp(program, "sair", 4) == 0) {
		return SAIR;
	} else return -1;
}

int main(void) {

	char *token;
	char* program;
	char* arg = NULL;

	initialize_filesystem();

    while(1) {
		printf("#");
        char instruction[1024];
		if (scanf(" %[^\n]%*c", instruction) == -1)
		{
			printf("exit\n");
			exit(0);
		}

		token = strtok(instruction, " ");
		program = token;
		if ((token = strtok(NULL, " ")) != NULL) {
			arg = token;
			printf("%s\n", arg);
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
				removeDir();
				break;

			case REMOVEA:
				removeFile();
				break;

			case VERD:
				seeDirectory();
				break;

			case VERSET:
				seeSectors();
				break;

			case MAPA:
				showMap();
				break;

			case ARVORE:
				showTree();
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