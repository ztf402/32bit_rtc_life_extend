/**
 * @file rtc_32bit.hpp
 * @author ztf402
 * @brief fixed 32bit RTC for using 64bit timestamp
 * @details After using the setTime function, the RTC will extend life to about 30 years.
 *          If you want to use timestamp to init RTC,you must use function calc_time_offset first
 * @version 0.1
 * @date 2025-07-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once
#include <ctime>
#include <ch32yyxx.h>
#include <cstdio>
#define RTC_EPOCH_OFFSET  1735689600UL  // 2025-01-01 00:00:00 UTC
uint64_t offset_for_32bit=0;
void rtc_init() {
  RCC->APB1PCENR |= RCC_APB1Periph_PWR | RCC_APB1Periph_BKP;
  PWR->CTLR |= PWR_CTLR_DBP;

  // 判断是否首次初始化
  if (BKP->DATAR1 != 0xA5A5) {
    RCC->BDCTLR |= (1 << 0); // LSEON
    while((RCC->BDCTLR & (1 << 1)) == 0); // LSERDY
    RCC->BDCTLR |= (1 << 8); // RTCSEL_LSE
    RCC->BDCTLR |= (1 << 15); // RTCEN

    RTC_WaitForSynchro();
    RTC_EnterConfigMode();
    RTC_SetPrescaler(32767);
    RTC_SetCounter(0);
    RTC_ExitConfigMode();
    RTC_WaitForLastTask();

    BKP->DATAR1 = 0xA5A5; // 标记已初始化
  } else {
    RTC_WaitForSynchro();
  }
}


static const uint8_t _days_in_month[12] = {
    31,28,31,30,31,30,31,31,30,31,30,31
};

static int _is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static uint64_t _days_since_1970(int year, int month, int day) {
    uint64_t days = 0;
    // 累加整年
    for (int y = 1970; y < year; y++) {
        days += _is_leap(y) ? 366 : 365;
    }
    // 累加当年整月
    for (int m = 1; m < month; m++) {
        days += _days_in_month[m-1] + (m==2 && _is_leap(year) ? 1 : 0);
    }
    // 累加当月天数（day 从 1 开始）
    days += (day - 1);
    return days;
}

uint64_t datetime_to_timestamp(int year,
                               int month,
                               int day,
                               int hour,
                               int minute,
                               int second)
{
    uint64_t days = _days_since_1970(year, month, day);
    return days * 86400UL
         + hour * 3600UL
         + minute * 60UL
         + second;
}

void timestamp_to_datetime(uint64_t ts,
                           int *year,
                           int *month,
                           int *day,
                           int *hour,
                           int *minute,
                           int *second)
{
    // 先算天和当天秒数
    uint64_t days = ts / 86400;
    uint64_t secs = ts % 86400;
    // 计算时分秒
    *hour   = secs / 3600;      secs %= 3600;
    *minute = secs / 60;        *second = secs % 60;

    // 计算年
    int y = 1970;
    while (1) {
        uint64_t d = _is_leap(y) ? 366 : 365;
        if (days >= d) {
            days -= d;
            y++;
        } else {
            break;
        }
    }
    *year = y;

    // 计算月、日
    int m = 1;
    for (; m <= 12; m++) {
        uint32_t dim = _days_in_month[m-1] + (m==2 && _is_leap(y) ? 1 : 0);
        if (days < dim) {
            break;
        }
        days -= dim;
    }
    *month = m;
    *day   = days + 1;
}

void timestamp_to_string(uint64_t ts, char *days,char * time)
{
    int Y, M, D, h, m, s;
    timestamp_to_datetime(ts, &Y, &M, &D, &h, &m, &s);
    sprintf(days, "%04d-%02d-%02d",Y, M, D);
    sprintf(time, "%02d:%02d:%02d", h, m, s);
}


/**
 * @brief A special 64bit fixed for 32bit RTC
 * 
 * @return uint64_t 
 */
uint64_t rtc_get_epoch() {
  uint32_t counter = RTC_GetCounter();
  return static_cast<uint64_t>(counter) + offset_for_32bit;
}

uint64_t calc_time_offset(int year){
    offset_for_32bit=datetime_to_timestamp(year, 1, 1, 0, 0, 0);
    return offset_for_32bit;
}

uint64_t rtc_setTime(int year, int month, int day, int hour, int min, int sec,int houroffset) {
    uint64_t epoch = datetime_to_timestamp(year, month, day, hour, min, sec);
    calc_time_offset(year);
    epoch += houroffset * 3600;
    RTC_SetCounter(epoch-offset_for_32bit); // 设置RTC计数器
    return static_cast<uint64_t>(epoch); // 可根据需要调整
}

uint16_t rtc_get_hour(){
    uint64_t epoch = rtc_get_epoch();
    int year, month, day, hour, min, sec;
    timestamp_to_datetime(epoch, &year, &month, &day, &hour, &min, &sec);
    return static_cast<uint16_t>(hour);
}

uint16_t rtc_get_minute(){
    uint64_t epoch = rtc_get_epoch();
    int year, month, day, hour, min, sec;
    timestamp_to_datetime(epoch, &year, &month, &day, &hour, &min, &sec);
    return static_cast<uint16_t>(min);
}
uint16_t rtc_get_second(){
    uint64_t epoch = rtc_get_epoch();
    int year, month, day, hour, min, sec;
    timestamp_to_datetime(epoch, &year, &month, &day, &hour, &min, &sec);
    return static_cast<uint16_t>(sec);
}
uint16_t rtc_get_year(){
    uint64_t epoch = rtc_get_epoch();
    int year, month, day, hour, min, sec;
    timestamp_to_datetime(epoch, &year, &month, &day, &hour, &min, &sec);
    return static_cast<uint16_t>(year);
}
uint8_t rtc_get_month(){
    uint64_t epoch = rtc_get_epoch();
    int year, month, day, hour, min, sec;
    timestamp_to_datetime(epoch, &year, &month, &day, &hour, &min, &sec);
    return static_cast<uint8_t>(month);
}
uint8_t rtc_get_day(){
    uint64_t epoch = rtc_get_epoch();
    int year, month, day, hour, min, sec;
    timestamp_to_datetime(epoch, &year, &month, &day, &hour, &min, &sec);
    return static_cast<uint8_t>(day);
}
