#Building the compiler frontend:

```cmake -S . -B build```

#Building an executable:

```./build/driver.out <path/to/codefile> -o <output> && clang <output> IO.c```

There are simple examples in `tests` directory.
