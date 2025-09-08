#include "gk_bag.h"
#include "tal_log.h"
#include "lvgl.h"
#include "math.h"

#define TAG "GUI_GRAPHICS"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 战斗统计画布
static lv_obj_t* g_combat_stats_canvas = NULL;
static lv_draw_buf_t g_combat_draw_buf;
static uint8_t g_combat_canvas_buf[400 * 300 * LV_COLOR_DEPTH / 8];

// 火柴人画布
static lv_obj_t* g_stick_figure_canvas = NULL;
static lv_draw_buf_t g_stick_draw_buf;
static uint8_t g_stick_canvas_buf[200 * 300 * LV_COLOR_DEPTH / 8];

// 当前统计
static gk_combat_stats_t g_current_stats = {50, 50, 50, 50, 50}; // Default values

void gk_gui_draw_stick_figure(lv_obj_t* parent, int height, int weight, int gender)
{
    TAL_PR_INFO(TAG, "绘制火柴人: H=%d W=%d G=%d", height, weight, gender);
    
    if (!g_stick_figure_canvas) {
        // 为火柴人创建画布
        g_stick_figure_canvas = lv_canvas_create(parent);
        lv_canvas_set_buffer(g_stick_figure_canvas, g_stick_canvas_buf, 200, 300, LV_COLOR_FORMAT_RGB565);
        lv_obj_align(g_stick_figure_canvas, LV_ALIGN_LEFT_MID, 20, 0);
    }
    
    // 清空画布
    lv_canvas_fill_bg(g_stick_figure_canvas, lv_color_hex(0x001122), LV_OPA_COVER);
    
    lv_layer_t layer;
    lv_canvas_init_layer(g_stick_figure_canvas, &layer);
    
    // 根据身高和体重计算比例
    float height_scale = (float)height / 170.0f; // Normalize to 170cm
    float weight_scale = (float)weight / 70.0f;   // Normalize to 70kg
    
    // 调整尺寸
    int head_radius = (int)(15 * height_scale);
    int torso_length = (int)(80 * height_scale);
    int arm_length = (int)(60 * height_scale);
    int leg_length = (int)(90 * height_scale);
    int line_width = (int)(2 + weight_scale * 2); // Thicker for heavier weight
    
    // 中心位置
    int center_x = 100;
    int start_y = 50;
    
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = lv_color_white();
    line_dsc.width = line_width;
    
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = lv_color_white();
    arc_dsc.width = line_width;
    
    // 绘制头部 (圆形)
    lv_point_t head_center = {center_x, start_y + head_radius};
    lv_draw_arc(&layer, &arc_dsc, &head_center, head_radius, 0, 360);
    
    // 绘制躯干 (垂直线)
    lv_point_t torso_start = {center_x, start_y + head_radius * 2};
    lv_point_t torso_end = {center_x, start_y + head_radius * 2 + torso_length};
    lv_draw_line(&layer, &line_dsc, &torso_start, &torso_end);
    
    // 绘制手臂 (水平线带微小角度，看起来更自然)
    int arm_y = start_y + head_radius * 2 + torso_length / 3;
    lv_point_t arm_left = {center_x - arm_length, arm_y - 10};
    lv_point_t arm_right = {center_x + arm_length, arm_y - 10};
    lv_point_t shoulder = {center_x, arm_y};
    lv_draw_line(&layer, &line_dsc, &arm_left, &shoulder);
    lv_draw_line(&layer, &line_dsc, &shoulder, &arm_right);
    
    // 绘制腿部 (从躯干末端的两条线)
    lv_point_t leg_left = {center_x - 30, start_y + head_radius * 2 + torso_length + leg_length};
    lv_point_t leg_right = {center_x + 30, start_y + head_radius * 2 + torso_length + leg_length};
    lv_draw_line(&layer, &line_dsc, &torso_end, &leg_left);
    lv_draw_line(&layer, &line_dsc, &torso_end, &leg_right);
    
    // 绘制性别特征
    if (gender == 1) { // Female
        // 绘制头发 (头部顶部的弧线)
        lv_point_t hair_center = {center_x, start_y + head_radius - 5};
        lv_draw_arc(&layer, &arc_dsc, &hair_center, head_radius + 5, 180, 360);
        
        // 绘制裙子 (腰部的三角形)
        lv_draw_triangle_dsc_t triangle_dsc;
        lv_draw_triangle_dsc_init(&triangle_dsc);
        triangle_dsc.bg_color = lv_color_white();
        triangle_dsc.bg_opa = LV_OPA_50;
        
        lv_point_t skirt_points[3] = {
            {center_x - 25, start_y + head_radius * 2 + torso_length * 2 / 3},
            {center_x + 25, start_y + head_radius * 2 + torso_length * 2 / 3},
            {center_x, start_y + head_radius * 2 + torso_length}
        };
        lv_draw_triangle(&layer, &triangle_dsc, skirt_points);
    }
    
    lv_canvas_finish_layer(g_stick_figure_canvas, &layer);
}

void gk_gui_update_combat_stats(gk_combat_stats_t* stats)
{
    TAL_PR_INFO(TAG, "更新战斗统计");
    
    if (stats) {
        g_current_stats = *stats;
    }
    
    if (!g_combat_stats_canvas) {
        return; // 画布尚未创建
    }
    
    // 清空画布
    lv_canvas_fill_bg(g_combat_stats_canvas, lv_color_hex(0x001122), LV_OPA_COVER);
    
    lv_layer_t layer;
    lv_canvas_init_layer(g_combat_stats_canvas, &layer);
    
    // 五边形中心和半径
    int center_x = 200;
    int center_y = 150;
    int max_radius = 100;
    
    // 统计值 (0-100刻度)
    int stats_values[5] = {
        g_current_stats.speed,
        g_current_stats.power,
        g_current_stats.endurance,
        g_current_stats.accuracy,
        g_current_stats.technique
    };
    
    const char* stat_labels[5] = {"速度", "爆发力", "耐力", "准确度", "技巧"};
    
    // 绘制五边形网格线
    lv_draw_line_dsc_t grid_line_dsc;
    lv_draw_line_dsc_init(&grid_line_dsc);
    grid_line_dsc.color = lv_color_hex(0x444444);
    grid_line_dsc.width = 1;
    
    // 绘制同心五边形 (20%, 40%, 60%, 80%, 100%)
    for (int level = 1; level <= 5; level++) {
        float radius = max_radius * level / 5.0f;
        lv_point_t pentagon_points[6]; // 6 points to close the shape
        
        for (int i = 0; i < 6; i++) {
            float angle = -M_PI / 2 + (2 * M_PI * (i % 5)) / 5.0f; // Start from top
            pentagon_points[i].x = center_x + (int)(radius * cos(angle));
            pentagon_points[i].y = center_y + (int)(radius * sin(angle));
        }
        
        // 绘制五边形线条
        for (int i = 0; i < 5; i++) {
            lv_draw_line(&layer, &grid_line_dsc, &pentagon_points[i], &pentagon_points[i + 1]);
        }
    }
    
    // 从中心到顶点绘制轴线
    for (int i = 0; i < 5; i++) {
        float angle = -M_PI / 2 + (2 * M_PI * i) / 5.0f;
        lv_point_t axis_end = {
            center_x + (int)(max_radius * cos(angle)),
            center_y + (int)(max_radius * sin(angle))
        };
        lv_point_t axis_start = {center_x, center_y};
        lv_draw_line(&layer, &grid_line_dsc, &axis_start, &axis_end);
    }
    
    // 绘制实际统计多边形
    lv_draw_line_dsc_t stats_line_dsc;
    lv_draw_line_dsc_init(&stats_line_dsc);
    stats_line_dsc.color = lv_color_hex(0x00FF88);
    stats_line_dsc.width = 3;
    
    lv_point_t stats_points[6]; // 6 points to close the shape
    
    for (int i = 0; i < 6; i++) {
        float angle = -M_PI / 2 + (2 * M_PI * (i % 5)) / 5.0f;
        float radius = (max_radius * stats_values[i % 5]) / 100.0f;
        stats_points[i].x = center_x + (int)(radius * cos(angle));
        stats_points[i].y = center_y + (int)(radius * sin(angle));
    }
    
    // 绘制统计多边形
    for (int i = 0; i < 5; i++) {
        lv_draw_line(&layer, &stats_line_dsc, &stats_points[i], &stats_points[i + 1]);
    }
    
    // 填充统计多边形
    lv_draw_triangle_dsc_t fill_dsc;
    lv_draw_triangle_dsc_init(&fill_dsc);
    fill_dsc.bg_color = lv_color_hex(0x00FF88);
    fill_dsc.bg_opa = LV_OPA_30;
    
    // 绘制三角形以填充多边形
    for (int i = 0; i < 5; i++) {
        lv_point_t triangle_points[3] = {
            {center_x, center_y},
            stats_points[i],
            stats_points[i + 1]
        };
        lv_draw_triangle(&layer, &fill_dsc, triangle_points);
    }
    
    // 绘制统计标签
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = lv_color_white();
    label_dsc.font = &lv_font_montserrat_14;
    
    for (int i = 0; i < 5; i++) {
        float angle = -M_PI / 2 + (2 * M_PI * i) / 5.0f;
        int label_radius = max_radius + 20;
        lv_point_t label_pos = {
            center_x + (int)(label_radius * cos(angle)) - 20,
            center_y + (int)(label_radius * sin(angle)) - 7
        };
        
        lv_area_t label_area = {
            label_pos.x, label_pos.y,
            label_pos.x + 40, label_pos.y + 14
        };
        
        lv_draw_label(&layer, &label_dsc, &label_area, stat_labels[i], NULL);
        
        // 绘制统计值
        char value_str[8];
        snprintf(value_str, sizeof(value_str), "%d", stats_values[i]);
        
        lv_point_t value_pos = {
            label_pos.x + 5,
            label_pos.y + 16
        };
        
        lv_area_t value_area = {
            value_pos.x, value_pos.y,
            value_pos.x + 30, value_pos.y + 12
        };
        
        label_dsc.color = lv_color_hex(0x00FF88);
        lv_draw_label(&layer, &label_dsc, &value_area, value_str, NULL);
    }
    
    lv_canvas_finish_layer(g_combat_stats_canvas, &layer);
}

// 使用火柴人和战斗统计更新主统计页面
void gk_gui_update_main_stats_page(void)
{
    if (!g_pages[GK_PAGE_MAIN_STATS]) {
        return;
    }
    
    // 清空现有内容
    lv_obj_clean(g_pages[GK_PAGE_MAIN_STATS]);
    
    // 创建标题
    lv_obj_t* title = lv_label_create(g_pages[GK_PAGE_MAIN_STATS]);
    lv_label_set_text(title, "个人战力分析");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // 获取用户信息
    gk_user_info_t* user_info = gk_get_user_info();
    
    // 绘制火柴人
    gk_gui_draw_stick_figure(g_pages[GK_PAGE_MAIN_STATS], user_info->height, user_info->weight, user_info->gender);
    
    // 创建战斗统计画布
    if (!g_combat_stats_canvas) {
        g_combat_stats_canvas = lv_canvas_create(g_pages[GK_PAGE_MAIN_STATS]);
        lv_canvas_set_buffer(g_combat_stats_canvas, g_combat_canvas_buf, 400, 300, LV_COLOR_FORMAT_RGB565);
        lv_obj_align(g_combat_stats_canvas, LV_ALIGN_RIGHT_MID, -20, 0);
    }
    
    // 更新战斗统计
    gk_gui_update_combat_stats(NULL); // Use current stats
    
    // 添加导航提示
    lv_obj_t* hint = lv_label_create(g_pages[GK_PAGE_MAIN_STATS]);
    lv_label_set_text(hint, "← 左滑查看打击可视化  ← 再左滑进入训练模式");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}