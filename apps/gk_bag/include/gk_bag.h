#ifndef __GK_BAG_H__
#define __GK_BAG_H__

#include "tuya_cloud_types.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 用户信息结构
typedef struct {
    char user_id[64];
    char username[32];
    int height;  // 厘米
    int weight;  // 千克
    int gender;  // 0: 男性, 1: 女性
    bool is_logged_in;
} gk_user_info_t;

// Combat stats structure (五边形战力图)
typedef struct {
    int speed;      // 速度 (0-100)
    int power;      // 爆发力 (0-100)
    int endurance;  // 耐力 (0-100)
    int accuracy;   // 准确度 (0-100)
    int technique;  // 技巧 (0-100)
} gk_combat_stats_t;

// 力传感器数据结构
typedef struct {
    float fx, fy, fz;  // 力分量
    float mx, my, mz;  // 力矩分量
    uint32_t timestamp;
} gk_force_data_t;

// 增强的打击点计算结果
typedef struct {
    float x, y;        // 打击坐标（厘米）
    float force;       // 打击力大小（牛顿）
    float residual;    // 算法残差，用于精度评估
    uint32_t timestamp;
} gk_hit_point_t;

// GUI页面枚举 (已简化)
typedef enum {
    GK_PAGE_MAIN_STATS = 0,
    GK_PAGE_HIT_VISUAL,
    GK_PAGE_TRAINING,
    GK_PAGE_MAX
} gk_page_t;

// 函数声明
int gk_bag_init(void);
int gk_gui_init(void);
int gk_sensor_init(void);

// 简化的用户管理功能
void gk_set_default_user_info(void);
gk_user_info_t* gk_get_user_info(void);

// GUI功能
void gk_gui_show_page(gk_page_t page);
void gk_gui_update_combat_stats(gk_combat_stats_t* stats);
void gk_gui_update_hit_visual(gk_hit_point_t* hit_point);  // 使用punchingBag算法增强
void gk_gui_update_main_stats_page(void);
void gk_gui_draw_stick_figure(lv_obj_t* parent, int height, int weight, int gender);

// 传感器功能
int gk_sensor_read_data(gk_force_data_t* data);
int gk_calculate_hit_point(gk_force_data_t* force_data, gk_hit_point_t* hit_point);
uint32_t gk_sensor_get_hit_count(void);
float gk_sensor_get_max_force_session(void);
void gk_sensor_reset_session_stats(void);
int gk_sensor_calibrate(void);

#ifdef __cplusplus
}
#endif

#endif /* __GK_BAG_H__ */