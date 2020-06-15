int sum(int a, int b)
{
  int sum = 0;
  goto cond;
loop:
  sum += a;
  a++;
cond:
  if (a <= b) {
    goto loop;
  }
  return sum;
}

int main(void)
{
  return sum(3, 5);
}
