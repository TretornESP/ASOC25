# Memoria

Preguntas a responder:

1. ¿Que diferencia hay entre la memoria física y la memoria virtual?
2. ¿Que es la segmentación? ¿Qué es la segmentación plana?
3. ¿Qué es la paginación? ¿Que hay en una tabla de páginas? ¿Cómo se activa la paginación en x86?
4. Muestra el proceso de traducción de una dirección virtual a una dirección física usando paginación.
5. ¿Qué es un fallo de página (page fault) y cómo lo maneja el sistema operativo?
6. ¿Cómo funciona el SWAP? ¿Y el Copy on Write (COW)?

Que enseñar:

1. Un ejemplo de inicialización de memoria física (iteráis por las regiones de memoria haciendo un accounting sencillo).
y mostrais como reserváis y liberáis un par de bloques de memoria.
2. Un ejemplo de traducción de dirección virtual a física.
3. Un ejemplo en el que dos procesos se pisan en memoria virtual pero no en memoria física.
4. Un ejemplo de swapping.
5. Un ejemplo de Copy on Write (COW).

Algunas Referencias, no son suficientes pero pueden ayudar:

https://wiki.osdev.org/Paging
https://wiki.osdev.org/Exceptions#Page_Fault
https://wiki.osdev.org/Protected_Mode#Entering_Protected_Mode