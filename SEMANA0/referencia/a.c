int suma(int x, int * y) {
    printf("Y: %p\n", y);
    y += 2;
    return x + *y;
}

int main() {
    int a = 5;
    int b = 6;
    printf("A está en: %p\n", &a);
    int result = suma(a, b);


    printf("La suma es: %d\n", result);
    printf("B es: %d\n", b);

    return 0;
}