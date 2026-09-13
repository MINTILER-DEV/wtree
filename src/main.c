#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct counter {
  size_t dirs;
  size_t files;
} counter_t;

typedef struct entry {
  char *name;
  int is_dir;
  struct entry *next;
} entry_t;

int max_depth = -1;

void print_help() {
  printf(
"wtree - display directory tree\n\n"
"Usage:\n"
"  wtree [options] [directory]\n\n"
"Options:\n"
"  -h, --help        Show this help message\n"
"  -L <level>        Max display depth\n\n"
"Examples:\n"
"  wtree\n"
"  wtree src\n"
"  wtree -L 2\n"
  );
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

    current = malloc(sizeof(entry_t));
    current->name = strcpy(malloc(strlen(file_dirent->d_name) + 1), file_dirent->d_name);
    
    // cross system
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, file_dirent->d_name);
    
    struct stat path_stat;
    if (stat(full_path, &path_stat) == 0) {
        current->is_dir = S_ISDIR(path_stat.st_mode);
    } else {
        current->is_dir = 0; // Fallback if stat fails
    }

    current->next = NULL;

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

  for (index = 0; index < size; index++) {

    if (index == size - 1) {
      pointer = "└── ";
      segment = "    ";
    } else {
      pointer = "├── ";
      segment = "│   ";
    }

    printf("%s%s%s\n", prefix, pointer, head->name);

    if (head->is_dir) {
      full_path = malloc(strlen(directory) + strlen(head->name) + 2);
      sprintf(full_path, "%s/%s", directory, head->name);

      next_prefix = malloc(strlen(prefix) + strlen(segment) + 1);
      sprintf(next_prefix, "%s%s", prefix, segment);

      walk(full_path, next_prefix, counter, depth + 1);

      free(full_path);
      free(next_prefix);
    } else {
      counter->files++;
    }

    current = head;
    head = head->next;

    free(current->name);
    free(current);
  }

  return 0;
}

int main(int argc, char *argv[]) {

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

    else {
      directory = argv[i];
    }
  }

  printf("%s\n", directory);

  counter_t counter = {0, 0};

  walk(directory, "", &counter, 0);

  printf("\n%zu directories, %zu files\n",
    counter.dirs ? counter.dirs - 1 : 0,
    counter.files);

  return 0;
}
