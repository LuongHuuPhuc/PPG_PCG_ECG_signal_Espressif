set(req max30102 driver esp_timer) #Yeu cau dung thu vien tu cac folder nay
idf_component_register(SRCS "sensor_init.c"
                      INCLUDE_DIRS "."
                      REQUIRES ${req})