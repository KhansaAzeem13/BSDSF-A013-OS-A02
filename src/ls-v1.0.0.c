/*
 * Programming Assignment 02: ls v1.2.0 (Column Display)
 *
 * - Adds multi-column display (down then across) for default output.
 * - Keeps long listing (-l) support from v1.1.0.
 * - Dynamically adapts to terminal width using ioctl().
 *
 * Usage:
 *   ./bin/ls           → Column display
 *   ./bin/ls -l        → Long listing
 *
 * Author: Khansa Azeem
 * Version: v1.2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>

extern int errno;

/* ---------- Function Declarations ---------- */
void do_ls(const char *dir);            // Column display
void do_ls_long(const char *dir);       // Long listing
void print_permissions(mode_t mode);    // Permission formatter

/* ---------- MAIN ---------- */
int main(int argc, char *argv[])
{
    int opt;
    int long_format = 0;

    // Parse command line (-l optional)
    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_format = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [dir...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        if (long_format)
            do_ls_long(".");
        else
            do_ls(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s:\n", argv[i]);
            if (long_format)
                do_ls_long(argv[i]);
            else
                do_ls(argv[i]);
            puts("");
        }
    }

    return 0;
}

/* ---------- LONG LISTING (-l) ---------- */
void do_ls_long(const char *dir)
{
    DIR *dp;
    struct dirent *entry;
    struct stat info;
    char path[1024];

    dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (stat(path, &info) == -1)
        {
            perror("stat failed");
            continue;
        }

        print_permissions(info.st_mode);
        printf(" %2ld", (long)info.st_nlink);

        struct passwd *pw = getpwuid(info.st_uid);
        struct group *gr = getgrgid(info.st_gid);

        if (pw)
            printf(" %-8s", pw->pw_name);
        else
            printf(" %-8d", (int)info.st_uid);

        if (gr)
            printf(" %-8s", gr->gr_name);
        else
            printf(" %-8d", (int)info.st_gid);

        printf(" %8ld", (long)info.st_size);

        char *t = ctime(&info.st_mtime);
        if (t)
        {
            t[strcspn(t, "\n")] = '\0'; // remove newline
            printf(" %s", t);
        }
        else
        {
            printf(" ???");
        }

        printf(" %s\n", entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

/* ---------- DEFAULT COLUMN DISPLAY ---------- */
void do_ls(const char *dir)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    /* Step 1: Read all file names */
    size_t capacity = 64;
    size_t count = 0;
    char **names = malloc(capacity * sizeof(char *));
    if (!names)
    {
        perror("malloc");
        closedir(dp);
        return;
    }

    size_t maxlen = 0;
    struct dirent *entry;
    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count >= capacity)
        {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (!tmp)
            {
                perror("realloc");
                for (size_t i = 0; i < count; i++)
                    free(names[i]);
                free(names);
                closedir(dp);
                return;
            }
            names = tmp;
        }

        names[count] = strdup(entry->d_name);
        if (!names[count])
        {
            perror("strdup");
            for (size_t i = 0; i < count; i++)
                free(names[i]);
            free(names);
            closedir(dp);
            return;
        }

        size_t len = strlen(names[count]);
        if (len > maxlen)
            maxlen = len;

        count++;
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);

    if (count == 0)
    {
        free(names);
        return;
    }

    /* Step 2: Determine terminal width */
    struct winsize ws;
    int term_width = 80; // fallback width
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        term_width = ws.ws_col;

    /* Step 3: Calculate columns and rows */
    int spacing = 2;
    int col_width = (int)maxlen + spacing;
    if (col_width <= 0)
        col_width = 1;

    int num_cols = term_width / col_width;
    if (num_cols < 1)
        num_cols = 1;

    int num_rows = (count + num_cols - 1) / num_cols; // ceil division

    /* Step 4: Print down then across */
    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_cols; c++)
        {
            int idx = c * num_rows + r;
            if (idx < (int)count)
            {
                printf("%-*s", col_width, names[idx]);
            }
        }
        printf("\n");
    }

    /* Step 5: Free memory */
    for (size_t i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* ---------- PERMISSION FORMATTER ---------- */
void print_permissions(mode_t mode)
{
    char perms[11];
    perms[0] = S_ISDIR(mode) ? 'd' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';

    printf("%s", perms);
}

