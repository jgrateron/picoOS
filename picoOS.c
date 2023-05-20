#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/adc.h"

#define TASK_BLINK   100
#define TASK_LED_ON  101
#define TASK_LED_OFF 102
#define MAX_LEN_COMMAND 20
#define TEMPERATURE_UNITS 'C'

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

typedef struct
{
    uint32_t task ;
    uint32_t data;
} queue_entry_t;

queue_t task_queue;

/*
*/
void core1_entry() {
    while(true){
        queue_entry_t entry;
        queue_remove_blocking(&task_queue, &entry);
        if (entry.task == TASK_BLINK){
            uint32_t rept = entry.data;
            rept--;
            if (rept > 0){
                queue_entry_t result;
                result.task = TASK_BLINK;
                result.data = rept;
                queue_add_blocking(&task_queue, &result);
            }
            gpio_put(LED_PIN, 1);
            sleep_ms(250);
            gpio_put(LED_PIN, 0);
            sleep_ms(250);
        }
        if (entry.task == TASK_LED_ON){
            gpio_put(LED_PIN, 1);
        }
        if (entry.task == TASK_LED_OFF){
            gpio_put(LED_PIN, 0);
        }
    }
}

/*
*/

void help(){
    printf("Ayuda del sistema\n");
    printf("Comandos disponibles\n");
    printf("* ledon\n");
    printf("* ledoff\n");
    printf("* blink\n");
    printf("* temp\n");
    printf("* version\n");
    printf("* help\n");
    printf("* clear\n");
}
/*
*/
void version(){
    printf("picoOS 0.0.1 2023-05-20\n");
    printf("Escrito por Jairo Grateron\n");
}
/*
*/
void limpiarColaTareas(){
    while(queue_is_empty(&task_queue) == false){
        queue_entry_t entry;
        queue_try_remove(&task_queue,&entry);
    }
}
/*
*/
void blink(const char * command){

    bool error = false;
    bool arg = false;
    char repeticiones[MAX_LEN_COMMAND];

    bzero(repeticiones, sizeof(repeticiones));
    if (strlen(command) > 5){
        int i,k,j;
        arg = true;
        i = 0;
        k = 0;
        j = 0;
        while (command[i] != '\0' && error == false){
            if (j == 1){
                if (command[i] >= 48 && command[i] <= 57){
                    repeticiones[k] = command[i];
                    k++;
                }
                else {
                    error = true;
                }
            }
            if (command[i] == ' '){
                j = 1;
            }
            i++;
        }
    }
    int rept = 10;
    if (arg){
        if (strlen(repeticiones) == 0){
            error = true;
        }
        else {
            rept = atoi(repeticiones);
        }
    }
    if (error){
        printf("argumento incorrecto para la opcion blink\n");
    }
    else {
        limpiarColaTareas();
        queue_entry_t entry;
        entry.task = TASK_BLINK;
        entry.data = rept;
        queue_add_blocking(&task_queue, &entry);
    }
}
/*
*/
void ledon(){
    limpiarColaTareas();
    queue_entry_t entry;
    entry.task = TASK_LED_ON;
    entry.data = 0;
    queue_add_blocking(&task_queue, &entry);
}
/*
*/
void ledoff(){
    limpiarColaTareas();
    queue_entry_t entry;
    entry.task = TASK_LED_OFF;
    entry.data = 0;
    queue_add_blocking(&task_queue, &entry);
}
/*
*/
float read_onboard_temperature(const char unit) {
    
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    if (unit == 'C') {
        return tempC;
    } else if (unit == 'F') {
        return tempC * 9 / 5 + 32;
    }

    return -1.0f;
}
/*
*/
void temperature(){
    float temperature = read_onboard_temperature(TEMPERATURE_UNITS);
    printf("Onboard temperature = %.02f %c\n", temperature, TEMPERATURE_UNITS);
}

void clear(){
    printf("\033[2J");
}
/*
*/
int main()
{
    stdio_init_all();
    
    //iniciamos el otro nucleo para tareas de fondo
    queue_init(&task_queue, sizeof(queue_entry_t), 10);
    multicore_launch_core1(core1_entry);
    
    /*init led*/
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    /*init adc*/
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    while (true){
        printf("root@picoOs: ");
        char c;
        char command[MAX_LEN_COMMAND + 1];
        bzero(command, sizeof(command));
        int pos = 0;
        while(true){
            c = getchar();
            if (c == 13){
                printf("\n");
                break;
            }
            if (c == 127){
                if(strlen(command) > 0){
                    printf("\033[1D");
                    printf(" ");
                    printf("\033[1D");
                    command[strlen(command) -1] = '\0';
                    pos--;
                }
            }
            if (c >= 32 && c <= 125){
                printf("%c", c);
                command [pos] = c;
                pos++;
                if (pos == MAX_LEN_COMMAND){
                    printf("\n");
                    break;
                }
            }
        }
        sleep_ms(100);
        if (strcmp("help",command) == 0){
            help();
        }
        else
        if (strcmp("ledon",command) == 0){
           ledon();
        }
        else
        if (strcmp("ledoff",command) == 0){
            ledoff();
        }
        else
        if ((strncmp("blink",command,5) == 0 && strlen(command) == 5) || (strncmp("blink ",command, 6) == 0)){
            blink(command);
        }
        else
        if(strcmp("temp",command) == 0){
            temperature();
        }
        else
        if (strcmp("version",command) == 0){
            version();            
        }
        else
        if (strcmp("clear",command) == 0){
            clear();            
        }
        else {
            if (strlen(command) > 0){
                printf("%s: no econtrada\n",command);
            }
        }
    }
}
