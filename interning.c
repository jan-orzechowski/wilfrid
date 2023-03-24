typedef struct intern_str intern_str;
struct intern_str
{
    size_t len;
    intern_str *next;
    char str[];
};

hashmap *interns;
memory_arena *string_arena;

const char *str_intern_range(const char *start, const char *end)
{
    size_t len = end - start;

    uint64_t hash = hash_bytes(start, len);
    // wartość 0 jest zarezerwowana jako sentinel - na niej przerwiemy wyszukiwanie
    void *key = (void *)(uintptr_t)(hash ? hash : 1);

    intern_str *intern = map_get(interns, key);
    for (intern_str *it = intern; it; it = it->next)
    {
        // musimy wcześniej sprawdzić długość, by uniknąć sytuacji, w której 
        // tylko prefix się zgadza - strncmp nie sprawdza długości
        if (it->len == len && strncmp(it->str, start, len) == 0)
        {
            return it->str;
        }
    }

    // jeśli nie znaleźliśmy
    intern_str *new_intern = push_size(string_arena, offsetof(intern_str, str) + len + 1);
    new_intern->len = len;
    new_intern->next = intern;

    memcpy(new_intern->str, start, len);
    new_intern->str[len] = 0;
    map_put(interns, key, new_intern);

    return new_intern->str;
}

const char *str_intern(const char *str)
{
    return str_intern_range(str, str + strlen(str));
}