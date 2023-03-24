string_ref read_file(char *filename)
{
    FILE *src;
    errno_t err = fopen_s(&src, filename, "rb");
    if (err != 0)
    {
        // błąd
        return (string_ref) { 0 };
    }

    fseek(src, 0, SEEK_END);
    size_t size = ftell(src);
    fseek(src, 0, SEEK_SET);

    if (size > 0)
    {
        char *str_buf = xmalloc(size + 1);
        str_buf[size] = 0; // null terminator

        size_t objects_read = fread(str_buf, size, 1, src);
        if (objects_read == 1)
        {
            fclose(src);
            return (string_ref) { .str = str_buf, .length = size };
        }
        else
        {
            fclose(src);
            free(str_buf);
            return (string_ref) { 0 };
        }
    }
    else
    {
        fclose(src);
        return (string_ref) { 0 };
    }
}

bool write_file(const char *path, const char *buf, size_t len)
{
    FILE *file;
    errno_t err = fopen_s(&file, path, "w");
    if (err != 0)
    {
        // błąd
        return false;
    }

    if (!file)
    {
        return false;
    }
    size_t elements_written = fwrite(buf, len, 1, file);
    fclose(file);
    return (elements_written == 1);
}

string_ref read_file_for_parsing(char *filename)
{
    string_ref file_buf = read_file(filename);
    if (file_buf.length > 3)
    {
        // pomijanie BOM
        if (file_buf.str[0] == (char)0xef
            && file_buf.str[1] == (char)0xbb
            && file_buf.str[2] == (char)0xbf)
        {
            file_buf.str[0] = 0x20;
            file_buf.str[1] = 0x20;
            file_buf.str[2] = 0x20;
        }
    }
    return file_buf;
}