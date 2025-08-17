/* 
 joycart.c ver. 0.1 alpha
 Prosty program testuje joystick ArSoft joycart by larek z interfrejsem pc/rpi | (the c64 - nie sprawdzone) 
 Pozdrowienia dla larka i całej bandy Retro w Polsce ;-)
 Autor: jms@data.pl, bofh@retro-technology.pl
 
 Uruchamiamy z konsoli lub termianala Linux 
 Uwaga: Progarm loguje do pliku joystick_log.txt 
 Kompilacja: gcc -Wall -o joycart joycart.c 

 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <time.h>

#define JOYSTICK_DEV "/dev/input/js1" // Domyślny plik urządzenia joysticka, zmieniamy jeśli program nie działa przed kompilacją
#define AF_BUTTON 3 // Przycisk odpowiedzialny za autofire

#define RESET   "\033[0m"
#define WHITE   "\033[97m"
#define RED     "\033[91m"

int debug_mode = 0;
FILE *log_file = NULL;

const char* map_button(int button) {
    switch (button) {
        case 0: return "Fire (Góra)";
        case 1: return "F2 (Dolny rząd)";
        case 2: return "F3 (Dolny rząd)";
        case 3: return "AF (Dolny rząd)";
        case 4: return "UP (Góra)";
        case 5: return "Start (Górny interfejs)";
        case 6: return "Select (Górny interfejs)";
        case 7: return "System (Górny interfejs)";
        case 8: return "Coin (Górny interfejs)";
        default: return "Nieznany przycisk";
    }
}

void log_event(const char* msg) {
    if (log_file) {
        fprintf(log_file, "%s\n", msg);
        fflush(log_file);
    }
}

void test_joystick(int js_fd) {
    struct js_event jse;
    int x_axis = 0, y_axis = 0;
    int button_states[32] = {0};
    struct timespec last_autofire_time = {0};

    fcntl(js_fd, F_SETFL, O_NONBLOCK); // Ustawienie odczytu jako nieblokującego

    while (1) {
        ssize_t bytes = read(js_fd, &jse, sizeof(struct js_event));
        if (bytes > 0) {
            jse.type &= ~JS_EVENT_INIT; // Ignoruj zdarzenia inicjalizacji

            if (jse.type == JS_EVENT_AXIS) {
                if (jse.number == 0) {
                    x_axis = jse.value;
                } else if (jse.number == 1) {
                    y_axis = jse.value;
                }

                if (x_axis > 10000 && y_axis < -10000)
                    printf("Ruch: prawo-góra\n");
                else if (x_axis < -10000 && y_axis < -10000)
                    printf("Ruch: lewo-góra\n");
                else if (x_axis > 10000 && y_axis > 10000)
                    printf("Ruch: prawo-dół\n");
                else if (x_axis < -10000 && y_axis > 10000)
                    printf("Ruch: lewo-dół\n");
                else if (x_axis > 10000)
                    printf("Ruch w prawo\n");
                else if (x_axis < -10000)
                    printf("Ruch w lewo\n");
                else if (y_axis > 10000)
                    printf("Ruch w dół\n");
                else if (y_axis < -10000)
                    printf("Ruch w górę\n");
            } else if (jse.type == JS_EVENT_BUTTON) {
                button_states[jse.number] = jse.value;

                const char* name = map_button(jse.number);
                if (jse.value)
                    printf("%s wciśnięty!\n", name);
                else
                    printf("%s zwolniony!\n", name);

                if (debug_mode) {
                    fprintf(stderr, "DEBUG: button %d (%s) = %d\n", jse.number, name, jse.value);
                }

                char buf[64];
                snprintf(buf, sizeof(buf), "%s = %d", name, jse.value);
                log_event(buf);
            }
        }

        // Autofire obsługa
        if (button_states[AF_BUTTON]) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            long diff = (now.tv_sec - last_autofire_time.tv_sec) * 1000 +
                       (now.tv_nsec - last_autofire_time.tv_nsec) / 1000000;

            if (diff > 100) {
                printf(RED "[AF] Autofire!\n" RESET);
                log_event("[AF] Autofire!");
                last_autofire_time = now;
            }
        }

        usleep(10000); // małe opóźnienie, żeby nie przeciążać CPU
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "--debug") == 0) {
        debug_mode = 1;
    }

    log_file = fopen("joystick_log.txt", "w");
    if (!log_file) {
        perror("Nie można otworzyć pliku logu");
        return 1;
    }

    int js_fd = open(JOYSTICK_DEV, O_RDONLY);
    if (js_fd == -1) {
        perror("Nie udało się otworzyć urządzenia joysticka");
        return 1;
    }

    char name[128];
    if (ioctl(js_fd, JSIOCGNAME(sizeof(name)), name) < 0) {
        strcpy(name, "Nieznane urządzenie");
    }

    printf(WHITE "Nazwa joysticka: %s\n" RESET, name);
    printf(WHITE "Proszę zacząć naciskać przyciski i kierunki.\n" RESET);
    printf("joycart.c ver. 0.1 alpha, autor - jms@data.pl, bofh@retro-technology.pl - poprawki Autor i AI \n");	
    
    for (int i = 0; i < 6; i++) {
        if (i % 2 == 0)
            printf(WHITE "\rTestowanie joysticka...     " RESET);
        else
            printf("\r                          ");
        fflush(stdout);
        usleep(500000);
    }
    printf(WHITE "\rTestowanie joysticka...\n" RESET);

    test_joystick(js_fd);
    close(js_fd);
    fclose(log_file);
    return 0;
}
