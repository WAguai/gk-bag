#include "gk_bag.h"
#include "tal_log.h"
#include "cJSON.h"
#include "tuya_cloud_types.h"
#include "tal_system.h"

#define TAG "USER_AUTH"

static gk_user_info_t g_user_info = {0};

int gk_user_auth_init(void)
{
    TAL_PR_INFO(TAG, "用户认证模块初始化");
    
    // 初始化用户信息
    memset(&g_user_info, 0, sizeof(gk_user_info_t));
    g_user_info.is_logged_in = false;
    
    return OPRT_OK;
}

int gk_user_login(const char* qr_code_data)
{
    TAL_PR_INFO(TAG, "用户使用二维码登录");
    
    if (!qr_code_data || strlen(qr_code_data) == 0) {
        TAL_PR_ERR(TAG, "无效的二维码数据");
        return OPRT_INVALID_PARM;
    }
    
    // 解析二维码JSON数据
    cJSON *json = cJSON_Parse(qr_code_data);
    if (!json) {
        TAL_PR_ERR(TAG, "解析二维码JSON失败");
        return OPRT_CJSON_PARSE_ERR;
    }
    
    // 提取用户信息
    cJSON *user_id = cJSON_GetObjectItem(json, "user_id");
    cJSON *username = cJSON_GetObjectItem(json, "username");
    cJSON *token = cJSON_GetObjectItem(json, "token");
    
    if (!user_id || !username || !token) {
        TAL_PR_ERR(TAG, "二维码中缺少必需字段");
        cJSON_Delete(json);
        return OPRT_INVALID_PARM;
    }
    
    // 使用涂鸦云验证令牌 (模拟)
    // 在实际实现中，这将发送HTTP请求来验证令牌
    bool token_valid = true;  // Simulated validation
    
    if (!token_valid) {
        TAL_PR_ERR(TAG, "无效令牌");
        cJSON_Delete(json);
        return OPRT_COM_ERROR;
    }
    
    // 存储用户信息
    strncpy(g_user_info.user_id, cJSON_GetStringValue(user_id), sizeof(g_user_info.user_id) - 1);
    strncpy(g_user_info.username, cJSON_GetStringValue(username), sizeof(g_user_info.username) - 1);
    g_user_info.is_logged_in = true;
    
    TAL_PR_INFO(TAG, "用户登录成功: %s", g_user_info.username);
    
    cJSON_Delete(json);
    return OPRT_OK;
}

int gk_user_register(const char* qr_code_data)
{
    TAL_PR_INFO(TAG, "用户使用二维码注册");
    
    if (!qr_code_data || strlen(qr_code_data) == 0) {
        TAL_PR_ERR(TAG, "无效的二维码数据");
        return OPRT_INVALID_PARM;
    }
    
    // 解析注册二维码
    cJSON *json = cJSON_Parse(qr_code_data);
    if (!json) {
        TAL_PR_ERR(TAG, "解析注册二维码JSON失败");
        return OPRT_CJSON_PARSE_ERR;
    }
    
    cJSON *reg_token = cJSON_GetObjectItem(json, "reg_token");
    cJSON *username = cJSON_GetObjectItem(json, "username");
    
    if (!reg_token || !username) {
        TAL_PR_ERR(TAG, "缺少注册字段");
        cJSON_Delete(json);
        return OPRT_INVALID_PARM;
    }
    
    // 向涂鸦云发送注册请求 (模拟)
    // 在实际实现中，这将创建新用户账户
    
    // 生成用户ID (简化)
    snprintf(g_user_info.user_id, sizeof(g_user_info.user_id), 
             "gk_user_%08x", (unsigned int)tal_time_get_posix());
    
    strncpy(g_user_info.username, cJSON_GetStringValue(username), sizeof(g_user_info.username) - 1);
    g_user_info.is_logged_in = true;
    g_user_info.height = 0;   // 将在用户信息页面设置
    g_user_info.weight = 0;   // 将在用户信息页面设置
    g_user_info.gender = 0;   // 将在用户信息页面设置
    
    TAL_PR_INFO(TAG, "用户注册成功: %s", g_user_info.username);
    
    cJSON_Delete(json);
    return OPRT_OK;
}

int gk_user_update_info(int height, int weight, int gender)
{
    TAL_PR_INFO(TAG, "更新用户信息: H=%d W=%d G=%d", height, weight, gender);
    
    if (!g_user_info.is_logged_in) {
        TAL_PR_ERR(TAG, "用户未登录");
        return OPRT_COM_ERROR;
    }
    
    if (height < 100 || height > 220) {
        TAL_PR_ERR(TAG, "无效身高: %d", height);
        return OPRT_INVALID_PARM;
    }
    
    if (weight < 30 || weight > 200) {
        TAL_PR_ERR(TAG, "无效体重: %d", weight);
        return OPRT_INVALID_PARM;
    }
    
    if (gender < 0 || gender > 1) {
        TAL_PR_ERR(TAG, "无效性别: %d", gender);
        return OPRT_INVALID_PARM;
    }
    
    g_user_info.height = height;
    g_user_info.weight = weight;
    g_user_info.gender = gender;
    
    // 保存到持久化存储 (模拟)
    // 在实际实现中，这将保存到flash或云端
    
    TAL_PR_INFO(TAG, "用户信息更新成功");
    return OPRT_OK;
}

gk_user_info_t* gk_get_user_info(void)
{
    return &g_user_info;
}

// 简化的设置默认用户信息功能
void gk_set_default_user_info(void)
{
    TAL_PR_INFO(TAG, "设置默认用户信息");
    
    strcpy(g_user_info.user_id, "default_user");
    strcpy(g_user_info.username, "Demo User");
    g_user_info.height = 175;  // 默认身高175cm
    g_user_info.weight = 70;   // 默认体重70kg
    g_user_info.gender = 0;    // 默认男性
    g_user_info.is_logged_in = true;
    
    TAL_PR_INFO(TAG, "默认用户信息设置: %dcm, %dkg, %s", 
                g_user_info.height, g_user_info.weight, 
                g_user_info.gender == 0 ? "男" : "女");
}