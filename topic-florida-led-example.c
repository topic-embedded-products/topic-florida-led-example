/*
 * (C) Copyright 2016 Topic Embedded Products
 *
 * SPDX-License-Identifier:	GPL-2.0+
 * 
 * Example program that reads raw input events to detect button presses, and
 * uses these to manipulate the board LEDs via /dev/mem (which is obviously
 * an ugly hack).
 */
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/input.h>

/* GPIO keys are on /dev/input/event1 */
static const char *event_device_default = "/dev/input/event1";
 
/* Address of GPIO controller in logic where the LEDs are */
#define GPIO_CONTROLLER_ADDRESS 0x40000000u
/* Available bits mask */
#define GPIO_CONTROLLER_MASK	0x1F
 
/* Reading GPIO controller doesn't actually work, so use a "shadow"  */
static unsigned int led_state = 0;

static void perror_exit(const char *error)
{
	perror(error);
	exit(1);
}

static void gpio_write(void *mem_map, unsigned int value)
{
	*((volatile int *)mem_map) = value;
	led_state = value;
}

static unsigned int gpio_read(void *mem_map)
{
	
	return led_state;
}

static void on_key_f2(void *mem_map)
{
	unsigned int v = gpio_read(mem_map);

	printf("F2\n");
	
	v  = (v << 1) & GPIO_CONTROLLER_MASK;
	if (!v)
		v = 1;
	gpio_write(mem_map, v);
}

static void on_key_f3(void *mem_map)
{
	unsigned int v = gpio_read(mem_map);

	printf("F3\n");

	v >>= 1;
	if (!v)
		v = (GPIO_CONTROLLER_MASK+1) >> 1;
	gpio_write(mem_map, v);
}

static void on_key_power(void *mem_map)
{
	printf("Power\n");
	gpio_write(mem_map, GPIO_CONTROLLER_MASK);
}

int main(int argc, char *argv[])
{
	struct input_event ev[64];
	int event_fd;
	char name[80];
	ssize_t rd;
	int count;
	int i;
	const char *event_device = event_device_default;
	int mem_fd;
	void *mem_map;

	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (mem_map == (void *)-1)
		perror("/dev/mem");

	mem_map = mmap(NULL, 4096, PROT_WRITE, MAP_SHARED, mem_fd, GPIO_CONTROLLER_ADDRESS);
	if (mem_map == (void*) -1)
		perror("mmap");

	/* Allow device name on commandline */
	if (argc > 1)
		event_device = argv[1];

	event_fd = open(event_device, O_RDONLY);
	if (event_fd == -1)
		perror_exit(event_device);
 
	if (ioctl(event_fd, EVIOCGNAME (sizeof (name)), name) >= 0)
		printf("Reading From: %s (%s)\n", event_device, name);
 
	for(;;) {
		rd = read(event_fd, ev, sizeof(ev));
		if (rd < sizeof(ev[0]))
			perror_exit ("read()");
		count = rd / sizeof(ev[0]);
		for (i = 0; i < count; ++i) {
			if ((ev[i].type == EV_KEY) && (ev[i].value)) {
				/* Keypress event */
				switch (ev[i].code) {
					case KEY_F2:
						on_key_f2(mem_map);
						break;
					case KEY_F3:
						on_key_f3(mem_map);
						break;
					case KEY_POWER:
						on_key_power(mem_map);
						break;
				}
			}
		}
	}

	return 0;
} 
