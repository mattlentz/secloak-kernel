#include <stdbool.h>
#include <sys/queue.h>

#define KEY_VOLUMEDOWN 114
#define KEY_VOLUMEUP 115
#define KEY_POWER 116
#define KEY_MENU 139
#define KEY_BACK 158
#define KEY_HOMEPAGE 172

struct button_handler {
	bool (*on_press)(int code);
	SLIST_ENTRY(button_handler) entry;
};

bool gpio_keys_acquire(struct button_handler *handler);
void gpio_keys_release(struct button_handler *handler);

