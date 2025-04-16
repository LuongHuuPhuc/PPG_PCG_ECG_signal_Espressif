/**
 * @brief Thu vien de dinh nghia cac ham thuc thi & khoi tao cua PPG - PCG - ECG
 * @author Luong Huu Phuc
 */
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "esp_log.h"
#include "esp_err.h"
#include "sensor_init.h"

/****Thu vien cho I2S****/
#include "driver/i2s_std.h"
#include "driver/i2c_master.h"
#include "driver/i2s_types.h"
#include "driver/i2s_common.h"
#include "driver/i2s_types_legacy.h"

/** Thu vien cho I2C */
#include <string.h>
#include <esp_timer.h>
#include "driver/i2c.h"
#include "driver/i2c_types.h"
#include "driver/gpio.h"
#include "max30102.h"

/** Thu vien cho ADC  */
#include "driver/adc.h"
#include "driver/ledc.h"

/** Global variables for INMP441 & MAX30102 & AD8232 */
volatile unsigned long global_red = 0;
volatile unsigned long global_ir = 0;
volatile int global_adc_value = 0;
volatile int16_t global_inmp441_data = 0;

/** Global mutex variables */
SemaphoreHandle_t print_mutex = NULL; 

/**** INMP441 ****/
i2s_chan_handle_t rx_channel = NULL; //Tao kenh RX
int32_t buffer32[DMA_BUFFER_SIZE / sizeof(int32_t)] = {0}; //Luu 768 / 4 = 192 mau trong 6ms
int16_t buffer16[DMA_BUFFER_SIZE / sizeof(int32_t) * 3 / 2]=  {0}; //192 mau 

/**** MAX30102 ****/
i2c_dev_t dev;  
struct max30102_record record;
// unsigned long red, ir;

static const char *TAG1 = "INMP441";
static const char *TAG2 = "MAX30102";
static const char *TAG3 = "AD8232";

/** INMP441 configure */
void inmp441_configure(void){
  i2s_chan_config_t i2s_conf = {
    .dma_desc_num = dmaDesc, 
    .dma_frame_num = dmaLength,//128 bytes
    .id = I2S_PORT,
    .role = I2S_ROLE_MASTER,
    .auto_clear = true,
  };

  //Khoi tao RX va kiem tra loi
  ESP_ERROR_CHECK(i2s_new_channel(&i2s_conf, NULL, &rx_channel));

  //Cau hinh i2s che do chuan 
  i2s_std_config_t std_conf = {
    .clk_cfg = {
      .sample_rate_hz = SAMPLE_RATE,
      .clk_src = I2S_CLK_SRC_DEFAULT,
      .mclk_multiple = I2S_MCLK_MULTIPLE_1152//Cang cao thi jitter(nhieu) cua BLCK va LRCL cang it  
    },

    //Cau hinh du lieu trong 1 frame
    .slot_cfg = {
      .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT, //So bit dau vao la 32-bit
      .slot_mask = I2S_STD_SLOT_LEFT, //Kenh trai
      .slot_mode = I2S_SLOT_MODE_MONO, //1 Kenh
      .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT, //So bit moi kenh
    },

    //Cau hinh GPIO
    .gpio_cfg = {
      .bclk = BCLK_PIN,
      .din = DIN_PIN,
      .ws = LRCL_PIN,
      .dout = I2S_PIN_NO_CHANGE,
    },
  };

  //Khoi tao che do chuan 
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_channel, &std_conf));
  //Bat kenh RX
  ESP_ERROR_CHECK(i2s_channel_enable(rx_channel));
  ESP_LOGI(TAG1, "INMP441 da duoc cau hinh thanh cong !!!");
}

/** AD8232 configure */
void ad8232_configure(void){
  adc1_config_width(ADC_WIDTH);
  adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN); //Suy hao
  ESP_LOGI(TAG3, "ADC Configured: Channel: %d, Attenuation: %d", ADC_CHANNEL, ADC_ATTEN);
}

/** MAX30102 configure */
void max30102_configure(void){
  memset(&dev, 0, sizeof(i2c_dev_t));
  ESP_ERROR_CHECK(max30102_initDesc(&dev, 0, I2C_SDA_GPIO, I2C_SCL_GPIO));
  if(max30102_readPartID(&dev) == ESP_OK) {
    ESP_LOGI(TAG2, "Found MAX30102!");
  }
  else {
    ESP_LOGE(TAG2, "Not found MAX30102");
  }
  ESP_ERROR_CHECK(max30102_init(powerLed, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange, &record, &dev));
  max30102_clearFIFO(&dev);
}

/** muxtex init */
void mutex_init(void){
  //Khoi tao Semaphore
  print_mutex = xSemaphoreCreateMutex();  
  if(print_mutex == NULL){
    ESP_LOGE("MAIN", "Khong the khoi tao Mutex");
    return;
  }
}

/** MAX30102 task */
void readMAX30102_task(void *pvParameter){
  ESP_LOGI(TAG2, "Bat dau doc cam bien AD8232");

  while(1){
    vTaskDelay(1);
    max30102_check(&record, &dev); //Check the sensor, read up to 3 samples
    while (max30102_available(&record)){
      global_red = max30102_getFIFORed(&record);
      global_ir = max30102_getFIFOIR(&record);
      max30102_nextSample(&record);
    }
  }
}

/** INMP441 task */
void readINMP441_task(void *pvParameter){
  ESP_LOGI(TAG1, "Bat dau doc data tu INMP441...");
  size_t bytes_read = 0;

  while(true){
    vTaskDelay(1); //Neu de delay la pdMS_TO_TICKS(1) thi sau n mau se bi watchdog trigger (cpu nghi bi treo he thong)
    esp_err_t ret = i2s_channel_read(rx_channel, &buffer32, sizeof(buffer32), &bytes_read, portMAX_DELAY);
    if(ret == ESP_ERR_TIMEOUT){
      ESP_LOGE(TAG2, "Timeout error, bo qua frame loi... %s", esp_err_to_name(ret));
      continue;
    }else if(ret != ESP_OK){
      ESP_LOGE(TAG2, "Loi khong xac dinh... %s", esp_err_to_name(ret));
      break;
    }

    int samplesRead = bytes_read / sizeof(int32_t); //So mau doc tren 1 kenh la du lieu 32-bit
    for(size_t i = 0; i < samplesRead; i++){
      buffer16[i] = (int16_t)(buffer32[i] >> 8); 
      global_inmp441_data = buffer16[i];
      // printf("\n%ld", buffer17[i]);
    }
  }
}

/** AD8232 task */
void readAD8232_task(void *pvParameter){
  ESP_LOGI(TAG3, "Bat dau doc cam bien AD8232");

  while(true){
    vTaskDelay(1);
    global_adc_value = (adc1_get_raw(ADC_CHANNEL));
    vTaskDelay(pdMS_TO_TICKS(1000 / ADC_SAMPLE_RATE)); //5ms - Tan so lay mau cua ADC duoc the hien qua ham nay
  }
}

/** print mutex task */
void printData_task(void *pvParameter){
  while(1){
    //Lay mutex truoc khi in
    if(xSemaphoreTake(print_mutex, portTICK_PERIOD_MS) == pdTRUE){
      printf("%d,%lu,%lu,%d\n", global_inmp441_data, global_red, global_ir, global_adc_value);
      xSemaphoreGive(print_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
