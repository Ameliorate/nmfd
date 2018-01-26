#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <linux/input.h>
#include <unistd.h>

#define DEVICES_COUNT 8
#define OFFSET 262

void rd_error(void);

const char *devices[DEVICES_COUNT][3] = {
        {"Naga Epic",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Epic-if01-event-kbd",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Epic-event-mouse"},
        {"Naga Epic Dock",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-if01-event-kbd",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Dock-event-mouse"},
        {"Naga 2014",
         "/dev/input/by-id/usb-Razer_Razer_Naga_2014-if02-event-kbd",
         "/dev/input/by-id/usb-Razer_Razer_Naga_2014-event-mouse"},
        {"Naga Molten",
         "/dev/input/by-id/usb-Razer_Razer_Naga-if01-event-kbd",
         "/dev/input/by-id/usb-Razer_Razer_Naga-event-mouse"},
        {"Naga Epic Chroma",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-if01-event-kbd",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma-event-mouse"},
        {"Naga Epic Chroma Dock",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-if01-event-kbd",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Epic_Chroma_Dock-event-mouse"},
        {"Naga Chroma",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-if02-event-kbd",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Chroma-event-mouse"},
        {"Naga Hex",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Hex-if01-event-kbd",
         "/dev/input/by-id/usb-Razer_Razer_Naga_Hex-event-mouse"},

};

int main(int argc, char **argv)
{
        int help = (argc == 2) && (!strcmp(argv[1], "--help")
                                 || !strcmp(argv[1], "-h"));
        if (help) {
                printf("USAGE: %s [-h] [-n]\n", argv[0]);
                puts("nmfd acts as a driver for Razer Naga mice.");
                puts("-n --no_grab: Don't grab the keypad.");
                puts("See `man nmfd` for more details");
                return 0;
        }
        
        int grab = !(argc == 2 && (!strcmp(argv[1], "--no_grab") || !strcmp(argv[1], "-n")));

        int sidefd, extrafd = -1;
        for (int i = 0; i < DEVICES_COUNT; i++) {
                if ((sidefd = open(devices[i][1], O_RDONLY)) != -1 && (extrafd = open(devices[i][2], O_RDONLY)) != -1) {
                        fprintf(stderr, "Using device files for %s\n", devices[i][0]);
                        break;
                }
        }
        if (sidefd == -1 || extrafd == -1) {
                fputs("No naga devices found or you don't have permission to access them.\n", stderr);
                exit(1);
        }

        if (grab) {
                // Give application exclusive control over side buttons.
                ioctl(sidefd, EVIOCGRAB, 1);
        }

        while (1) {
                int rd, rd1, rd2;
                fd_set readset;
                int button_num;
                char sign;
                struct input_event ev1[64], ev2[64];

                FD_ZERO(&readset);
                FD_SET(sidefd, &readset);
                FD_SET(extrafd, &readset);
                rd = select(FD_SETSIZE, &readset, NULL, NULL, NULL);

                if (rd == -1) {
                        printf("Unknown Error: %m\n");
                        exit(2);        // Only programming errors here.
                }

                if (FD_ISSET(sidefd, &readset)) {       // Side buttons
                        rd1 = read(sidefd, ev1, sizeof(ev1));
                        if (rd1 == -1)
                                rd_error();
                        if (ev1[1].value > 1 || ev1[1].value < 0)       // Irrelevant stuff
                                goto unimportant;
                        if (ev1[1].code == 0) { // Button is held.
                                sign = ':';
                        } else {
                                button_num = ev1[1].code - 1;
                                sign = ev1[1].value ? '+' : '-';
                        }

                } else if (FD_ISSET(extrafd, &readset)) {       // Extra Buttons
                        rd2 = read(extrafd, ev2, sizeof(ev2));
                        if (rd2 == -1)
                                rd_error();
                        if (ev2[1].value > 1 || ev2[1].code < OFFSET || ev2[1].code < (OFFSET + 13) || ev2[1].value < 0)        // Irrelevant stuff
                                goto unimportant;
                        button_num = ev2[1].code - OFFSET;
                        sign = ev2[1].value ? '+' : '-';

                }

                printf("%c%d\n", sign, button_num);
 unimportant:
                continue;
        }

        return 0;
}

void rd_error(void)
{
        fprintf(stderr, "Error: %m\n");
        exit(2); 
        // In the future we might try to restart, in case of the condition that the mouse was unplugged for a short time.
        // For now exiting is good.
}
