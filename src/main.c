#include <stdio.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <std_msgs/msg/string.h>
#include <rmw_microros/rmw_microros.h>
#include <rcl/logging.h>

#include "pico/stdlib.h"
#include "pico_uart_transports.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwipopts.h"

#include "limit.h"
#include "motor.h"
#include "fsm.h"


const uint LED_PIN = 0;
const unsigned char LOWER_LIMIT_PIN = 18;
const unsigned char UPPER_LIMIT_PIN = 22;

const unsigned char MOTOR_PWM_PIN = 4;
const unsigned char MOTOR_FWD_PIN = 6;
const unsigned char MOTOR_REV_PIN = 8;

rcl_publisher_t publisher, debugPublisher;
rcl_subscription_t subscriber;
std_msgs__msg__Int32 msgOut, msgIn;
std_msgs__msg__String debug;
rmw_message_info_t info;

limit_t lowerLimit, upperLimit;
motor_t motor;

volatile int ledState = 0;
volatile int wifiLED = 0;

void debugf(const char* format, ...){
    char str[128];
    
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);

    rosidl_runtime_c__String data;
    data.data = str;
    data.size = strlen(str);
    data.capacity = strlen(str)*sizeof(char);

    debug.data = data;

    rcl_ret_t ret = rcl_publish(&debugPublisher, &debug, NULL);
}

void timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    msgOut.data = fsm_current_state();
    rcl_ret_t ret = rcl_publish(&publisher, &msgOut, NULL);
    
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, wifiLED);
    wifiLED = ~wifiLED;
}

void led_callback() {
    rcl_ret_t ret = rcl_take(&subscriber, &msgIn, &info, NULL);
    debugf("Message callback\tstat: %d\tdata:%d", ret, msgIn.data);
    debugf("FSM sig sent %s", fsm_signal_name(msgIn.data));
    fsm_signal(msgIn.data);
}

void lower_callback(){
    debugf("Lower limit edge");
    fsm_signal(LOWER_LIMIT);
}

void upper_callback(){
    debugf("Upper limit edge");
    fsm_signal(UPPER_LIMIT);
}

void state_change_callback(state_t old, state_t new){
    debugf("State %s -> %s", fsm_state_name(old), fsm_state_name(new));
    
    msgOut.data = fsm_current_state();
    rcl_ret_t ret = rcl_publish(&publisher, &msgOut, NULL);
}

int main()
{
    rcl_ret_t ret;
    rmw_uros_set_custom_transport(
		true,
		NULL,
		pico_serial_transport_open,
		pico_serial_transport_close,
		pico_serial_transport_write,
		pico_serial_transport_read
	);

    debugf("Wifi Init %d", cyw43_arch_init());

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    init_limit(&lowerLimit, LOWER_LIMIT_PIN, lower_callback);
    init_limit(&upperLimit, UPPER_LIMIT_PIN, upper_callback);

    init_motor(&motor, MOTOR_PWM_PIN, MOTOR_FWD_PIN, MOTOR_REV_PIN);

    fsm_init(state_change_callback);

    rcl_timer_t timer;
    rcl_node_t node;
    rcl_allocator_t allocator;
    rclc_support_t support;
    rclc_executor_t executor;

    allocator = rcl_get_default_allocator();

    // Wait for agent successful ping for 2 minutes.
    const int timeout_ms = 1000; 
    const uint8_t attempts = 120;

    ret = rmw_uros_ping_agent(timeout_ms, attempts);

    if (ret != RCL_RET_OK)
    {
        // Unreachable agent, exiting program.
        return ret;
    }

    rclc_support_init(&support, 0, NULL, &allocator);

    rclc_node_init_default(&node, "pico_node", "", &support);
    rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "led_publisher");
    rclc_publisher_init_default(
        &debugPublisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
        "debug");
    debugf("Init");
    ret = rclc_subscription_init_default(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "led_subscriber");    
    debugf("Subscription created\tstat: %d", ret);

    rclc_timer_init_default(
        &timer,
        &support,
        RCL_MS_TO_NS(1000),
        timer_callback);

    rclc_executor_init(&executor, &support.context, 2, &allocator);
    rclc_executor_add_timer(&executor, &timer);
    ret = rclc_executor_add_subscription(
        &executor, &subscriber, &msgIn, &led_callback, ON_NEW_DATA);
    debugf("Callback set\tstat: %d", ret);

    msgOut.data = ledState;
    while (true)
    {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        update_limit(&upperLimit);
        update_limit(&lowerLimit);
        fsm_update(&motor, &upperLimit, &lowerLimit);
    }
    return 0;
}
