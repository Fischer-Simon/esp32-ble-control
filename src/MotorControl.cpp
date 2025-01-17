#include "MotorControl.h"


/* MCPWM BLDC control with Hall sensor

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <KeyValueStore.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_log.h"

#define PWM_DEFAULT_FREQ   20000
#define PWM_MIN_DUTY       0.0
#define PWM_MAX_DUTY       50.0
#define PWM_DUTY_STEP      5.0
#define BLDC_MCPWM_GROUP   static_cast<mcpwm_unit_t>(0)
#define BLDC_MCPWM_TIMER_U static_cast<mcpwm_timer_t>(0)
#define BLDC_MCPWM_TIMER_V static_cast<mcpwm_timer_t>(1)
#define BLDC_MCPWM_TIMER_W static_cast<mcpwm_timer_t>(2)
#define BLDC_MCPWM_GEN_HIGH MCPWM_GEN_A
#define BLDC_MCPWM_GEN_LOW  MCPWM_GEN_B


#define BLDC_DRV_EN_U_GPIO    static_cast<gpio_num_t>(17)
#define BLDC_DRV_EN_V_GPIO    static_cast<gpio_num_t>(16)
#define BLDC_DRV_EN_W_GPIO    static_cast<gpio_num_t>(4)
// #define BLDC_DRV_FAULT_GPIO static_cast<gpio_num_t>(19)
// #define BLDC_DRV_OVER_CURRENT_FAULT MCPWM_SELECT_F0

#define BLDC_PWM_UH_GPIO static_cast<gpio_num_t>(19)
// #define BLDC_PWM_UL_GPIO static_cast<gpio_num_t>(22)
#define BLDC_PWM_VH_GPIO static_cast<gpio_num_t>(18)
// #define BLDC_PWM_VL_GPIO static_cast<gpio_num_t>(25)
#define BLDC_PWM_WH_GPIO static_cast<gpio_num_t>(5)
// #define BLDC_PWM_WL_GPIO static_cast<gpio_num_t>(27)
#define HALL_CAP_U_GPIO  static_cast<gpio_num_t>(23)
#define HALL_CAP_V_GPIO  static_cast<gpio_num_t>(22)
#define HALL_CAP_W_GPIO  static_cast<gpio_num_t>(21)

static const char* TAG = "example";

static inline uint32_t bldc_get_hall_sensor_value(bool ccw) {
    uint32_t hall_val = gpio_get_level(HALL_CAP_U_GPIO) * 4 + gpio_get_level(HALL_CAP_V_GPIO) * 2 +
                        gpio_get_level(HALL_CAP_W_GPIO) * 1;
    return ccw ? hall_val ^ (0x07) : hall_val;
}

static bool IRAM_ATTR bldc_hall_updated(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_channel,
                                        const cap_event_data_t* edata, void* user_data) {
    TaskHandle_t task_to_notify = (TaskHandle_t) user_data;
    BaseType_t high_task_wakeup = pdFALSE;
    vTaskNotifyGiveFromISR(task_to_notify, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void update_bldc_speed(float duty) {
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH, duty);
    mcpwm_set_duty(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW, duty);
}

// U+V- / A+B-
static void bldc_set_phase_up_vm(void) {
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // U+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // U- = _PWM_
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH); // V+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW); // V- = 1
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH); // W+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW); // W- = 0
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 1);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 1);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);
}

// W+U- / C+A-
static void bldc_set_phase_wp_um(void) {
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH); // U+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW); // U- = 1
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH); // V+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW); // V- = 0
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // W+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // W- = _PWM_
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 1);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 1);
}

// W+V- / C+B-
static void bldc_set_phase_wp_vm(void) {
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH); // U+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW); // U- = 0
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH); // V+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW); // V- = 1
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // W+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // W- = _PWM_
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 1);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 1);
}

// V+U- / B+A-
static void bldc_set_phase_vp_um(void) {
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH); // U+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW); // U- = 1
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // V+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // V- = _PWM_
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH); // W+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW); // W- = 0
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 1);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 1);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);
}

// V+W- / B+C-
static void bldc_set_phase_vp_wm(void) {
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH); // U+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_LOW); // U- = 0
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // V+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // V- = _PWM_
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH); // W+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW); // W- = 1
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 1);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 1);
}

// U+W- / A+C-
static void bldc_set_phase_up_wm(void) {
std::string test;
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);
    mcpwm_set_duty_type(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_MCPWM_GEN_HIGH, MCPWM_DUTY_MODE_0); // U+ = PWM
    mcpwm_deadtime_enable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE, 3, 3); // U- = _PWM_
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_HIGH); // V+ = 0
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_MCPWM_GEN_LOW); // V- = 0
    mcpwm_deadtime_disable(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W);
    mcpwm_set_signal_low(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_HIGH); // W+ = 0
    mcpwm_set_signal_high(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_MCPWM_GEN_LOW); // W- = 1
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 1);
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 1);
}

typedef void (* bldc_hall_phase_action_t)(void);

static const bldc_hall_phase_action_t s_hall_actions[] = {
    nullptr,
    bldc_set_phase_wp_vm,
    bldc_set_phase_vp_um,
    bldc_set_phase_wp_um,
    bldc_set_phase_up_wm,
    bldc_set_phase_up_vm,
    bldc_set_phase_vp_wm,
};

std::shared_ptr<KeyValueStore::SimpleValue<float> > hallMetric;

void bldc_init(std::shared_ptr<KeyValueStore> kvs) {
    hallMetric = kvs->createValue<float>("Motor", "Hall", false, 0);
}

[[noreturn]] void bldc_run(void*) {
    uint32_t hall_sensor_value = 0;
    uint32_t last_hall_sensor_value = -1;
    uint32_t last_valid_hall_sensor_value = -1;
    TaskHandle_t cur_task = xTaskGetCurrentTaskHandle();

    ESP_LOGI(TAG, "Disable gate driver");
    gpio_config_t drv_en_u_config = {
        .pin_bit_mask = 1 << BLDC_DRV_EN_U_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&drv_en_u_config));
    gpio_set_level(BLDC_DRV_EN_U_GPIO, 0);

    gpio_config_t drv_en_v_config = {
        .pin_bit_mask = 1 << BLDC_DRV_EN_V_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&drv_en_v_config));
    gpio_set_level(BLDC_DRV_EN_V_GPIO, 0);

    gpio_config_t drv_en_w_config = {
        .pin_bit_mask = 1 << BLDC_DRV_EN_W_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&drv_en_w_config));
    gpio_set_level(BLDC_DRV_EN_W_GPIO, 0);

    ESP_LOGI(TAG, "Setup PWM and Hall GPIO (pull up internally)");
    mcpwm_pin_config_t mcpwm_gpio_config = {
        .mcpwm0a_out_num = BLDC_PWM_UH_GPIO,
        .mcpwm0b_out_num = -1,
        .mcpwm1a_out_num = BLDC_PWM_VH_GPIO,
        .mcpwm1b_out_num = -1,
        .mcpwm2a_out_num = BLDC_PWM_WH_GPIO,
        .mcpwm2b_out_num = -1,
        .mcpwm_sync0_in_num = -1, //Not used
        .mcpwm_sync1_in_num = -1, //Not used
        .mcpwm_sync2_in_num = -1, //Not used
        .mcpwm_fault0_in_num = -1,
        .mcpwm_fault1_in_num = -1, //Not used
        .mcpwm_fault2_in_num = -1, //Not used
        .mcpwm_cap0_in_num = HALL_CAP_U_GPIO,
        .mcpwm_cap1_in_num = HALL_CAP_V_GPIO,
        .mcpwm_cap2_in_num = HALL_CAP_W_GPIO,
    };
    ESP_ERROR_CHECK(mcpwm_set_pin(BLDC_MCPWM_GROUP, &mcpwm_gpio_config));
    // In case there's no pull-up resister for hall sensor on board
    gpio_pullup_en(HALL_CAP_U_GPIO);
    gpio_pullup_en(HALL_CAP_V_GPIO);
    gpio_pullup_en(HALL_CAP_W_GPIO);
    // gpio_pullup_en(BLDC_DRV_FAULT_GPIO);

    ESP_LOGI(TAG, "Initialize PWM (default to turn off all MOSFET)");
    mcpwm_config_t pwm_config = {
        .frequency = PWM_DEFAULT_FREQ,
        .cmpr_a = PWM_MIN_DUTY,
        .cmpr_b = PWM_MIN_DUTY,
        .duty_mode = MCPWM_HAL_GENERATOR_MODE_FORCE_LOW,
        .counter_mode = MCPWM_UP_COUNTER,
    };
    ESP_ERROR_CHECK(mcpwm_init(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, &pwm_config));
    ESP_ERROR_CHECK(mcpwm_init(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, &pwm_config));
    ESP_ERROR_CHECK(mcpwm_init(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, &pwm_config));

    // ESP_LOGI(TAG, "Initialize over current fault action");
    // ESP_ERROR_CHECK(mcpwm_fault_init(BLDC_MCPWM_GROUP, MCPWM_LOW_LEVEL_TGR, BLDC_DRV_OVER_CURRENT_FAULT));
    // ESP_ERROR_CHECK(
    //     mcpwm_fault_set_cyc_mode(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_U, BLDC_DRV_OVER_CURRENT_FAULT,
    //         MCPWM_ACTION_FORCE_LOW, MCPWM_ACTION_FORCE_LOW));
    // ESP_ERROR_CHECK(
    //     mcpwm_fault_set_cyc_mode(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_V, BLDC_DRV_OVER_CURRENT_FAULT,
    //         MCPWM_ACTION_FORCE_LOW, MCPWM_ACTION_FORCE_LOW));
    // ESP_ERROR_CHECK(
    //     mcpwm_fault_set_cyc_mode(BLDC_MCPWM_GROUP, BLDC_MCPWM_TIMER_W, BLDC_DRV_OVER_CURRENT_FAULT,
    //         MCPWM_ACTION_FORCE_LOW, MCPWM_ACTION_FORCE_LOW));

    ESP_LOGI(TAG, "Initialize Hall sensor capture");
    mcpwm_capture_config_t cap_config = {
        .cap_edge = MCPWM_BOTH_EDGE,
        .cap_prescale = 1,
        .capture_cb = bldc_hall_updated,
        .user_data = cur_task,
    };
    ESP_ERROR_CHECK(
        mcpwm_capture_enable_channel(BLDC_MCPWM_GROUP, static_cast<mcpwm_capture_channel_id_t>(0), &cap_config));
    ESP_ERROR_CHECK(
        mcpwm_capture_enable_channel(BLDC_MCPWM_GROUP, static_cast<mcpwm_capture_channel_id_t>(1), &cap_config));
    ESP_ERROR_CHECK(
        mcpwm_capture_enable_channel(BLDC_MCPWM_GROUP, static_cast<mcpwm_capture_channel_id_t>(2), &cap_config));
    ESP_LOGI(TAG, "Please turn on the motor power");
    ESP_LOGI(TAG, "Enable gate driver");
    update_bldc_speed(0);

    while (1) {
        // The rotation direction is controlled by inverting the hall sensor value
        hall_sensor_value = bldc_get_hall_sensor_value(false);
        if (hall_sensor_value != last_hall_sensor_value && hall_sensor_value != last_valid_hall_sensor_value) {
            if (hall_sensor_value >= 1 && hall_sensor_value <= sizeof(s_hall_actions) / sizeof(s_hall_actions[0])) {
                s_hall_actions[hall_sensor_value]();
            } else {
                ESP_LOGE(TAG, "invalid bldc phase, wrong hall sensor value:%d", hall_sensor_value);
            }
            last_valid_hall_sensor_value = last_hall_sensor_value;
            last_hall_sensor_value = hall_sensor_value;
        }
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}
