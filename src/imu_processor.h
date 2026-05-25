#ifndef IMU_PROCESSOR_H
#define IMU_PROCESSOR_H

#include <M5Unified.h>
#include "kalman_filter.h"

struct MotionState {
    float turn_intensity;   // 左右转动 (用于 offset x)
    float accel_intensity;  // 加速/刹车 (用于 offset y/前后倾)
    float vibration;        // 颠簸 (垂直加速度)
    
    float horizon_roll;     // 地平线 Roll
    float horizon_pitch;    // 地平线 Pitch
    
    // Layer 3 (预测层) 的前瞻值
    float predicted_turn;
    float predicted_accel;
};

class IMUProcessor {
public:
    IMUProcessor();
    void init();
    void update();
    
    MotionState getMotionState() const;
    bool isAbnormal() const;

    // 校准功能（归零）
    void calibrateZero();
    void resetCalibration();

private:
    MotionState currentState;
    
    // 原始数据
    float gyro_x, gyro_y, gyro_z;
    float accel_x, accel_y, accel_z;
    float ahrs_pitch, ahrs_roll, ahrs_yaw;

    // 低通滤波历史值
    float filtered_gyro_yaw = 0.0f;
    float filtered_accel_forward = 0.0f;
    float filtered_accel_vertical = 0.0f;
    
    float filtered_horizon_roll = 0.0f;
    float filtered_horizon_pitch = 0.0f;

    // 校准偏移量
    float pitch_offset = 0.0f;
    float roll_offset = 0.0f;
    float accel_forward_offset = 0.0f; // 消除非水平摆放时的 Z 轴重力分量

    // 参数
    const float alpha_gyro = 0.1f;
    const float alpha_accel = 0.08f;
    const float alpha_horizon = 0.03f; // 极致平滑，让主线变成绝对沉稳的锚点

    // 卡尔曼滤波器 (Layer 3)
    SimpleKalmanFilter turn_kf;
    SimpleKalmanFilter accel_kf;

    // 异常检测状态
    bool abnormal_state = false;
    unsigned long last_update_time = 0;
};

#endif // IMU_PROCESSOR_H
