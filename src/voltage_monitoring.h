// voltage_monitoring.h

#ifndef VOLTAGE_MONITORING_H
#define VOLTAGE_MONITORING_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Конфигурация (будет вынесена в config.h)
// ============================================================================

#ifndef CONFIG_LOADED
// Напряжения и пороги
#define V_NOMINAL_MV                    2570  // 13.0 В - Граничное напряжение бортсети *1
#define V_DELTA_MV                      40    // +-0,2 В - гистерезис (от 12,8v до 13,2v ) *1
/* *1 При V_DIVIDER_RATIO=1 указывается напряжение на входе АЦП в миливольтах.*/

#define V_ADC_REF_MV                    3300    // 3.3 В - опорное напряжение АЦП
#define V_DIVIDER_RATIO                 1       // 5,058:1 - делитель напряжения (задаем 1 для удобства пересчета) 

// Тайминги (в циклах PROCESSING_INTERVAL_MS = 500 мс) *2
/* *2 Замечания: Правильнее было бы их задавать в секундах и переводить в целое количество циклов с учетом PROCESSING_INTERVAL_MS*/
#define TIMING_CONFIRM_CYCLES           4       // 2 секунды подтверждения
#define TIMING_PAUSE_DOWN_INIT          3       // 1.5 сек - пауза после увеличения ограничения
#define TIMING_PAUSE_UP_HV_INIT         20      // 10 сек - пауза между ступенями снятия

#define TIMING_PAUSE_UP_LV_INIT         120     // 60 сек - пауза после снятия нагрузки (Low→High)
#define TIMING_MAX_NO_NOMINAL_CYCLES    120     // 60 сек - ошибка "нет номинала"

// Параметры ограничения
#define MAX_LOAD_STEP                   3       // МАксимально возможное ограничение

// Фильтрация АЦП
#define ADC_SMA_BUFFER_SIZE             8       // скользящее среднее по 8 измерениям
#endif

// ============================================================================
// Типы данных
// ============================================================================

typedef enum {
    VOLTAGE_LOW,        // Напряжение ниже нормы (просадка)
    VOLTAGE_HIGH,       // Напряжение выше нормы (восстановлено)
    VOLTAGE_NA          // Неопределённое состояние (в гистерезисе или неподтверждённое)
} voltage_state_t;

typedef enum {
    VM_ERROR_NONE = 0,
    VM_ERROR_NO_NOMINAL     // Нет устойчивого HIGH в течение минуты
} vm_error_t;

// Состояние модуля
typedef struct {
    // Текущее состояние
    int8_t load_limit;         // текущее ограничение 0..3
    int8_t last_required;      // предыдущий запрос "требуется"

    voltage_state_t raw_state;  // мгновенное состояние (с фильтром)
    
    // Таймеры и счётчики
    uint16_t pause_down;
    uint16_t pause_up;
    uint8_t  lv_counter;        // подтверждение низкого уровня (4→0)
    uint8_t  hv_counter;        // подтверждение высокого уровня (4→0)
    uint16_t no_nominal_counter; // счётчик отсутствия подтверждённого HIGH
    
    // Статистика
    uint32_t error_count;
    vm_error_t last_error;
    uint32_t timestamp_ms;
    
    // ADC
    uint16_t adc_filtered_mv;   // отфильтрованное напряжение
} vm_state_t;

// Результат операции
typedef struct {
    bool success;
    bool limit_changed;
    uint8_t new_limit;
    uint32_t timestamp_ms;
} vm_result_t;

int8_t voltage_monitoring_task(int8_t required);
int8_t voltage_monitoring_get_limit(void);

void voltage_monitoring_init(void);
void voltage_monitoring_reset_limit(void);

#endif // VOLTAGE_MONITORING_H


