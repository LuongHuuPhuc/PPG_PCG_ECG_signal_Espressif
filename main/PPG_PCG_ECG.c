#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "esp_err.h"
#include "sensor_init.h"

/***Task handle global variables */
TaskHandle_t readMAXTask_handle = NULL;
TaskHandle_t readINMP441_handle = NULL;
TaskHandle_t readADTask_handle = NULL;
TaskHandle_t printData_handle = NULL;

void app_main(void){
  ESP_ERROR_CHECK(i2cdev_init()); 
  max30102_configure();
  ad8232_configure();
  inmp441_configure();
  mutex_init();

  xTaskCreatePinnedToCore(readMAX30102_task, "readmax30102", 1024 * 5,NULL, 5, &readMAXTask_handle, 1);
  xTaskCreatePinnedToCore(readINMP441_task, "readINMP441", 1024 * 15, NULL, 5, &readINMP441_handle, 0);
  xTaskCreatePinnedToCore(readAD8232_task, "readAD8232", 1024 * 4, NULL, 5, &readADTask_handle, 1);
  xTaskCreatePinnedToCore(printData_task, "printData", 2048, NULL, 6, &printData_handle, 1);
}