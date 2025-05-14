#include <stdio.h>
#include <unistd.h>

int
main(void)
{
  for (;;)
    {
      puts("Hello Universe from StarshipOS!");
      sleep(1);
    }
}
