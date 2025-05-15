#ifndef JFETCH_H
#define JFETCH_H

#include <stddef.h>

#define BUFFERSIZE 256 // Default length for all used buffers
#define PADDING 44     // Jorb size
#define FPS 20         // Speed of animation/stats reloading

// Escape codes for drawing
#define CLEARSCREEN "\033[2J"
#define COLOR_CYAN "\033[38;2;45;120;180m"
#define COLOR_RESET "\033[0m"
#define POS "\033[%d;%dH" // Move cursor to y;x

#define NULL_RETURN(ptr) do { if (ptr == NULL) return; } while (0)

typedef struct system_stats {
    char user_name[BUFFERSIZE],
         host_name[BUFFERSIZE],
         datetime[BUFFERSIZE],
         os_name[BUFFERSIZE],
         kernel_version[BUFFERSIZE],
         desktop_name[BUFFERSIZE],
         shell_name[BUFFERSIZE],
         terminal_name[BUFFERSIZE],
         cpu_name[BUFFERSIZE],
         gpu_name[BUFFERSIZE],
         cpu_usage[BUFFERSIZE],
         ram_usage[BUFFERSIZE],
         swap_usage[BUFFERSIZE],
         disk_usage[BUFFERSIZE],
         process_count[BUFFERSIZE],
         uptime[BUFFERSIZE],
         battery_charge[BUFFERSIZE];
} system_stats;

typedef struct animation_object {
    size_t current_frame;
    size_t frame_count;
    const char *color;
    char *frames[];
} animation_object;

#endif // JFETCH_H
