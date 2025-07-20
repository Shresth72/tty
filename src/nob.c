#include <dirent.h>
#include <stdio.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "lib/nob.h"

#define builder_cc(cmd) cmd_append(cmd, "cc")
#define builder_output(cmd, output_path) cmd_append(cmd, "-o", output_path)
#define builder_inputs(cmd, ...) cmd_append(cmd, __VA_ARGS__)
#define builder_libs(cmd)                                                      \
  cmd_append(cmd, "-lleif", "-lrunara", "-lm", "-lGL", "-lX11", "-lXrender",   \
             "-lfreetype", "-lfontconfig", "-lharfbuzz", "-lglfw")
#define builder_flags(cmd)                                                     \
  cmd_append(cmd, "-Wall", "-Wextra", "-Wswitch-enum", "-ggdb")
#define builder_include_path(cmd, include_path)                                \
  cmd_append(cmd, temp_sprintf("-I%s", include_path))

void builder_inputs_auto(Nob_Cmd *cmd, const char *path);

int main(int argc, char *argv[]) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  Nob_Cmd cmd = {0};

  builder_cc(&cmd);
  builder_output(&cmd, "main");
  builder_inputs_auto(&cmd, ".");
  builder_libs(&cmd);
  builder_flags(&cmd);

  if (!cmd_run_sync_and_reset(&cmd))
    return 1;

  const char *program_name = shift(argv, argc);

  if (argc > 0) {
    const char *subcommand = shift(argv, argc);

    if (strcmp(subcommand, "run") == 0) {
      cmd_append(&cmd, "./main");
      if (!cmd_run_sync_and_reset(&cmd))
        return 1;
    } else if (strcmp(subcommand, "gui") == 0) {
      cmd_append(&cmd, "./main", "gui");
    } else {
      nob_log(ERROR, "Unknown command: %s", subcommand);
      return 1;
    }
  }

  return 0;
}

// Impl
void builder_inputs_auto(Nob_Cmd *cmd, const char *path) {
  DIR *dir = opendir(path);
  if (!dir) {
    perror(path);
    return;
  }

  struct dirent *entry;
  char fullpath[512];

  while ((entry = readdir(dir)) != NULL) {
    const char *filename = entry->d_name;
    size_t len = strlen(filename);

    if (len > 2 && strcmp(filename + len - 2, ".c") == 0) {
      if (strcmp(path, ".") == 0 && strcmp(filename, "nob.c") == 0) {
        continue;
      }

      if (strcmp(path, ".") == 0) {
        snprintf(fullpath, sizeof(fullpath), "%s", filename);
      } else {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);
      }

      // Filepath is stack allocated, so need to be copied
      // So, Nob_Cmd only stores the pointer and not the copy of the string
      // After the loop ends, the pointer now points to invalid memory
      cmd_append(cmd, nob_temp_strdup(fullpath));
    }
  }

  closedir(dir);
}
