// integration_check.с
/*модуль ничего не делает. (проверить)
В ТЗ его нет.
Он не участвует в безопасности.
Он не формирует ошибки.
Он не влияет на FSM.
👉 Да, модуль избыточный.
Его скорее всего можно удалить в Фазе 11 или 14.*/

#include "integration_check.h"
#include "hardware.h"
#include <string.h>

// ============================================================================
// Внутренние переменные
// ============================================================================

static integration_state_t integration_ctx;

// ============================================================================
// Публичные функции
// ============================================================================

void integration_check_init(void) {
    memset(&integration_ctx, 0, sizeof(integration_ctx));
    
    integration_ctx.check_interval_ms = 1000;  // Проверка каждую секунду
    integration_ctx.enabled = true;
}

void integration_check_background_task(void) {
    if (!integration_ctx.enabled) {
        return;
    }
    
    uint32_t current_time = time_ms();
    
    if ((current_time - integration_ctx.last_check_time) >= integration_ctx.check_interval_ms) {
        integration_ctx.last_check_time = current_time;
        
        // Здесь можно добавить проверки логической целостности системы
        // Например, проверка согласованности данных между модулями
        
        // Пример проверки: если система в FATAL режиме, 
        // то измерения должны быть остановлены
        
        // Эти проверки можно расширять по мере необходимости
    }
}

const integration_state_t* integration_get_state(void) {
    return &integration_ctx;
}