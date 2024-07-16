int add(int a, int b) {
    return a + b;
}

int sub(int a, int b) {
    return a - b;
}

int mul(int a, int b) {
    return a * b;
}

int div(int a, int b) {
    return a / b;
}

typedef int (*fptr)(int, int);

fptr run(int a, int b, int op) {
    if (op == '+') {
        return add;
    } else if (op == '-') {
        return sub;
    } else if (op == '*') {
        return mul;
    } else if (op == '/') {
        return div;
    }
}


int main() {
    int a, b;
    char op;

    scanf("%d %c %d", &a, &op, &b);

    printf("%d %c %d = %d\n", a, op, b, run(a, b, op)(a, b));

    return 0;
}
