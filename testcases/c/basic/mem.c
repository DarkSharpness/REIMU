
int main() {
    int *a = malloc(sizeof(int) * 100);
    for (int i = 0; i < 100; ++i)
        a[i] = i;
    int sum = 0;
    for (int i = 0; i < 100; ++i)
        sum += (++a[i]);
    printf("sum 1..100 = %d\n", sum);
    free(a);
    return 0;
}