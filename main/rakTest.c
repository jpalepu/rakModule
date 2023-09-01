#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

#define UART_NUM UART_NUM_1
#define TXD_PIN GPIO_NUM_1
#define RXD_PIN GPIO_NUM_3
#define BUF_SIZE (1024)
#define COMMAND_QUEUE_SIZE 10

QueueHandle_t command_queue;

void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, BUF_SIZE, 0, 0, NULL, 0);
}



void send_at_command(const char* command) {
    uart_write_bytes(UART_NUM, command, strlen(command));
    uart_write_bytes(UART_NUM, "\r\n", 2); // Send carriage return and newline
}

void at_command_task(void* pvParameter) {
     char uart_data[BUF_SIZE]; 
    while (1) {
        char* command;
        if (xQueueReceive(command_queue, &command, portMAX_DELAY)) {
            send_at_command(command);
            vTaskDelay(pdMS_TO_TICKS(1000)); 

            int len = uart_read_bytes(UART_NUM, uart_data, BUF_SIZE, pdMS_TO_TICKS(5000)); 
            if (len > 0) {
                uart_data[len] = '\0';
                printf("Response: %s\n", uart_data);
            }
        }
    }
}

void app_main() {
    uart_init();
    command_queue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(char*));

    xTaskCreate(at_command_task, "at_command_task", 2048, NULL, 5, NULL);

    // Example: Sending AT commands
    while (1) {
        const char* at_commands[] = {
            "AT\r\n",
            "AT+GMR\r\n",
            // we can add more and mroe at commands here. 
        };

        for (int i = 0; i < sizeof(at_commands) / sizeof(at_commands[0]); i++) {
            char* command = strdup(at_commands[i]);
            xQueueSend(command_queue, &command, portMAX_DELAY);
        }

        // delay for certain time interval before sending commands again
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
