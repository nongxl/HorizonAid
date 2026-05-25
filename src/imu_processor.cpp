#include "imu_processor.h"
#include <cmath>

IMUProcessor::IMUProcessor() :
    turn_kf(0.05f, 0.2f, 1.0f),   // 允许较快的过程变化，适中的测量噪声
    accel_kf(0.05f, 0.2f, 1.0f) { // 允许较快的过程变化，适中的测量噪声
}

void IMUProcessor::init() {
    last_update_time = millis();
}

void IMUProcessor::update() {
    unsigned long current_time = millis();
    unsigned long dt = current_time - last_update_time;
    last_update_time = current_time;

    if (dt > 100) {
        // 如果两次更新间隔过长，可能是系统卡顿，标记为异常
        abnormal_state = true;
    } else {
        abnormal_state = false;
    }

    M5.Imu.update();

    M5.Imu.getGyroData(&gyro_x, &gyro_y, &gyro_z);
    M5.Imu.getAccelData(&accel_x, &accel_y, &accel_z);

    // 经测试，在横屏放置时，M5StickS3 的坐标系为：
    // X 轴：水平向右 (长边)
    // Y 轴：垂直向下 (短边，重力方向)
    // Z 轴：指向屏幕前方 (垂直于屏幕，乘客视线方向)
    
    // 转向是绕着垂直轴(Y)旋转
    float raw_gyro_yaw = gyro_y;         
    
    // 加减速是沿前向轴(Z)，并减去归零时记录的 Z 轴重力分量
    float raw_accel_forward = accel_z - accel_forward_offset;   
    
    // 颠簸是沿垂直轴(Y)
    float raw_accel_vertical = accel_y;  

    // 利用加速度计算地平线的 Pitch 和 Roll
    // 当水平静止时，重力沿 Y 轴向下 (accel_y = 1.0)
    // Roll (左右倾斜)，围绕 Z 轴，并减去校准偏移
    float raw_horizon_roll = atan2(-accel_x, accel_y) * 180.0f / M_PI - roll_offset;
    // Pitch (前后倾斜)，围绕 X 轴，并减去校准偏移
    float raw_horizon_pitch = atan2(accel_z, accel_y) * 180.0f / M_PI - pitch_offset;

    // 低通滤波 (IIR) - 用于原有的平滑逻辑
    filtered_gyro_yaw = alpha_gyro * raw_gyro_yaw + (1.0f - alpha_gyro) * filtered_gyro_yaw;
    filtered_accel_forward = alpha_accel * raw_accel_forward + (1.0f - alpha_accel) * filtered_accel_forward;
    filtered_accel_vertical = alpha_accel * raw_accel_vertical + (1.0f - alpha_accel) * filtered_accel_vertical;
    
    // 卡尔曼滤波 (Layer 3) - 用于预测
    float dt_sec = dt / 1000.0f;
    turn_kf.update(raw_gyro_yaw, dt_sec);
    accel_kf.update(raw_accel_forward, dt_sec);
    
    filtered_horizon_roll = alpha_horizon * raw_horizon_roll + (1.0f - alpha_horizon) * filtered_horizon_roll;
    filtered_horizon_pitch = alpha_horizon * raw_horizon_pitch + (1.0f - alpha_horizon) * filtered_horizon_pitch;

    // 如果 IMU 读数为 NaN，则触发安全模式
    if (std::isnan(filtered_gyro_yaw) || std::isnan(filtered_accel_forward)) {
        abnormal_state = true;
    }

    // 更新抽象状态
    currentState.turn_intensity = filtered_gyro_yaw;
    // 真正的动态加减速（已在 raw 阶段减去了重力偏移）
    currentState.accel_intensity = filtered_accel_forward;
    // 重力为 1G，去掉重力分量后看绝对值作为颠簸程度
    currentState.vibration = std::abs(filtered_accel_vertical - 1.0f);
    
    currentState.horizon_roll = filtered_horizon_roll;
    currentState.horizon_pitch = filtered_horizon_pitch;

    // Layer 3：提取前瞻预测值 (提前 200ms)
    // 采用预测值作为高频响应，低通作为稳定锚点
    currentState.predicted_turn = turn_kf.predictAhead(200.0f);
    currentState.predicted_accel = accel_kf.predictAhead(200.0f);
}

void IMUProcessor::calibrateZero() {
    // 将当前的滤波后角度累加到 offset 中，使得下一帧计算出的 raw 角度变为 0
    pitch_offset += filtered_horizon_pitch;
    roll_offset += filtered_horizon_roll;
    
    // 同时归零前向加速度（消除因设备倾斜产生的 Z 轴重力分量）
    // 这样不仅地平线会归零，由于倾斜导致跑偏的惯性场色块也会回到屏幕中心
    accel_forward_offset += filtered_accel_forward;
    
    // 重置滤波器状态以避免震荡
    turn_kf.reset(0.0f);
    accel_kf.reset(0.0f);
}

void IMUProcessor::resetCalibration() {
    pitch_offset = 0.0f;
    roll_offset = 0.0f;
    accel_forward_offset = 0.0f;
    
    turn_kf.reset(0.0f);
    accel_kf.reset(0.0f);
}

MotionState IMUProcessor::getMotionState() const {
    return currentState;
}

bool IMUProcessor::isAbnormal() const {
    return abnormal_state;
}
