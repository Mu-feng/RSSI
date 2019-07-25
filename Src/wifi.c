#include "wifi.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "usart.h"

const char cmd_at[] = "AT\r\n";
const char cmd_at_rst[] = "AT+RST\r\n";
const char cmd_ate[] = "ATE0\r\n";
const char cmd_cwmode[] = "AT+CWMODE_DEF=3\r\n";
const char cmd_at_cwsap[] = "AT+CWSAP_DEF=\"RSSI_STATION2\",\"12345678\",5,3\r\n";
const char cmd_at_cipap[] = "AT+CIPAP_DEF=\"192.168.199.1\",\"192.168.199.1\",\"255.255.255.0\"\r\n";
const char cmd_at_cipmux[] = "AT+CIPMUX=1\r\n";
const char cmd_at_tcp[] = "AT+CIPSERVER=1,8090\r\n";
const char cmd_at_tcp_send[] = "AT+CIPSEND=%d,%d\r\n";
const char cmd_at_cwlap[] = "AT+CWLAP\r\n";
const char *cmd_ssid[] = {"RSSI_STATION1","RSSI_STATION2","RSSI_STATION3","RSSI_STATION4"};


uint8_t wifi_recv_buff[WIFI_RECV_SIZE];
/**
  * @brief  查找字符串中是否有 某个子字符串，如果有返回其索引，如果没有，返回-1
  * @param  str_checked: 待查找的字符串
  * @param  str_mark: 查找的子字符串
  * @retval -1 表示没有找到，其他数值，子字符串在待检测字符串中的位置
  */
int16_t find_str(uint8_t *str_cheked, uint8_t *str_mark, uint16_t length)
{
    uint16_t n2 = strlen((char*)str_mark);
    int16_t index = 0;
    for(; index < length; index++)
    {
        if (memcmp(&(str_cheked[index]), str_mark, n2) == 0)
        {
            break;
        }
    }
    if(index < length)
    {
        return index;
    }
    else
    {
        return -1;
    }
}

bool config_wifi(uint8_t *cmd,uint8_t size)
{
    uint16_t len = 0;
    wifi_send_cmd(cmd,size);
    HAL_Delay(10);
    if(get_recv_flag() == 1)
    {
        set_recv_flag();
        len = get_recv_data(wifi_recv_buff);
        if(0 != len)
        {
            if(-1 != find_str(wifi_recv_buff,"OK",len))
            {
                return true;
            }
        }
    }
    return false;
}

void wifi_init(void)
{    
//    while(false == config_wifi((uint8_t*)cmd_at_rst,sizeof(cmd_at_rst)))
//    {
//        HAL_Delay(200);
//    }
//    HAL_Delay(1000);
    while(false == config_wifi((uint8_t*)cmd_ate,sizeof(cmd_ate)))
    {
        HAL_Delay(200);
    }
    
    while(false == config_wifi((uint8_t*)cmd_cwmode,sizeof(cmd_cwmode)))
    {
        HAL_Delay(200);
    }
    
    while(false == config_wifi((uint8_t*)cmd_at_cwsap,sizeof(cmd_at_cwsap)))
    {
        HAL_Delay(200);
    }
    
    while(false == config_wifi((uint8_t*)cmd_at_cipap,sizeof(cmd_at_cipap)))
    {
        HAL_Delay(200);
    }    
    
    while(false == config_wifi((uint8_t*)cmd_at_cipmux,sizeof(cmd_at_cipmux)))
    {
        HAL_Delay(200);
    }
    
    while(false == config_wifi((uint8_t*)cmd_at_tcp,sizeof(cmd_at_tcp)))
    {
        HAL_Delay(200);
    }
}

void send_tcp_packet(uint8_t socket,uint8_t *data,uint16_t size)
{
    
    uint16_t len = 0;
    uint8_t cmd[256];
    if(size > 0)
    {
        sprintf((char*)cmd, "AT+CIPSEND=%d,%d\r\n", socket, size);
        wifi_send_cmd(cmd,strlen((char*)cmd));
        HAL_Delay(10);
        if(get_recv_flag() == 1)
        {
            set_recv_flag();
            len = get_recv_data(wifi_recv_buff);
            if(0 != len)
            {
                if(-1 != find_str(wifi_recv_buff,(uint8_t*)"OK\r\n>",len))
                {
                    wifi_send_cmd(data,size);
                    HAL_Delay(10);                    
                    if(get_recv_flag() == 1)
                    {
                        set_recv_flag();
                    }
                }
            }
        }
    }
}

bool get_tcp_data(uint8_t *socket,uint8_t *data)
{
    uint16_t len;   // 接收到的数据长度
    int16_t socket_start_point;   // 有效数据相对偏移位置
    int16_t data_start_point;
    
    len = get_recv_data(wifi_recv_buff);
    if(len > 0)
    {
        socket_start_point = find_str(wifi_recv_buff,(uint8_t*)"+IPD,",len);
        
        // 找到了TCP数据的起始
        if(socket_start_point >=0)
        {
            *socket = wifi_recv_buff[socket_start_point + 5] - '0';
            
            data_start_point = find_str(&wifi_recv_buff[socket_start_point],(uint8_t*)":",len - socket_start_point); 
            if(data_start_point >= 0)
            {
                memcpy(data,&wifi_recv_buff[socket_start_point + data_start_point + 1],len - socket_start_point - data_start_point - 1);
                data[len - data_start_point - 1] = 0; //添加结束符
                return true;
            }
        }
    }
    return false;
}

bool get_signal_strength(int16_t *signal_strenth)
{
    uint8_t n = 50;
    uint16_t recv_len;
    uint16_t offset = 0;
    wifi_send_cmd((uint8_t*)cmd_at_cwlap,sizeof(cmd_at_cwlap));   
    while(n > 0)
    {
        // 表示接收到数据
        if(get_recv_flag() == 1)
        {
            set_recv_flag();
            recv_len = get_recv_data(wifi_recv_buff);
            // 接收到 CWLAP的返回数据
            if((recv_len > 0) && (find_str(wifi_recv_buff,(uint8_t*)"+CWLAP:",recv_len) >= 0))
            {
                break;
            }
        }
        HAL_Delay(100);
        n--;
    }
    // 成功接收到数据返回
    if(n > 0)
    {
        int16_t signal_point = 0;
        for(uint8_t i = 0; i < 4; i++)
        {
            // 每次查找前确定长度大于0
            if(recv_len > 0)
            {
                // 查找SSID
                signal_point = find_str(&wifi_recv_buff[0],(uint8_t *)cmd_ssid[i],recv_len);
                if(signal_point >= 0)
                {
                    // 更新查找起始位置
                    offset = signal_point;
                    // 跟新剩余数组长度
                    //recv_len -= offset;
                    if(recv_len > 0)
                    {                    
                        // 从ssid后面找到','
                        signal_point = find_str(&wifi_recv_buff[offset],(uint8_t *)",",recv_len - offset);
                        // 更新查找起始位置
                        //offset += signal_point;
                        // 更新数组剩余长度
                        //recv_len -= offset;
                        // 找到','后面的数字
                        signal_strenth[i] = atoi((char*)&wifi_recv_buff[offset + signal_point + 1]);                    
                    }
                    else
                    {
                        signal_strenth[i] = 0;
                    }
                }
            }
        }
        return true;
    }
    return false;
}

