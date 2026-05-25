#include "safety_manager.h"

SafetyManager::SafetyManager() {
}

void SafetyManager::update(const IMUProcessor& imu, unsigned long dt) {
    bool trigger_safe_mode = false;

    // 1. 检查 IMU 内部状态
    if (imu.isAbnormal()) {
        trigger_safe_mode = true;
    }

    // 2. 延迟检测
    if (dt > 80) {
        trigger_safe_mode = true;
    }

    // 更新状态
    if (trigger_safe_mode) {
        safe_mode_active = true;
        safe_mode_start_time = millis();
    } else {
        // 如果目前是安全模式，经过一段时间后自动恢复
        if (safe_mode_active) {
            if (millis() - safe_mode_start_time > 2000) {
                safe_mode_active = false;
            }
        }
    }
}

bool SafetyManager::isSafeMode() const {
    return safe_mode_active;
}
