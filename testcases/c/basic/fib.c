int fib(int n) {
  if (n == 0 || n == 1)
    return 1;
  return fib(n - 1) + fib(n - 2);
}

int main() {
  int n;
  scanf("%d", &n);
  for (int i = 0; i <= n; i++)
      printf("fib(%d) = %d\n", i, fib(i));
  return 0;
}
