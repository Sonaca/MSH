#ifndef JOBS_H
#define JOBS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

typedef struct job {
    int indice;           
    pid_t pid;            
    int argc;             
    int estado;       // El estado que será 0 o 1 en funcón de si Running o Stopped;
    char** argv;          
    struct job* sig,*ant;      
} job;

job* inicializarLista();
job* agregarTrabajo(job* cabeza, pid_t pid, int argc, char** argv, int estado);
job* eliminarTrabajo(job* cabeza, pid_t pid);
void mostrarTrabajos(job* cabeza);
void liberarTrabajos(job* cabeza);
job* obtenerPorIndice(job* cabeza, int indice);
void actualizarEstado(job* cabeza, int indice);
#endif