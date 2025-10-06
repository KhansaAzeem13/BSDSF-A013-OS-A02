

/*
 * Programming Assignment 02: ls v1.1.0
 * Feature 2 — Long Listing Format (-l)
 *
 * This version adds support for the -l option to display
 * file details similar to the original 'ls -l' command.
 *
 * System calls used:
 *  stat(), opendir(), readdir(), getpwuid(), getgrgid(), ctime()
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

extern int errno;

/* ---------- Function Declarations ---------- */
void do_ls(const char *dir);            // Old short format
void do_ls_long(const char *dir);       // New long format
void print_permissions(mode_t mode);    // Helper for permission bits

/* ---------- main() ---------- */
int main(int argc, char *argv[])
{
    int opt;           // for getopt()
    int long_format = 0; // flag for -l option

    // Parse command-line arguments using getopt()
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

    // If no directory arguments provided, use current directory
    if (optind == argc)
    {
        if (long_format)
            do_ls_long(".");
        else
            do_ls(".");
    }
    else
    {
        // Loop through all directory arguments
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

/* ---------- Basic Short Listing (v1.0.0 logic) ---------- */
void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        // Skip hidden files
        if (entry->d_name[0] == '.')
            continue;
        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

/* ---------- Long Listing Format (-l option) ---------- */
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
        // Skip hidden files
        if (entry->d_name[0] == '.')
            continue;

        // Build full path: dir + "/" + filename
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        // Retrieve file info
        if (stat(path, &info) == -1)
        {
            perror("stat failed");
            continue;
        }

        // Permissions
        print_permissions(info.st_mode);

        // Number of hard links
        printf(" %2ld", (long)info.st_nlink);

        // Owner and Group names
        struct passwd *pw = getpwuid(info.st_uid);
        struct group *gr = getgrgid(info.st_gid);

        if (pw != NULL)
            printf(" %-8s", pw->pw_name);
        else
            printf(" %-8d", info.st_uid);

        if (gr != NULL)
            printf(" %-8s", gr->gr_name);
        else
            printf(" %-8d", info.st_gid);

        // File size
        printf(" %8ld", (long)info.st_size);

        // Modification time
        char *time_str = ctime(&info.st_mtime);
        time_str[strlen(time_str) - 1] = '\0'; // remove newline
        printf(" %s", time_str);

        // File name
        printf(" %s\n", entry->d_name);
    }

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

/* ---------- Helper: Permission String ---------- */
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
