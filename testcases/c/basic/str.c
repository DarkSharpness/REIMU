void test_strcmp() {
    char str[2][10];
    scanf("%s%s", str[0], str[1]);
    int result = strcmp(str[0], str[1]);
    char op = result < 0 ? '<' : result > 0 ? '>' : '=';
    printf("strcmp %s %c %s\n", str[0], op, str[1]);
}

void test_strcpy() {
    char str[2][20];
    scanf("%s", str[0]);
    strcpy(str[1], str[0]);
    printf("strcpy %s -> %s\n", str[0], str[1]);
}

void test_strcat() {
    char str[2][20];
    scanf("%s%s", str[0], str[1]);
    char buf[40];
    strcpy(buf, str[0]);
    strcat(buf, str[1]);
    printf("strcat %s + %s = %s\n", str[0], str[1], buf);
}

void test_strlen() {
    char str[20];
    scanf("%s", str);
    printf("strlen %s = %d\n", str, strlen(str));
}

int main() {
    for (int i = 0; i < 3; i++) test_strcmp();
    for (int i = 0; i < 3; i++) test_strcpy();
    for (int i = 0; i < 3; i++) test_strcat();
    for (int i = 0; i < 3; i++) test_strlen();
}
