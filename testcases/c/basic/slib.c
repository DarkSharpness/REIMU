int main() {
    int sum = 0;
    for (int i = 0; i < 100; i++)
        sum += i + (i & 1) - ((i + 1) & 1);
    char buf[16];
    sprintf(buf, "= %d", sum);
    printf("sum 0..99 %s\n", buf);
    return 0;
}
