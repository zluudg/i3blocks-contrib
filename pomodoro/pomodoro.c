#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>

#define BUF_SIZE 2000

#define PURPLE "#9988FF"
#define ORANGE "#FFA500"

#define IDLE "\xE2\x97\x8F"
#define LOCKED "\xE2\x97\x89"
#define ACTIVATED "\xE2\x97\xAF"

#define s_IDLE      0
#define s_LONGWAIT  1
#define s_SHORTWAIT 2
#define s_NOTIFYING 3

#define CLICK_RIGHT 3
#define CLICK_LEFT  1

typedef struct {
    pthread_t tid;
    int timer;
    int fd;
} thread_info;

char *FMT_STRING =
"{\"full_text\": "
    "\"<span fallback='true' color='%s'>%s  </span>"
    "<span fallback='true' color='%s'>%s</span>\""
"}\n";

int pipefd[2];

void display(int state)
{
    int ret;

    switch (state)
    {
        case s_IDLE:
            printf(FMT_STRING, PURPLE, IDLE, ORANGE, IDLE);
            fflush(stdout);
        break;

        case s_LONGWAIT:
            printf(FMT_STRING, PURPLE, LOCKED, ORANGE, ACTIVATED);
            fflush(stdout);
        break;

        case s_SHORTWAIT:
            printf(FMT_STRING, PURPLE, ACTIVATED, ORANGE, LOCKED);
            fflush(stdout);
        break;

        case s_NOTIFYING:
            printf(FMT_STRING, PURPLE, ACTIVATED, ORANGE, ACTIVATED);
            fflush(stdout);
            ret = system("i3-nagbar -t warning -m \"Pomodoro!\" &> /dev/null");
            (void) ret;
        break;

        default: // should not happen
            exit(1);
        break;
    }
}

void *timer(void *arg)
{
    int ret;

    thread_info *tinfo = arg;

    sleep(tinfo->timer*60);

    ret = write(tinfo->fd, "!", 1);
    (void) ret;

    pthread_exit(NULL);
}

void spawn_thread(int timeout, thread_info *tinfo)
{
    pthread_attr_t attr;

    if (pthread_attr_init(&attr) != 0)
        exit(1);

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
        exit(1);

    tinfo->timer = timeout;

    if (pthread_create(&tinfo->tid, &attr, &timer, tinfo) != 0)
        exit(1);

    if (pthread_attr_destroy(&attr) != 0)
        exit(1);
}

int is_longwait(char *buff)
{
    json_object *click;
    json_object *width;
    json_object *xpos;
    int x = 0, w = 0;

    click = json_tokener_parse(buff);
    width = json_object_object_get(click, "width");
    xpos  = json_object_object_get(click, "relative_x");

    w = json_object_get_int(width);
    x = json_object_get_int(xpos);

    return x >= w/2;
}

int is_click(char *buff, int clicktype)
{
    json_object *click;
    json_object *button_json;
    int button;

    click = json_tokener_parse(buff);
    button_json = json_object_object_get(click, "button");
    button = json_object_get_int(button_json);

    return button==clicktype;
}

int main()
{
    int ret_poll; ssize_t ret_read;
    char buff[BUF_SIZE];
    int state = s_IDLE;
    thread_info tinfo;
    int active_fd = -1;
    struct pollfd input[2] = {{fd: STDIN_FILENO, events: POLLIN},
                              {fd: -1, events: POLLIN}};

    if (pipe(pipefd) == -1)
        exit(1);

    input[1].fd = pipefd[0]; // poll read end
    tinfo.fd = pipefd[1]; // pass write end to timer thread

    display(state);

    while(1) {
        ret_poll = poll(input, 2, -1);

        if (ret_poll < 0)
        {
            exit(1);
        }

        if (input[0].revents != 0) // click event from stdin
        {
            active_fd = input[0].fd;
        }
        else if (input[1].revents != 0) // event from timer thread's fd
        {
            active_fd = input[1].fd;
        }

        ret_read = read(active_fd, buff, BUF_SIZE-1);

        if (ret_read <= 0)
        {
            exit(1);
        }

        switch (state)
        {
            case s_IDLE:
                if (!is_click(buff, CLICK_LEFT)) {
                    // Do nothing
                }
                else if (is_longwait(buff)) // clicked to the left
                {
                    state = s_LONGWAIT;
                    spawn_thread(25, &tinfo);
                }
                else // clicked to the right
                {
                    state = s_SHORTWAIT;
                    spawn_thread(5, &tinfo);
                }
            break;

            case s_LONGWAIT:
                if (active_fd == input[1].fd) // Timer event
                {
                    state = s_NOTIFYING;
                }
                else if (active_fd == input[0].fd) // Click to reset event
                {
                    if (is_click(buff, CLICK_RIGHT))
                    {
                        pthread_cancel(tinfo.tid);
                        state = s_IDLE;
                    }
                }
                else
                {
                    exit(1);
                }
            break;

            case s_SHORTWAIT:
                if (active_fd == input[1].fd) // Timer event
                {
                    state = s_NOTIFYING;
                }
                else if (active_fd == input[0].fd) // Click to reset event
                {
                    if (is_click(buff, CLICK_RIGHT))
                    {
                        pthread_cancel(tinfo.tid);
                        state = s_IDLE;
                    }
                }
                else
                {
                    exit(1);
                }
            break;

            case s_NOTIFYING:
                state = s_IDLE;
            break;

            default: // should not happen
                exit(1);
            break;
        }

        display(state);
    }

    exit(0);
}
