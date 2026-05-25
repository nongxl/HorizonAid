#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <M5Unified.h>
#include "imu_processor.h"

class SafetyManager {
public:
    SafetyManager();
    void update(const IMUProcessor& imu, unsigned long dt);
    bool isSafeMode() const;

private:
    bool safe_mode_active = false;
    unsigned long safe_mode_start_time = 0;
};

#endif // SAFETY_MANAGER_H
