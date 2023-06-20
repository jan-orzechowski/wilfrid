void allocate_memory(void)
{
    arena = allocate_memory_arena(megabytes(1));
    string_arena = allocate_memory_arena(megabytes(1));
    interns = xcalloc(sizeof(hashmap));
    map_grow(interns, 32);

    map_grow(&global_symbols, 32);
    map_chain_grow(&cached_pointer_types, 32);

    vm_global_memory = allocate_memory_arena(kilobytes(100));
    map_grow(&global_identifiers, 32);

    xprintf_buf_size = string_arena->block_size + 1;
    xprintf_buf = xmalloc(xprintf_buf_size);
}

void clear_memory(void)
{
    buf_free(all_tokens);

    free_memory_arena(string_arena);
    map_free(interns);
    free(interns);
    keywords_initialized = false;
    buf_free(keywords_list);

    free_memory_arena(arena);

    buf_free(global_symbols_list);
    buf_free(ordered_global_symbols);
    map_chain_free(&cached_pointer_types);
    mangle_clear();

    map_free(&global_symbols);

    memset(local_symbols, 0, MAX_LOCAL_SYMBOLS);
    last_local_symbol = local_symbols;

    installed_types_initialized = false;

    free_memory_arena(vm_global_memory);
    memset(vm_stack, 0, MAX_VM_STACK_SIZE);
    last_used_vm_stack_byte = vm_stack;

    ___clean_memory___();

    map_free(&global_identifiers);

    for (size_t i = 0; i < buf_len(enum_values_hashmaps); i++)
    {
        map_free(&enum_values_hashmaps[i]);
    }
    buf_free(enum_values_hashmaps);

    free(xprintf_buf);
    buf_free(errors);
    buf_free(ast_buf);
    buf_free(gen_buf);
}