🧾 REPORT.md
Feature 2 — Long Listing Format (ls -l)

Q1. What is the crucial difference between the stat() and lstat() system calls? In the context of the ls command, when is it more appropriate to use lstat()?
Answer:
stat() retrieves information about the file a pathname points to, while lstat() retrieves information about the link itself if the pathname is a symbolic link.
In the context of ls, lstat() is more appropriate because it allows us to correctly display details about symbolic links instead of the files they point to. Without lstat(), the program would show metadata for the target file rather than the link itself.

Q2. Explain how you can use bitwise operators and macros to extract file type and permission information from st_mode.
Answer:
The st_mode field encodes both the file type and permissions using specific bits. Bitwise AND (&) operations with macros are used to isolate them:

if (st.st_mode & S_IFDIR)  // Check if directory
if (st.st_mode & S_IRUSR)  // Owner has read permission


Examples:

S_IFDIR — file type: directory

S_IRUSR, S_IWUSR, S_IXUSR — user (owner) permissions

S_IRGRP, S_IROTH — group and others permissions

By combining bitwise operations with these macros, we can construct strings like "drwxr-xr-x".

Feature 3 — Column Display (Down Then Across)

Q1. Explain the general logic for printing items in a "down then across" format. Why is a single loop insufficient?
Answer:
In a “down then across” layout, filenames are printed column by column. For each row, we print elements spaced by num_rows in the filename array:

for (int r = 0; r < num_rows; r++) {
    for (int c = 0; c < num_cols; c++) {
        int index = c * num_rows + r;
        if (index < total_files)
            printf("%-*s", column_width, filenames[index]);
    }
    printf("\n");
}


A single loop prints all filenames sequentially, which only creates a row-wise layout. The “down then across” format needs nested loops to map indices correctly by column and row.

Q2. What is the purpose of the ioctl system call here? What are the limitations of using a fixed-width fallback?
Answer:
ioctl() with TIOCGWINSZ retrieves the current terminal width dynamically, allowing the program to adjust the number of columns to fit the screen.
If only a fixed width (e.g., 80 columns) is used, resizing the terminal won’t change the output layout, leading to either overflow or excessive spacing.

Feature 4 — Horizontal Column Display (-x)

Q1. Compare the implementation complexity of “down then across” vs. “across” (horizontal) printing. Which one is more complex and why?
Answer:
The “down then across” format is more complex because it requires pre-calculating the number of rows and mapping each filename to the correct (row, column) position.
The horizontal “across” format simply prints filenames left to right until the terminal width is reached, making it easier to implement.

Q2. How did you manage the different display modes (-l, -x, default)?
Answer:
A mode flag variable or enum was used to track which display mode is active:

int mode = DEFAULT;
if (l_flag) mode = LONG_LIST;
else if (x_flag) mode = HORIZONTAL;


Based on this flag, the main function calls:

print_long_listing() for -l

print_horizontal() for -x

print_vertical() for default display

This structure makes mode handling modular and easy to extend.

Feature 5 — Alphabetical Sort

Q1. Why must all directory entries be read into memory before sorting? What are the drawbacks for huge directories?
Answer:
Sorting requires access to the complete dataset at once. By reading all filenames into memory first, we can use sorting algorithms like qsort() to order them alphabetically.
Drawback: For directories with millions of files, this approach can lead to high memory usage and slow performance. A streaming or external sort would be more scalable.

Q2. Explain the purpose and signature of the comparison function used by qsort(). Why does it take const void * arguments?
Answer:
qsort() uses a generic comparison function to compare any data type:

int cmpfunc(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}


The const void * pointers allow qsort() to be generic — it doesn’t know the data type being sorted. The function casts them to the correct type (e.g., char **) to compare strings.

Feature 6 — Colorized Output

Q1. How do ANSI escape codes produce color? Show the code sequence for green text.
Answer:
ANSI escape codes modify terminal text appearance using sequences like:

\033[<style>;<color>m


For green text, use:

printf("\033[0;32mHello\033[0m");


\033[ — escape sequence start

0;32m — style 0 (normal) and color 32 (green)

\033[0m — resets color back to default

Q2. Which bits in st_mode determine if a file is executable?
Answer:
Executable permission bits are:

S_IXUSR → Owner execute permission

S_IXGRP → Group execute permission

S_IXOTH → Others execute permission

You can check executability as:

if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
    // File is executable

Feature 7 — Recursive Listing (-R)

Q1. What is a base case in recursion? What is the base case for recursive ls?
Answer:
A base case is the condition that stops recursive calls.
In recursive ls, the base case occurs when there are no more subdirectories to traverse (i.e., when the directory has no child folders other than . or ..). The recursion stops there to avoid infinite looping.

Q2. Why is constructing the full path (parent/subdir) before recursion essential?
Answer:
Without a full path, recursive calls will fail to locate files correctly because relative paths are resolved from the current working directory, not the parent.
For example, calling do_ls("subdir") from within do_ls("parent") won’t find parent/subdir unless the working directory changes. Constructing parent/subdir ensures accurate navigation through nested directories.
