#include "gk_bag.h"
#include "tal_log.h"
#include "tal_time.h"
#include "math.h"

#define TAG "FORCE_SENSOR"

// 传感器配置
#define SENSOR_I2C_ADDR         0x48
#define SENSOR_SAMPLE_RATE_HZ   100
#define FORCE_THRESHOLD         5.0f    // 注册为打击的最小力值 (N)
#define NOISE_FILTER_ALPHA      0.8f    // 低通滤波器系数

// 传感器校准数据 (实际实现中将从flash加载)
static struct {
    float fx_offset, fy_offset, fz_offset;
    float mx_offset, my_offset, mz_offset;
    float fx_scale, fy_scale, fz_scale;
    float mx_scale, my_scale, mz_scale;
    bool is_calibrated;
} g_sensor_cal = {0};

// 增强的传感器状态，包含滤波历史
static struct {
    gk_force_data_t last_data;
    gk_force_data_t filtered_data;
    bool is_initialized;
    uint32_t hit_count;
    float max_force_session;
    
    // 数据滤波缓冲区 (基于punchingBag dataloader)
    float history[5][6];        // Raw data history for median filter
    float smoothed_history[5][6]; // Smoothed data history
    int history_index;
    int history_count;
} g_sensor_state = {0};

// 模拟传感器硬件接口 (替换为实际驱动程序)
static int read_raw_sensor_data(float* raw_data)
{
    // 模拟传感器读取 - 替换为实际I2C/SPI通信
    // 目前生成一些真实的测试数据
    static float time_counter = 0;
    time_counter += 0.01f; // 100Hz simulation
    
    // 模拟环境噪声
    float noise_level = 0.1f;
    
    // 生成带有小随机变化的基线读数
    raw_data[0] = (rand() % 1000 - 500) * noise_level / 1000.0f; // Fx
    raw_data[1] = (rand() % 1000 - 500) * noise_level / 1000.0f; // Fy  
    raw_data[2] = -9.81f + (rand() % 100 - 50) * noise_level / 100.0f; // Fz (gravity)
    raw_data[3] = (rand() % 1000 - 500) * noise_level / 1000.0f; // Mx
    raw_data[4] = (rand() % 1000 - 500) * noise_level / 1000.0f; // My
    raw_data[5] = (rand() % 1000 - 500) * noise_level / 1000.0f; // Mz
    
    // 偶尔模拟一次打击 (用于测试)
    if ((rand() % 500) == 0) { // Random hit every ~5 seconds at 100Hz
        float hit_force = 50.0f + (rand() % 100); // 50-150N hit
        float hit_angle = (rand() % 360) * M_PI / 180.0f;
        
        raw_data[0] += hit_force * cos(hit_angle) * 0.5f;
        raw_data[1] += hit_force * sin(hit_angle) * 0.5f;
        raw_data[2] += hit_force * 0.8f; // Mainly in Z direction
        
        // 添加对应的力矩
        raw_data[3] += hit_force * 0.1f * sin(hit_angle);
        raw_data[4] += hit_force * 0.1f * cos(hit_angle);
        
        TAL_PR_INFO(TAG, "模拟打击: F=%.1f N, 角度=%.1f 度", hit_force, hit_angle * 180.0f / M_PI);
    }
    
    return OPRT_OK;
}

int gk_sensor_init(void)
{
    TAL_PR_INFO(TAG, "初始化力传感器");
    
    // 初始化I2C/SPI接口 (平台特定)
    // 为模拟，只标记为已初始化
    
    // 从flash加载校准数据
    // 在实际实现中，这将从持久化存储中读取
    g_sensor_cal.fx_offset = 0.0f;
    g_sensor_cal.fy_offset = 0.0f;
    g_sensor_cal.fz_offset = 9.81f; // Gravity offset
    g_sensor_cal.mx_offset = 0.0f;
    g_sensor_cal.my_offset = 0.0f;
    g_sensor_cal.mz_offset = 0.0f;
    
    // 缩放因子 (传感器特定)
    g_sensor_cal.fx_scale = 1.0f;
    g_sensor_cal.fy_scale = 1.0f;
    g_sensor_cal.fz_scale = 1.0f;
    g_sensor_cal.mx_scale = 1.0f;
    g_sensor_cal.my_scale = 1.0f;
    g_sensor_cal.mz_scale = 1.0f;
    
    g_sensor_cal.is_calibrated = true;
    
    // 初始化传感器状态
    memset(&g_sensor_state, 0, sizeof(g_sensor_state));
    g_sensor_state.is_initialized = true;
    
    TAL_PR_INFO(TAG, "力传感器初始化成功");
    return OPRT_OK;
}

// 增强的中值滤波实现 (来自punchingBag)
static float calculate_median(float values[], int count)
{
    // 小数组的简单冒泡排序
    float temp_array[5];
    for (int i = 0; i < count; i++) {
        temp_array[i] = values[i];
    }
    
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (temp_array[j] > temp_array[j + 1]) {
                float temp = temp_array[j];
                temp_array[j] = temp_array[j + 1];
                temp_array[j + 1] = temp;
            }
        }
    }
    
    return temp_array[count / 2];
}

int gk_sensor_read_data(gk_force_data_t* data)
{
    if (!g_sensor_state.is_initialized || !data) {
        return OPRT_INVALID_PARM;
    }
    
    float raw_data[6];
    if (read_raw_sensor_data(raw_data) != OPRT_OK) {
        return OPRT_COM_ERROR;
    }
    
    // 应用校准
    float calibrated_data[6];
    calibrated_data[0] = (raw_data[0] - g_sensor_cal.fx_offset) * g_sensor_cal.fx_scale;
    calibrated_data[1] = (raw_data[1] - g_sensor_cal.fy_offset) * g_sensor_cal.fy_scale;
    calibrated_data[2] = (raw_data[2] - g_sensor_cal.fz_offset) * g_sensor_cal.fz_scale;
    calibrated_data[3] = (raw_data[3] - g_sensor_cal.mx_offset) * g_sensor_cal.mx_scale;
    calibrated_data[4] = (raw_data[4] - g_sensor_cal.my_offset) * g_sensor_cal.my_scale;
    calibrated_data[5] = (raw_data[5] - g_sensor_cal.mz_offset) * g_sensor_cal.mz_scale;
    
    // 基于punchingBag dataloader算法的增强滤波
    
    // 步骤1: 添加到历史缓冲区
    int idx = g_sensor_state.history_index % 5;
    for (int i = 0; i < 6; i++) {
        g_sensor_state.history[idx][i] = calibrated_data[i];
    }
    g_sensor_state.history_index++;
    if (g_sensor_state.history_count < 5) {
        g_sensor_state.history_count++;
    }
    
    // 步骤2: 应用中值滤波
    float median_filtered[6];
    for (int channel = 0; channel < 6; channel++) {
        float channel_values[5];
        for (int i = 0; i < g_sensor_state.history_count; i++) {
            channel_values[i] = g_sensor_state.history[i][channel];
        }
        median_filtered[channel] = calculate_median(channel_values, g_sensor_state.history_count);
    }
    
    // 步骤3: 应用移动平均 (平滑)
    int smooth_idx = (g_sensor_state.history_index - 1) % 5;
    for (int i = 0; i < 6; i++) {
        g_sensor_state.smoothed_history[smooth_idx][i] = median_filtered[i];
    }
    
    float smoothed_data[6] = {0};
    int smooth_count = (g_sensor_state.history_count < 5) ? g_sensor_state.history_count : 5;
    for (int channel = 0; channel < 6; channel++) {
        float sum = 0;
        for (int i = 0; i < smooth_count; i++) {
            sum += g_sensor_state.smoothed_history[i][channel];
        }
        smoothed_data[channel] = sum / smooth_count;
    }
    
    // 填充输出数据结构
    data->fx = smoothed_data[0];
    data->fy = smoothed_data[1];
    data->fz = smoothed_data[2];
    data->mx = smoothed_data[3];
    data->my = smoothed_data[4];
    data->mz = smoothed_data[5];
    data->timestamp = tal_time_get_posix();
    
    g_sensor_state.last_data = *data;
    g_sensor_state.filtered_data = *data;
    
    return OPRT_OK;
}

// 基于punchingBag calculateBoxing算法的增强打击点计算
int gk_calculate_hit_point(gk_force_data_t* force_data, gk_hit_point_t* hit_point)
{
    if (!force_data || !hit_point) {
        return OPRT_INVALID_PARM;
    }
    
    // 力和扭矩向量
    float F[3] = {force_data->fx, force_data->fy, force_data->fz};
    float tau[3] = {force_data->mx, force_data->my, force_data->mz};
    
    // 传感器位置补偿 (来自punchingBag算法)
    // tau[0] = tau[0] + F[1] * 0.365;  // Sensor offset compensation
    // tau[1] = tau[1] + F[0] * 0.185;
    // 对于嵌入式系统，使用更简单的补偿
    tau[0] = tau[0] + F[1] * 0.20f;  // Simplified sensor offset
    tau[1] = tau[1] + F[0] * 0.15f;
    
    // 计算力大小的平方
    float F_norm_sq = F[0]*F[0] + F[1]*F[1] + F[2]*F[2];
    const float eps = 1e-6f;
    
    // 检查力是否显著
    float force_magnitude = sqrtf(F_norm_sq);
    if (force_magnitude < FORCE_THRESHOLD || F_norm_sq < eps) {
        return OPRT_COM_ERROR; // 不是显著的打击
    }
    
    // 计算垂直分量: γ_perp = F × τ / |F|²
    float gamma_perp[3];
    gamma_perp[0] = (F[1] * tau[2] - F[2] * tau[1]) / F_norm_sq;
    gamma_perp[1] = (F[2] * tau[0] - F[0] * tau[2]) / F_norm_sq;
    gamma_perp[2] = (F[0] * tau[1] - F[1] * tau[0]) / F_norm_sq;
    
    // 沙袋半径 (转换为米进行计算)
    float radius = 0.10f; // 10cm radius
    
    // 构造二次方程系数
    float a = F[0]*F[0] + F[1]*F[1];
    float b = 2.0f * (F[0]*gamma_perp[0] + F[1]*gamma_perp[1]);
    float c = gamma_perp[0]*gamma_perp[0] + gamma_perp[1]*gamma_perp[1] - radius*radius;
    
    // 求解参数k1
    float k1;
    if (a < eps) {
        // 力向量几乎垂直
        k1 = -(F[2] * gamma_perp[2]) / (F[2]*F[2] + eps);
    } else {
        float discriminant = b*b - 4.0f*a*c;
        if (discriminant >= 0) {
            float sqrt_disc = sqrtf(discriminant);
            k1 = (-b - sqrt_disc) / (2.0f * a);  // 取物理上有意义的解
        } else {
            // 无解，取最近点
            k1 = -(F[0]*gamma_perp[0] + F[1]*gamma_perp[1]) / a;
        }
    }
    
    // 计算最终撒击点: γ = k1*F + γ_perp
    float gamma[3];
    gamma[0] = k1 * F[0] + gamma_perp[0];
    gamma[1] = k1 * F[1] + gamma_perp[1];
    gamma[2] = k1 * F[2] + gamma_perp[2];
    
    // 转换为左手坐标系 (交换x,y用于显示)
    hit_point->x = gamma[1] * 100.0f;  // Convert m to cm, swap for display
    hit_point->y = gamma[0] * 100.0f;  // Convert m to cm, swap for display
    hit_point->force = force_magnitude;
    hit_point->timestamp = force_data->timestamp;
    
    // 限制在合理范围内
    hit_point->x = fmaxf(-15.0f, fminf(15.0f, hit_point->x));  // ±15cm
    hit_point->y = fmaxf(-15.0f, fminf(15.0f, hit_point->y));  // ±15cm
    
    // 更新会话统计
    g_sensor_state.hit_count++;
    if (force_magnitude > g_sensor_state.max_force_session) {
        g_sensor_state.max_force_session = force_magnitude;
    }
    
    TAL_PR_INFO(TAG, "增强打击计算: 位置(%.1f,%.1f)cm F=%.1fN", 
                hit_point->x, hit_point->y, hit_point->force);
    
    return OPRT_OK;
}

// 额外的传感器实用功能
uint32_t gk_sensor_get_hit_count(void)
{
    return g_sensor_state.hit_count;
}

float gk_sensor_get_max_force_session(void)
{
    return g_sensor_state.max_force_session;
}

void gk_sensor_reset_session_stats(void)
{
    g_sensor_state.hit_count = 0;
    g_sensor_state.max_force_session = 0.0f;
    TAL_PR_INFO(TAG, "会话统计已重置");
}

int gk_sensor_calibrate(void)
{
    TAL_PR_INFO(TAG, "开始传感器校准");
    
    // 校准程序:
    // 1. 读取多个样本以获取基线偏移
    // 2. 将校准数据保存到flash
    
    const int cal_samples = 100;
    float sum_fx = 0, sum_fy = 0, sum_fz = 0;
    float sum_mx = 0, sum_my = 0, sum_mz = 0;
    
    for (int i = 0; i < cal_samples; i++) {
        float raw_data[6];
        if (read_raw_sensor_data(raw_data) == OPRT_OK) {
            sum_fx += raw_data[0];
            sum_fy += raw_data[1]; 
            sum_fz += raw_data[2];
            sum_mx += raw_data[3];
            sum_my += raw_data[4];
            sum_mz += raw_data[5];
        }
        tal_system_sleep(10); // 10ms延迟
    }
    
    // 计算平均值作为偏移
    g_sensor_cal.fx_offset = sum_fx / cal_samples;
    g_sensor_cal.fy_offset = sum_fy / cal_samples;
    g_sensor_cal.fz_offset = sum_fz / cal_samples - (-9.81f); // Account for gravity
    g_sensor_cal.mx_offset = sum_mx / cal_samples;
    g_sensor_cal.my_offset = sum_my / cal_samples;
    g_sensor_cal.mz_offset = sum_mz / cal_samples;
    
    g_sensor_cal.is_calibrated = true;
    
    // 在实际实现中，在此保存到flash
    
    TAL_PR_INFO(TAG, "传感器校准完成");
    TAL_PR_INFO(TAG, "Offsets: Fx=%.3f Fy=%.3f Fz=%.3f Mx=%.3f My=%.3f Mz=%.3f",
                g_sensor_cal.fx_offset, g_sensor_cal.fy_offset, g_sensor_cal.fz_offset,
                g_sensor_cal.mx_offset, g_sensor_cal.my_offset, g_sensor_cal.mz_offset);
    
    return OPRT_OK;
}