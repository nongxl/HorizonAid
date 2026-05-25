#ifndef VISUAL_ENGINE_H
#define VISUAL_ENGINE_H

#include <M5Unified.h>
#include "imu_processor.h"

struct Vector2D {
    float x;
    float y;
};

class VisualEngine {
public:
    VisualEngine();
    void init(M5GFX* display);
    void update(const MotionState& motion, bool safeMode);
    void render();

private:
    M5GFX* display;
    M5Canvas canvas;

    int width;
    int height;

    // Layer 2: Inertia Field 深度层变量
    static const int N = 24;
    
    enum DepthLayer { 
        BACKGROUND = 0, 
        MIDGROUND = 1, 
        FOREGROUND = 2 
    };

    struct LineElement {
        float default_y;
        float base_x;
        DepthLayer layer;
        Vector2D offset;
        Vector2D velocity;
    };

    LineElement lines[N];



    // 状态
    bool is_safe_mode = false;
    MotionState current_motion;

    void drawHorizon();
    void drawInertiaField();
};

#endif // VISUAL_ENGINE_H
