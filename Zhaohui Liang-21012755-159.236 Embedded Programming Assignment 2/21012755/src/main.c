#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "graphics.h"
#include <fonts.h>
#include <st7789v.h>
#include "esp_timer.h"

#define TFT_WIDTH 135
#define TFT_HEIGHT 240
#define BLOCK_SIZE 10
#define MAX_BLOCKS 30 // 最大块的数量

// Define button GPIO pins
#define BUTTON_LEFT_GPIO 0
#define BUTTON_RIGHT_GPIO 35
#define NSTARS 55

typedef struct pos {
    float x;
    float y;
    float speed;
    int colour;
} pos;

// Structure to represent falling blocks
typedef struct {
    int x;
    int y;
    int speed;
    int size; // 块的边长
} Block;

enum GameState {
    GAME_START,
    GAME_PLAYING,
    GAME_OVER
};

// Initialize block properties with a uniform speed
void initBlock(Block *block, int speed) {
    block->x = rand() % (TFT_WIDTH - block->size);
    block->y = 0;
    block->speed = speed; // 设置块的速度，这里所有块的速度一致
}

int score = 0;
int blockCount = 3; // 初始化块的数量
int gameStartTime = 0;
int elapsedTime = 0;
enum GameState gameState = GAME_START;
bool scoreDisplayed = false;
Block blocks[MAX_BLOCKS]; // 增大块的数量上限
int playerX = TFT_WIDTH / 2 - BLOCK_SIZE / 2;
pos stars[NSTARS];

void timer_callback(void* arg) {
    // 在这里执行定时任务的逻辑
    if (gameState == GAME_PLAYING) {
        // 更新和绘制下落块
        for (int i = 0; i < blockCount; i++) {
            blocks[i].y += blocks[i].speed;
            if (blocks[i].y > TFT_HEIGHT) {
                // 块达到底部，重置它和增加分数
                blocks[i].x = rand() % (TFT_WIDTH - blocks[i].size);
                blocks[i].y = 0;
                score += 100;
            }

            // 检查是否与玩家块发生碰撞
            if (blocks[i].x + blocks[i].size > playerX &&
                blocks[i].x < playerX + BLOCK_SIZE &&
                blocks[i].y + blocks[i].size > TFT_HEIGHT - BLOCK_SIZE) {
                // 游戏结束
                gameState = GAME_OVER;
                cls(rgbToColour(0, 0, 0)); // 清除屏幕
                setFont(FONT_UBUNTU16);
                setFontColour(255, 255, 255); // 设置字体颜色为白色
                gprintf("\n\n\n\n\n\n\n");
                gprintf("     Game Over!\n      Score: %d", score);
                scoreDisplayed = true;
                vTaskDelay(1000 / portTICK_PERIOD_MS); // 显示分数2秒
                for (int j = 0; j < MAX_BLOCKS; j++) {
                    initBlock(&blocks[j], 2); // 重新设置速度
                }
                blockCount = 3; // 重新开始游戏时恢复初始块数量
                gameStartTime = 0; // 重置游戏开始时间
                elapsedTime = 0; // 重新开始游戏时重置已经进行的时间
            }

            // 绘制下落块
            draw_rectangle(blocks[i].x, blocks[i].y, blocks[i].size, blocks[i].size, rgbToColour(255, 0, 0));
        }

        // 根据游戏时间调整块的数量和下落速度
        elapsedTime = esp_log_timestamp() - gameStartTime;
        if (elapsedTime > 3000) { // 每过30秒增加块的数量和加速下落速度
            blockCount += 1; // 每次增加1个块
            if (blockCount > MAX_BLOCKS) {
                blockCount = MAX_BLOCKS; // 最多有MAX_BLOCKS个块
            }
            for (int i = 0; i < blockCount; i++) {
                blocks[i].speed += 1; // 加速速度
            }
            gameStartTime = esp_log_timestamp(); // 重置游戏开始时间
        }

        // 绘制玩家块
        draw_rectangle(playerX, TFT_HEIGHT - BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, rgbToColour(0, 0, 255));
        if (!scoreDisplayed) {
            // 显示分数
            setFont(FONT_UBUNTU16);
            setFontColour(255, 255, 255); // 设置字体颜色为白色
            gprintf("Score: %d", score);
        }
    }
}

void app_main() {

    // Initialize graphics library
    graphics_init();
    set_orientation(PORTRAIT);
    for(int i=0;i<NSTARS;i++) {
        stars[i].x=rand()%display_width;
        stars[i].y=rand()%display_height;
        stars[i].speed=(rand()%512+64)/256.0;
        stars[i].colour=rand();
    }

    // 定义硬件定时器的参数
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback, // 回调函数
        .name = "my_timer" // 定时器的名称
    };
    esp_timer_handle_t my_timer;
    esp_timer_create(&timer_args, &my_timer);

    // 启动硬件定时器，设置定时周期为1000毫秒（1秒）
    esp_timer_start_periodic(my_timer, 1000000); // 周期为1秒，单位是微秒

    // Initialize GPIOs for buttons
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BUTTON_LEFT_GPIO) | (1ULL << BUTTON_RIGHT_GPIO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    bool frame_sent = false;

    // Seed random number generator
    srand(time(NULL));

    // Create an array of blocks
    for (int i = 0; i < MAX_BLOCKS; i++) { // 随机初始化块
        blocks[i].size = BLOCK_SIZE;
        initBlock(&blocks[i], 2); // 设置统一的速度为2
    }


    while (1) {
        // 清屏
        cls(rgbToColour(0, 0, 0));

        // 检查按钮输入
        if (gpio_get_level(BUTTON_LEFT_GPIO) == 0) {
            playerX -= 5;
        }
        if (gpio_get_level(BUTTON_RIGHT_GPIO) == 0) {
            playerX += 5;
        }

        // 确保玩家不会超出屏幕边界
        playerX = fmax(0, fmin(TFT_WIDTH - BLOCK_SIZE, playerX));
        // 在全局范围内定义一个标志变量
        // 游戏逻辑
        switch (gameState) {
            case GAME_START:
                // 显示游戏开始提示
                setFont(FONT_UBUNTU16);
                setFontColour(255, 255, 255); // 设置字体颜色为白色
                gprintf("\n\n\n\n\nPress LEFT button \n         to start\n");
                if (gpio_get_level(BUTTON_LEFT_GPIO) == 0) {
                    gameState = GAME_PLAYING;
                    gameStartTime = esp_log_timestamp();
                }
                break;

            case GAME_PLAYING:
                for(int i=0;i<NSTARS;i++) {
                    draw_pixel(stars[i].x,stars[i].y,stars[i].colour);
                    stars[i].y += stars[i].speed;
                    if(stars[i].y>=display_height) {
                        stars[i].x=rand()%display_width;
                        stars[i].y=0;
                        stars[i].speed=(rand()%512+64)/256.0;
                    }
                }
                // 绘制玩家块
                draw_rectangle(playerX, TFT_HEIGHT - BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, rgbToColour(0, 0, 255));

                // 更新和绘制下落块
                for (int i = 0; i < blockCount; i++) {
                    blocks[i].y += blocks[i].speed;
                    if (blocks[i].y > TFT_HEIGHT) {
                        // 块达到底部，重置它和增加分数
                        blocks[i].x = rand() % (TFT_WIDTH - blocks[i].size);
                        blocks[i].y = 0;
                        score += 100;
                    }
                    // 绘制下落块
                    draw_rectangle(blocks[i].x, blocks[i].y, blocks[i].size, blocks[i].size, rgbToColour(255, 0, 0));

                    // 检查是否与玩家块发生碰撞
                    if (blocks[i].x + blocks[i].size > playerX &&
                        blocks[i].x < playerX + BLOCK_SIZE &&
                        blocks[i].y + blocks[i].size > TFT_HEIGHT - BLOCK_SIZE) {
                        // 游戏结束
                        gameState = GAME_OVER;
                        cls(rgbToColour(0, 0, 0)); // 清除屏幕
                        setFont(FONT_UBUNTU16);
                        setFontColour(255, 255, 255); // 设置字体颜色为白色
                        gprintf("\n\n\n\n\n\n\n");
                        gprintf("     Game Over!\n      Score: %d", score);
                        scoreDisplayed = true;
                        vTaskDelay(1000 / portTICK_PERIOD_MS); // 显示分数2秒
                        // score = 0;
                        for (int j = 0; j < MAX_BLOCKS; j++) {
                            initBlock(&blocks[j], 2); // 重新设置速度
                        }
                        blockCount = 3; // 重新开始游戏时恢复初始块数量
                        gameStartTime = 0; // 重置游戏开始时间
                        elapsedTime = 0; // 重新开始游戏时重置已经进行的时间
                    }


                }

                // 根据游戏时间调整块的数量和下落速度
                elapsedTime = esp_log_timestamp() - gameStartTime;
                if (elapsedTime > 3000) { // 每过30秒增加块的数量和加速下落速度
                    blockCount += 1; // 每次增加1个块
                    if (blockCount > MAX_BLOCKS) {
                        blockCount = MAX_BLOCKS; // 最多有MAX_BLOCKS个块
                    }
                    for (int i = 0; i < blockCount; i++) {
                        blocks[i].speed += 1; // 加速速度
                    }
                    gameStartTime = esp_log_timestamp(); // 重置游戏开始时间
                }

                if (!scoreDisplayed) {
                    // 显示分数
                    setFont(FONT_UBUNTU16);
                    setFontColour(255, 255, 255); // 设置字体颜色为白色
                    gprintf("Score: %d", score);
                }

                break;

            case GAME_OVER:
                // 显示游戏结束提示
                                        for (int j = 0; j < MAX_BLOCKS; j++) {
                            initBlock(&blocks[j], 2); // 重新设置速度
                        }
                cls(rgbToColour(0, 0, 0)); // 清除屏幕
                setFont(FONT_SMALL);
                setFontColour(255, 255, 255); // 设置字体颜色为白色
                gprintf("\n\n\n\n\n\n\n");
                gprintf("Game Over!\n Score: %d", score);
                score = 0;
                scoreDisplayed = true;
                vTaskDelay(5000 / portTICK_PERIOD_MS); // 显示分数2秒
                gameState = GAME_START; // 返回游戏开始状态
                break;
        }

        // 更新屏幕
        flip_frame();
        frame_sent = !frame_sent;

        // 控制游戏速度
        vTaskDelay(80 / portTICK_PERIOD_MS);
    }
    esp_timer_delete(my_timer);

}