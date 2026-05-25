#include <M5Unified.h>
#include "imu_processor.h"
#include "visual_engine.h"
#include "safety_manager.h"

IMUProcessor imuProcessor;
VisualEngine visualEngine;
SafetyManager safetyManager;

unsigned long last_loop_time = 0;

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    // 设置屏幕为横屏，USB在右侧
    M5.Display.setRotation(1);
    
    imuProcessor.init();
    visualEngine.init(&M5.Display);

    last_loop_time = millis();
}

void loop() {
    M5.update();

    unsigned long current_time = millis();
    unsigned long dt = current_time - last_loop_time;
    last_loop_time = current_time;

    // 1. 读取并处理 IMU 数据
    imuProcessor.update();

    // 2. 状态与安全检测
    safetyManager.update(imuProcessor, dt);

    // 2.5 按键检测：短按归零校准，长按恢复真实读数
    if (M5.BtnA.wasHold()) {
        imuProcessor.resetCalibration();
    } else if (M5.BtnA.wasClicked()) {
        imuProcessor.calibrateZero();
    }

    // 3. 视觉引擎更新与渲染
    visualEngine.update(imuProcessor.getMotionState(), safetyManager.isSafeMode());
    visualEngine.render();
    
    // 限制最高帧率，例如 60fps (16ms)
    unsigned long process_time = millis() - current_time;
    if (process_time < 16) {
        delay(16 - process_time);
    }
}
