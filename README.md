[中文版 (Chinese)](#中文) | [English Version](#english)

---

<h1 id="中文">HorizonAid</h1>

一个基于 IMU 的车载视觉惯性补偿系统，通过“极简地平线 + 惯性偏移场”构建稳定外部参考系，在余光模式下降低视觉-前庭冲突，从而缓解晕车。

**核心关键词：**
低延迟 | 极简参考系 | 惯性而非动画 | 余光感知 | 零内部静态锚点

## 状态 (Status)
**已完结 (Finished)** - 核心架构与预测层均已完成部署，无需进一步修改即可进行实车运行。

## 技术架构
本项目基于 **M5StickS3 (ESP32-S3)** 硬件平台，使用 C++ 与 PlatformIO 开发，采用 `M5Unified` 获取底层硬件数据，并通过 `LovyanGFX` 进行高性能的双缓冲界面渲染。

系统界面在设计上被严格划分为三个核心逻辑层：

### 🟢 Layer 1：地平线锚点 (Horizon Layer)
**主参考系统，必须保持绝对稳定。**
- **核心作用**：在余光中提供“世界不动”的绝对空间锚点。
- **数据来源**：IMU 实时解算的 Pitch 与 Roll 角度。
- **视觉特性**：极度平滑、超强阻尼、禁止任何数据跳变。它是一条极其可信的绝对参考线，不受微小的高频震动干扰。

### 🌫️ Layer 2：惯性偏移场 (Inertia Field)
**本项目的核心创新层，负责建立空间拖拽感。**
- **核心作用**：通过不同屏幕区域的“延迟运动响应”，建立一种视觉粘滞感。
- **数据来源**：IMU 的加速度 (Accel) 与角速度 (Gyro)。
- **视觉特性**：
  - **三维深度视差**：打散为前景、中景、背景三个深度层，每层拥有完全独立的物理刚度与阻尼。
  - **非流体非粒子**：不使用花哨的粒子或流体动画，而是通过低对比度的短线组合，仅仅在余光中暗示空间拖拽的方向。
  - **强滞后性**：旋转或加减速时不会突变，而是表现出具有质量的缓慢滑动。
  - **无内部静态锚点 (Zero Static Anchors)**：刻意拒绝在屏幕中央绘制任何静止的十字或基准标记。因为固定的几何图形会强制吸引中心视觉焦点导致分心，而设备的物理边框及车体内部环境本身已构成了完美的绝对静止参考系。保留屏幕内部 100% 的纯动态像素，可最大程度激发大脑边缘视觉的运动补偿效率。

### 🔮 Layer 3：行为预测层 (Prediction Layer - 已实现)
**本项目的核心响应引擎，利用卡尔曼滤波消除物理硬件延迟。**
- **核心作用**：通过追踪传感器变化趋势，提前 100–300ms 推动视觉 Offset，彻底消除底层低通滤波带来的视觉滞后感，实现极致的“跟手”体验。
- **数据来源**：1D 卡尔曼滤波器提取的加速度与角速度的趋势导数 (Trend / Derivative)。

> [!WARNING]
> **关于卡尔曼滤波预测的权衡**
> 引入 Layer 3 预测层后，系统会对运动趋势（导数）极其敏感。这虽然能完美消除刹车和转向时的视觉滞后，但也可能导致画面在遇到瞬间颠簸（如过减速带）时出现轻微的**高频颤动 (Jitter)**。系统默认的观测噪声(R)与过程噪声(Q)已取得平衡，若实车测试中发现颤动过于影响观感，可在代码中调整降低滤波器的敏感度。

## 使用说明
本项目针对 M5StickS3 设计，使用 PlatformIO 进行编译和烧录。
由于系统设计为车载前置横屏，设备 USB 接口应当放置在**左侧**（即横屏状态）。
1. 烧录后将设备水平放置于车辆中控台等边缘。
2. **姿态校准（归零）**：由于车台不一定完全水平，设备固定好后，**短按 M5 按钮 (BtnA)**，系统会将当前角度设为绝对水平。如果需要恢复原始真实读数，可以**长按 M5 按钮**。
3. 当车辆转向、加减速时，屏幕会产生对应的低频惯性偏移，为您建立外部空间的稳定参考系。
4. 剧烈晃动或传感器异常时，系统会自动切入红色的 Safe Mode。

## License
[MIT License](LICENSE)

---

<h1 id="english">HorizonAid</h1>

An IMU-based in-vehicle visual-inertial compensation system that builds a stable external reference frame through a "minimalist horizon + inertia offset field" to reduce visual-vestibular conflict in peripheral vision, thereby alleviating motion sickness.

**Core Keywords:**
Low Latency | Minimalist Reference | Inertia Not Animation | Peripheral Awareness | Zero Static Anchors

## Status
**Finished** - The core architecture and prediction layers are fully deployed and ready for real-world vehicle testing without further modification.

## Architecture
This project is built on the **M5StickS3 (ESP32-S3)** hardware platform using C++ and PlatformIO. It utilizes `M5Unified` for low-level hardware data acquisition and `LovyanGFX` for high-performance double-buffered UI rendering.

The system interface is strictly divided into three core logical layers:

### 🟢 Layer 1: Horizon Layer (Anchor)
**The primary reference system, which must remain absolutely stable.**
- **Core Function**: Provides an "unmoving world" absolute spatial anchor in the peripheral vision.
- **Data Source**: Real-time calculated Pitch and Roll angles from the IMU.
- **Visual Features**: Extremely smooth, highly damped, and strictly forbids any data jumping. It acts as an incredibly reliable absolute reference line, immune to minor high-frequency vibrations.

### 🌫️ Layer 2: Inertia Field
**The core innovation layer of this project, responsible for creating a sense of spatial drag.**
- **Core Function**: Establishes a visual sense of viscosity through "delayed motion response" across different screen areas.
- **Data Source**: Acceleration (Accel) and Angular Velocity (Gyro) from the IMU.
- **Visual Features**:
  - **3D Depth Parallax**: Separated into foreground, midground, and background depth layers, each with completely independent physical stiffness and damping.
  - **Non-fluid & Non-particle**: Avoids flashy particle or fluid animations. Instead, it uses low-contrast short line combinations to subtly hint at the direction of spatial drag purely in the peripheral vision.
  - **Strong Hysteresis**: No sudden jumps during rotation or acceleration/deceleration; instead, it exhibits a slow, mass-like sliding motion.
  - **Zero Static Anchors (Put & Forget)**: Deliberately refuses to draw any static crosshairs or reference markers in the center of the screen. Fixed geometric shapes would forcefully attract foveal vision (central focus) and cause distraction. The physical bezels of the device and the interior environment of the car already form a perfect absolute static reference frame. Retaining 100% dynamic pixels on the screen maximizes the motion compensation efficiency of the brain's peripheral vision.

### 🔮 Layer 3: Prediction Layer (Implemented)
**The core response engine of this project, utilizing Kalman filtering to eliminate physical hardware latency.**
- **Core Function**: By tracking sensor trend derivatives, it pushes the visual Offset 100-300ms ahead of time. This completely eliminates the visual lag caused by low-pass filtering, achieving an ultimate "zero-latency" tracking experience.
- **Data Source**: Trend derivatives of acceleration and angular velocity extracted by a 1D Kalman Filter.

> [!WARNING]
> **Trade-offs of Kalman Filter Prediction**
> With the introduction of the Layer 3 prediction layer, the system becomes highly sensitive to motion trends (derivatives). While this perfectly eliminates visual lag during braking and turning, it may also cause slight **high-frequency jitter** on the screen when encountering sudden bumps (e.g., speed bumps). The system's default observation noise (R) and process noise (Q) are currently balanced. If the jitter is too distracting during real-world testing, you can adjust the code to lower the filter's sensitivity.

## Usage Instructions
This project is designed for the M5StickS3, compiled and flashed using PlatformIO.
Since the system is designed to be a front-mounted landscape display in a vehicle, the device's USB port should be placed on the **left side** (landscape mode).
1. After flashing, place the device horizontally on the edge of the vehicle's dashboard.
2. **Attitude Calibration (Zeroing)**: Since the dashboard may not be perfectly level, firmly fix the device, then **short press the M5 Button (BtnA)**. The system will set the current angle as absolute horizontal. To restore the raw sensor readings, **long press the M5 Button**.
3. As the vehicle turns, accelerates, or decelerates, the screen will generate corresponding low-frequency inertial offsets, establishing a stable reference frame of the external space for you.
4. During violent shaking or sensor anomalies, the system will automatically enter a red Safe Mode.

## License
[MIT License](LICENSE)
