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

const uint LED_PIN = 0;
const uint LIMIT_NC = 8;

rcl_publisher_t publisher, debugPublisher;
rcl_subscription_t subscriber;
std_msgs__msg__Int32 msgOut, msgIn;
std_msgs__msg__String debug;
rmw_message_info_t info;

volatile int ledState = 1;

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
    msgOut.data = ledState;
    rcl_ret_t ret = rcl_publish(&publisher, &msgOut, NULL);
}

void led_callback() {
    rcl_ret_t ret = rcl_take(&subscriber, &msgIn, &info, NULL);
    debugf("Message callback\tstat: %d\tdata:%d", ret, msgIn.data);
    ledState = msgIn.data;
}

void irq_callback(uint pin, uint32_t event) {
    if(pin == LIMIT_NC && gpio_get(pin) && event == GPIO_IRQ_EDGE_RISE){
        gpio_acknowledge_irq(LIMIT_NC, GPIO_IRQ_EDGE_RISE);
        ledState ++;
    }
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

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(LIMIT_NC);
    gpio_set_dir(LIMIT_NC, GPIO_IN);
    gpio_pull_up(LIMIT_NC);

    // gpio_set_irq_callback(&irq_callback);
    gpio_set_irq_enabled_with_callback(LIMIT_NC, GPIO_IRQ_EDGE_RISE, true, &irq_callback);

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
        gpio_put(LED_PIN, ledState);
    }
    return 0;
}
