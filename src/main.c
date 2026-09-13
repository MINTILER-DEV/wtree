#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct counter {
  size_t dirs;
  size_t files;
} counter_t;

typedef struct entry {
  char *name;
  int is_dir;
  struct entry *next;
} entry_t;

// Configuration options
int max_depth = -1;
int max_entries = -1;       // -n flag: max entries displayed per depth level
int dirs_only = 0;          // -d flag: skip all files
char *exclude_ext = NULL;   // -x flag: extension to exclude (e.g., ".log")

void print_help() {
  printf(
"wtree - display directory tree\n\n"
"Usage:\n"
"  wtree [options] [directory]\n\n"
"Options:\n"
"  -h, --help        Show this help message\n"
"  -L <level>        Max display depth\n"
"  -n <count>        Max entries displayed per directory level\n"
"  -d, --dirs-only   Display directories only\n"
"  -x, --exclude <extension>    Exclude files matching specific extension (e.g. .log)\n\n"
"Examples:\n"
"  wtree\n"
"  wtree -L 2\n"
"  wtree -n 10 -d\n"
"  wtree -x .sys\n"
  );
}

// Helper to check if a filename ends with a specific extension string
int ends_with(const char *str, const char *suffix) {
  if (!str || !suffix) return 0;
  size_t len_str = strlen(str);
  size_t len_suffix = strlen(suffix);
  if (len_suffix > len_str) return 0;
  return strcasecmp(str + len_str - len_suffix, suffix) == 0;
}

int walk(const char* directory, const char* prefix, counter_t *counter, int depth) {
  if (max_depth != -1 && depth > max_depth)
    return 0;

  entry_t *head = NULL, *current, *iter;
  size_t size = 0, index;

  struct dirent *file_dirent;
  DIR *dir_handle;

  char *full_path, *segment, *pointer, *next_prefix;

  dir_handle = opendir(directory);
  if (!dir_handle) {
    fprintf(stderr, "Cannot open directory \"%s\"\n", directory);
    return -1;
  }

  counter->dirs++;

  while ((file_dirent = readdir(dir_handle)) != NULL) {
    if (file_dirent->d_name[0] == '.')
      continue;

    // Cross-system directory check using stat
    char path_buf[1024];
    snprintf(path_buf, sizeof(path_buf), "%s/%s", directory, file_dirent->d_name);
    
    struct stat path_stat;
    int resolved_is_dir = 0;
    if (stat(path_buf, &path_stat) == 0) {
        resolved_is_dir = S_ISDIR(path_stat.st_mode);
    }

    // Feature: Filter out files if dirs_only flag is set
    if (dirs_only && !resolved_is_dir) {
      continue;
    }

    // Feature: Filter out files matching specific exclusion extension
    if (!resolved_is_dir && exclude_ext && ends_with(file_dirent->d_name, exclude_ext)) {
      continue;
    }

    current = malloc(sizeof(entry_t));
    current->name = strcpy(malloc(strlen(file_dirent->d_name) + 1), file_dirent->d_name);
    current->is_dir = resolved_is_dir;
    current->next = NULL;

    // Insertion Sort to keep directory stream alphabetical
    if (head == NULL) {
      head = current;
    } else if (strcmp(current->name, head->name) < 0) {
      current->next = head;
      head = current;
    } else {
      for (iter = head; iter->next && strcmp(current->name, iter->next->name) > 0; iter = iter->next);
      current->next = iter->next;
      iter->next = current;
    }

    size++;
  }

  closedir(dir_handle);
  if (!head)
    return 0;

  // Track how many items we are actually going to display out of total items found
  size_t display_limit = size;
  if (max_entries != -1 && (size_t)max_entries < size) {
    display_limit = (size_t)max_entries;
  }

  for (index = 0; index < size; index++) {
    current = head;
    head = head->next;

    // Feature: Cut off visualization early if max_entries is exceeded
    if (index >= display_limit) {
      // Print cutoff tracker at the very last step
      if (index == display_limit) {
        size_t hidden_count = size - display_limit;
        printf("%s└── ... (+%zu more)\n", prefix, hidden_count);
      }
      
      // Update global counters anyway for accuracy behind the scenes
      if (current->is_dir) counter->dirs++; else counter->files++;
      
      free(current->name);
      free(current);
      continue;
    }

    if (index == display_limit - 1) {
      pointer = "└── ";
      segment = "    ";
    } else {
      pointer = "├── ";
      segment = "│   ";
    }

    printf("%s%s%s\n", prefix, pointer, current->name);

    if (current->is_dir) {
      full_path = malloc(strlen(directory) + strlen(current->name) + 2);
      sprintf(full_path, "%s/%s", directory, current->name);

      next_prefix = malloc(strlen(prefix) + strlen(segment) + 1);
      sprintf(next_prefix, "%s%s", prefix, segment);

      walk(full_path, next_prefix, counter, depth + 1);

      free(full_path);
      free(next_prefix);
    } else {
      counter->files++;
    }

    free(current->name);
    free(current);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  // Force Windows console to use UTF-8 encoding out of the box
  #ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);
  #endif

  char *directory = ".";
  int i;

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      print_help();
      return 0;
    }
    else if (!strcmp(argv[i], "-L")) {
      if (i + 1 >= argc) {
        fprintf(stderr, "-L requires a number\n");
        return 1;
      }
      max_depth = atoi(argv[++i]);
    }
    // New Option: Max Entries per level
    else if (!strcmp(argv[i], "-n")) {
      if (i + 1 >= argc) {
        fprintf(stderr, "-n requires a entry limit count number\n");
        return 1;
      }
      max_entries = atoi(argv[++i]);
    }
    // New Option: Directories Only (Flags: -d or --dirs-only)
    else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dirs-only")) {
      dirs_only = 1;
    }
    // New Option: Exclude file extension matches
    else if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--exclude")) {
      if (i + 1 >= argc) {
        fprintf(stderr, "-x requires an extension parameter (e.g. .log)\n");
        return 1;
      }
      exclude_ext = argv[++i];
    }
    else {
      directory = argv[i];
    }
  }

  printf("%s\n", directory);

  counter_t counter = {0, 0};

  walk(directory, "", &counter, 0);

  // Offset summary totals accurately depending on whether files were included
  printf("\n%zu directories, %zu files\n",
    counter.dirs ? counter.dirs - 1 : 0,
    dirs_only ? 0 : counter.files);

  return 0;
}
