# Procesos

Que responder:

1. ¿Qué es un proceso? Diferencia entre proceso e hilo.
2. ¿Qué es el PCB (Process Control Block)? ¿Qué es el contexto de un proceso? ¿Qué información contienen ambos?
3. ¿Cómo crea el primer proceso un sistema operativo?
4. ¿Qué hace fork y qué hace exec?
5. ¿Cómo se realiza el cambio de contexto entre procesos? Explicar en máximo detalle para el caso de un único core x86.
6. ¿Qué problemas tiene la planificación round robin?
7. ¿Qué pasa cuando un proceso tiene que esperar con un recurso?
8. ¿Cómo se implementa la comunicación entre procesos (IPC)?
9. ¿Cómo procesa un proceso las señales (signals)?
10. ¿Qué es un deadlock? ¿Cómo se puede evitar o manejar?

Que enseñar:

1. Un ejemplo de creación de un proceso (muestra el contexto y el TCB).
2. Un ejemplo de cambio de contexto entre dos procesos.
3. Un ejemplo de comunicación entre procesos (IPC) usando señales.
4. Un ejemplo de deadlock y cómo se puede resolver.

Algunas Referencias, no son suficientes pero pueden ayudar:

https://github.com/TretornESP/bec/blob/main/s2/a.c (p1 y p2 son dos procesos haciendo cambio de contexto)
https://github.com/omen-osdev/omen/blob/bc66800c505021e7403f33098d574340ae273150/src/modules/managers/cpu/process.c#L1274C6-L1274C18 (ejemplo de inicializacion de procesos en un OS real, algo lioso)
https://wiki.osdev.org/Brendan%27s_Multi-tasking_Tutorial