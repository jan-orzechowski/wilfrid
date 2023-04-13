typedef struct source_pos
{
    const char *filename;
    size_t line;
    size_t character;
} source_pos;

bool compare_source_pos(source_pos a, source_pos b)
{
    bool pos_matches = (a.character == b.character && a.line == b.line);

    bool file_matches = false;
    if (a.filename == null && b.filename == null)
    {
        file_matches = true;
    }
    else if (a.filename == null || b.filename == null)
    {
        file_matches = false;
    }
    else
    {
        file_matches = strcmp(a.filename, b.filename);
    }

    return (file_matches && pos_matches);
}

typedef struct error_message
{
    const char *text;
    source_pos pos;
    size_t length;
} error_message;

error_message *errors;

void error(const char *error_text, source_pos pos, size_t length)
{
    if (buf_len(errors) > 0)
    {
        // nie wrzucamy wielu błędów w tym samym miejscu
        error_message last = errors[buf_len(errors) - 1];
        if (compare_source_pos(last.pos, pos))
        {
            return;
        }
    }

    error_message message =
    {
        .text = error_text,
        .pos = pos,
        .length = length
    };
    buf_push(errors, message);
}

bool shorten_source_pos = false;

void print_source_pos(char **buffer, source_pos pos)
{
    if (shorten_source_pos)
    {
        buf_printf(*buffer, "('%s':%lld:%lld)", pos.filename, pos.line, pos.character);
    }
    else
    {
        buf_printf(*buffer, "(file '%s', line %lld, position %lld)", pos.filename, pos.line, pos.character);
    }
}

char *print_errors(void)
{
    char *buffer = null;
    size_t errors_count = buf_len(errors);
    if (errors_count > 0)
    {
        buf_printf(buffer, "\n%lld errors:\n", errors_count);
        for (size_t i = 0; i < errors_count; i++)
        {
            error_message msg = errors[i];
            buf_printf(buffer, "- %s", msg.text);
            if (msg.pos.filename)
            {
                buf_printf(buffer, " ");
                print_source_pos(&buffer, msg.pos);
            }
            buf_printf(buffer, "\n");
        }
    }
    return buffer;
}

void print_errors_to_console(void)
{
    char *list = print_errors();
    if (list)
    {
        printf("%s", list);
    }
    buf_free(list);
}