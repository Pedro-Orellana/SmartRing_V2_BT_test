#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
#include <string.h>

#include <bluetooth/services/nus.h>


//labels
#define REDLED_NODELABEL        DT_NODELABEL(redled)
#define GREENLED_NODELABEL      DT_NODELABEL(greenled)
#define BLUELED_NODELABEL       DT_NODELABEL(blueled)

//bt device name
#define BT_DEVICE_NAME          CONFIG_BT_DEVICE_NAME
#define BT_DEVICE_NAME_LEN      (sizeof(CONFIG_BT_DEVICE_NAME) - 1)

//devices
static const struct gpio_dt_spec redled = GPIO_DT_SPEC_GET(REDLED_NODELABEL, gpios);
static const struct gpio_dt_spec greenled = GPIO_DT_SPEC_GET(GREENLED_NODELABEL, gpios);
static const struct gpio_dt_spec blueled = GPIO_DT_SPEC_GET(BLUELED_NODELABEL, gpios);


//helper function for LEDs and for sending message back

void send_message(uint8_t *message) {
        bt_nus_send(NULL, message, strlen(message));
}


void activate_led(char color) {
        switch (color)
        {
        case 'R':
                gpio_pin_set_dt(&redled, 1);
                gpio_pin_set_dt(&greenled, 0);
                gpio_pin_set_dt(&blueled, 0);
                send_message("Red LED is on!");
                break;
        case 'G':
                gpio_pin_set_dt(&redled, 0);
                gpio_pin_set_dt(&greenled, 1);
                gpio_pin_set_dt(&blueled, 0);
                send_message("Green LED is on!");
                break;

        case 'B':
                gpio_pin_set_dt(&redled, 0);
                gpio_pin_set_dt(&greenled, 0);
                gpio_pin_set_dt(&blueled, 1);
                send_message("Blue LED is on!");
                break;

        case 'O':
                gpio_pin_set_dt(&redled, 0);
                gpio_pin_set_dt(&greenled, 0);
                gpio_pin_set_dt(&blueled, 0);
                send_message("All LEDs are off!");
                break;
        
        default:
                send_message("Invalid CMD.");
                break;
        }
}

//create advertising parameters
static struct bt_le_adv_param *adv_params = BT_LE_ADV_PARAM(
        BT_LE_ADV_OPT_CONN | BT_LE_AD_NO_BREDR,
        800,
        850,
        NULL
);

//create advertising data
static struct bt_data ad [] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
        BT_DATA(BT_DATA_NAME_COMPLETE, BT_DEVICE_NAME, BT_DEVICE_NAME_LEN)
};

//create advertising work and handler function
void adv_work_handler(struct k_work* work) {
        int err;
        err = bt_le_adv_start(adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
        if(err) {
                printk("There was an error trying to start BLE advertising\n");
        }
}

K_WORK_DEFINE(adv_work, adv_work_handler);


//function to submit work
void start_advertising() {
        k_work_submit(&adv_work);
}


//create callback for BT and recycle function
void on_connection_recycled(void) {
        start_advertising();
}

static struct bt_conn_cb bt_cb = {
        .recycled = on_connection_recycled
};



//create callback for NUS service and on_data_received function

void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len) {
        if (len == 1) {
                char color = (char)*data;
                activate_led(color);
        }
}

static struct bt_nus_cb nus_cb = {
        .received = on_data_received
};




int main(void)
{
        int ret;
        //initializing LEDs
        gpio_pin_configure_dt(&redled, GPIO_OUTPUT_INACTIVE);
        gpio_pin_configure_dt(&greenled, GPIO_OUTPUT_INACTIVE);
        gpio_pin_configure_dt(&blueled, GPIO_OUTPUT_INACTIVE);

        //attach bt_cb
        ret = bt_conn_cb_register(&bt_cb);
        if(ret) {
                printk("Could not add connection callback to BT\n");
        }

        //initialize BT
        bt_enable(NULL);

        //initialize NUS service
        bt_nus_init(&nus_cb);

        //start advertising
        start_advertising();


        return 0;
}
