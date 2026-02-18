readme write by chinese
/*
 * 使用ESP32-C3 super mini做為主處理模組
 * 以webserver當基礎擴充
 * 加上AHT20溫濕感測模組
 * 創建HiveMQ帳號連接MQTT推播溫溼度msg
 * Github帳號雲端空間page.網頁填入雲端空間url,即可訂閱MQTT,接收溫濕度資料,並且發送GPIO控制訊號給ESP32
 * ESP32創建webserver,使用spiffs置放網頁(同github內容)透過WIFI連接網路,連接HiveMQ,並推播及接收外網資料
 * 目前GPIO控制c3 super mini的內建LED
 * 另外也使用MIT App Inventor 2建立如同網頁控制般的App作為終端使用
 * 
*/
