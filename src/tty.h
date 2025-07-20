#pragma once

#include <limits.h>
#include <pty.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

int32_t utf8decode(const char *s, uint32_t *out_cp);
