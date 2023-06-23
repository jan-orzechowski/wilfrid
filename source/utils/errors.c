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
} error_message;

error_message *errors;

void error(const char *error_text, source_pos pos)
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
        .pos = pos
    };
    buf_push(errors, message);
}

void error_without_pos(const char *error_text)
{
    error_message message =
    {
        .text = error_text,
        .pos = { 0 }
    };
    buf_push(errors, message);
}

typedef enum source_pos_print_mode
{
    SOURCE_POS_PRINT_FULL = 0,
    SOURCE_POS_PRINT_SHORTEN = 1,
    SOURCE_POS_PRINT_WITHOUT_FILE = 2
} source_pos_print_mode;

source_pos_print_mode print_source_pos_mode = SOURCE_POS_PRINT_FULL;

void print_source_pos(char **buffer, source_pos pos)
{
    if (pos.filename == null)
    {
        return;
    }

    if (print_source_pos_mode == SOURCE_POS_PRINT_FULL)
    {
        buf_printf(*buffer, "(file '%s', line %zu, position %zu)", pos.filename, pos.line, pos.character);
    }
    else if (print_source_pos_mode == SOURCE_POS_PRINT_SHORTEN)
    {
        buf_printf(*buffer, "('%s':%04zu:%02zu)", pos.filename, pos.line, pos.character);
    }
    else if (print_source_pos_mode == SOURCE_POS_PRINT_WITHOUT_FILE)
    {
        buf_printf(*buffer, "(line %zu, position %zu)", pos.line, pos.character);
    }
}

char *print_errors(void)
{
    char *buffer = null;
    size_t errors_count = buf_len(errors);
    if (errors_count > 0)
    {        
        buf_printf(buffer, "\n%zu %s:\n\n", errors_count, errors_count == 1 ? "ERROR" : "ERRORS");
        for (size_t i = 0; i < errors_count; i++)
        {
            error_message msg = errors[i];
            if (msg.pos.filename)
            {
                buf_printf(buffer, "- ");
                print_source_pos(&buffer, msg.pos);
                buf_printf(buffer, " %s", msg.text);
            }
            else
            {
                buf_printf(buffer, "- %s", msg.text);
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