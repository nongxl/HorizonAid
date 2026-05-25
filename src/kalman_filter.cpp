#include "kalman_filter.h"

SimpleKalmanFilter::SimpleKalmanFilter(float process_noise, float measurement_noise, float estimation_error) {
    q = process_noise;
    r = measurement_noise;
    initial_p = estimation_error;
    reset(0.0f);
}

void SimpleKalmanFilter::reset(float initial_value) {
    x_hat = initial_value;
    v_hat = 0.0f;
    
    p00 = initial_p;
    p01 = 0.0f;
    p10 = 0.0f;
    p11 = initial_p;
}

void SimpleKalmanFilter::update(float measurement, float dt) {
    if (dt <= 0.0f) return;

    // --- 1. 预测阶段 (Predict) ---
    // 状态外推: x_new = x + v * dt
    x_hat += v_hat * dt;
    
    // 协方差外推: P = F * P * F^T + Q
    // 其中 F = [1, dt]
    //          [0,  1]
    float p00_temp = p00 + dt * (p10 + p01 + dt * p11);
    float p01_temp = p01 + dt * p11;
    float p10_temp = p10 + dt * p11;
    float p11_temp = p11 + q; // 将过程噪声加在速度的方差上，允许速度发生变化
    
    p00 = p00_temp;
    p01 = p01_temp;
    p10 = p10_temp;
    p11 = p11_temp;
    
    // --- 2. 更新阶段 (Update) ---
    // 计算卡尔曼增益: K = P * H^T * (H * P * H^T + R)^-1
    // 其中 H = [1, 0] (我们只测量了位置/数值，没有直接测量速度)
    float s = p00 + r; // 预测残差的方差
    float k0 = p00 / s;
    float k1 = p10 / s;
    
    // 测量残差
    float y = measurement - x_hat;
    
    // 状态更新
    x_hat += k0 * y;
    v_hat += k1 * y;
    
    // 协方差更新: P = (I - K * H) * P
    float p00_new = (1.0f - k0) * p00;
    float p01_new = (1.0f - k0) * p01;
    float p10_new = -k1 * p00 + p10;
    float p11_new = -k1 * p01 + p11;
    
    p00 = p00_new;
    p01 = p01_new;
    p10 = p10_new;
    p11 = p11_new;
}

float SimpleKalmanFilter::getValue() const {
    return x_hat;
}

float SimpleKalmanFilter::getVelocity() const {
    return v_hat;
}

float SimpleKalmanFilter::predictAhead(float time_ms) const {
    float dt = time_ms / 1000.0f;
    return x_hat + v_hat * dt;
}
