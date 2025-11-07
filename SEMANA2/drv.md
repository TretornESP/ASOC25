# Device drivers

Que responder:

1. ¿Qué diferencias hay entre un driver y otra pieza de software?
2. ¿Qué tipos de mecanismos de IO existen?
3. ¿Qué estructuras de datos gestionan las interrupciones en x86? (IDT/IVT)
4. ¿Qué tipos de dispositivos existen? (Bloque, carácter, red, etc)
5. ¿Que operaciones básicas debe soportar un driver de dispositivo? (open, close, read, write, ioctl)
6. ¿Qué hace ioctl?
7. ¿Qué es un número mayor y un número menor en un driver de dispositivo?
8. ¿Cómo puede un driver manejar interrupciones?
9. ¿Qué es el polling y qué es el DMA?
10. ¿Qué es el PIC 8259 y qué es el APIC?
11. ¿Que son las MSI y las MSI-X? (Ventajas y desventajas)
12. ¿Cómo se trabaja con dispositivos PCI? Particularidades de los dispositivos PCI y PCIe con respecto al manejo de interrupciones (si no usamos msi/msix)

Que enseñar:

1. Ejemplo de arquitectura de un driver en un kernel, ¿Cómo lo implementaríais vosotros de cero? La idea es que los drivers se pueden
cargar al vuelo, sin recompilar el kernel. Tiene mucha chicha y no hay una única forma de hacerlo.
2. Ejemplo de driver serial
3. Ejemplo de manejo de interrupciones PCI (sin msi/msix)
4. Demo real, crear un rootkit en Linux (Que permita ejecutar una shell con privilegios de root)
5. Extra: Driver de disco o de red simple (si da tiempo)

Algunas Referencias, no son suficientes pero pueden ayudar:

https://github.com/Open-Driver-Interface/odi/blob/main/src/core/device.c
https://github.com/Open-Driver-Interface/odi/blob/main/src/drivers/com/serial/serial.c
https://github.com/Open-Driver-Interface/odi/blob/main/src/drivers/net/e1000/e1000.c
https://wiki.osdev.org/Interrupt_Descriptor_Table
https://wiki.osdev.org/Interrupts_Tutorial
https://f.osdev.org/viewtopic.php?t=37112
El tema de la pregunta 12. https://github.com/TretornESP/bloodmoon/blob/f0ba69b87e719a82f273c418dca1982c16666c2d/src/devices/pci/pci.c#L320 (pci comparte lineas de interrupcion para todos los dispositivos, con lo que tienes que checkear cual ha sido el que ha generado la interrupcion uno a uno!)