
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include <string>

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    if (event == ESP_GAP_BLE_SCAN_RESULT_EVT)
    {
        auto *sr = &param->scan_rst;

        std::string name = "<none>";
        uint8_t adv_name_len = 0;
        uint8_t *adv_name = esp_ble_resolve_adv_data(sr->ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
        if (adv_name == nullptr)
            return;

        name = std::string((char*)adv_name, adv_name_len);    
        
        Serial.printf("GAP SCAN ev=%d addr=%0x:%0x:%0x:%0x:%0x:%0x dev-type=%d addr-type=%d evt-type=%d rssi=%d name=`%s` flag=%d num_resps=%d d_len=%d sr_len=%d num_dis=%d\n", 
             sr->search_evt, 
             sr->bda[0],sr->bda[1],sr->bda[2],sr->bda[3],sr->bda[4],sr->bda[5],
             sr->dev_type, 
             sr->ble_addr_type,
             sr->ble_evt_type,
             sr->rssi,
             name.c_str(),
             sr->flag,
             sr->num_resps,
             sr->adv_data_len, 
             sr->scan_rsp_len, 
             sr->num_dis);
             
    }
    else {
        Serial.printf("Got GAP event %d\n", event);
    }
}

void setup() 
{
    Serial.begin(115200);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        Serial.printf("BLE: initialize controller failed, error code = %x\n", ret);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) {
        Serial.printf("BLE: enable controller failed, error code = %x\n", ret);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        Serial.printf("BLE: init bluetooth failed, error code = %x\n", ret);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        Serial.printf("BLE: enable bluetooth failed, error code = %x\n", ret);
        return;
    }

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        Serial.printf("BLE: gap register failed, error code = %x\n", ret);
        return;
    }

    ret = esp_ble_gap_start_scanning(1000);
    if (ret){
        Serial.printf("BLE: start scan failed = %x\n", ret);
        return;
    }
    

    Serial.printf("BLE: init succeeded\n");

}

void loop() {
  // put your main code here, to run repeatedly:

}
