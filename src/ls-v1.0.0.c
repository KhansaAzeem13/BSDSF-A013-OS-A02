/*
 * Programming Assignment 02: lsv1.5.0
 * Features:
 *   -a  show hidden files
 *   -l  long listing
 *   -R  recursive listing
 *   colorized output for better readability
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

extern int errno;

/* ANSI Color Codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_DIR     "\033[34m"  // Blue
#define COLOR_EXEC    "\033[32m"  // Green
#define COLOR_LINK    "\033[36m"  // Cyan
#define COLOR_ERROR   "\033[31m"  // Red
#define COLOR_FILE    "\033[37m"  // White

/* ---------- Function Prototypes ---------- */
void do_ls(const char *dir, int show_all);
void do_ls_long(const char *dir, int show_all);
void do_ls_recursive(const char *dir, int depth, int show_all);
void print_permissions(mode_t mode);
void print_colored_name(const char *path, const char *name);

/* ---------- Permission Printer ---------- */
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

/* ---------- Color Printer ---------- */
void print_colored_name(const char *path, const char *name)
{
    struct stat st;
    if (lstat(path, &st) == -1)
    {
        printf(COLOR_ERROR "%s" COLOR_RESET, name);
        return;
    }

    if (S_ISDIR(st.st_mode))
        printf(COLOR_DIR "%s" COLOR_RESET, name);
    else if (S_ISLNK(st.st_mode))
        printf(COLOR_LINK "%s" COLOR_RESET, name);
    else if (st.st_mode & S_IXUSR)
        printf(COLOR_EXEC "%s" COLOR_RESET, name);
    else
        printf(COLOR_FILE "%s" COLOR_RESET, name);
}

/* ---------- Sort Helper ---------- */
int compare_names(const void *a, const void *b)
{
    const char *n1 = *(const char **)a;
    const char *n2 = *(const char **)b;
    return strcasecmp(n1, n2);
}

/* ---------- Long Listing (-l) ---------- */
void do_ls_long(const char *dir, int show_all)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char path[1024];

    dp = opendir(dir);
    if (!dp)
    {
        fprintf(stderr, COLOR_ERROR "Cannot open directory: %s\n" COLOR_RESET, dir);
        return;
    }

    char **names = NULL;
    int count = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue;
        names = realloc(names, sizeof(char *) * (count + 1));
        names[count] = strdup(entry->d_name);
        count++;
    }
    closedir(dp);

    qsort(names, count, sizeof(char *), compare_names);

    printf("\n%s:\n", dir);
    for (int i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        if (lstat(path, &statbuf) == -1)
        {
            perror("stat");
            continue;
        }

        struct passwd *pw = getpwuid(statbuf.st_uid);
        struct group *gr = getgrgid(statbuf.st_gid);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&statbuf.st_mtime));

        print_permissions(statbuf.st_mode);
        printf(" %3ld %-8s %-8s %8ld %s ",
               statbuf.st_nlink,
               pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown",
               statbuf.st_size,
               timebuf);
        print_colored_name(path, names[i]);
        printf("\n");
        free(names[i]);
    }
    free(names);
}

/* ---------- Column Display (default) ---------- */
void do_ls(const char *dir, int show_all)
{
    DIR *dp;
    struct dirent *entry;
    struct winsize w;
    int term_width = 80;
    int count = 0, maxlen = 0;

    dp = opendir(dir);
    if (!dp)
    {
        fprintf(stderr, COLOR_ERROR "Cannot open directory: %s\n" COLOR_RESET, dir);
        return;
    }

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (w.ws_col > 0)
        term_width = w.ws_col;

    char **names = NULL;
    while ((entry = readdir(dp)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue;
        names = realloc(names, sizeof(char *) * (count + 1));
        names[count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > maxlen)
            maxlen = len;
        count++;
    }
    closedir(dp);

    if (count == 0)
        return;

    qsort(names, count, sizeof(char *), compare_names);

    int spacing = 2;
    int cols = term_width / (maxlen + spacing);
    if (cols < 1)
        cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int i = c * rows + r;
            if (i < count)
            {
                char path[1024];
                snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
                print_colored_name(path, names[i]);
                int pad = maxlen - strlen(names[i]) + spacing;
                for (int s = 0; s < pad; s++)
                    printf(" ");
            }
        }
        printf("\n");
    }

    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* ---------- Recursive Listing (-R) ---------- */
void do_ls_recursive(const char *dir, int depth, int show_all)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char path[1024];

    dp = opendir(dir);
    if (!dp)
    {
        fprintf(stderr, COLOR_ERROR "Cannot open directory: %s : %s\n" COLOR_RESET, dir, strerror(errno));
        return;
    }

    printf("\n%s:\n", dir);
    while ((entry = readdir(dp)) != NULL)
    {
        if (!show_all && entry->d_name[0] == '.')
            continue;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        print_colored_name(path, entry->d_name);
        printf("\n");

        if (lstat(path, &statbuf) == -1)
            continue;

        if (S_ISDIR(statbuf.st_mode))
            do_ls_recursive(path, depth + 1, show_all);
    }
    closedir(dp);
}

/* ---------- main() ---------- */
int main(int argc, char *argv[])
{
    int opt, long_flag = 0, recursive_flag = 0, show_all = 0;

    while ((opt = getopt(argc, argv, "lRa")) != -1)
    {
        switch (opt)
        {
        case 'l': long_flag = 1; break;
        case 'R': recursive_flag = 1; break;
        case 'a': show_all = 1; break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-R] [-a] [dir...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        if (recursive_flag)
            do_ls_recursive(".", 0, show_all);
        else if (long_flag)
            do_ls_long(".", show_all);
        else
            do_ls(".", show_all);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (recursive_flag)
                do_ls_recursive(argv[i], 0, show_all);
            else if (long_flag)
                do_ls_long(argv[i], show_all);
            else
                do_ls(argv[i], show_all);
            puts("");
        }
    }
    return 0;
}
