#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <p2plib.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#include <fcntl.h>

#include <audio.h>
#include <video.h>

#define DEFAULT_WIDTH 100
#define DEFAULT_HEIGHT 40

typedef struct {
  char *ipaddr;
  char *port;
} network_options_t;

void *spawn_audio_thread(void *args) {
  network_options_t *netopts = args;
  start_audio(netopts->ipaddr, netopts->port);
  return NULL;
}

void audio_shutdown(int signal) {
  kill(getpid(), SIGUSR1);
}

void all_shutdown(int signal) {
  video_shutdown(signal);
  audio_shutdown(signal);
  exit(0);
}

void get_dimensions(char dim[], int *width, int *height) {
  int i;
  char *wstr = (char *)dim, *hstr;
  for (i = 0; i < sizeof(dim); i++) {
    if ((dim[i] == 'x' || dim[i] == ':') && (i + 1 < sizeof(dim))) {
      dim[i] = '\0';
      hstr = &(dim[i+1]);
    }
  }

  *width = atoi(wstr);
  *height = atoi(hstr);
}

void usage(FILE *stream) {
  fprintf(stream,
    "Usage: p2pvc [-h] [server] [options]\n"
    "A point to point color terminal video chat.\n"
    "\n"
    "  -v    Enable video chat.\n"
    "  -d    Dimensions of video in either [width]x[height] or [width]:[height]\n"
    "  -A    Audio port.\n"
    "  -V    Video port.\n"
    "  -b    Display incoming bandwidth in the top-right of the video display.\n"
    "  -n    No custom color rendering.\n"
    "  -e    Print stderr (which is by default routed to /dev/null).\n"
    "\n"
    "Report bugs to https://github.com/mofarrell/p2pvc/issues.\n"
  );
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(stderr);
    exit(1);
  }

  char *peer = argv[1];
  /* Check if the user actually wanted help. */
  if (!strncmp(peer, "-h", 2)) {
    usage(stdout);
    exit(0);
  }
  char *audio_port = "55555";
  char *video_port = "55556";
  vid_options_t vopt;
  int spawn_video = 0, print_error = 0;
  int c;
  int width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT;

  setlocale(LC_ALL, "");

  memset(&vopt, 0, sizeof(vid_options_t));
  vopt.width = DEFAULT_WIDTH;
  vopt.height = DEFAULT_HEIGHT;
  vopt.render_type = 0;
  vopt.color_enabled = 1;
  vopt.refresh_rate = 20;

  while ((c = getopt (argc - 1, &(argv[1]), "bvnd:A:V:heBr:")) != -1) {
    switch (c) {
      case 'v':
        spawn_video = 1;
        break;
      case 'A':
        audio_port = optarg;
        break;
      case 'V':
        video_port = optarg;
        break;
      case 'd':
        get_dimensions(optarg, &width, &height);
        vopt.width = width;
        vopt.height = height;
        break;
      case 'b':
        vopt.disp_bandwidth = 1;
        break;
      case 'r':
        vopt.refresh_rate = atol(optarg);
        break;
      case 'B':
        vopt.render_type = 1;
        break;
      case 'n':
        vopt.color_enabled = 0;
        break;
      case 'h':
        usage(stdout);
        exit(0);
        break;
      case 'e':
        print_error = 1;
        break;
      default:
        break;
    }
  }

  if (!print_error) {
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, STDERR_FILENO);
  }

  if (spawn_video) {
    signal(SIGINT, all_shutdown);
    pthread_t thr;
    network_options_t netopts;
    netopts.ipaddr = peer;
    netopts.port = audio_port;
    pthread_create(&thr, NULL, spawn_audio_thread, (void *)&netopts);
    start_video(peer, video_port, &vopt);
  } else {
    signal(SIGINT, audio_shutdown);
    start_audio(peer, audio_port);
  }

  return 0;
}

