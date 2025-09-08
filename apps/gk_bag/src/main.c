#include "gk_bag.h"
#include "tuya_cloud_types.h"
#include "tuya_iot_config.h"
#include "tal_log.h"
#include "tal_thread.h"
#include "tal_system.h"

#define TAG "GK_BAG_MAIN"

static void gk_bag_main_task(void *arg)
{
    TAL_PR_INFO(TAG, "智能AI沙袋启动中...");
    
    // 初始化核心子系统
    if (gk_gui_init() != OPRT_OK) {
        TAL_PR_ERR(TAG, "GUI初始化失败");
        return;
    }
    
    if (gk_sensor_init() != OPRT_OK) {
        TAL_PR_ERR(TAG, "传感器初始化失败");
        return;
    }
    
    // 设置默认用户信息并直接显示主统计页面
    gk_set_default_user_info();
    gk_gui_show_page(GK_PAGE_MAIN_STATS);
    
    TAL_PR_INFO(TAG, "智能AI沙袋初始化成功");
    
    // 主循环
    while (1) {
        // 处理传感器数据和GUI更新
        gk_force_data_t force_data;
        if (gk_sensor_read_data(&force_data) == OPRT_OK) {
            // 处理力传感器数据并根据需要更新GUI
            gk_hit_point_t hit_point;
            if (gk_calculate_hit_point(&force_data, &hit_point) == OPRT_OK) {
                gk_gui_update_hit_visual(&hit_point);
            }
        }
        
        // 任务延迟
        tal_system_sleep(50); // 20Hz update rate
    }
}

int gk_bag_init(void)
{
    TAL_PR_INFO(TAG, "GK沙袋应用初始化");
    
    // 创建主任务
    THREAD_HANDLE task_handle;
    THREAD_CFG_T task_cfg = {
        .priority = THREAD_PRIO_4,
        .stackDepth = 4096,
        .thrdname = "gk_bag_main"
    };
    
    if (tal_thread_create_and_start(&task_handle, NULL, NULL, 
                                   gk_bag_main_task, NULL, &task_cfg) != OPRT_OK) {
        TAL_PR_ERR(TAG, "创建主任务失败");
        return OPRT_COM_ERROR;
    }
    
    return OPRT_OK;
}