#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "main.h"
#define MEM_SIZE 4096


void (*iset[])(uint8_t reg, uint16_t data) = {st, ld, add, br, bz, clr, dec};
char *iset_names[] = {"st", "ld", "add", "br", "bz", "clr", "dec"};
char *ext_names[] = {"halt", "ei", "di"};
void (*ext[])(uint8_t reg, uint16_t data) = {halt, ei, di};

uint16_t mem[MEM_SIZE];
uint16_t x;
uint16_t acc;
uint16_t pc;

struct status {
    uint8_t z : 1;
    uint8_t n : 1;
    uint8_t c : 1;
    uint8_t i : 1;
    uint8_t v : 1;
    uint8_t h : 1;
};

struct status status;

void st(uint8_t reg, uint16_t data) {
    if (reg) {
        mem[data] = acc;
    } else {
        mem[data] = x;
    }
}

void ld(uint8_t reg, uint16_t data) {
    if (reg) {
        acc = mem[data];
    } else {
        x = mem[data];
    }
}

void add(uint8_t reg, uint16_t data) {
    if (reg) {
        acc += mem[data];
    } else {
        x += mem[data];
    }
}

void br(uint8_t reg, uint16_t data) {
    pc = data;
    pc--;
}

void bz(uint8_t reg, uint16_t data) {
    if (status.z) {
        pc = data;
        pc--;
    }
}

void clr(uint8_t reg, uint16_t data) {
    if (reg) {
        acc = 0;
    } else {
        x = 0;
    }
}

void dec (uint8_t reg, uint16_t data) {
    if (reg) {
        acc--;
        if (acc == 0) {
            status.z = 1;
        }
    } else {
        x--;
        if (x == 0) {
            status.z = 1;
        }
    }
}

void halt(uint8_t reg, uint16_t data) {
    status.h = 1;
    pc--;
}

void ei(uint8_t reg, uint16_t data) {
    status.i = 1;
}

void di(uint8_t reg, uint16_t data) {
    status.i = 0;
}

void loop() {
    while (1) {
        status.z = 0;

        uint16_t inst = mem[pc];
        uint8_t op = inst >> 9;
        if (op == 7) 
            op = inst >> 7;
        uint8_t reg = (inst & 0x100) >> 8;
        uint8_t dirm = (inst & 0xC) >> 6;
        uint8_t cd = (inst & 0x3F);

        uint16_t ea = cd;
        if (dirm & 0x1) {
            uint16_t ea = mem[cd];
        }
        if (dirm & 0x2) {
            uint16_t ea = mem[ea + x];
        }

        printf("DEBUG: op: %x, reg: %x, dirm: %x, cd: %x, ea: %x, data: %d\n", op, reg, dirm, cd, ea, ea);
        if (op >= 7) {
            printf("Executing ext %s %x, %x\n", ext_names[op&0x3], reg, ea);
            ext[op&0x3](reg, ea);
        } else {
            printf("Executing %s %x, %x\n", iset_names[op], reg, ea );
            iset[op](reg, ea);
        }
        pc++;

        printf("PC:%x X:%x ACC:%x\n", pc, x, acc);
        printf("STATUS: [Z:%x N:%x C:%x I:%x V:%x H:%x]\n", status.z, status.n, status.c, status.i, status.v, status.h);
        for (int i = 0; i < 30; i++) {
            printf("%x ", mem[i]);
            if (i % 10 == 9) {
                printf("\n");
            }
        }
        getchar();

    }
}

int main() {
    uint16_t example_program[] = {
                  //OOORJICDCDCD
        0x24A,    //0010 0100 1010,
        0x44B,    //0100 0100 1011,
        0x04C,    //0000 0100 1100,
        0xE00,    //1110 0000 0000,
        0x000,
        0x000,
        0x000,
        0x000,
        0x000,
        0x000,
        0x005,
        0x006
    };

    for (int i = 0; i < (sizeof(example_program)/sizeof(uint16_t)); i++) {
        mem[i] = example_program[i];
    }

    loop();
}
