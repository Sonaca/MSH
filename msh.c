#include "jobs.h"
#include "parser.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//VARIABLES GLOBALES
job* hijos = NULL;  //Lista Doblemente enlazada que almacena jobs(procesos con su pid, estado, argc y argv...)
pid_t padre;
int i;

void manejador_sigint() {
    if (getpid() == padre) {
        printf("\n");
        printf("msh> ");
        fflush(stdout);
    } else {
        kill(getpid(), SIGKILL);
    }
}

void manejador_sigstp() {
    if (getpid() == padre) { // Proceso padre
        printf("\n");
        printf("msh> ");
        fflush(stdout);
    } else { 
        kill(getpid(), SIGSTOP);
    }
}


void manejador_sigchld() {
    int estado;
    pid_t pid;
    while ((pid = waitpid(-1, &estado, WNOHANG)) > 0) {
        hijos = eliminarTrabajo(hijos, pid);
    }
    fflush(stdout);
}

int main(void) {
	tline *line;
	int input, output, error;
    char buf[1024];
    padre = getpid();

    printf("\033[1;35m");
    printf("msh> ");
	fflush(stdout);

    signal(SIGCHLD, manejador_sigchld);
    signal(SIGINT, manejador_sigint);
    signal(SIGTSTP, manejador_sigstp);

    while (fgets(buf, 1024, stdin)) {
        line = tokenize(buf);
        if (line == NULL) {
			printf("msh> ");
			fflush(stdout);
            continue;
        }

        if (strcmp(line->commands[0].argv[0], "exit") == 0) {
            liberarTrabajos(hijos);
            exit(0);
        }

        if (strcmp(line->commands[0].argv[0], "jobs") == 0) {
            if (hijos == NULL) {
                printf("La lista de trabajos está vacía.\n");
            } else {
                mostrarTrabajos(hijos);
            }
            printf("msh> ");
			fflush(stdout);
            continue;
        }

        if (strcmp(line->commands[0].argv[0], "cd") == 0) {
				if (line->commands[0].argc == 1) {
					if(chdir(getenv("HOME")) == 0){
						printf("Cambio de directorio correcto a HOME.\n");
					}
					else{
						printf("Error al cambiar de directorio.\n");
					}
				} else {
					if(chdir(line->commands[0].argv[1])==0){
						printf("Cambio de directorio correcto.\n");
					}
					else{
						printf("Error al cambiar de directorio.\n");
					}
				}
				printf("msh> ");
				fflush(stdout);
				continue;
        }
		
		if (strcmp(line->commands[0].argv[0], "bg") == 0) {
			if (line->commands[0].argc < 2) {
				printf("Debe proporcionar un índice válido\n");
				printf("msh> ");
				fflush(stdout);
				continue; 
			}
			int indice = atoi(line->commands[0].argv[1]); 
			job* aux = obtenerPorIndice(hijos, indice); 
			if (aux != NULL) {
				if (kill(aux->pid, SIGCONT) == 0) {
					if (aux->estado == 1) { 
						actualizarEstado(hijos, indice); //== que hacer aux -> pid = 0
						printf("Proceso reanudado: PID=%d\n", aux->pid);
					}
				} 
			} else {
				printf("Índice o proceso erróneo\n");
			}
			printf("msh> ");
			fflush(stdout);
			continue;
		}


        int **pipes = (int **)malloc((line->ncommands - 1) * sizeof(int *));

        for (i = 0; i < line->ncommands - 1; i++) {
            pipes[i] = (int *)malloc(2 * sizeof(int));
            if (pipe(pipes[i]) < 0) {
                printf("pipe.\n");
                exit(1);
            }
        }
		
		pid_t* pids = (pid_t*)malloc(line->ncommands * sizeof(pid_t));

        for (i = 0; i < line->ncommands; i++) {
            pid_t pid = fork();

            if (pid < 0) {
                printf("Falló el fork().\n");
                exit(1);
            }

            if (pid == 0) {  // Proceso hijo
			
				if (line->redirect_input != NULL && i == 0) {
                    input = open(line->redirect_input, O_RDONLY);
                    if (input == -1) {
                        printf("No se pudo abrir archivo de entrada.\n");
                        exit(1);
                    }
                    dup2(input, STDIN_FILENO);
					close(input);
                }

                if (line->redirect_output != NULL && i == line->ncommands-1) {
                    output = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (output == -1) {
                        printf("No se pudo abrir archivo de salida.\n");
                        exit(1);
                    }
                    dup2(output, STDOUT_FILENO);
					close(output);
                }

                if (line->redirect_error != NULL && i == line->ncommands-1) {
                    error = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (error == -1) {
                        printf("No se pudo abrir archivo de error.\n");
                        exit(1);
                    }
                    dup2(error, STDERR_FILENO);
					close(error);
                }
				
                // Si no es el primer comando, leemos de la pipe anterior
                if (i > 0) {
                    dup2(pipes[i-1][0], STDIN_FILENO);  // Lee de la pipe anterior
                }
                // Si no es el último comando, escribimos en la pipe siguiente
                if (i < line->ncommands - 1) {
                    dup2(pipes[i][1], STDOUT_FILENO);  // Escribe en la pipe siguiente
                }

                // Cerramos las pipes en el hijo
                for (int j = 0; j < line->ncommands - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                execvp(line->commands[i].argv[0], line->commands[i].argv);
                printf("msh -> Comando invalido.\n");
                exit(1);
            } else {
                pids[i] = pid;
                if (line->background) {
                    hijos = agregarTrabajo(hijos, pid, line->commands[i].argc, line->commands[i].argv, 0);
                }
            }
        }
		
		for (i = 0; i < line->ncommands - 1; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
            free(pipes[i]);  // Liberamos cada pipe
        }

        free(pipes);
		
		if (!line->background) {
            for (int i = 0; i < line->ncommands; i++) {
                int status;
                waitpid(pids[i], &status, WUNTRACED);
                if (WIFSTOPPED(status)) {
                    hijos = agregarTrabajo(hijos, pids[i], line->commands[i].argc, line->commands[i].argv, 1); 
                }
            }
        }
		
		free(pids);
		
        printf("msh> ");
        fflush(stdout);
    }

    liberarTrabajos(hijos);
    return 0;
}
