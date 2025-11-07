# El bootloader

Recordad, la idea no es que esto se use en el proyecto final (vamos a usar limine porque si no no hay dios que lo haga arrancar)

Que responder:

1. ¿Qué es un bootloader? ¿Qué tipos de bootloaders existen?
1. ¿Qué es el MBR y qué es el GPT?
2. ¿Qué es una interrupción BIOS? ¿Qué servicios ofrece?
3. ¿Que secuencia sigue la máquina al arrancar desde cero hasta que el bootloader toma el control?
4. ¿Cómo detecta la bios que hay un bootloader válido? ¿Donde lo carga y que información le pasa?
5. ¿Que es un second stage bootloader y por qué se usa?
6. ¿Qué es el modo real y el modo protegido en x86? ¿Cómo se pasa de uno a otro?
7. ¿Qué estructuras de datos básicas gestiona la cpu? (GDT/LDT, IDT, tablas de páginas, etc)
8. ¿Cómo se carga un binario ELF en memoria? (Explicar el formato ELF y los pasos necesarios para cargarlo)

Que enseñar:

1. Un ejemplo de un bootloader simple que imprima algo en pantalla usando interrupciones BIOS.
2. Un ejemplo de un bootloader simple que cargue un segundo stage desde disco y sea capaz de leer del teclado y escribir en pantalla.
2. Un ejemplo de un bootloader que cargue un kernel ELF en memoria y le pase el control.
3. Un ejemplo de inicialización de la GDT y paso a modo protegido.

Algunas Referencias, no son suficientes pero pueden ayudar:

https://www.youtube.com/watch?v=HEZ-HvrRAI0
https://wiki.osdev.org/Protected_Mode
https://wiki.osdev.org/Global_Descriptor_Table
https://wiki.osdev.org/GDT_Tutorial
https://wiki.osdev.org/Interrupt_Descriptor_Table
https://wiki.osdev.org/Interrupts_Tutorial
https://wiki.osdev.org/8259_PIC