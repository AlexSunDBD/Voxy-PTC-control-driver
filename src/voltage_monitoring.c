// voltage_monitoring.c

#include "voltage_monitoring.h"
#include "hardware.h"
#include <string.h>

// ============================================================================
// Аппаратные определения (STM32F103C8T6 + HAL)
// ============================================================================

#define ADC_PORT               GPIOA
#define ADC_PIN                GPIO_PIN_1
#define ADC_CHANNEL            ADC_CHANNEL_1
#define ADC_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define ADC_CLK_ENABLE()       __HAL_RCC_ADC1_CLK_ENABLE()

// Простое прерывание по таймеру или однократное измерение
// Не используем DMA, достаточно одного значения с фильтрацией

// ============================================================================
// Глобальные переменные HAL
// ============================================================================

static ADC_HandleTypeDef hadc1;

// ============================================================================
// Внутренние переменные
// ============================================================================

static vm_state_t vm_ctx;
static vm_result_t vm_last_result;

// Фильтр скользящего среднего
static uint16_t adc_sma_buffer[ADC_SMA_BUFFER_SIZE];
static uint8_t adc_sma_index = 0;
static uint64_t adc_sma_sum = 0;

// ============================================================================
// Внутренние функции (ADC + скользящее среднее)
// ============================================================================

/**
 * @brief Инициализация GPIO для PA1 (аналоговый вход)
 */
static void adc_gpio_init(void) {
    GPIO_InitTypeDef gpio_init = {0};
    
    ADC_GPIO_CLK_ENABLE();
    
    gpio_init.Pin = ADC_PIN;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    
    HAL_GPIO_Init(ADC_PORT, &gpio_init);
}

/**
 * @brief Инициализация ADC1
 */
static void adc_init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};
    
    ADC_CLK_ENABLE();
    
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;      // Однократный режим
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    
    HAL_ADC_Init(&hadc1);
    
    sConfig.Channel = ADC_CHANNEL;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

/**
 * @brief Выполнить измерение АЦП
 * @return сырое значение 0-4095
 */
static uint16_t adc_read_raw(void) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        return HAL_ADC_GetValue(&hadc1);
    }
    return 0;
}

/**
 * @brief Обновить фильтр скользящего среднего
 * @return отфильтрованное значение в мВ на входе делителя
 */
static uint16_t adc_update_filter(void) {
    // Читаем новое значение
    uint16_t raw = adc_read_raw();
    
    // Удаляем старое значение из суммы
    adc_sma_sum -= adc_sma_buffer[adc_sma_index];
    
    // Добавляем новое значение
    adc_sma_buffer[adc_sma_index] = raw;
    adc_sma_sum += raw;
    
    // Сдвигаем указатель
    adc_sma_index = (adc_sma_index + 1) % ADC_SMA_BUFFER_SIZE;
    
    // Среднее арифметическое
    uint32_t avg_raw = adc_sma_sum / ADC_SMA_BUFFER_SIZE;
    
    // Пересчёт в напряжение на пине
    uint32_t voltage_at_pin_mv = (avg_raw * V_ADC_REF_MV) / 4095;
    
    // Пересчёт на вход делителя
    return (uint16_t)(voltage_at_pin_mv * V_DIVIDER_RATIO);
}

/**
 * @brief Определить мгновенное состояние напряжения по уровню
 */
static voltage_state_t get_raw_state(uint16_t voltage_mv) {
    if (voltage_mv < (V_NOMINAL_MV - V_DELTA_MV)) {
        return VOLTAGE_LOW;
    } else if (voltage_mv > (V_NOMINAL_MV + V_DELTA_MV)) {
        return VOLTAGE_HIGH;
    } else {
        return VOLTAGE_NA;  // зона гистерезиса
    }
}

// ============================================================================
// Внутренние функции (алгоритм)
// ============================================================================

/**
 * @brief Событие: просадка напряжения (Low)
 */
static int8_t event_voltage_low(void) {
    if (vm_ctx.pause_down == 0 && vm_ctx.load_limit < MAX_LOAD_STEP) {
        vm_ctx.pause_down = TIMING_PAUSE_DOWN_INIT;
        vm_ctx.pause_up = TIMING_PAUSE_UP_LV_INIT;
        return +1;
    }
    return 0;
}

/**
 * @brief Событие: восстановление напряжения (High)
 */
static int8_t event_voltage_high(void) {
    
    vm_ctx.no_nominal_counter = 0;

    if (vm_ctx.pause_up == 0 && vm_ctx.load_limit > 0) {
        vm_ctx.pause_down = TIMING_PAUSE_DOWN_INIT;
        vm_ctx.pause_up = TIMING_PAUSE_UP_HV_INIT;
        return -1;
    }
    return 0;
}

// ============================================================================
// Публичные функции
// ============================================================================

void voltage_monitoring_init(void) {
    memset(&vm_ctx, 0, sizeof(vm_ctx));
    memset(&vm_last_result, 0, sizeof(vm_last_result));
    memset(adc_sma_buffer, 0, sizeof(adc_sma_buffer));
    
    // Инициализация железа
    adc_gpio_init();
    adc_init();
    
    // Инициализация фильтра
    adc_sma_sum = 0;
    adc_sma_index = 0;
    
    // Прогрев фильтра
    for (int i = 0; i < ADC_SMA_BUFFER_SIZE; i++) {
        adc_update_filter();
    }
    
    // Начальное состояние
    vm_ctx.load_limit = 0;
    vm_ctx.last_required = MAX_LOAD_STEP;
    vm_ctx.raw_state = VOLTAGE_NA;
    vm_ctx.lv_counter = TIMING_CONFIRM_CYCLES;
    vm_ctx.hv_counter = TIMING_CONFIRM_CYCLES;
    vm_ctx.no_nominal_counter = 0;
    vm_ctx.timestamp_ms = time_ms();
    
    vm_last_result.success = true;
    vm_last_result.timestamp_ms = vm_ctx.timestamp_ms;
}

int8_t voltage_monitoring_task(int8_t required) {
    
    vm_error_t error = VM_ERROR_NONE;
    vm_last_result.limit_changed = false;
    int8_t delta_from_voltage = 0;
    int8_t delta_required = (int8_t)required - (int8_t)vm_ctx.last_required;


    // 1. Получаем отфильтрованное напряжение
    vm_ctx.adc_filtered_mv = adc_update_filter();
    vm_ctx.timestamp_ms = time_ms();
    
    // 2. Определяем мгновенное состояние
    voltage_state_t current_state = get_raw_state(vm_ctx.adc_filtered_mv);
    vm_ctx.raw_state = current_state;
    
    // 3. Обновление пауз
    if (vm_ctx.pause_down > 0) vm_ctx.pause_down--;
    if (vm_ctx.pause_up > 0) vm_ctx.pause_up--;
    
    // 4. Обработка LOW
    if (current_state == VOLTAGE_LOW) {
        // Сброс счётчика HIGH
        vm_ctx.hv_counter = TIMING_CONFIRM_CYCLES;
        
        // Подтверждение LOW
        if (vm_ctx.lv_counter > 0) {
            vm_ctx.lv_counter--;
        }
        
        // Если подтвердили LOW (4 цикла подряд)
        if (vm_ctx.lv_counter == 0) {
            delta_from_voltage = event_voltage_low();
        }
        
        // Нет подтверждённого HIGH - увеличиваем счётчик ошибки
        if (vm_ctx.no_nominal_counter < TIMING_MAX_NO_NOMINAL_CYCLES) {
            vm_ctx.no_nominal_counter++;
        }
    }
    
    // 5. Обработка HIGH
    else if (current_state == VOLTAGE_HIGH) {
        // Сброс счётчика LOW
        vm_ctx.lv_counter = TIMING_CONFIRM_CYCLES;
        
        // Подтверждение HIGH
        if (vm_ctx.hv_counter > 0) {
            vm_ctx.hv_counter--;
        }
        
        // Если подтвердили HIGH (4 цикла подряд)
        if (vm_ctx.hv_counter == 0) {
            delta_from_voltage = event_voltage_high();
        } else {
            // HIGH, но ещё не подтверждён - считаем как "нет номинала"
            if (vm_ctx.no_nominal_counter < TIMING_MAX_NO_NOMINAL_CYCLES) {
                vm_ctx.no_nominal_counter++;
            }
        }
    }
    
    // 6. Обработка NA (гистерезис)
    else {
        // Сбрасываем счётчики подтверждения
        vm_ctx.lv_counter = TIMING_CONFIRM_CYCLES;
        vm_ctx.hv_counter = TIMING_CONFIRM_CYCLES;
        
        // Нет подтверждённого HIGH - увеличиваем счётчик ошибки
        if (vm_ctx.no_nominal_counter < TIMING_MAX_NO_NOMINAL_CYCLES) {
            vm_ctx.no_nominal_counter++;
        }
    }

    // 7. Расчет лимита
    int16_t new_limit = (int16_t)vm_ctx.load_limit;

    new_limit += delta_from_voltage;   // ±1 или 0
    new_limit += delta_required;       // любое значение

    // Saturation
    if (new_limit < 0)
        new_limit = 0;

    if (new_limit > MAX_LOAD_STEP)
        new_limit = MAX_LOAD_STEP;

    // Проверяем изменение
    if (new_limit != vm_ctx.load_limit) {
        vm_ctx.load_limit = (int8_t)new_limit;
        vm_last_result.limit_changed = true;
        vm_last_result.new_limit = vm_ctx.load_limit;
    }    

    // 8. Проверка ошибки "нет номинала"
    if (vm_ctx.no_nominal_counter >= TIMING_MAX_NO_NOMINAL_CYCLES) {
        error = VM_ERROR_NO_NOMINAL;
        vm_ctx.error_count++;
        vm_ctx.last_error = error;
        vm_last_result.success = false;
        vm_last_result.timestamp_ms = vm_ctx.timestamp_ms;
        return -1; // Ввозвращаем "-1" это означает ошибка
    }
    
    vm_last_result.success = (error == VM_ERROR_NONE);
    vm_last_result.timestamp_ms = vm_ctx.timestamp_ms;
    vm_ctx.last_error = error;
    vm_ctx.last_required = required;
    
    return vm_ctx.load_limit;
}


void voltage_monitoring_increase_request(int8_t delta_required)
{
    // Проверяем, что текущее ограничение не максимальное
    if (vm_ctx.load_limit < MAX_LOAD_STEP) {
        // Временная переменная для нового значения
        int16_t new_limit = (int16_t)vm_ctx.load_limit + (int16_t)delta_required;;
        
        // Ограничиваем сверху
        if (new_limit > MAX_LOAD_STEP) {new_limit = MAX_LOAD_STEP;}
        
        // Применяем только если значение изменилось
        if (new_limit != vm_ctx.load_limit) {
            vm_ctx.load_limit = new_limit;
            vm_last_result.limit_changed = true;
            vm_last_result.new_limit = vm_ctx.load_limit;
        }
    }
}

void voltage_monitoring_decrease_request(int8_t delta_required)
{
    // Проверяем, что текущее ограничение не нулевое
    if (vm_ctx.load_limit > 0) {
        // Временная переменная для нового значения
        int16_t new_limit = (int16_t)vm_ctx.load_limit + (int16_t)delta_required; // delta_required отрицательный
        
        // Ограничиваем снизу
        if (new_limit < 0) {new_limit = 0;}
        
        // Применяем только если значение изменилось
        if (new_limit != vm_ctx.load_limit) {
            vm_ctx.load_limit = new_limit;
            vm_last_result.limit_changed = true;
            vm_last_result.new_limit = vm_ctx.load_limit;
        }
    }
}

int8_t voltage_monitoring_get_limit(void) {
    return vm_ctx.load_limit;
}

const vm_state_t* voltage_monitoring_get_state(void) {
    return &vm_ctx;
}

vm_result_t voltage_monitoring_get_last_result(void) {
    return vm_last_result;
}

void voltage_monitoring_reset_limit(void) {
    vm_ctx.load_limit = 0;
    vm_ctx.last_required = MAX_LOAD_STEP;
    vm_ctx.pause_down = 0;
    vm_ctx.pause_up = 0;
    vm_ctx.lv_counter = TIMING_CONFIRM_CYCLES;
    vm_ctx.hv_counter = TIMING_CONFIRM_CYCLES;
    vm_ctx.no_nominal_counter = 0;
    vm_ctx.last_error = VM_ERROR_NONE;
}

void voltage_monitoring_clear_error(void) {
    vm_ctx.last_error = VM_ERROR_NONE;
    vm_ctx.no_nominal_counter = 0;
}

// ============================================================================
// HAL Callbacks
// ============================================================================

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle) {
    if (adcHandle->Instance == ADC1) {
        // GPIO уже инициализирован
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle) {
    if (adcHandle->Instance == ADC1) {
        HAL_ADC_DeInit(&hadc1);
        HAL_GPIO_DeInit(ADC_PORT, ADC_PIN);
    }
}


