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
        // Y轴 (加减速) 依然使用卡尔曼预测值以消除迟滞
        float accel_val = current_motion.predicted_accel;
        
        // X轴 (转向) 作为速度积分器，必须放弃使用含有求导外推的卡尔曼预测值！
        // 因为急停时角速度导数为极大的负值，卡尔曼外推会预测出一个“反向速度”，导致色块回弹。
        // 直接使用经过低通滤波的真实角速度，确保速度只会平滑降至0而绝不穿过零点回弹。
        float turn_val = current_motion.turn_intensity;
        if (std::abs(turn_val) < VISUAL_TURN_DEADZONE) {
            turn_val = 0.0f;
        }
        
        // Y 轴 (加减速)：保持目标位置的弹簧阻尼模型
        float normalized_i = (float(i) - N / 2.0f) / (N / 2.0f); 
        float target_y = accel_val * cfg.k_accel * normalized_i;
        target_y += current_motion.vibration * 2.0f;

        // X 轴 (转向)：重构为目标速度模型
        float target_v_x = turn_val * cfg.k_turn;

        // -------------------------
        // Y 轴计算 (位置弹簧控制)
        // -------------------------
        float force_y = (target_y - lines[i].offset.y) * cfg.stiffness;
        lines[i].velocity.y = (lines[i].velocity.y + force_y) * cfg.damping;
        lines[i].offset.y += lines[i].velocity.y;

        // -------------------------
        // X 轴计算 (速度积分控制)
        // -------------------------
        // 用低通滤波模拟速度上的惯性迟滞，让起步和刹停更加平滑
        lines[i].velocity.x = lines[i].velocity.x * 0.9f + target_v_x * 0.1f;
        lines[i].offset.x += lines[i].velocity.x;

        // -------------------------
        // X 轴 Wrap-around 逻辑 (无缝循环滚动)
        // -------------------------
        float WRAP = 240.0f; // 屏幕宽度周期
        if (lines[i].offset.x > WRAP) {
            lines[i].offset.x -= WRAP;
        } else if (lines[i].offset.x < 0.0f) {
            lines[i].offset.x += WRAP;
        }
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

    // 为不同深度分配横线的长度 (缩短以提升高速运动时的方向辨别度)
    int layer_lengths[3] = { 14, 30, 54 };

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

            float vx = lines[i].velocity.x;
            float stretch = (current_layer == FOREGROUND) ? OPTIC_FLOW_STRETCH_FG : ((current_layer == MIDGROUND) ? OPTIC_FLOW_STRETCH_MG : OPTIC_FLOW_STRETCH_BG);
            int tail_len = (int)(std::abs(vx) * stretch);

            // 绘制单个色块的 Lambda
            auto drawBlock = [&](float x) {
                // 1. 绘制尾迹
                if (tail_len > 2) {
                    int tx = (vx > 0) ? ((int)x - tail_len) : ((int)x + len);
                    canvas.fillRect(tx, rect_y, tail_len, thickness, t_color);
                }
                // 2. 绘制主体
                canvas.fillRect((int)x, rect_y, len, thickness, color);
            };

            // 周期为 240px，绘制左中右 3 个周期，确保在 offset wrap-around 时视觉上完全无缝
            float WRAP = 240.0f;
            for (int k = -1; k <= 1; k++) {
                drawBlock(draw_x1 + k * WRAP);
                drawBlock(draw_x2 + k * WRAP);
            }
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
    // 并且反转 Roll 方向，使车体右倾时，地平线相对屏幕左倾，严格锚定真实世界
    float display_roll = -current_motion.horizon_roll;
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
