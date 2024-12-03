#include "jobs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int contadorIndice = 1;

job* inicializarLista() {
    return NULL;
}

job* agregarTrabajo(job* cabeza, pid_t pid, int argc, char** argv, int estado) {
    job* nuevo = (job*)malloc(sizeof(job));
    nuevo->indice = contadorIndice++;
    nuevo->pid = pid;
    nuevo->argc = argc;
    nuevo->estado = estado;
    nuevo->argv = (char**)malloc(argc * sizeof(char*));
    for (int i = 0; i < argc; i++) {
        nuevo->argv[i] = strdup(argv[i]);
    }
    nuevo->sig = NULL;
    nuevo->ant = NULL;

    if (cabeza == NULL) {
        return nuevo;
    } else {
        job* actual = cabeza;
        while (actual->sig != NULL) {
            actual = actual->sig;
        }
        actual->sig = nuevo;
        nuevo->ant = actual;
    }
    return cabeza;
}

job* eliminarTrabajo(job* cabeza, pid_t pid) {
    job* actual = cabeza;
    while (actual != NULL && actual->pid != pid) {
        actual = actual->sig;
    }
    if (actual == NULL) {
        return cabeza;
    }

    if (actual->ant == NULL) {
        cabeza = actual->sig;
        if (cabeza != NULL) {
            cabeza->ant = NULL;
        }
    } else {
        actual->ant->sig = actual->sig;
        if (actual->sig != NULL) {
            actual->sig->ant = actual->ant;
        }
    }

    for (int i = 0; i < actual->argc; i++) {
        free(actual->argv[i]);
    }
    free(actual->argv);
    free(actual);

    return cabeza;
}

void mostrarTrabajos(job* cabeza) {
    if (cabeza == NULL) {
        printf("No hay trabajos.\n");
        return;
    }
    job* actual = cabeza;
    while (actual != NULL) {
        printf("[%d] PID: %d Estado: %s Comando: ",
               actual->indice, actual->pid,
               actual->estado == 0 ? "Running" : "Stopped");
        for (int i = 0; i < actual->argc; i++) {
            printf("%s ", actual->argv[i]);
        }
        printf("\n");
        actual = actual->sig;
    }
}

void liberarTrabajos(job* cabeza) {
    while (cabeza != NULL) {
        job* temp = cabeza;
        cabeza = cabeza->sig;
        kill(temp->pid, SIGKILL);
        for (int i = 0; i < temp->argc; i++) {
            free(temp->argv[i]);
        }
        free(temp->argv);
        free(temp);
    }
}

job* obtenerPorIndice(job* cabeza, int indice) {
    job* actual = cabeza;
    int i = 1; 
    while (actual != NULL && i < indice) {
        actual = actual->sig;
        i++;
    }
    if (actual != NULL && i == indice) {
        return actual;
    }
    return NULL;
}

void actualizarEstado(job* cabeza, int indice) {
    job* act = obtenerPorIndice(cabeza,indice);
	if(act != NULL){
		act->estado = 0;
	}
}




