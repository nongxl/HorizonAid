#pragma once
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────
//  M5StickS3 Hardware Template Config
// ─────────────────────────────────────────────────────────────

static constexpr int    SCREEN_W_P         = 135;
static constexpr int    SCREEN_H_P         = 240;

static constexpr int    SCREEN_W_L         = 240;
static constexpr int    SCREEN_H_L         = 135;

static constexpr float  IMU_LPF_ALPHA      = 0.92f;
static constexpr float  IMU_DEADZONE       = 0.12f;

static constexpr uint8_t SYSTEM_VOLUME     = 120;
static constexpr uint8_t SYSTEM_BRIGHTNESS = 80;

static constexpr int    VIBR_PIN           = 0;
static constexpr int    VIBR_PWM_CHANNEL   = 2;
static constexpr int    VIBR_PWM_FREQ      = 10000;
static constexpr int    VIBR_PWM_BITS      = 8;

// ─────────────────────────────────────────────────────────────
//  HorizonAid 视觉与物理引擎配置 (Visual & Physics Config)
// ─────────────────────────────────────────────────────────────

// [地平线锚点参数 (Layer 1)]
// 地平线俯仰角(Pitch)的视觉放大倍率。
// 由于真实车内的物理倾角极小(常在2度以内)，需放大此系数以向大脑提供足够夸张的运动预期。
static constexpr float VISUAL_PITCH_MULTIPLIER = 8.0f;

// 地平线白线的渲染粗细(像素)。加大粗细能使其在动态色块中保持绝对的核心辨识度。
static constexpr int VISUAL_HORIZON_THICKNESS = 3;

// 地平线软限幅的屏幕边缘安全距离(像素)。保证在急刹车等极度颠簸时主参考线也不会飞出屏幕失效。
static constexpr float VISUAL_HORIZON_MARGIN = 5.0f;

// 翻滚角(Roll)的视觉死区(度)。过滤掉路面微小不平整导致的地平线单像素阶梯跳动。
static constexpr float VISUAL_ROLL_DEADZONE = 1.5f;

// [三维惯性偏移场参数 (Layer 2)]
// 转向敏感度 (Yaw 旋转角速度 -> X轴横向位移的映射系数)
static constexpr float INERTIA_K_TURN_BG = 0.6f;  // 背景层 (最远，位移极小，稳定)
static constexpr float INERTIA_K_TURN_MG = 1.2f;  // 中景层 
static constexpr float INERTIA_K_TURN_FG = 1.8f;  // 前景层 (最近，横向撕扯感强烈)

// 加减速敏感度 (Accel Z 线性加速度 -> Y轴纵向位移的映射系数)
// 同样需要巨大放大，以使得 0.2G 左右的微小制动力在视觉上也能产生明显的“深潜”拖拽感
static constexpr float INERTIA_K_ACCEL_BG = -15.0f;
static constexpr float INERTIA_K_ACCEL_MG = -30.0f;
static constexpr float INERTIA_K_ACCEL_FG = -60.0f;

// 光流尾迹拉伸倍率 (Optic Flow Stretch)
// 决定了在一定移动速度下，色块拖尾的长度。前景层需要最强烈的“星际穿越”极速感。
static constexpr float OPTIC_FLOW_STRETCH_BG = 1.0f;
static constexpr float OPTIC_FLOW_STRETCH_MG = 3.0f;
static constexpr float OPTIC_FLOW_STRETCH_FG = 5.0f;
