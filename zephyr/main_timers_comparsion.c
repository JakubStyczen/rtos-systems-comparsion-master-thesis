/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
// 10kHz system clock
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
// #define SLEEP_TIME_MS   500

/* The devicetree node identifier for the "led0" alias. */
// #define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

volatile float x = 1.001f;

void cpu_load_fpu(void)
{
    for (int i = 0; i < 5000; i++)
        x = x * 1.0001f + 0.0001f;
}

// struct k_timer my_timer;
extern void fn_clock_50us(struct k_timer *timer_id);
extern void fn_clock_250us(struct k_timer *timer_id);
extern void fn_clock_500us(struct k_timer *timer_id);
extern void fn_clock_1000us(struct k_timer *timer_id);
extern void fn_clock_5000us(struct k_timer *timer_id);
extern void fn_clock_10000us(struct k_timer *timer_id);

static uint32_t time_50[1000];
static uint32_t time_250[1000];
static uint32_t time_500[1000];
static uint32_t time_1000[1000];
static uint32_t time_5000[1000];
static uint32_t time_10000[1000];

static int count_50 = 0;
static int count_250 = 0;
static int count_500 = 0;
static int count_1000 = 0;
static int count_5000 = 0;
static int count_10000 = 0;

extern void fn_clock_50us(struct k_timer *timer_id)
{

    if (count_50 < 1000)
    {
        uint32_t time = k_cycle_get_32();
        time_50[count_50] = time;
        count_50++;
    }
    else if (count_10000 == 1000)
    {
        k_timer_stop(timer_id);
    }
}

extern void fn_clock_250us(struct k_timer *timer_id)
{

    if (count_250 < 1000)
    {
        uint32_t time = k_cycle_get_32();
        time_250[count_250] = time;
        count_250++;
    }
    else if (count_10000 == 1000)
    {
        k_timer_stop(timer_id);
    }
}

extern void fn_clock_500us(struct k_timer *timer_id)
{

    if (count_500 < 1000)
    {
        uint32_t time = k_cycle_get_32();
        time_500[count_500] = time;
        count_500++;
    }
    else if (count_10000 == 1000)
    {
        k_timer_stop(timer_id);
    }
}

extern void fn_clock_1000us(struct k_timer *timer_id)
{

    if (count_1000 < 1000)
    {
        uint32_t time = k_cycle_get_32();
        time_1000[count_1000] = time;
        count_1000++;
    }
    else if (count_10000 == 1000)
    {
        k_timer_stop(timer_id);
    }
}

extern void fn_clock_5000us(struct k_timer *timer_id)
{

    if (count_5000 < 1000)
    {
        uint32_t time = k_cycle_get_32();
        time_5000[count_5000] = time;
        count_5000++;
    }
    else if (count_10000 == 1000)
    {
        k_timer_stop(timer_id);
    }
}

extern void fn_clock_10000us(struct k_timer *timer_id)
{

    if (count_10000 < 1000)
    {
        uint32_t time = k_cycle_get_32();
        cpu_load_fpu();
        time_10000[count_10000] = time;
        count_10000++;
    }
    else if (count_10000 == 1000)
    {
        k_timer_stop(timer_id);
        for (size_t i = 0; i < 1000; i++)
        {
            printk("50us;%lu\n", time_50[i]);
            printk("250us;%lu\n", time_250[i]);
            printk("500us;%lu\n", time_500[i]);
            printk("1000us;%lu\n", time_1000[i]);
            printk("5000us;%lu\n", time_5000[i]);
            printk("10000us;%lu\n", time_10000[i]);
        }
    }
}

K_TIMER_DEFINE(my_timer_50us, fn_clock_50us, NULL);
K_TIMER_DEFINE(my_timer_250us, fn_clock_250us, NULL);
K_TIMER_DEFINE(my_timer_500us, fn_clock_500us, NULL);
K_TIMER_DEFINE(my_timer_1000us, fn_clock_1000us, NULL);
K_TIMER_DEFINE(my_timer_5000us, fn_clock_5000us, NULL);
K_TIMER_DEFINE(my_timer_10000us, fn_clock_10000us, NULL);

int main(void)
{
    k_timer_start(&my_timer_50us, K_USEC(100), K_USEC(100));
    k_timer_start(&my_timer_250us, K_USEC(200), K_USEC(200));
    k_timer_start(&my_timer_500us, K_USEC(500), K_USEC(500));
    k_timer_start(&my_timer_1000us, K_USEC(1000), K_USEC(1000));
    k_timer_start(&my_timer_5000us, K_USEC(5000), K_USEC(5000));
    k_timer_start(&my_timer_10000us, K_USEC(10000), K_USEC(10000));

    printk("Timers started\n");

    return 0;
}
