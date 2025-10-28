#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <stdatomic.h>

//0x0 -> leer del dispositivo
//0x1 -> escribir al dispositivo
#define IO_OP_READ 0x0
#define IO_OP_WRITE 0x1

#define GPU_DATA_ADDR 0xFFF0
#define GPU_STATUS_ADDR 0xFFF1
#define TECLADO_DATA_ADDR 0xFFF2
#define TECLADO_STATUS_ADDR 0xFFF3

#define ROM_FILE "rom.bin"

#define INHIBIR_BUS 0xFFFF

#define MEMORY_DATA_BARRIER 0x200

//#define step_by_step //Uncomment to press enter to advance clock
#define VELOCIDAD_RELOJ_US 500 // 0.0005 segundos

typedef atomic_int * bus;
#define LEER_BUS(b) atomic_load_explicit((b), memory_order_acquire)
#define ESCRIBIR_BUS(b, c) atomic_store_explicit((b), (c), memory_order_release)

struct io_channel {
    bus direcciones;
    bus datos;
    bus control;
};

//Por comodidad, el reloj es memoria global
static atomic_int reloj_val = ATOMIC_VAR_INIT(0);
bus reloj = &reloj_val;

#define CLOCK_SYNC() \
    do { \
        int flanco = LEER_BUS(reloj); \
        while (LEER_BUS(reloj) == flanco){} \
    } while (0)
   
void error(const char * mensaje) {
    printf("Error: %s\n", mensaje);
    exit(1);
}

void * clk(void * arg) {
    (void)arg;
    while (1) {
        usleep(VELOCIDAD_RELOJ_US); //Un ciclo de reloj cada segundo
    ESCRIBIR_BUS(reloj, !LEER_BUS(reloj));
    }

    return NULL;
}

// Shared memory ring buffers for terminal I/O
#define SHM_NAME "/asoc_shm"
#define IO_BUF_SIZE 4096
struct shared_io {
    volatile unsigned int vth_head, vth_tail; // vm -> host (GPU output)
    volatile unsigned int htv_head, htv_tail; // host -> vm (keyboard input)
    char vth_buf[IO_BUF_SIZE];
    char htv_buf[IO_BUF_SIZE];
};

static struct shared_io *g_shm = NULL;

void memory_protection_emulation(int addr) {
    if (addr < MEMORY_DATA_BARRIER) {
        error("Escritura sobre memoria protegida de solo lectura");
    }
}

void * gpu(void * arg) {
    struct io_channel * io = (struct io_channel *) arg;
    (void)io;

    while (1) {
        CLOCK_SYNC();

        if (LEER_BUS(io->direcciones) == GPU_STATUS_ADDR && LEER_BUS(io->control) == IO_OP_READ) {
            printf("[DEV][GPU] Leyendo del registro de estado de la GPU\n");
            ESCRIBIR_BUS(io->datos, (int)0x1); //We can always print to the GPU
            ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
        } else if (LEER_BUS(io->direcciones) == GPU_STATUS_ADDR && LEER_BUS(io->control) == IO_OP_WRITE) {
            printf("[DEV][GPU] Escritura ignorada en el registro de estado de la GPU\n");
            ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
        } else if (LEER_BUS(io->direcciones) == GPU_DATA_ADDR && LEER_BUS(io->control) == IO_OP_READ) {
            printf("[DEV][GPU] Leyendo del registro de datos de la GPU\n");
            ESCRIBIR_BUS(io->datos, (int)0x0); //No data available
            ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
        } else if (LEER_BUS(io->direcciones) == GPU_DATA_ADDR && LEER_BUS(io->control) == IO_OP_WRITE) {
            printf("[DEV][GPU] Escritura en el registro de datos de la GPU: 0x%04X\n", LEER_BUS(io->datos));
            // Escribir en buffer compartido VM->Host
            if (g_shm) {
                char c = (char)LEER_BUS(io->datos);
                unsigned int head = g_shm->vth_head;
                unsigned int next = (head + 1) % IO_BUF_SIZE;
                if (next != g_shm->vth_tail) {
                    g_shm->vth_buf[head] = c;
                    g_shm->vth_head = next;
                } else {
                    // buffer lleno: descartar carácter
                }
            }
            ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
        }
    }
    
    return NULL;
}


void * teclado(void * arg) {
    struct io_channel * io = (struct io_channel *) arg;

    char c;
    while (1) {
        CLOCK_SYNC();
        if (LEER_BUS(io->direcciones) == TECLADO_DATA_ADDR && LEER_BUS(io->control) == IO_OP_WRITE) {
            printf("[DEV][KBD] Escritura ignorada en el registro de datos del teclado\n");
            ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
        } else if (LEER_BUS(io->direcciones) == TECLADO_DATA_ADDR && LEER_BUS(io->control) == IO_OP_READ) {
            printf("[DEV][KBD] Leyendo del registro de datos del teclado\n");
            // Leer de buffer compartido Host->VM
            if (g_shm && g_shm->htv_head != g_shm->htv_tail) {
                unsigned int tail = g_shm->htv_tail;
                c = g_shm->htv_buf[tail];
                g_shm->htv_tail = (tail + 1) % IO_BUF_SIZE;
                printf("[DEV][KBD] Carácter leído del teclado: '%c'\n", c);
                ESCRIBIR_BUS(io->datos, (int)c);
            } else {
                printf("[DEV][KBD] No hay datos disponibles en el teclado\n");
                ESCRIBIR_BUS(io->datos, (int)0); //No data available
            }
            ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
        } else if (LEER_BUS(io->direcciones) == TECLADO_STATUS_ADDR && LEER_BUS(io->control) == IO_OP_WRITE) {
            printf("[DEV][KBD] Escritura ignorada en el registro de estado del teclado\n");
            ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
        } else if (LEER_BUS(io->direcciones) == TECLADO_STATUS_ADDR && LEER_BUS(io->control) == IO_OP_READ) {
            printf("[DEV][KBD] Leyendo del registro de estado del teclado\n");
            int bytes_available = 0;
            if (g_shm) {
                unsigned int head = g_shm->htv_head;
                unsigned int tail = g_shm->htv_tail;
                bytes_available = (int)((head + IO_BUF_SIZE - tail) % IO_BUF_SIZE);
            }
            printf("[DEV][KBD] Bytes disponibles en el teclado: %d\n", bytes_available);
            ESCRIBIR_BUS(io->datos, (int)(bytes_available > 0 ? 0x1 : 0x0));
            ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
        }

    }

    return NULL;
}

static atomic_int guard = ATOMIC_VAR_INIT(0);
void * memoria(void * arg) {
    struct io_channel * io = (struct io_channel *) arg;
    static int memoria[0x10000] = {0}; // 64KB de memoria

    //Load rom
    FILE * rom_file = fopen(ROM_FILE, "rb");
    if (rom_file != NULL) {
        if (fread(memoria, sizeof(int), 0x10000, rom_file) != 0x10000) {
            printf("Advertencia: No se pudo leer toda la ROM. La memoria se inicializa parcialmente.\n");
        }
        fclose(rom_file);
    } else {
        printf("Advertencia: No se pudo abrir el archivo ROM. La memoria se inicializa en cero.\n");
    }

    while (1) {
        CLOCK_SYNC();

        int direccion = LEER_BUS(io->direcciones);
        //Address in modulus of memory size
        direccion = direccion % 0x10000;

        //Ignoramos las direcciones de IO (esto se hace físicamente con puertas lógicas)
        if (direccion != GPU_DATA_ADDR && direccion != GPU_STATUS_ADDR && direccion != TECLADO_DATA_ADDR && direccion != TECLADO_STATUS_ADDR && direccion != INHIBIR_BUS) {
            //printf(" [DEV] Responde la memoria: ADDR 0x%04X, CTRL 0x%01X, DAT 0x%04X\n",
            //    direccion,
            //    LEER_BUS(io->control),
            //    LEER_BUS(io->datos));
            
            // Simular lectura/escritura
            if (LEER_BUS(io->control) == 0) { // Lectura
                printf("[DEV][MEM] Leyendo de dirección 0x%04X: 0x%04X\n", direccion, memoria[direccion]);
                ESCRIBIR_BUS(io->datos, memoria[direccion]);
                atomic_store_explicit(&guard, memoria[direccion], memory_order_release);
                ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
            } else { // Escritura
                printf("[DEV][MEM] Escribiendo en dirección 0x%04X: 0x%04X\n", direccion, memoria[direccion]);
                memory_protection_emulation(direccion);
                memoria[direccion] = LEER_BUS(io->datos);
                ESCRIBIR_BUS(io->direcciones, INHIBIR_BUS); //Inhibir bus after operation
            }

        }

    }

    return NULL;
}

struct estado {
    int z : 1; // Zero flag
    int n : 1; // Negative flag
    int c : 1; // Carry flag
    int v : 1; // Overflow flag
};

struct cpu {
    volatile int pc; // Program counter
    volatile struct estado flags;
    volatile int registros[2]; // Registros generales
};

struct computador {
    struct cpu * procesador;
    struct io_channel * io;
};

const char *operaciones[] = {
    "ST", // Cargar de memoria a registro
    "LD", // Almacenar de registro a memoria
    "LDI", // Cargar inmediato a registro
    "ADD", // Sumar
    "SUB", // Restar
    "MUL", // Multiplicar
    "DIV", // Dividir
    "MOD",  // Módulo
    "AND", // AND bit a bit
    "OR",  // OR bit a bit
    "XOR", // XOR bit a bit
    "NOT", // NOT bit a bit
    "JMP", // Salto incondicional
    "JZ",  // Salto si cero
    "JN",  // Salto si negativo
    "CLR", // Limpiar registro
    "NOP", // No operación
    "DEC", // Decrementar
    "INC", // Incrementar
    "HALT" // Detener ejecución
};

const char *registros[] = {
    "X",
    "ACC"
};

const char *modos_direccionamiento[] = {
    "Inmediato",
    "Directo",
    "Indirecto",
    "Indexado"
};

#define ALU_MODE_ARITHMETHIC 0
#define ALU_OP_ADD 0
#define ALU_OP_SUB 1
#define ALU_OP_MUL 2
#define ALU_OP_DIV 3
#define ALU_OP_MOD 4
#define ALU_MODE_LOGIC 1
#define ALU_OP_AND 0
#define ALU_OP_OR 1
#define ALU_OP_XOR 2
#define ALU_OP_NOT 3
int alu_operation(struct cpu * cpu_inst, int mode, int opcode, int operand_1, int operand_2) {
    //Check if the number is signed 16-bit
    if (operand_1 > 32767 || operand_1 < -32768 || operand_2 > 32767 || operand_2 < -32768) {
        printf("Operadores: %d, %d\n", operand_1, operand_2);
        error("Operando fuera de rango de 16 bits con signo");
    }
    int has_signed = (operand_1 < 0 || operand_2 < 0);
    if (has_signed) printf("[ALU] Operación con números con signo\n");
    //Make the operation and update the cpu flags accordingly
    if (mode == ALU_MODE_ARITHMETHIC) {
        printf("[ALU] Operación aritmética\n");
        switch (opcode) {
            case ALU_OP_ADD:
                {
                    printf("[ALU] Sumar %d + %d\n", operand_1, operand_2);
                    int result = operand_1 + operand_2;
                    cpu_inst->flags.c = (result > 32767 || result < -32768);
                    cpu_inst->flags.v = ((operand_1 > 0 && operand_2 > 0 && result < 0) || (operand_1 < 0 && operand_2 < 0 && result > 0));
                    cpu_inst->flags.z = (result == 0);
                    cpu_inst->flags.n = (result < 0);
                    return result & 0xFFFF; //Return as 16-bit value
                }
            case ALU_OP_SUB:
                {
                    printf("[ALU] Restar %d - %d\n", operand_1, operand_2);
                    int result = operand_1 - operand_2;
                    cpu_inst->flags.c = (result > 32767 || result < -32768);
                    cpu_inst->flags.v = ((operand_1 > 0 && operand_2 < 0 && result < 0) || (operand_1 < 0 && operand_2 > 0 && result > 0));
                    cpu_inst->flags.z = (result == 0);
                    cpu_inst->flags.n = (result < 0);
                    return result & 0xFFFF; //Return as 16-bit value
                }
            case ALU_OP_MUL:
                {
                    printf("[ALU] Multiplicar %d * %d\n", operand_1, operand_2);
                    int result = operand_1 * operand_2;
                    cpu_inst->flags.c = (result > 32767 || result < -32768);
                    cpu_inst->flags.v = cpu_inst->flags.c; //Overflow if carry
                    cpu_inst->flags.z = (result == 0);
                    cpu_inst->flags.n = (result < 0);
                    return result & 0xFFFF; //Return as 16-bit value
                }
            case ALU_OP_DIV:
                {
                    printf("[ALU] Dividir %d / %d\n", operand_1, operand_2);
                    if (operand_2 == 0) {
                        error("División por cero");
                    }
                    int result = operand_1 / operand_2;
                    cpu_inst->flags.c = 0; //No carry in division
                    cpu_inst->flags.v = 0; //No overflow in division
                    cpu_inst->flags.z = (result == 0);
                    cpu_inst->flags.n = (result < 0);
                    return result & 0xFFFF; //Return as 16-bit value
                }
        }
    } else if (mode == ALU_MODE_LOGIC) {
        printf("[ALU] Operación lógica\n");
        switch (opcode) {
            case ALU_OP_AND:
                {
                    printf("[ALU] Y %d & %d\n", operand_1, operand_2);
                    int result = operand_1 & operand_2;
                    cpu_inst->flags.z = (result == 0);
                    cpu_inst->flags.n = (result < 0);
                    return result & 0xFFFF; //Return as 16-bit value
                }
            case ALU_OP_OR:
                {
                    printf("[ALU] O %d | %d\n", operand_1, operand_2);
                    int result = operand_1 | operand_2;
                    cpu_inst->flags.z = (result == 0);
                    cpu_inst->flags.n = (result < 0);
                    return result & 0xFFFF; //Return as 16-bit value
                }
            case ALU_OP_XOR:
                {
                    printf("[ALU] XOR %d ^ %d\n", operand_1, operand_2);
                    int result = operand_1 ^ operand_2;
                    cpu_inst->flags.z = (result == 0);
                    cpu_inst->flags.n = (result < 0);
                    return result & 0xFFFF; //Return as 16-bit value
                }
            case ALU_OP_NOT:
                {
                    printf("[ALU] NOT ~%d\n", operand_1);
                    int result = ~operand_1;
                    cpu_inst->flags.z = (result == 0);
                    cpu_inst->flags.n = (result < 0);
                    return result & 0xFFFF; //Return as 16-bit value
                }
        }
    } else {
        error("Modo de ALU inválido");
    }

    return 0;

}

void unidad_de_control(struct computador * comp) {
    //Print CPU state
    printf("PC: 0x%04X X:%04X ACC: %04X Z=%x N=%x C=%x V=%x\n", comp->procesador->pc, comp->procesador->registros[0], comp->procesador->registros[1], comp->procesador->flags.z, comp->procesador->flags.n, comp->procesador->flags.c, comp->procesador->flags.v);
    ESCRIBIR_BUS(comp->io->direcciones, INHIBIR_BUS); //Inhibir bus at the start of the cycle
    CLOCK_SYNC();
    int direccion_instr = comp->procesador->pc;
    ESCRIBIR_BUS(comp->io->control, IO_OP_READ);
    ESCRIBIR_BUS(comp->io->direcciones, direccion_instr);
    comp->procesador->pc += 1;
    CLOCK_SYNC();
    CLOCK_SYNC();
    int instr = LEER_BUS(comp->io->datos);
    if (instr != atomic_load_explicit(&guard, memory_order_acquire)) {
        printf("[DEBUG] Valor guardado en memoria: 0x%04X\n", guard);
        printf("Difierente de instrucción leída: 0x%04X\n", instr);
        exit(1);
    }
    if (comp->procesador->pc - 1 == 0x0 && instr == 0x0) {
        printf("Ejecución detenida por instrucción nula en dirección 0x0000.\n");
        exit(1);
    }
    printf("[IF] Instrucción leída: 0x%08X\n", instr);

    //Decodificación de la instrucción
    //Formato de instrucción (32 bits):
    // opcode (8 bits) | registro (4 bits) | direccionamiento (4 bits) | operando (16 bits)
    int opcode = (instr >> 24) & 0xFF;
    int reg = (instr >> 20) & 0x0F;
    int addr_mode = (instr >> 16) & 0x0F;
    int operando = instr & 0xFFFF;

    //Aplicamos direccionamiento para obtener la dirección efectiva o valor efectivo
    int direccion_efectiva = 0;
    int valor_efectivo = 0;
    switch (addr_mode) {
        case 0: // Inmediato
            valor_efectivo = operando;
            break;
        case 1: // Directo
            direccion_efectiva = operando;
            ESCRIBIR_BUS(comp->io->direcciones, direccion_efectiva);
            ESCRIBIR_BUS(comp->io->control, IO_OP_READ);
            CLOCK_SYNC();
            CLOCK_SYNC();
            valor_efectivo = LEER_BUS(comp->io->datos);
            break;
        case 2: // Indirecto
            ESCRIBIR_BUS(comp->io->direcciones, operando);
            ESCRIBIR_BUS(comp->io->control, IO_OP_READ);
            CLOCK_SYNC();
            CLOCK_SYNC();
            direccion_efectiva = LEER_BUS(comp->io->datos);
            ESCRIBIR_BUS(comp->io->direcciones, direccion_efectiva);
            ESCRIBIR_BUS(comp->io->control, IO_OP_READ);
            CLOCK_SYNC();
            CLOCK_SYNC();
            valor_efectivo = LEER_BUS(comp->io->datos);
            break;
        case 3: // Indexado (usamos siempre el registro X para este ejemplo)
            direccion_efectiva = operando + comp->procesador->registros[0];
            ESCRIBIR_BUS(comp->io->direcciones, direccion_efectiva);
            ESCRIBIR_BUS(comp->io->control, IO_OP_READ);
            CLOCK_SYNC();
            CLOCK_SYNC();
            valor_efectivo = LEER_BUS(comp->io->datos);
            break;
        default:
            error("Modo de direccionamiento inválido");
            break;
    }
    printf("[ID] Instrucción: %s %s, %s 0x%04X => DE: 0x%04X VE: 0x%04X\n", operaciones[opcode], registros[reg], modos_direccionamiento[addr_mode], operando, direccion_efectiva, valor_efectivo);
    
    //Ejecutamos la instrucción
    //Importante tener en cuenta cómo se alteran las flags del procesador
    switch (opcode) {
        case 0: // ST
            printf("[EX] Ejecutando ST\n");
            //Cuidado con el orden en las escrituras: ¿Que pasa si reordeno?
            ESCRIBIR_BUS(comp->io->control, IO_OP_WRITE);
            ESCRIBIR_BUS(comp->io->datos, comp->procesador->registros[reg]);
            ESCRIBIR_BUS(comp->io->direcciones, direccion_efectiva);
            CLOCK_SYNC();
            CLOCK_SYNC();
            printf("[EX] Registro %s almacenado en memoria en dirección 0x%04X con valor 0x%04X\n", registros[reg], direccion_efectiva, comp->procesador->registros[reg]);
            break;
        case 1: // LD
            printf("[EX] Ejecutando LD\n");
            comp->procesador->registros[reg] = valor_efectivo;
            printf("[EX] Registro %s cargado con valor 0x%04X\n", registros[reg], comp->procesador->registros[reg]);
            comp->procesador->flags.z = (comp->procesador->registros[reg] == 0);
            comp->procesador->flags.n = (comp->procesador->registros[reg] < 0);
            break;
        case 2: // LDI es igual a LD ya que precomputamos el valor efectivo
            printf("[EX] Ejecutando LDI\n");
            comp->procesador->registros[reg] = valor_efectivo;
            comp->procesador->flags.z = (comp->procesador->registros[reg] == 0);
            comp->procesador->flags.n = (comp->procesador->registros[reg] < 0);
            printf("[EX] Registro %s cargado con valor inmediato 0x%04X\n", registros[reg], comp->procesador->registros[reg]);
            break;
        case 3: // ADD
            printf("[EX] Ejecutando ADD\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_ARITHMETHIC, ALU_OP_ADD, comp->procesador->registros[reg], valor_efectivo);
            break;
        case 4: // SUB
            printf("[EX] Ejecutando SUB\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_ARITHMETHIC, ALU_OP_SUB, comp->procesador->registros[reg], valor_efectivo);
            break;
        case 5: // MUL
            printf("[EX] Ejecutando MUL\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_ARITHMETHIC, ALU_OP_MUL, comp->procesador->registros[reg], valor_efectivo);
            break;
        case 6: // DIV
            printf("[EX] Ejecutando DIV\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_ARITHMETHIC, ALU_OP_DIV, comp->procesador->registros[reg], valor_efectivo);
            break;
        case 7: // MOD
            printf("[EX] Ejecutando MOD\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_ARITHMETHIC, ALU_OP_MOD, comp->procesador->registros[reg], valor_efectivo);
            break;
        case 8: // AND
            printf("[EX] Ejecutando AND\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_LOGIC, ALU_OP_AND, comp->procesador->registros[reg], valor_efectivo);
            break;
        case 9: // OR
            printf("[EX] Ejecutando OR\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_LOGIC, ALU_OP_OR, comp->procesador->registros[reg], valor_efectivo);
            break;
        case 10: // XOR
            printf("[EX] Ejecutando XOR\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_LOGIC, ALU_OP_XOR, comp->procesador->registros[reg], valor_efectivo);
            break;
        case 11: // NOT
            printf("[EX] Ejecutando NOT\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_LOGIC, ALU_OP_NOT, comp->procesador->registros[reg], 0);
            break;
        case 12: // JMP
            printf("[EX] Ejecutando JMP\n");
            comp->procesador->pc = direccion_efectiva;
            break;
        case 13: // JZ
            printf("[EX] Ejecutando JZ\n");
            if (comp->procesador->flags.z) {
                comp->procesador->pc = direccion_efectiva;
            }
            break;
        case 14: // JN
            printf("[EX] Ejecutando JN\n");
            if (comp->procesador->flags.n) {
                comp->procesador->pc = direccion_efectiva;
            }
            break;
        case 15: // CLR
            printf("[EX] Ejecutando CLR\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_ARITHMETHIC, ALU_OP_SUB, comp->procesador->registros[reg], comp->procesador->registros[reg]);
            break;
        case 16: // NOP
            printf("[EX] Ejecutando NOP\n");
            //No hacer nada
            break;
        case 17: // DEC
            printf("[EX] Ejecutando DEC\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_ARITHMETHIC, ALU_OP_SUB, comp->procesador->registros[reg], 1);
            break;
        case 18: // INC
            printf("[EX] Ejecutando INC\n");
            comp->procesador->registros[reg] = alu_operation(comp->procesador, ALU_MODE_ARITHMETHIC, ALU_OP_ADD, comp->procesador->registros[reg], 1);
            break;
        case 19: // HALT
            printf("[EX] Ejecutando HALT\n");
            printf("Ejecución detenida por instrucción HALT.\n");
            exit(0);
            break;
        default:
            error("Código de operación inválido");
            break;
    }

    CLOCK_SYNC();
    CLOCK_SYNC();

#ifdef step_by_step
    getchar();
#endif
    // En algunas arquitecturas aquí van los pasos de memoria y write-back, nosotros ya los hicimos en la ejecución directamente
}

int main() {
    // Crear hilos para reloj, GPU, teclado, memoria
    pthread_t clock_thread, gpu_thread, teclado_thread, memoria_thread;

    struct io_channel io_channel;
    atomic_int direccion_bus = ATOMIC_VAR_INIT(0);
    atomic_int datos_bus = ATOMIC_VAR_INIT(0);
    atomic_int control_bus = ATOMIC_VAR_INIT(0);

    io_channel.direcciones = &direccion_bus;
    io_channel.datos = &datos_bus;
    io_channel.control = &control_bus;

    // Configurar memoria compartida para IO con el terminal
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(shm_fd, sizeof(struct shared_io)) == -1) {
        perror("ftruncate");
        exit(1);
    }
    g_shm = (struct shared_io *)mmap(NULL, sizeof(struct shared_io), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (g_shm == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    close(shm_fd);
    // Inicializar colas
    memset((void*)g_shm, 0, sizeof(*g_shm));

    pthread_create(&clock_thread, NULL, clk, NULL);
    pthread_create(&gpu_thread, NULL, gpu, (void *)&io_channel);
    pthread_create(&teclado_thread, NULL, teclado, (void *)&io_channel);
    pthread_create(&memoria_thread, NULL, memoria, (void *)&io_channel);

    // Crear computador y unidad de control
    struct computador comp;
    struct cpu cpu_inst;
    comp.procesador = &cpu_inst;
    comp.io = &io_channel;

    // Inicializar CPU y buses
    cpu_inst.pc = 0;
    cpu_inst.registros[0] = 0; // X
    cpu_inst.registros[1] = 0; // ACC
    cpu_inst.flags.z = 0;
    cpu_inst.flags.n = 0;
    cpu_inst.flags.c = 0;
    cpu_inst.flags.v = 0;

    ESCRIBIR_BUS(io_channel.datos, 0);
    ESCRIBIR_BUS(io_channel.control, 0);

    // Ciclo principal de la unidad de control
    while (1) {
        unidad_de_control(&comp);
        // Aquí se implementaría la decodificación y ejecución de la instrucción
        // Por simplicidad, solo incrementamos el PC
    }

    // Unir hilos (nunca se alcanza en este ejemplo)
    pthread_join(clock_thread, NULL);
    pthread_join(gpu_thread, NULL);
    pthread_join(teclado_thread, NULL);
    pthread_join(memoria_thread, NULL);

    return 0;
}