#include <stdint.h>
#include <stdbool.h>

// --- Pin Constants ---
#define IR_SENSOR_PIN      34
#define HAPTIC_MOTOR_PIN   23
#define BUZZER_PIN         25

// --- Firmware Algorithmic Thresholds ---
#define EYE_CLOSED_THRESHOLD     3000   // High ADC value = closed eye
#define MICROSLEEP_TIME_MS       350    // 350ms duration threshold
#define HEAD_DROOP_THRESHOLD_DEG -15.0f

// --- System State Enum ---
typedef enum {
    STATE_ALERT,
    STATE_DROWSY_WARNING,
    STATE_CRITICAL_MICROSLEEP,
    STATE_ENV_HAZARD
} SystemState;

// --- System Telemetry Data Structure ---
typedef struct {
    int raw_ir_value;
    float head_pitch_deg;
    uint32_t vehicle_speed_kmh;
    bool is_misty_or_rainy;
} DriverTelemetry;

// --- Global Timing and State Variables ---
uint32_t eye_closed_start_time = 0;
bool is_eye_currently_closed = false;
SystemState current_state = STATE_ALERT;

/**
 * Evaluates the driver's physiological state based on sensory duration inputs.
 * This function handles the pure temporal logic in Standard C.
 */
void process_driver_state(const DriverTelemetry* telemetry, uint32_t current_time_ms) {
    
    // 1. Evaluate Eyelid Activity over Time
    if (telemetry->raw_ir_value >= EYE_CLOSED_THRESHOLD) {
        if (!is_eye_currently_closed) {
            eye_closed_start_time = current_time_ms; // Mark initial closure timestamp
            is_eye_currently_closed = true;
        }
        
        // Evaluate if continuous duration threshold is breached
        if ((current_time_ms - eye_closed_start_time) >= MICROSLEEP_TIME_MS) {
            current_state = STATE_CRITICAL_MICROSLEEP;
        }
    } else {
        is_eye_currently_closed = false;
        
        // 2. Fall back to Environmental Context if eyes are wide open
        if (telemetry->is_misty_or_rainy && telemetry->vehicle_speed_kmh > 60) {
            current_state = STATE_ENV_HAZARD;
        } else {
            current_state = STATE_ALERT;
        }
    }
}

/**
 * Acts upon the resolved system state to drive hardware pins or trigger warnings.
 */
void execute_safety_responses(const DriverTelemetry* telemetry) {
    switch (current_state) {
        
        case STATE_CRITICAL_MICROSLEEP:
            if (telemetry->head_pitch_deg <= HEAD_DROOP_THRESHOLD_DEG) {
                // SENSOR FUSION MATCHED: Driver is fully unconscious
                // Action: Direct pin execution commands here (e.g., gpio_set_level)
                // Set BUZZER -> HIGH, HAPTIC -> HIGH
            } else {
                // False alarm filter: Eyes closed briefly, head remains upright
                // Set BUZZER -> LOW, HAPTIC -> LOW
            }
            break;
            
        case STATE_ENV_HAZARD:
            // Action: Trigger the unique temporal haptic pulse warning patterns
            // to signal "Reduce Speed" due to localized mist/rain conditions.
            break;
            
        case STATE_ALERT:
        default:
            // Safe baseline state: Clear all alert pins
            // Set BUZZER -> LOW, HAPTIC -> LOW
            break;
    }
}