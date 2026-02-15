# EME Link Budget Calculator

完整的EME（月面反射通信）链路预算计算系统，实现了从发射到接收的完整信号链路和噪声分析。

## 系统特性

### 完整的物理模型

1. **几何与多普勒模块**
   - 实时月球位置计算
   - 地月距离变化分析
   - 多普勒频移计算
   - 支持JPL Horizons星历数据

2. **路径损耗模块**
   - 自由空间传播损耗（双程）
   - 月球散射损耗（雷达截面积模型）
   - 大气气体衰减（频率相关）
   - 低仰角路径延长效应

3. **极化损耗模块**（集成Faraday Rotation）
   - 空间几何旋转（视差角）
   - 法拉第旋转（电离层双折射）
   - 月面反射极化反转
   - Jones矩阵完整计算
   - 支持任意极化类型（线性、圆极化、椭圆极化）

4. **噪声温度模块**
   - 天空噪声温度（408MHz地图缩放）
   - 地面溢出噪声（仰角相关）
   - 馈线热噪声
   - 接收机噪声链路
   - 系统噪声温度综合

5. **SNR与余量模块**
   - 接收信号功率计算
   - 信噪比分析
   - 月球天平动衰落余量
   - 链路可行性判断

## 项目结构

```
EMELinkBudget/
├── src/
│   ├── EMELinkBudget.h/cpp          # 主计算引擎
│   ├── LinkBudgetTypes.h            # 参数和结果结构定义
│   ├── GeometryCalculator.h/cpp     # 几何与多普勒计算
│   ├── PathLossCalculator.h/cpp     # 路径损耗计算
│   ├── PolarizationModule.h/cpp     # 极化模块适配器
│   ├── NoiseCalculator.h/cpp        # 噪声温度计算
│   ├── SNRCalculator.h/cpp          # SNR与余量计算
│   ├── FaradayRotation.h/cpp        # 法拉第旋转核心（原有模块）
│   ├── IonospherePhysics.h/cpp      # 电离层物理模型
│   ├── MaidenheadGrid.h             # 网格定位系统
│   ├── Parameters.h                 # 基础参数定义
│   └── main_linkbudget.cpp          # 示例程序
├── data/
│   ├── calendar.dat                 # 月球星历数据
│   └── WMMHR.COF                    # 世界磁场模型
└── doc/
    └── Architecture.md              # 系统架构文档
```

## 使用示例

### 交互式模式（推荐）

```cpp
// 运行 main_linkbudget_interactive.cpp
// 程序会引导你输入所有参数

EME Link Budget Calculator - Interactive Mode
================================================================================

TX (DX) Station Configuration
Callsign []: W1ABC
Maidenhead Grid Locator (e.g., FN20xa) []: FN20xa
  → Latitude: 41.7083 deg
  → Longitude: -73.9583 deg

Polarization Configuration:
  1. Linear Horizontal
  2. Linear Vertical
  3. RHCP
  4. LHCP
  5. Custom
Select polarization [1.0]: 1

// ... 继续输入其他参数 ...

✅ Link Status: VIABLE - QSO possible!
🎯 LINK MARGIN: 7.5 dB
```

### 编程模式（API）

```cpp
#include "EMELinkBudget.h"

// 创建链路参数
LinkBudgetParameters params;
params.frequency_MHz = 144.0;
params.txPower_dBm = 50.0;  // 100W
params.txGain_dBi = 20.0;
params.rxGain_dBi = 20.0;
params.rxNoiseFigure_dB = 0.5;

// 设置站点（使用Maidenhead网格）
MaidenheadGrid::gridToLatLon("FN20xa",
    params.txSite.latitude, params.txSite.longitude);
MaidenheadGrid::gridToLatLon("PM95vr",
    params.rxSite.latitude, params.rxSite.longitude);

// 设置月球星历和电离层数据
params.moonEphemeris.rightAscension = deg2rad(180.0);
params.moonEphemeris.declination = deg2rad(15.0);
params.ionosphereData.vTEC_DX = 25.0;
params.ionosphereData.vTEC_Home = 30.0;

// 执行计算
EMELinkBudget linkBudget(params);
LinkBudgetResults results = linkBudget.calculate();

// 检查结果
if (results.calculationSuccess) {
    std::cout << "Link Margin: " << results.snr.linkMargin_dB << " dB\n";
    std::cout << "Link Viable: " << results.snr.linkViable << "\n";
}
```

### 频率扫描分析

```cpp
std::vector<double> frequencies = {50.0, 144.0, 432.0, 1296.0, 2400.0};

for (double freq : frequencies) {
    params.frequency_MHz = freq;
    EMELinkBudget calc(params);
    LinkBudgetResults res = calc.calculate();

    std::cout << freq << " MHz: "
              << "Margin = " << res.snr.linkMargin_dB << " dB\n";
}
```

## 数学模型

### 链路预算方程

```
P_RX = P_TX + G_TX + G_RX - L_FS - L_moon - L_atm - L_pol - L_feed
SNR = P_RX - P_Noise
Link Margin = SNR - M_fading - SNR_required
```

### 关键公式

**自由空间损耗**:
```
L_FS = 20·log₁₀(4πR/λ)
```

**月球散射损耗**:
```
L_moon = -10·log₁₀(ρ·πR_moon²/4π) ≈ 51.5 dB
```

**法拉第旋转**:
```
Ω = (0.23647/f²) · sTEC · B_parallel
```

**系统噪声温度**:
```
T_sys = T_ant_eff + T_rx
T_ant_eff = T_ant/L + T_phy·(1-1/L)
```

**极化损耗因子**:
```
PLF = |⟨J_RX | R(Φ_down)·M_moon·R(Φ up)·J_TX⟩|²
```

## 典型参数

### 144 MHz EME
- 发射功率: 100W (50 dBm)
- 天线增益: 20 dBi (4x17 Yagi)
- 接收机NF: 0.5 dB
- 带宽: 2500 Hz (WSJT-X)
- 所需SNR: -21 dB

### 预期性能
- 路径损耗: ~250 dB
- 系统噪声: ~150 K
- 典型余量: 5-10 dB

## 数据源

### 实时数据支持（规划中）
- JPL Horizons: 月球星历
- IONEX/GLOTEC: 电离层TEC数据
- WMM: 地磁场模型
- 408MHz全天图: 天空噪声温度

### 当前版本
- 手动输入所有参数
- 简化的天空噪声模型
- 基于赤纬的银河纬度估算

## 编译

### 选择主程序模式

项目提供两种模式：

1. **交互式模式** (main_linkbudget_interactive.cpp) - 默认启用
   - 支持用户输入所有参数
   - 适合日常使用和实验

2. **演示模式** (main_linkbudget.cpp)
   - 预配置的示例
   - 适合快速测试和学习

切换方法：编辑 `EMELinkBudget.vcxproj`，设置对应文件的 `<ExcludedFromBuild>` 标签。

详见：[主程序切换指南](doc/Main_Program_Guide.md)

### 编译步骤

```bash
# 使用Visual Studio
# 打开 EMELinkBudget.slnx
# 选择 main_linkbudget.cpp 作为启动项
# 编译并运行

# 或使用命令行（需要配置编译器）
cl /EHsc /std:c++17 /I"src" src/*.cpp
```

## 验证

系统已通过以下验证：
- 与WSJT-X内置计算器对比
- 与VK3UM EME Calculator对比
- 实际EME QSO数据验证

## 扩展计划

1. **数据源集成**
   - JPL Horizons API
   - 实时IONEX数据下载
   - 408MHz天空地图加载

2. **高级功能**
   - 时间序列分析（月球轨道周期）
   - 最佳通联时间预测
   - 多站点同时分析

3. **用户界面**
   - 图形化界面
   - 实时监控
   - 结果可视化

## 参考文献

1. ITU-R P.676: Attenuation by atmospheric gases
2. ITU-R P.372: Radio noise
3. Faraday Rotation in EME Communications (ARRL)
4. Moon Bounce Calculator (VK3UM)
5. WSJT-X User Guide

## 许可证

MIT License - 详见 LICENSE 文件

## 作者

Izumi Chino - 2026

基于原有的Faraday Rotation模块扩展而成。
