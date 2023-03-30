void map_test(void)
{
    hashmap map = { 0 };
    enum { N = 2048 };

    for (size_t i = 1; i < N; i++)
    {
        map_put(&map, (void *)i, (void *)(i + 1));
    }

    for (size_t i = 1; i < N; i++)
    {
        void *val = map_get(&map, (void *)i);
        assert(val == (void *)(i + 1));
    }

    debug_breakpoint;
}

void intern_str_test(void)
{
    char x[] = "hello";
    char y[] = "hello";

    assert(x != y);

    const char *px = str_intern(x);
    const char *py = str_intern(y);
    assert(px == py);

    char z[] = "hello!";
    const char *pz = str_intern(z);
    assert(pz != px);
}

void buf_remove_at_test(void)
{
    int *integers = null;
    assert(buf_len(integers) == 0);
    buf_push(integers, 0);
    buf_push(integers, 1);
    buf_push(integers, 2);
    assert(buf_len(integers) == 3);
    buf_remove_at(integers, 1);
    assert(buf_len(integers) == 2);
    assert(integers[2] == 0);
    assert(integers[1] == 2);
    buf_remove_at(integers, 1);
    assert(buf_len(integers) == 1);
    assert(integers[1] == 0);
    buf_remove_at(integers, 0);
    assert(buf_len(integers) == 0);
    buf_remove_at(integers, 0);
}

void stretchy_buffers_test(void)
{
    intern_str *str = null;

    assert(buf_len(str) == 0);

    for (size_t index = 0; index < 1024; index++)
    {
        buf_push(str, ((intern_str){ index, null }));
    }

    assert(buf_len(str) == 1024);

    for (size_t index = 0; index < buf_len(str); index++)
    {
        assert(str[index].len == index);
    }

    buf_free(str);

    assert(buf_len(str) == 0);

    char *char_buf = null;
    buf_printf(char_buf, "One: %d\n", 1);
    assert(strcmp(char_buf, "One: 1\n") == 0);
    buf_printf(char_buf, "Hex: 0x%x\n", 0x12345678);
    assert(strcmp(char_buf, "One: 1\nHex: 0x12345678\n") == 0);

    buf_remove_at_test();
}

void buf_copy_test(void)
{
    memory_arena *test = allocate_memory_arena(kilobytes(1));

    int *buffer = null;
    for (int i = 0; i < 128; i++)
    {
        buf_push(buffer, i);
    }

    int *new_array = (int *)copy_buf_to_arena(test, buffer);

    for (int i = 0; i < 128; i++)
    {
        if (new_array[i] != i)
        {
            debug_breakpoint;
        }
    }

    free(test);
    buf_free(buffer);
}