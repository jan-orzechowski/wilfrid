#include <io.h>

#define MAX_PATH _MAX_PATH

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

void path_normalize(char *path)
{
    char *ptr = path;
    while (*ptr)
    {
        if (*ptr == '\\')
        {
            *ptr = '/';
        }
        ptr++;
    }

    if (ptr != path && ptr[-1] == '/')
    {
        ptr[-1] = 0;
    }
}

void path_copy(char *dest, size_t dest_array_size, const char *src)
{
    strncpy_s(dest, dest_array_size, src, dest_array_size);
    dest[dest_array_size - 1] = 0;
    path_normalize(dest);
}

void path_join(char *path, size_t dest_array_size, const char *src)
{
    char *ptr = path + strlen(path);
    if (ptr != path && ptr[-1] == '/')
    {
        ptr--;
    }
    if (*src == '/')
    {
        src++;
    }
    snprintf(ptr, path + dest_array_size - ptr, "/%s", src);
}

void path_absolute(char path[MAX_PATH])
{
    char rel_path[MAX_PATH];
    path_copy(rel_path, MAX_PATH, path);
    _fullpath(path, rel_path, MAX_PATH);
}

char *path_get_extension_part(char path[MAX_PATH])
{
    for (char *ptr = path + strlen(path); ptr != path; ptr--)
    {
        if (ptr[-1] == '.')
        {
            return ptr;
        }
    }
    return path;
}

bool path_has_extension(char *path, char *extension)
{
    char *ext = path_get_extension_part(path);
    return (ext != null && 0 == strcmp(ext, extension));
}

char *path_get_file_part(char path[MAX_PATH])
{
    path_normalize(path);
    for (char *ptr = path + strlen(path); ptr != path; ptr--)
    {
        if (ptr[-1] == '/')
        {
            return ptr;
        }
    }
    return path;
}

void get_source_files_in_dir(char *path, char ***source_files_buf, char ***directories_buf)
{
    char filespec_buffer[MAX_PATH] = { 0 };
    path_copy(filespec_buffer, MAX_PATH, path);
    path_join(filespec_buffer, MAX_PATH, "*");
    struct _finddata_t fileinfo;

    bool is_find_handle_valid = false;
    int error = 0;

    _set_errno(0);
    intptr_t find_handle = _findfirst(filespec_buffer, &fileinfo);
    if (find_handle == -1)
    {
        if (errno != 0 && errno != ENOENT)
        {
            error = errno;
        }
        return; // bez valid handle _findnext wywali się
    }
    else
    {
        is_find_handle_valid = true;
    }

    const char *default_parent_directory = str_intern("..");
    while (true)
    {
        struct _finddata_t fileinfo = { 0 };

        _set_errno(0);
        int result = _findnext((intptr_t)find_handle, &fileinfo);
        if (result == 0) // success
        {
            char filename_buffer[MAX_PATH] = { 0 };
            path_copy(filename_buffer, MAX_PATH, path);
            path_join(filename_buffer, MAX_PATH, fileinfo.name);
            bool is_directory = (fileinfo.attrib & _A_SUBDIR);
            if (is_directory)
            {
                if (0 != strcmp(fileinfo.name, ".."))
                {                   
                    buf_push(*directories_buf, str_intern(filename_buffer));
                }
            }
            else
            {
                if (path_has_extension(filename_buffer, SOURCEFILE_EXTENSION))
                {
                    buf_push(*source_files_buf, str_intern(filename_buffer));
                }
            }
        }
        else // -1
        {
            if (errno != 0 && errno != ENOENT)
            {
                is_find_handle_valid = false;
                error = errno;
            }
            break;
        }
    }

    if (is_find_handle_valid)
    {
        _findclose(find_handle);
    }

    if (error)
    {
        char error_buffer[256] = { 0 };
        strerror_s(error_buffer, 256, error);
        printf("Error while reading %s directory contents: %s",
            filespec_buffer, error_buffer);
    }
}

char **get_source_files_in_dir_and_subdirs(char *path)
{
    char **source_files = 0;
    char **directories = 0;

    get_source_files_in_dir(path, &source_files, &directories);

    while (buf_len(directories) > 0)
    {
        char *dir_to_scan = directories[buf_len(directories) - 1];
        buf_remove_at(directories, buf_len(directories) - 1);

        get_source_files_in_dir(dir_to_scan, &source_files, &directories);
    }

    buf_free(directories);
    return source_files;
}