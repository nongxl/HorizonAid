#include "visual_engine.h"
#include <cmath>
#include "config.h"

VisualEngine::VisualEngine() {
    for (int i = 0; i < N; i++) {
        lines[i].offset = {0.0f, 0.0f};
        lines[i].velocity = {0.0f, 0.0f};
        // 利用取模打乱分布，形成自然的深度交错感
        lines[i].layer = (DepthLayer)((i * 7) % 3);
    }
}

void VisualEngine::init(M5GFX* disp) {
    display = disp;
    width = display->width();
    height = display->height();

    // 初始化 Sprite (双缓冲)
    // 换回8位色深，彻底解决 16位色深可能导致的底层黑色背景发灰/发亮问题
    canvas.setColorDepth(8); 
    canvas.createSprite(width, height);

    float bandHeight = (float)height / N;
    for (int i = 0; i < N; i++) {
        lines[i].default_y = i * bandHeight + bandHeight / 2.0f;
        // 随机错开起始X，打破垂直对齐规律
        lines[i].base_x = (i % 2 == 0) ? -20.0f : (width / 3.0f);
    }
}

void VisualEngine::update(const MotionState& motion, bool safeMode) {
    current_motion = motion;
    is_safe_mode = safeMode;

    if (is_safe_mode) {
        // 安全模式下 Inertia Field 禁用，逐渐归零
        for (int i = 0; i < N; i++) {
            lines[i].velocity.x *= 0.8f;
            lines[i].velocity.y *= 0.8f;
            lines[i].offset.x *= 0.9f;
            lines[i].offset.y *= 0.9f;
        }
        return;
    }

    // 定义三层物理参数：创造极低频的空间错位（视差）感
    struct LayerConfig {
        float stiffness;
        float damping;
        float k_turn;
        float k_accel;
    };
    
    const LayerConfig layer_cfg[3] = {
        // BACKGROUND: 极低频漂移，但在放大惯性后也能有明显的跟随
        { 0.003f, 0.95f, INERTIA_K_TURN_BG, INERTIA_K_ACCEL_BG },
        // MIDGROUND: 
        { 0.010f, 0.90f, INERTIA_K_TURN_MG, INERTIA_K_ACCEL_MG },
        // FOREGROUND: 极大放大加减速时的垂直拉扯感
        { 0.025f, 0.82f, INERTIA_K_TURN_FG, INERTIA_K_ACCEL_FG }
    };

    for (int i = 0; i < N; i++) {
        const LayerConfig& cfg = layer_cfg[lines[i].layer];

        // 目标偏移计算
        // 引入 Layer 3 预测值：利用卡尔曼滤波器提取的趋势，提前推动目标位置
        // 这消除了原本 IIR 低通滤波带来的迟滞感，使视觉反馈更“跟手”
        float turn_val = current_motion.predicted_turn;
        float accel_val = current_motion.predicted_accel;
        
        float target_x = turn_val * cfg.k_turn;
        
        float normalized_i = (float(i) - N / 2.0f) / (N / 2.0f); 
        float target_y = accel_val * cfg.k_accel * normalized_i;
        target_y += current_motion.vibration * 2.0f;

        // 弹簧阻尼系统
        float force_x = (target_x - lines[i].offset.x) * cfg.stiffness;
        float force_y = (target_y - lines[i].offset.y) * cfg.stiffness;

        lines[i].velocity.x = (lines[i].velocity.x + force_x) * cfg.damping;
        lines[i].velocity.y = (lines[i].velocity.y + force_y) * cfg.damping;

        lines[i].offset.x += lines[i].velocity.x;
        lines[i].offset.y += lines[i].velocity.y;
    }
}

void VisualEngine::render() {
    // 1. 绘制背景
    uint16_t bgColor = is_safe_mode ? canvas.color565(10, 10, 10) : canvas.color565(15, 20, 25);
    canvas.fillScreen(bgColor);

    // 2. 绘制 Inertia Field (在底下，不抢注意力)
    if (!is_safe_mode) {
        drawInertiaField();
    }

    // 3. 绘制 Horizon (最上层)
    drawHorizon();

    // 4. 将缓冲推送到屏幕
    canvas.pushSprite(display, 0, 0);
}

void VisualEngine::drawInertiaField() {
    // 为不同深度分配不同对比度主体颜色
    uint16_t layer_colors[3] = {
        canvas.color565(30, 45, 30),   // BACKGROUND: 极暗
        canvas.color565(60, 90, 60),   // MIDGROUND: 中等暗淡
        canvas.color565(90, 140, 90)   // FOREGROUND: 最亮
    };
    
    // 光轨尾迹颜色（使用明显区别于主体的颜色，以更清晰地指示运动方向）
    uint16_t tail_colors[3] = {
        canvas.color565(15, 25, 15),
        canvas.color565(30, 50, 30),
        canvas.color565(45, 75, 45)
    };

    // 为不同深度分配横线的长度
    int layer_lengths[3] = { 20, 45, 80 };

    // 为不同深度分配基础厚度 (像素)
    int layer_base_thickness[3] = { 1, 2, 4 };

    // =========================================================
    // 渲染管线：严格的 Z-Sorting (从深到浅: 0 -> 1 -> 2)
    // =========================================================
    
    for (int current_layer = 0; current_layer < 3; current_layer++) {
        for (int i = 0; i < N; i++) {
            if (lines[i].layer != current_layer) continue;

            float draw_y = lines[i].default_y + lines[i].offset.y;
            float draw_x1 = lines[i].base_x + lines[i].offset.x;
            float draw_x2 = lines[i].base_x + 140.0f + lines[i].offset.x;
            
            uint16_t color = layer_colors[current_layer];
            uint16_t t_color = tail_colors[current_layer];
            int len = layer_lengths[current_layer];

            // --- 动态厚度计算 (3D 透视原理) ---
            // 越偏离地平线（屏幕中心），视线与该“色块”平面的夹角越大，看起来就越厚
            float dist_from_center = std::abs(draw_y - (height / 2.0f));
            int extra_thickness = (int)(dist_from_center * 0.05f); // 距离越远，额外厚度越大
            int thickness = layer_base_thickness[current_layer] + extra_thickness;

            // 为了让色块以 Y 坐标居中，向上偏移一半的厚度
            int rect_y = (int)draw_y - (thickness / 2);

            // 1. 先绘制该线段的尾迹光流 (色块)
            float vx = lines[i].velocity.x;
            float stretch = (current_layer == FOREGROUND) ? OPTIC_FLOW_STRETCH_FG : ((current_layer == MIDGROUND) ? OPTIC_FLOW_STRETCH_MG : OPTIC_FLOW_STRETCH_BG);
            int tail_len = (int)(std::abs(vx) * stretch);

            if (tail_len > 2) {
                // 速度为正（向右移动），尾迹在左侧；反之在右侧
                int tx1 = (vx > 0) ? ((int)draw_x1 - tail_len) : ((int)draw_x1 + len);
                canvas.fillRect(tx1, rect_y, tail_len, thickness, t_color);

                int tx2 = (vx > 0) ? ((int)draw_x2 - tail_len) : ((int)draw_x2 + len);
                canvas.fillRect(tx2, rect_y, tail_len, thickness, t_color);
            }

            // 2. 再绘制该线段的主体 (色块)
            canvas.fillRect((int)draw_x1, rect_y, len, thickness, color);
            canvas.fillRect((int)draw_x2, rect_y, len, thickness, color);
        }
    }
}

void VisualEngine::drawHorizon() {
    uint16_t horizonColor = is_safe_mode ? canvas.color565(150, 50, 50) : canvas.color565(200, 220, 255);
    
    // 计算地平线的两端点
    float cx = width / 2.0f;
    float cy = height / 2.0f;

    // 放大 Pitch 带来的 Y 轴垂直偏移，并使用 tanh() 进行柔性限幅 (Soft-Clipping)
    // 保证在小角度时依然有夸张的放大感，但大角度时平滑减速，绝对不会飞出屏幕
    float max_y_offset = (height / 2.0f) - VISUAL_HORIZON_MARGIN; // 限制最大偏移，保留边缘裕量
    float factor = VISUAL_PITCH_MULTIPLIER / max_y_offset;        // 推导系数：保证在接近 0 时的放大斜率
    
    float y_offset = std::tanh(current_motion.horizon_pitch * factor) * max_y_offset;
    cy += y_offset;

    // 添加 Roll 视觉死区 (Deadzone)
    // 因为在低分辨率下（240px），极小角度的倾斜会导致线段在屏幕中间断裂成阶梯
    float display_roll = current_motion.horizon_roll;
    if (std::abs(display_roll) < VISUAL_ROLL_DEADZONE) {
        display_roll = 0.0f;
    }

    // roll 带来的倾斜
    float rad = display_roll * M_PI / 180.0f;
    float cos_r = cos(rad);
    float sin_r = sin(rad);

    // 计算足够长的线段以覆盖屏幕
    float hw = width * 0.6f;
    
    // 根据设定的粗细绘制多条平行线，加粗以提高在色块中的辨识度
    for (int t = 0; t < VISUAL_HORIZON_THICKNESS; t++) {
        int offset = t - VISUAL_HORIZON_THICKNESS / 2;
        
        // 修复旋转变细问题：避免浮点数小数截断导致坐标重叠
        // 经典线宽算法：如果线接近水平，则沿 Y 轴垂直偏移画平行线；如果线接近垂直，则沿 X 轴偏移
        int ox = 0, oy = 0;
        if (std::abs(cos_r) > std::abs(sin_r)) {
            oy = offset; // 接近水平
        } else {
            ox = offset; // 接近垂直
        }
        
        canvas.drawLine(
            (int)(cx - hw * cos_r) + ox, (int)(cy - hw * sin_r) + oy,
            (int)(cx + hw * cos_r) + ox, (int)(cy + hw * sin_r) + oy,
            horizonColor
        );
    }
}
