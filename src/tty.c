#define LF_X11
#define LF_RUNARA
#include <leif/leif.h>
#include <leif/ui_core.h>
#include <leif/win.h>

#include "tty.h"

static int32_t master_fd;

size_t readfrompty(void) {
  static char buf[SHRT_MAX];
  static uint32_t buflen = 0;

  uint32_t nbytes = read(master_fd, buf + buflen, sizeof(buf) - buflen);
  buflen += nbytes;

  uint32_t iter = 0;
  while (iter < buflen) {
    uint32_t codepoint;
    int32_t len = utf8decode(&buf[iter], &codepoint);
    if (len == -1 || len > (int32_t)buflen)
      break;
    iter += len;

    printf("%u\n", codepoint);
  }

  if (iter < buflen) {
    memmove(buf, buf + iter, buflen - iter);
  }
  buflen -= iter;

  return nbytes;
}

void term_next_event(lf_ui_state_t *ui) {
  float cur_time = lf_ui_core_get_elapsed_time();
  ui->delta_time = cur_time - ui->_last_time;
  ui->_last_time = cur_time;

  bool rendered = lf_windowing_get_current_event() == LF_EVENT_WINDOW_REFRESH;
  lf_ui_core_shape_widgets_if_needed(ui, ui->root, false);

  if (ui->needs_render) {
    lf_win_make_gl_context(ui->win);

    vec2s winsize = lf_win_get_size(ui->win);
    ui->render_clear_color_area(lf_color_from_hex(0x1a1a1a),
                                LF_SCALE_CONTAINER(winsize.x, winsize.y),
                                winsize.y);

    ui->render_begin(ui->render_state);
    ui->render_rect(ui->render_state, (vec2s){50, 50}, (vec2s){50, 50}, LF_RED,
                    LF_NO_COLOR, 0.0f, 0.0f);
    ui->render_end(ui->render_state);
    lf_win_swap_buffers(ui->win);

    ui->needs_render = false;
    rendered = true;
  }

  lf_windowing_update();
}

int main() {

  if (forkpty(&master_fd, NULL, NULL, NULL) == 0) {
    execlp("/usr/bin/bash", "bash", NULL);
    perror("execlp");
    exit(1);
  }

  lf_windowing_init();
  lf_window_t win = lf_ui_core_create_window(1280, 720, "tty terminal");
  lf_ui_state_t *ui = lf_ui_core_init(win);

  bool first_time = true;

  fd_set read_fdset;
  int32_t x11fd = ConnectionNumber(lf_win_get_x11_display());
  while (ui->running) {
    FD_ZERO(&read_fdset);
    FD_SET(master_fd, &read_fdset);
    FD_SET(x11fd, &read_fdset);

    if (first_time) {
      term_next_event(ui);
      first_time = false;
    }

    // Listen to reads in fdset
    select(MAX(master_fd, x11fd) + 1, &read_fdset, NULL, NULL, NULL);

    if (FD_ISSET(master_fd, &read_fdset)) {
      readfrompty();
    }
    if (FD_ISSET(x11fd, &read_fdset)) {
      lf_windowing_next_event();

      lf_event_type_t curevent = lf_windowing_get_current_event();
      if (curevent == LF_EVENT_KEY_PRESS || curevent == LF_EVENT_TYPING_CHAR ||
          curevent == LF_EVENT_WINDOW_CLOSE ||
          curevent == LF_EVENT_WINDOW_REFRESH ||
          curevent == LF_EVENT_WINDOW_RESIZE) {
        term_next_event(ui);
      }
    }
  }

  return EXIT_SUCCESS;
}
