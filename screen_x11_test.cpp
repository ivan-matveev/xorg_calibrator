#include "screen_x11.h"
#include "log.h"

#include <thread>
#include <chrono>

bool verbose = true;

int main()
{
	screen_x11_t screen{};
	ASSERT(screen.is_valid);
	screen.rect({{500, 500}, {700, 600}}, RED);
	screen.circle({500, 500}, 100, BLACK);
	screen.cross({500, 500}, 105, WHITE);
	screen.text({510, 510}, "Hello world");
	screen.text({510, 530}, "Привет мир");
	xy_t xy{-1, -1};
	unsigned int keycode = 0;
	while(keycode == 0 && xy.x == -1)
		screen.get_touch_button_event(xy, keycode);
	return 0;
}
