
#include <stdio.h>
#include <stdlib.h>

struct transicion {
    //Condiciones desencadenantes
    char estado_actual;
    char simbolo_leido;

    //Acciones a realizar
    char estado_siguiente;
    char simbolo_escribir;
    char direccion_avance; // 'L' para izquierda, 'R' para derecha
};

struct turing {
    char * cinta;
    int cinta_tamano;
    char *alfabeto;
    int alfabeto_tamano;
    char *estados;
    int estados_tamano;
    char *estado_inicial;
    char *estados_aceptacion;
    int estados_aceptacion_tamano;
    int * estado_actual;
    char * posicion_cabezal;

    struct transicion *transiciones;
    int num_transiciones;
};

struct turing * crear_maquina_turing(
    const char * cinta_inicial,
    const int cinta_tamano,

    const char * alfabeto,
    const int alfabeto_tamano,

    const char * estados,
    const int estados_tamano,

    const char estado_inicial,
    
    const char * estados_aceptacion,
    const int estados_aceptacion_tamano,

    struct transicion * transiciones,
    const int num_transiciones,

    const int posicion_cabezal_inicial
) {
    struct turing * maquina = malloc(sizeof(struct turing));
    maquina->cinta = malloc(cinta_tamano * sizeof(char));
    for (int i = 0; i < cinta_tamano; i++) {
        maquina->cinta[i] = cinta_inicial[i];
    }
    maquina->cinta_tamano = cinta_tamano;

    maquina->alfabeto = malloc(alfabeto_tamano * sizeof(char));
    for (int i = 0; i < alfabeto_tamano; i++) {
        maquina->alfabeto[i] = alfabeto[i];
    }
    maquina->alfabeto_tamano = alfabeto_tamano;

    maquina->estados = malloc(estados_tamano * sizeof(char));
    for (int i = 0; i < estados_tamano; i++) {
        maquina->estados[i] = estados[i];
    }
    maquina->estados_tamano = estados_tamano;

    maquina->estado_inicial = malloc(sizeof(char));
    *(maquina->estado_inicial) = estado_inicial;

    maquina->estados_aceptacion = malloc(estados_aceptacion_tamano * sizeof(char));
    for (int i = 0; i < estados_aceptacion_tamano; i++) {
        maquina->estados_aceptacion[i] = estados_aceptacion[i];
    }
    maquina->estados_aceptacion_tamano = estados_aceptacion_tamano;

    maquina->estado_actual = malloc(sizeof(char));
    *(maquina->estado_actual) = estado_inicial;

    maquina->posicion_cabezal = &(maquina->cinta[posicion_cabezal_inicial]);

    maquina->transiciones = transiciones;
    maquina->num_transiciones = num_transiciones;

    return maquina;
}

void error(const char * mensaje) {
    printf("Error: %s\n", mensaje);
    exit(1);
}

void procesar(struct turing* m) {
    // Mientras el estado actual no sea de aceptación
    int aceptacion = 0;
    while (!aceptacion) {
        //Imprime la cinta
        printf("Cinta: ");
        for (int i = 0; i < m->cinta_tamano; i++) {
            if (&(m->cinta[i]) == m->posicion_cabezal) {
                printf("[%c] ", m->cinta[i]);
            } else {
                printf(" %c  ", m->cinta[i]);
            }
        }
        printf("\n");
        printf("Estado actual: '%c', Símbolo bajo el cabezal: '%c' (enter para ejecutar)\n", *(m->estado_actual), *(m->posicion_cabezal));
        getchar();
        // Buscar la transición correspondiente
        int encontrada = 0;
        for (int i = 0; i < m->num_transiciones; i++) {
            struct transicion t = m->transiciones[i];
            if (t.estado_actual == *(m->estado_actual) && t.simbolo_leido == *(m->posicion_cabezal)) {
                printf("Aplicando transición: (Estado actual: '%c', Símbolo leído: '%c') -> (Escribir: '%c', Mover: '%c', Nuevo estado: '%c')\n",
                    t.estado_actual, t.simbolo_leido,
                    t.simbolo_escribir, t.direccion_avance, t.estado_siguiente);
                // Realizar la acción
                *(m->posicion_cabezal) = t.simbolo_escribir;
                *(m->estado_actual) = t.estado_siguiente;
                if (t.direccion_avance == 'R') {
                    m->posicion_cabezal++;
                } else if (t.direccion_avance == 'L') {
                    m->posicion_cabezal--;
                }
                encontrada = 1;
                break;
            }
        }

        if (!encontrada) {
            printf("No se encontró una transición válida para el estado '%c' y el símbolo '%c'.\n", *(m->estado_actual), *(m->posicion_cabezal));
            error("La máquina se ha quedado sin transiciones aplicables.");
        }

        // Verificar si el estado actual es de aceptación
        for (int i = 0; i < m->estados_aceptacion_tamano; i++) {
            if (*(m->estado_actual) == m->estados_aceptacion[i]) {
                printf("La máquina ha alcanzado un estado de aceptación: '%c'\n", *(m->estado_actual));
                // Imprime la cinta final
                printf("Cinta final: ");
                for (int j = 0; j < m->cinta_tamano; j++) {
                    printf(" %c  ", m->cinta[j]);
                }
                printf("\n");
                aceptacion = 1;
                break;
            }
        }
    }
}


//Condiciones: El cabezal empieza en el primer dígito de un número binario de longitud finita

//Estados
// A: Moverse al final de la cadena
// B: Moverse hacia la izquierda hasta encontrar un 0, cambiarlo a 1
// C: Si solo hay 1s, cambiarlos a 0s y agregar un 1 al principio
// D: Estado de aceptación (Halt)

//Alfabeto: {0, 1, B} donde B es blanco

//Ejemplo:
// Entrada: 1101 (representado como ...BBB1101BBB...) ... significa infinito
// Salida:  1110 (representado como ...BBB1110BBB...)

// Entrada 111 (representado como ...BBB111BBB...)
// Salida:  1000 (representado como ...BBB1000BBB...)

int cinta_a_entero(const char * cinta, int tamano) {
    int resultado = 0;
    for (int i = 0; i < tamano; i++) {
        if (cinta[i] != '0' && cinta[i] != '1')
            return resultado; // Detenerse si no es un dígito binario
        resultado = resultado * 2 + (cinta[i] - '0');
    }
    return resultado;
}

int main() {
    // Definir la cinta inicial
    const char cinta_inicial[] = {'_', '_', '_', '1', '0', '1', '0', '_', '_', '_'};
    const int cinta_tamano = sizeof(cinta_inicial) / sizeof(cinta_inicial[0]);

    // Definir el alfabeto
    const char alfabeto[] = {'0', '1', '_'};
    const int alfabeto_tamano = sizeof(alfabeto) / sizeof(alfabeto[0]);

    // Definir los estados
    const char estados[] = {'A', 'B', 'C', 'D'};
    const int estados_tamano = sizeof(estados) / sizeof(estados[0]);

    // Estado inicial
    const char estado_inicial = 'A';

    // Estados de aceptación
    const char estados_aceptacion[] = {'D'};
    const int estados_aceptacion_tamano = sizeof(estados_aceptacion) / sizeof(estados_aceptacion[0]);

    // Definir las transiciones
    struct transicion transiciones[] = {
        //Al principio busco el final del número (simbolo vacio)
        {'A', '0', 'A', '0', 'R'},
        {'A', '1', 'A', '1', 'R'},
        {'A', '_', 'B', '_', 'L'},

        //Ahora busco el primer 0 para cambiarlo a 1
        {'B', '0', 'C', '1', 'L'},
        //Si lo que encuentro es un 1, lo cambio a 0 y sigo buscando
        {'B', '1', 'B', '0', 'L'},
        //Si encuentro un blanco, significa que no hay 0s, debo cambiar todos los 1s a 0s y agregar un 1 al principio
        {'B', '_', 'C', '1', 'R'},
        //Estado de aceptación
        {'C', '0', 'D', '0', 'R'},
        {'C', '1', 'D', '1', 'R'},
        {'C', '_', 'D', '_', 'R'},
        {'D', '0', 'D', '0', 'R'},
        {'D', '1', 'D', '1', 'R'},
        {'D', '_', 'D', '_', 'R'},
    };
    const int num_transiciones = sizeof(transiciones) / sizeof(transiciones[0]);
    const int posicion_cabezal_inicial = 3;
    // Crear la máquina de Turing
    struct turing* maquina = crear_maquina_turing(
        cinta_inicial,
        cinta_tamano,
        alfabeto,
        alfabeto_tamano,
        estados,
        estados_tamano,
        estado_inicial,
        estados_aceptacion,
        estados_aceptacion_tamano,
        transiciones,
        num_transiciones,
        posicion_cabezal_inicial
    );

    printf("Numero inicial en binario: %d\n", cinta_a_entero(&(maquina->cinta[posicion_cabezal_inicial]), 4));

    // Procesar la entrada
    procesar(maquina);

    printf("Numero final en binario: %d\n", cinta_a_entero(&(maquina->cinta[posicion_cabezal_inicial]), 5));

    // Liberar memoria
    free(maquina->cinta);
    free(maquina->alfabeto);
    free(maquina->estados);
    free(maquina->estado_inicial);
    free(maquina->estados_aceptacion);
    free(maquina->estado_actual);
    free(maquina);


    return 0;
}

