#include "gk_bag.h"
#include "tal_log.h"
#include "lvgl.h"

#define TAG "GUI_MAIN"

// GUI对象
static lv_obj_t* g_scr_main = NULL;
static lv_obj_t* g_pages[GK_PAGE_MAX] = {NULL};
static gk_page_t g_current_page = GK_PAGE_MAIN_STATS;

// 页面对象 (已简化 - 移除了登录和用户信息页面)
static lv_obj_t* g_main_stats_page = NULL;
static lv_obj_t* g_hit_visual_page = NULL;
static lv_obj_t* g_training_page = NULL;

// 前向声明
static void gk_create_main_stats_page(void);
static void gk_create_hit_visual_page(void);
static void gk_create_training_page(void);

int gk_gui_init(void)
{
    TAL_PR_INFO(TAG, "GUI初始化");
    
    // 创建主屏幕
    g_scr_main = lv_scr_act();
    lv_obj_set_style_bg_color(g_scr_main, lv_color_hex(0x001122), 0);
    
    // 仅创建核心页面
    gk_create_main_stats_page();
    gk_create_hit_visual_page();
    gk_create_training_page();
    
    // 初始显示主统计页面
    gk_gui_show_page(GK_PAGE_MAIN_STATS);
    
    TAL_PR_INFO(TAG, "GUI初始化完成");
    return OPRT_OK;
}

void gk_gui_show_page(gk_page_t page)
{
    TAL_PR_INFO(TAG, "显示页面: %d", page);
    
    if (page >= GK_PAGE_MAX) {
        TAL_PR_ERR(TAG, "无效页面: %d", page);
        return;
    }
    
    // 隐藏所有页面
    for (int i = 0; i < GK_PAGE_MAX; i++) {
        if (g_pages[i]) {
            lv_obj_add_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // 显示目标页面
    if (g_pages[page]) {
        lv_obj_clear_flag(g_pages[page], LV_OBJ_FLAG_HIDDEN);
        g_current_page = page;
    }
}

// 已简化 - 移除了登录和用户信息页面

// 图形功能的前向声明
void gk_gui_update_main_stats_page(void);
void gk_gui_update_hit_visual(gk_hit_point_t* hit_point);

// 导航事件处理程序
static void nav_to_hit_visual_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        gk_gui_show_page(GK_PAGE_HIT_VISUAL);
    }
}

static void nav_to_training_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        gk_gui_show_page(GK_PAGE_TRAINING);
    }
}

static void gk_create_main_stats_page(void)
{
    g_main_stats_page = lv_obj_create(g_scr_main);
    lv_obj_set_size(g_main_stats_page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(g_main_stats_page, lv_color_hex(0x001122), 0);
    g_pages[GK_PAGE_MAIN_STATS] = g_main_stats_page;
    
    // 实际内容将在页面首次显示时创建
    // 这在gk_gui_update_main_stats_page()函数中完成
}

static void gk_create_hit_visual_page(void)
{
    g_hit_visual_page = lv_obj_create(g_scr_main);
    lv_obj_set_size(g_hit_visual_page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(g_hit_visual_page, lv_color_hex(0x001122), 0);
    g_pages[GK_PAGE_HIT_VISUAL] = g_hit_visual_page;
    
    // 标题
    lv_obj_t* title = lv_label_create(g_hit_visual_page);
    lv_label_set_text(title, "沙袋打击可视化");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // 沙袋可视化区域 (算法实现的占位符)
    lv_obj_t* sandbag_area = lv_obj_create(g_hit_visual_page);
    lv_obj_set_size(sandbag_area, 300, 400);
    lv_obj_set_style_bg_color(sandbag_area, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_color(sandbag_area, lv_color_white(), 0);
    lv_obj_set_style_border_width(sandbag_area, 2, 0);
    lv_obj_align(sandbag_area, LV_ALIGN_LEFT_MID, 20, 10);
    
    lv_obj_t* sandbag_label = lv_label_create(sandbag_area);
    lv_label_set_text(sandbag_label, "沙袋示意图\n\n打击点实时显示区域\n\n(算法接口预留)\n\n可显示:\n• 打击位置\n• 力度大小\n• 打击轨迹");
    lv_obj_set_style_text_color(sandbag_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(sandbag_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(sandbag_label);
    
    // 实时统计面板
    lv_obj_t* stats_panel = lv_obj_create(g_hit_visual_page);
    lv_obj_set_size(stats_panel, 200, 400);
    lv_obj_set_style_bg_color(stats_panel, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_color(stats_panel, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_border_width(stats_panel, 1, 0);
    lv_obj_align(stats_panel, LV_ALIGN_RIGHT_MID, -20, 10);
    
    // 统计标签
    lv_obj_t* stats_title = lv_label_create(stats_panel);
    lv_label_set_text(stats_title, "实时数据");
    lv_obj_set_style_text_color(stats_title, lv_color_hex(0x00FF88), 0);
    lv_obj_set_style_text_font(stats_title, &lv_font_montserrat_16, 0);
    lv_obj_align(stats_title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 力数据 (占位符)
    lv_obj_t* force_label = lv_label_create(stats_panel);
    lv_label_set_text(force_label, "打击力度: 0 N");
    lv_obj_set_style_text_color(force_label, lv_color_white(), 0);
    lv_obj_align(force_label, LV_ALIGN_TOP_LEFT, 10, 50);
    
    lv_obj_t* pos_label = lv_label_create(stats_panel);
    lv_label_set_text(pos_label, "打击位置:\nX: 0 cm\nY: 0 cm");
    lv_obj_set_style_text_color(pos_label, lv_color_white(), 0);
    lv_obj_align(pos_label, LV_ALIGN_TOP_LEFT, 10, 80);
    
    lv_obj_t* combo_label = lv_label_create(stats_panel);
    lv_label_set_text(combo_label, "连击数: 0");
    lv_obj_set_style_text_color(combo_label, lv_color_white(), 0);
    lv_obj_align(combo_label, LV_ALIGN_TOP_LEFT, 10, 130);
    
    lv_obj_t* max_force_label = lv_label_create(stats_panel);
    lv_label_set_text(max_force_label, "最大力度: 0 N");
    lv_obj_set_style_text_color(max_force_label, lv_color_white(), 0);
    lv_obj_align(max_force_label, LV_ALIGN_TOP_LEFT, 10, 160);
    
    // 导航按钮
    lv_obj_t* back_btn = lv_btn_create(g_hit_visual_page);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(back_btn, nav_to_hit_visual_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "← 返回");
    lv_obj_center(back_label);
    
    lv_obj_t* next_btn = lv_btn_create(g_hit_visual_page);
    lv_obj_set_size(next_btn, 120, 40);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(next_btn, nav_to_training_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* next_label = lv_label_create(next_btn);
    lv_label_set_text(next_label, "训练模式 →");
    lv_obj_center(next_label);
}

// 训练模式事件处理程序
static void training_mode_select_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED) {
        TAL_PR_INFO(TAG, "选择训练模式");
        // 训练模式逻辑将在后续实现
    }
}

static void gk_create_training_page(void)
{
    g_training_page = lv_obj_create(g_scr_main);
    lv_obj_set_size(g_training_page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(g_training_page, lv_color_hex(0x001122), 0);
    g_pages[GK_PAGE_TRAINING] = g_training_page;
    
    // 标题
    lv_obj_t* title = lv_label_create(g_training_page);
    lv_label_set_text(title, "训练模式选择");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // 训练模式网格
    int btn_width = 200;
    int btn_height = 120;
    int spacing = 30;
    
    // 力量训练模式
    lv_obj_t* strength_btn = lv_btn_create(g_training_page);
    lv_obj_set_size(strength_btn, btn_width, btn_height);
    lv_obj_align(strength_btn, LV_ALIGN_TOP_LEFT, spacing, 100);
    lv_obj_set_style_bg_color(strength_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_add_event_cb(strength_btn, training_mode_select_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* strength_label = lv_label_create(strength_btn);
    lv_label_set_text(strength_label, "力量训练\n\n提升爆发力\n和打击强度");
    lv_obj_set_style_text_align(strength_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(strength_label);
    
    // 速度训练模式
    lv_obj_t* speed_btn = lv_btn_create(g_training_page);
    lv_obj_set_size(speed_btn, btn_width, btn_height);
    lv_obj_align(speed_btn, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_bg_color(speed_btn, lv_color_hex(0x44FF44), 0);
    lv_obj_add_event_cb(speed_btn, training_mode_select_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* speed_label = lv_label_create(speed_btn);
    lv_label_set_text(speed_label, "速度训练\n\n提升出拳\n速度和频率");
    lv_obj_set_style_text_align(speed_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(speed_label);
    
    // 精准训练模式
    lv_obj_t* accuracy_btn = lv_btn_create(g_training_page);
    lv_obj_set_size(accuracy_btn, btn_width, btn_height);
    lv_obj_align(accuracy_btn, LV_ALIGN_TOP_RIGHT, -spacing, 100);
    lv_obj_set_style_bg_color(accuracy_btn, lv_color_hex(0x4444FF), 0);
    lv_obj_add_event_cb(accuracy_btn, training_mode_select_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* accuracy_label = lv_label_create(accuracy_btn);
    lv_label_set_text(accuracy_label, "精准训练\n\n提升打击\n精确度");
    lv_obj_set_style_text_align(accuracy_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(accuracy_label);
    
    // 耐力训练模式
    lv_obj_t* endurance_btn = lv_btn_create(g_training_page);
    lv_obj_set_size(endurance_btn, btn_width, btn_height);
    lv_obj_align(endurance_btn, LV_ALIGN_CENTER, -100, 50);
    lv_obj_set_style_bg_color(endurance_btn, lv_color_hex(0xFF8800), 0);
    lv_obj_add_event_cb(endurance_btn, training_mode_select_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* endurance_label = lv_label_create(endurance_btn);
    lv_label_set_text(endurance_label, "耐力训练\n\n提升持续\n作战能力");
    lv_obj_set_style_text_align(endurance_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(endurance_label);
    
    // 组合技训练模式
    lv_obj_t* combo_btn = lv_btn_create(g_training_page);
    lv_obj_set_size(combo_btn, btn_width, btn_height);
    lv_obj_align(combo_btn, LV_ALIGN_CENTER, 100, 50);
    lv_obj_set_style_bg_color(combo_btn, lv_color_hex(0xFF44FF), 0);
    lv_obj_add_event_cb(combo_btn, training_mode_select_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* combo_label = lv_label_create(combo_btn);
    lv_label_set_text(combo_label, "组合技训练\n\n学习连击\n技巧");
    lv_obj_set_style_text_align(combo_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(combo_label);
    
    // 实现说明
    lv_obj_t* note = lv_label_create(g_training_page);
    lv_label_set_text(note, "注: 训练模式的具体算法和逻辑将后续实现\n每个模式将包含不同的训练目标和评分机制");
    lv_obj_set_style_text_color(note, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_align(note, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(note, LV_ALIGN_BOTTOM_MID, 0, -80);
    
    // 返回按钮
    lv_obj_t* back_btn = lv_btn_create(g_training_page);
    lv_obj_set_size(back_btn, 120, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(back_btn, nav_to_hit_visual_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "← 返回");
    lv_obj_center(back_label);
}

// 增强的打击可视化，包含详细的沙袋表示
static lv_obj_t* g_hit_canvas = NULL;
static lv_draw_buf_t g_hit_draw_buf;
static uint8_t g_hit_canvas_buf[300 * 400 * LV_COLOR_DEPTH / 8];
static lv_obj_t* g_stats_labels[4] = {NULL}; // Force, Position, Combo, Max Force labels

void gk_gui_update_hit_visual(gk_hit_point_t* hit_point)
{
    if (!hit_point || !g_pages[GK_PAGE_HIT_VISUAL]) {
        return; // 无打击可显示或页面未创建
    }
    
    // 如果画布不存在则创建
    if (!g_hit_canvas) {
        g_hit_canvas = lv_canvas_create(g_pages[GK_PAGE_HIT_VISUAL]);
        lv_canvas_set_buffer(g_hit_canvas, g_hit_canvas_buf, 300, 400, LV_COLOR_FORMAT_RGB565);
        lv_obj_align(g_hit_canvas, LV_ALIGN_LEFT_MID, 20, 10);
    }
    
    // 清空画布并重绘沙袋
    lv_canvas_fill_bg(g_hit_canvas, lv_color_hex(0x333333), LV_OPA_COVER);
    
    lv_layer_t layer;
    lv_canvas_init_layer(g_hit_canvas, &layer);
    
    // 绘制沙袋轮廓 (圆柱形状)
    lv_draw_rect_dsc_t bag_dsc;
    lv_draw_rect_dsc_init(&bag_dsc);
    bag_dsc.border_color = lv_color_white();
    bag_dsc.border_width = 2;
    bag_dsc.bg_opa = LV_OPA_TRANSP;
    bag_dsc.radius = 0;
    
    // 绘制沙袋主体 (简化为带圆角的矩形)
    lv_area_t bag_area = {120, 50, 180, 350};  // 60px wide, 300px tall
    lv_draw_rect(&layer, &bag_dsc, &bag_area);
    
    // 绘制沙袋顶部和底部圆形
    lv_draw_arc_dsc_t circle_dsc;
    lv_draw_arc_dsc_init(&circle_dsc);
    circle_dsc.color = lv_color_white();
    circle_dsc.width = 2;
    
    lv_point_t top_center = {150, 60};
    lv_point_t bottom_center = {150, 340};
    lv_draw_arc(&layer, &circle_dsc, &top_center, 30, 0, 360);
    lv_draw_arc(&layer, &circle_dsc, &bottom_center, 30, 0, 360);
    
    // 计算画布上的打击位置 (从真实坐标映射)
    // hit_point->x,y以厘米为单位，映射到画布坐标
    int hit_canvas_x = 150 + (int)(hit_point->x * 2.0f);  // Scale: 2px per cm
    int hit_canvas_y = 200 - (int)(hit_point->y * 2.0f);  // Flip Y axis, offset to middle
    
    // 绘制打击点指示器
    lv_draw_arc_dsc_t hit_dsc;
    lv_draw_arc_dsc_init(&hit_dsc);
    hit_dsc.color = lv_color_hex(0xFF4444);  // Red hit indicator
    hit_dsc.width = 4;
    
    lv_point_t hit_center = {hit_canvas_x, hit_canvas_y};
    int hit_radius = (int)(5 + hit_point->force * 0.2f);  // Scale radius with force
    lv_draw_arc(&layer, &hit_dsc, &hit_center, hit_radius, 0, 360);
    
    // 将力大小绘制为辐射线
    if (hit_point->force > 10.0f) {
        lv_draw_line_dsc_t force_line_dsc;
        lv_draw_line_dsc_init(&force_line_dsc);
        force_line_dsc.color = lv_color_hex(0xFF8844);
        force_line_dsc.width = 2;
        
        int force_radius = hit_radius + 10;
        for (int angle = 0; angle < 360; angle += 45) {
            float rad = (float)angle * M_PI / 180.0f;
            lv_point_t line_end = {
                hit_canvas_x + (int)(force_radius * cosf(rad)),
                hit_canvas_y + (int)(force_radius * sinf(rad))
            };
            lv_draw_line(&layer, &force_line_dsc, &hit_center, &line_end);
        }
    }
    
    lv_canvas_finish_layer(g_hit_canvas, &layer);
    
    // 更新统计标签
    if (!g_stats_labels[0]) {
        // 如果不存在则创建统计标签
        for (int i = 0; i < 4; i++) {
            g_stats_labels[i] = lv_label_create(g_pages[GK_PAGE_HIT_VISUAL]);
            lv_obj_set_style_text_color(g_stats_labels[i], lv_color_white(), 0);
            lv_obj_align(g_stats_labels[i], LV_ALIGN_TOP_RIGHT, -20, 50 + i * 30);
        }
    }
    
    // 更新统计文本
    lv_label_set_text_fmt(g_stats_labels[0], "打击力度: %.1f N", hit_point->force);
    lv_label_set_text_fmt(g_stats_labels[1], "打击位置: (%.1f, %.1f)cm", hit_point->x, hit_point->y);
    lv_label_set_text_fmt(g_stats_labels[2], "连击数: %d", gk_sensor_get_hit_count());
    lv_label_set_text_fmt(g_stats_labels[3], "最大力度: %.1f N", gk_sensor_get_max_force_session());
    
    TAL_PR_INFO(TAG, "增强打击可视化: (%.1f,%.1f)cm F=%.1fN", 
                hit_point->x, hit_point->y, hit_point->force);
}