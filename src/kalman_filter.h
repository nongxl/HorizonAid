#pragma once

// 轻量级 1D 卡尔曼滤波器
// 追踪单个变量的位置 (Value) 和速度 (Derivative)，用于趋势预测
class SimpleKalmanFilter {
public:
    // process_noise: 过程噪声 (Q)，越小则系统越认为速度恒定，预测较平滑但响应慢
    // measurement_noise: 测量噪声 (R)，越大则系统越不信任传感器，滤波效果强但滞后
    // estimation_error: 初始估计误差 (P)，通常设为一个适中值，如 1.0
    SimpleKalmanFilter(float process_noise, float measurement_noise, float estimation_error);
    
    // 更新测量值
    // measurement: 传感器原始读数
    // dt: 距离上次更新的时间间隔 (秒)
    void update(float measurement, float dt);
    
    // 获取当前滤波后的值
    float getValue() const;
    
    // 获取当前计算出的变化率 (导数)
    float getVelocity() const;
    
    // 获取提前 time_ms 毫秒的预测值 (基于当前值和当前速度)
    float predictAhead(float time_ms) const;
    
    // 重置滤波器状态
    void reset(float initial_value);

private:
    float x_hat; // 估计值
    float v_hat; // 估计速度
    
    // 协方差矩阵 P
    float p00, p01, p10, p11;
    
    float q; // 过程噪声方差
    float r; // 测量噪声方差
    float initial_p;
};
