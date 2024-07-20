int x[100];
int y[100];

int main() {
    for (int i = 0 ; i < 100 ; ++i) {
        x[i] = i + (i & 1);
        y[i] = i - (i & 1);
    }
    int sum = 0;
    for (int i = 0 ; i < 100 ; ++i)
        sum += x[i] + y[99 - i];

    return printf("0 = %d\n", sum - 4950 * 2);
}