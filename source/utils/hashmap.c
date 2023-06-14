uint64_t hash_uint64(uint64_t x)
{
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

uint64_t hash_ptr(void *ptr)
{
    return hash_uint64((uintptr_t)ptr);
}

uint64_t hash_bytes(const char *buf, size_t len)
{
    // FNV hash
    uint64_t x = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; i++)
    {
        x ^= buf[i];
        x *= 0x100000001b3;
        x ^= x >> 32;
    }
    return x;
}

typedef struct hashmap
{
    void **keys;
    void **values;
    size_t count;
    size_t capacity;
} hashmap;

#define map_put(map, key, val) _map_put((map), (void *)(key), (void *)(val))
#define map_get(map, key) _map_get((map), (void *)(key))

void *_map_get(hashmap *map, void *key)
{
    assert(map->capacity != 0);
    if (map->count == 0)
    {
        return null;
    }

    assert(is_power_of_2(map->capacity));

    size_t index = (size_t)hash_ptr(key);
    assert(map->count < map->capacity);
    while (true)
    {
        index &= map->capacity - 1;
        if (map->keys[index] == key)
        {
            return map->values[index];
        }
        else if (map->keys[index] == 0)
        {
            return null;
        }
        index++;
    }
    return null;
}

void _map_put(hashmap *map, void *key, void *val);

void map_grow(hashmap *map, size_t new_capacity)
{
    new_capacity = max(16, new_capacity);
    hashmap new_map = {
        .keys = xcalloc(new_capacity * sizeof(void *)),
        .values = xcalloc(new_capacity * sizeof(void *)),
        .capacity = new_capacity,
    };
    for (size_t i = 0; i < map->capacity; i++)
    {
        // rehashing już umieszczonych wartości
        if (map->keys[i])
        {
            map_put(&new_map, map->keys[i], map->values[i]);
        }
    }
    free(map->keys);
    free(map->values);
    *map = new_map;
}

void _map_put(hashmap *map, void *key, void *val)
{
    assert(key);
    assert(val);
    // dzięki temu mamy mniejsze prawdopodobieństwo kolizji
    if (2 * map->count >= map->capacity)
    {
        map_grow(map, 2 * map->capacity);
    }

    assert(2 * map->count < map->capacity);
    assert(is_power_of_2(map->capacity));

    size_t index = (size_t)hash_ptr(key);
    while (true)
    {
        index &= map->capacity - 1;
        if (!map->keys[index])
        {
            map->count++;
            map->keys[index] = key;
            map->values[index] = val;
            return;
        }
        else if (map->keys[index] == key)
        {
            map->values[index] = val;
            return;
        }
        index++;
    }
}

void map_free(hashmap *map)
{
    free(map->keys);
    free(map->values);
    map->keys = null;
    map->values = null;
    map->count = 0;
    map->capacity = 0;
}

typedef struct hashmap_value hashmap_value;
struct hashmap_value
{
    void *key;
    void *value;
    hashmap_value *next;
};

typedef struct chained_hashmap
{
    hashmap_value **values;
    size_t total_count;
    size_t used_capacity;
    size_t capacity;
} chained_hashmap;

#define map_chain_put(map, key, val) _map_chain_put(map, (void *)key, (void *)val)
#define map_chain_get(map, key) _map_chain_get(map, (void *)key)
#define map_chain_delete(map, key) _map_chain_delete(map, (void *)key)

void *_map_chain_get(chained_hashmap *map, void *key)
{
    assert(key);

    hashmap_value *val = null;
    size_t index = (size_t)hash_ptr(key) & (map->capacity - 1);
    val = map->values[index];
    while (val)
    {
        if (val->key == key)
        {
            return val->value;
        }

        if (val->next)
        {
            val = val->next;
        }
        else
        {
            break;
        }
    }
    return null;
}

void _map_chain_delete(chained_hashmap *map, void *key)
{
    assert(key);

    size_t index = (size_t)hash_ptr(key) & (map->capacity - 1);
    hashmap_value *val = map->values[index];

    bool found = false;
    hashmap_value *prev_val = null;
    size_t chain_length = 0;

    while (val)
    {
        chain_length++;
        if (val->key == key)
        {
            found = true;
            // usuwamy
            if (val->next)
            {
                if (prev_val)
                {
                    prev_val->next = val->next;
                }
                else
                {
                    map->values[index] = val->next;
                }
            }
            else
            {
                if (prev_val)
                {
                    prev_val->next = null;
                }

                if (chain_length == 1)
                {
                    assert(map->values[index] == val);
                    map->values[index] = null;
                    map->used_capacity--;
                }
            }

            free(val);
            map->total_count--;
            break;
        }
        else
        {
            prev_val = val;
            val = val->next;
        }
    }

#if DEBUG_BUILD
    if (found == false)
    {
        fatal("key not found");
    }
#endif
}

void _map_chain_put(chained_hashmap *map, void *key, void *value);

void map_chain_grow(chained_hashmap *map, size_t new_capacity)
{
    bool free_values = (map->capacity > 0);

    new_capacity = max(16, new_capacity);
    chained_hashmap new_map = {
        .values = xcalloc(new_capacity * sizeof(hashmap_value *)),
        .capacity = new_capacity,
    };
    
    for (size_t i = 0; i < map->capacity; i++)
    {
        // rehashing już umieszczonych wartości
        hashmap_value *val = map->values[i];
        while (val)
        {
            map_chain_put(&new_map, val->key, val->value);
            hashmap_value *temp = val->next;
            free(val);
            val = temp;
        }
    }

    if (free_values)
    {
        free(map->values);
    }
    
    *map = new_map;
}

void _map_chain_put(chained_hashmap *map, void *key, void *value)
{
    size_t index = (size_t)hash_ptr(key) & (map->capacity - 1);
    hashmap_value *val = map->values[index];

    if (val)
    {
        hashmap_value *last_val = val;
        while (val)
        {
            last_val = val;
            val = val->next;
        }

        last_val->next = xcalloc(sizeof(hashmap_value));
        last_val->next->key = key;
        last_val->next->value = value;
    }
    else
    {        
        if (2 * map->used_capacity >= map->capacity)
        {
            map_chain_grow(map, 2 * map->capacity);
        }

        hashmap_value *new_value = xcalloc(sizeof(hashmap_value));
        new_value->key = key;
        new_value->value = value;

        index = (size_t)hash_ptr(key) & (map->capacity - 1);
        val = map->values[index];

        if (val)
        {
            val->next = new_value;
        }
        else
        {
            map->values[index] = new_value;
            map->used_capacity++;
        }
    }

    map->total_count++;
}

void map_chain_free(chained_hashmap *map)
{
    for (size_t i = 0; i < map->capacity; i++)
    {
        hashmap_value *val = map->values[i];
        while (val)
        {
            hashmap_value *temp = val->next;
            free(val);
            val = temp;
        }
    }
    free(map->values);
    map->total_count = 0;
    map->used_capacity = 0;
    map->capacity = 0;
}