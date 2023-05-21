#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include "lcd.h"
#include "utils.h"

#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"

#define TASK_BLINK          100
#define TASK_LED_ON         101
#define TASK_LED_OFF        102
#define TASK_DISPLAY_CLEAR  103
#define TASK_DISPLAY_TEXT   104
#define TASK_DISPLAY_BANNER 105

#define MAX_ARGUMENT 20
#define TEMPERATURE_UNITS 'C'

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

static char * VERSION [] = {"picoOS",
                           "0.0.2",
                           "2023-05-21"};

typedef struct
{
    uint32_t task ;
    uint32_t data;
    void * buffer;
} queue_entry_t;

queue_t task_queue;

/*
*/
void limpiarTarea(uint32_t task){
    bool fin = false;
    uint32_t taskAnt = 0;
    if (not(queue_is_empty(&task_queue))){
        while(not(fin)){
            queue_entry_t entry;
            if (queue_try_remove(&task_queue,&entry)){
                if (entry.task != task){
                    queue_add_blocking(&task_queue, &entry);
                    if (taskAnt == 0){
                        taskAnt = entry.task;
                    }
                    else 
                    if (taskAnt == entry.task){
                        fin = true;
                    }
                }
                else {
                    fin = true;
                }
            }
            else {
                fin = true;
            }
        }
    }
}
/*
*/
void addtask(const uint32_t task, const uint32_t data, void * buffer){
    queue_entry_t result;
    result.task = task;
    result.data = data;
    result.buffer = buffer;
    queue_add_blocking(&task_queue, &result);
}
/*
*/
void task_blink(queue_entry_t entry){
    uint32_t rept = entry.data;
    rept--;
    if (rept > 0){
        addtask(TASK_BLINK, rept, NULL);
    }
    gpio_put(LED_PIN, 1);
    sleep_ms(250);
    gpio_put(LED_PIN, 0);
    sleep_ms(250);

}
/*
*/
void taskDisplayText(queue_entry_t entry){
    char * text = entry.buffer;
    char str[MAX_LEN_COMMAND];
    bzero(str,sizeof(str));
    lcd_clear();
    int k = 0;
    int line = 0;
    int pos = 0;
    for(int i = 0; i < strlen(text);i++){
        if (text[i] == ' '){
            if (pos + strlen(str) > MAX_CHARS){
                line++;
                pos = 0;
            }
            if (line < MAX_LINES){
                lcd_set_cursor(line,pos);
                lcd_string(str);
            }
            pos += strlen(str) + 1;
            bzero(str,sizeof(str));
            k = 0;
        }
        else {
            str[k] = text[i];
            k++;
        }
    }
    free(text);
}
/*
*/
void core1_entry() {
    while(true){
        queue_entry_t entry;
        queue_remove_blocking(&task_queue, &entry);
        switch(entry.task){
            case TASK_BLINK:
                task_blink(entry);
                break;
            case TASK_LED_ON:
                gpio_put(LED_PIN, 1);
                break;
            case TASK_LED_OFF:
                gpio_put(LED_PIN, 0);
                break;
            case TASK_DISPLAY_CLEAR:
                lcd_clear();
                break;
            case TASK_DISPLAY_TEXT:
                taskDisplayText(entry);
                break;
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
    printf("* display\n");
    printf("* temp\n");
    printf("* version\n");
    printf("* help\n");
    printf("* clear\n");
}
/*
*/
void version(){
    for (int i = 0; i < sizeof(VERSION) / sizeof(VERSION[0]);i++){
        printf("%s ", VERSION[i]);
    }
    printf("\n");
    printf("Escrito por Jairo Grateron\n");
}

/*
*/
void ledon(){
    limpiarTarea(TASK_BLINK);
    addtask(TASK_LED_ON, 0,NULL);
}
/*
*/
void ledoff(){
    limpiarTarea(TASK_BLINK);
    addtask(TASK_LED_OFF, 0,NULL);
}
/*
*/
void blink(char args[][MAX_ARGUMENT], const int cantArgs){
    bool error = false;
    int rept = 10;
    if (cantArgs > 1){
        if (isnumber(args[1])){
            rept = atoi(args[1]);
        }else {
            error = true;
        }
    }
    if (error){
        printf("argumento incorrecto para la opcion blink\n");
    }
    else {
        limpiarTarea(TASK_BLINK);
        addtask(TASK_BLINK, rept,NULL);
    }
}
/*
*/
void splitArgs(const char * command, char args[][MAX_ARGUMENT], int * cant){
    int i,n,j,z;
    i = 0;
    n = 0;
    j = 0;
    z = 0;
    while (command[i] != '\0'){
        if (command[i] == ' '){
            if (z > 0){
                n++;
            }
            j = 0;
            z = 0;
        }
        else {
            args[n][j] = command[i];
            j++;
            z++;
        }
        i++;
    }
    if (z > 0){
        n++;
    }
    *cant = n;
}
/*
*/
void display(char args[][MAX_ARGUMENT], const int cantArgs){
    char * displayHelp [] = {"display clear", "display text ...", "display banner ..."};
    bool error = false;
    if (cantArgs < 2){
        error = true;
    }
    else {
        if (strequal("clear",args[1])){
            addtask(TASK_DISPLAY_CLEAR, 0,NULL);
        }
        else
        if (strequal("text",args[1])){
            if (cantArgs > 2){
                char * text;
                text = malloc(MAX_LEN_COMMAND);
                bzero(text,MAX_LEN_COMMAND);
                for (int i = 0; i < cantArgs -2;i++){
                    strcat(text,args[2+i]);
                    strcat(text," ");
                }
                addtask(TASK_DISPLAY_TEXT,0, text);
            }
            else{
                error = true;
            }
        }
        else {
            error = true;
        }
    }
    if (error){
        for (int i = 0; i < sizeof(displayHelp) / sizeof(displayHelp[0]); i++){
            printf("%s\n",displayHelp[i]);
        }
    }
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
/*
*/
void clearScreen(){
    printf("\033[2J");
}
/*
*/
void initLcd(){
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    lcd_init();

    int pos = 0;
    int line = 0;
    for (int i = 0; i < sizeof(VERSION)/sizeof(VERSION[0]);i++){
        if (pos + strlen(VERSION[i]) > MAX_CHARS){
            line++;
            pos = 0;
        }
        if (line < MAX_LINES){
            lcd_set_cursor(line,pos);
            lcd_string(VERSION[i]);
            pos += strlen(VERSION[i]) + 1;
        }
    }
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
    /*init lcd*/
    initLcd();
    while (true){
        printf("root@picoOS: ");
        char command[MAX_LEN_COMMAND + 1];
        bzero(command, sizeof(command));
        readcommand(command);
        sleep_ms(100);
        char argumentos[MAX_LEN_COMMAND][MAX_ARGUMENT];
        int cantArgs;
        bzero(argumentos,sizeof(argumentos));
        splitArgs(command,argumentos,&cantArgs);
        if (cantArgs > 0){
            if (strequal("help",argumentos[0])){
                help();
            }
            else
            if (strequal("ledon",argumentos[0])){
                ledon();
            }
            else
            if (strequal("ledoff",argumentos[0])){
                ledoff();
            }
            else
            if (strequal("blink",argumentos[0])){
                blink(argumentos,cantArgs);
            }
            else
            if (strequal("display",argumentos[0])){
                display(argumentos,cantArgs);
            }
            else
            if(strequal("temp",argumentos[0])){
                temperature();
            }
            else
            if (strequal("version",argumentos[0])){
                version();            
            }
            else
            if (strequal("clear",argumentos[0])){
                clearScreen();            
            }
            else {
                printf("%s: no econtrada\n",argumentos[0]);
            }
        }
    }
}
