# Espacio de usuario

Antes de nada, este punto puede ser un poco lios a primera vista. 
La idea es que habléis de cómo el sistema operativo gestiona las transiciones entre el espacio de usuario y el espacio kernel
y sus respectivas idiosincrasias.

Que responder:

1. ¿Qué es el espacio de usuario y el espacio kernel? ¿Que sentido tiene aislarlos?
2. ¿Qué son las syscalls? ¿Cómo se implementan en un sistema operativo típico? ¿Cuales son las más comunes?
3. ¿Cómo se realiza la transición entre espacio de usuario y espacio kernel?
Aclaración: No pregunto el cuando (syscall/sysret/interrupción/etc) si no el cómo (Que hace el hardware cuando se
produce un syscall o un sysret, que registros se usan, etc). Hablad de los registros fs_base, gs_base, etc. de los MSR, etc.
4. ¿Cómo se entra por primera vez en modo usuario desde el kernel?
5. ¿Qué es la multitarea cooperativa? ¿y la apropiativa?
6. ¿Cómo se implementa la protección de memoria entre procesos en espacio de usuario?
7. ¿Qué es un VDSO? ¿Para qué se utiliza?
8. Extra: ¿Qué pasa cuando un proceso hace una syscall que requiere esperar (por ejemplo, leer de disco)?

Que enseñar:

1. Seguid el flujo de ejecución de una syscall.
2. Seguid el proceso de entrada de un proceso en modo usuario por primera vez. (Asumimos que estamos arrancando la máquina)

Algunas Referencias, no son suficientes pero pueden ayudar:

https://wiki.osdev.org/Getting_to_Ring_3

Conceptualmente es muy fácil:
 - El truco es crear un stack en modo usuario y escribirlo a mano (dirección de retorno)
   para que parezca que tenemos que hacer un return a una función en modo usuario, esa función es nuestro entry.
   https://github.com/omen-osdev/omen/blob/bc66800c505021e7403f33098d574340ae273150/src/modules/managers/cpu/process.c#L263
   https://github.com/omen-osdev/omen/blob/bc66800c505021e7403f33098d574340ae273150/src/modules/hal/arch/x86/context.asm#L109
   La implementación real es un puto cristo
