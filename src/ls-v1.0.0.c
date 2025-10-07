/*
 * Programming Assignment 02: lsv1.4.0
 * Features:
 *   -a  → show hidden files
 *   -l  → long listing format
 *   -R  → recursive directory listing
 * default → sorted multi-column display
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

/* ---------- Function Prototypes ---------- */
void do_ls(const char *dir, int show_all);
void do_ls_long(const char *dir, int show_all);
void do_ls_recursive(const char *dir, int depth, int show_all);
void print_permissions(mode_t mode);

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

/* ---------- Sort Helper ---------- */
int compare_names(const void *a, const void *b)
{
    const char *name1 = *(const char **)a;
    const char *name2 = *(const char **)b;
    return strcasecmp(name1, name2);
}

/* ---------- Long Listing (-l) ---------- */
void do_ls_long(const char *dir, int show_all)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char path[1024];

    dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
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
        if (stat(path, &statbuf) == -1)
        {
            perror("stat");
            continue;
        }

        struct passwd *pw = getpwuid(statbuf.st_uid);
        struct group *gr = getgrgid(statbuf.st_gid);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&statbuf.st_mtime));

        print_permissions(statbuf.st_mode);
        printf(" %3ld %-8s %-8s %8ld %s %s\n",
               statbuf.st_nlink,
               pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown",
               statbuf.st_size,
               timebuf,
               names[i]);
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
    int count = 0;
    int maxlen = 0;

    dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
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
                printf("%-*s", maxlen + spacing, names[i]);
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
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s : %s\n", dir, strerror(errno));
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
        printf("%s\n", entry->d_name);

        if (stat(path, &statbuf) == -1)
            continue;

        if (S_ISDIR(statbuf.st_mode))
            do_ls_recursive(path, depth + 1, show_all);
    }
    closedir(dp);
}

/* ---------- main() ---------- */
int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;
    int recursive_flag = 0;
    int show_all = 0;

    while ((opt = getopt(argc, argv, "lRa")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        case 'R':
            recursive_flag = 1;
            break;
        case 'a':
            show_all = 1;
            break;
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
