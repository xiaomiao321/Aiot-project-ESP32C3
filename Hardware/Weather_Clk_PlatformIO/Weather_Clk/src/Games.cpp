#include "RotaryEncoder.h"
#include <Preferences.h>
#include <TFT_eSPI.h>
#include "img.h"
#include "LED.h"
#include "Buzzer.h"
#include "Alarm.h"
#include "weather.h"
#include "performance.h"
#include "DS18B20.h"
#include "animation.h"
#include "Games.h"
#include "MQTT.h"

// --- 布局配置 ---
static const int ICON_SIZE = 200;      // 游戏图标大小
static const int ICON_SPACING = 220;   // 图标间距
static const int SCREEN_WIDTH = 240;   // 屏幕宽度
static const int SCREEN_HEIGHT = 240;  // 屏幕高度

// 计算出的布局值
static const int ICON_Y_POS = (SCREEN_HEIGHT / 2) - (ICON_SIZE / 2); // 图标的Y坐标
static const int TRIANGLE_BASE_Y = ICON_Y_POS - 5;               // 选择指示三角形的底部Y坐标
static const int TRIANGLE_PEAK_Y = TRIANGLE_BASE_Y - 20;             // 三角形顶部Y坐标
static const int INITIAL_X_OFFSET = (SCREEN_WIDTH / 2) - (ICON_SIZE / 2); // 初始X偏移
static const uint8_t ANIMATION_STEPS = 12; // 滚动动画的步数

// --- 状态变量 ---
int16_t game_display = INITIAL_X_OFFSET; // 游戏图标显示的当前X偏移
int16_t game_picture_flag = 0;           // 当前选中的游戏索引

// --- 函数原型声明 ---
void ConwayGame();
void BuzzerTapGame();
void TimeChallengeGame();
void drawGameIcons(int16_t offset);
void flappy_bird_game();

/**
 * @brief 检测双击事件
 * @param reset 如果为true，则重置双击检测状态
 * @return 如果检测到双击，返回true
 */
bool gamesDetectDoubleClick(bool reset = false)
{
    static unsigned long lastClickTime = 0;
    if (reset)
    {
        lastClickTime = 0;
        return false;
    }
    unsigned long currentTime = millis();
    if (currentTime - lastClickTime < 500)
    { // 500ms内两次点击视为双击
        lastClickTime = 0;
        return true;
    }
    lastClickTime = currentTime;
    return false;
}

// --- 按钮防抖变量 ---
static unsigned long lastButtonDebounceTime = 0;
static int lastDebouncedButtonState = HIGH; // HIGH = 未按下, LOW = 按下
const int BUTTON_DEBOUNCE_DELAY = 50; // 防抖延迟 (ms)
#define ENCODER_SW 25 // 编码器按钮引脚

// --- 游戏菜单定义 ---
struct GameItem
{
    const char *name;         // 游戏名称
    const uint16_t *image;    // 游戏图标图像数据指针
    void (*function)();       // 指向游戏主函数的函数指针
};

// 游戏列表
const GameItem gameItems[] = {
    {"Conway's Game", Games, ConwayGame},
    {"Flappy Bird", bird_big, flappy_bird_game},
    {"Buzzer Tap", Timer, BuzzerTapGame},
    {"Time Challenge", Timer, TimeChallengeGame},
};
const uint8_t GAME_ITEM_COUNT = sizeof(gameItems) / sizeof(gameItems[0]);

enum GameState
{
    STATE_INITIAL,      // 初始绿色屏幕，等待变红
    STATE_TIMING,       // 红色屏幕，正在计时
    STATE_RESULT,       // 显示结果
    STATE_TOO_SOON      // 按下过早
};
/**
 * @brief 绘制游戏选择菜单的图标界面
 * @param offset 当前图标滚动的X轴偏移量
 */
void drawGameIcons(int16_t offset)
{
    menuSprite.fillSprite(TFT_BLACK);

    // 绘制底部倒置的白色选择三角形指示器
    int16_t triangle_x = offset + (game_picture_flag * ICON_SPACING) + (ICON_SIZE / 2);
    menuSprite.fillTriangle(triangle_x, SCREEN_HEIGHT - 25, triangle_x - 12, SCREEN_HEIGHT - 5, triangle_x + 12, SCREEN_HEIGHT - 5, TFT_WHITE);

    // 绘制可见范围内的游戏图标
    for (int i = 0; i < GAME_ITEM_COUNT; i++)
    {
        int16_t x = offset + (i * ICON_SPACING);
        if (x >= -ICON_SIZE && x < SCREEN_WIDTH)
        {
            menuSprite.pushImage(x, ICON_Y_POS, ICON_SIZE, ICON_SIZE, gameItems[i].image);
        }
    }

    // 绘制文本信息
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    menuSprite.setTextSize(1);
    menuSprite.setTextDatum(TL_DATUM);
    menuSprite.drawString("GAMES:", 10, 10);
    menuSprite.drawString(gameItems[game_picture_flag].name, 60, 10);

    menuSprite.pushSprite(0, 0); // 将sprite内容推送到屏幕
}

/**
 * @brief 游戏选择的主菜单函数
 */
void GamesMenu()
{
    tft.fillScreen(TFT_BLACK);

    // 重置菜单状态
    game_picture_flag = 0;
    game_display = INITIAL_X_OFFSET;
    drawGameIcons(game_display);

    gamesDetectDoubleClick(true); // 进入菜单时重置双击检测

    // 用于处理单击/双击逻辑的变量
    static unsigned long gamesMenuLastClickTime = 0;
    static bool gamesMenuSingleClickPending = false;

    while (true)
    {
        // 检查全局退出条件
        if (exitSubMenu || g_alarm_is_ringing) { return; }
        if (readButtonLongPress()) { return; } // 长按退出

        int direction = readEncoder();
        if (direction != 0)
        { // 旋转编码器切换游戏
            if (direction == 1)
            {
                game_picture_flag = (game_picture_flag + 1) % GAME_ITEM_COUNT;
            }
            else if (direction == -1)
            {
                game_picture_flag = (game_picture_flag == 0) ? GAME_ITEM_COUNT - 1 : game_picture_flag - 1;
            }
            tone(BUZZER_PIN, 1000 * (game_picture_flag + 1), 20); // 播放提示音

            // --- 带缓动效果的滚动动画 ---
            int16_t target_display = INITIAL_X_OFFSET - (game_picture_flag * ICON_SPACING);
            int16_t start_display = game_display;
            for (uint8_t i = 0; i <= ANIMATION_STEPS; i++)
            {
                float t = (float) i / ANIMATION_STEPS;
                float eased_t = 0.5 * (1 - cos(t * PI)); // 使用余弦缓动
                game_display = start_display + (target_display - start_display) * eased_t;
                drawGameIcons(game_display);
                vTaskDelay(pdMS_TO_TICKS(15));
            }
            game_display = target_display;
            drawGameIcons(game_display);
        }

        if (readButton())
        { // 检测到按钮点击
            if (gamesDetectDoubleClick())
            { // 如果是双击
                gamesMenuSingleClickPending = false; // 取消待处理的单击
                return; // 退出游戏菜单
            }
            else
            { // 否则是单击
                gamesMenuLastClickTime = millis();
                gamesMenuSingleClickPending = true; // 标记为有待处理的单击
            }
        }

        // 检查待处理的单击事件是否应该被执行 (即等待双击窗口超时后)
        if (gamesMenuSingleClickPending && (millis() - gamesMenuLastClickTime > 500))
        {
            gamesMenuSingleClickPending = false; // 消费掉这个单击事件
            tone(BUZZER_PIN, 2000, 50);
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gameItems[game_picture_flag].function)
            {
                gameItems[game_picture_flag].function(); // 执行对应的游戏函数
            }
            // 游戏结束后，重绘菜单
            tft.fillScreen(TFT_BLACK);
            drawGameIcons(game_display);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
//... 后续游戏实现
// --- 康威生命游戏实现 ---
#define GRID_SIZE 60
#define CELL_SIZE 4 // 每个细胞的像素大小
bool grid[GRID_SIZE][GRID_SIZE];
bool nextGrid[GRID_SIZE][GRID_SIZE];

/**
 * @brief 初始化康威生命游戏的网格
 */
void initConwayGrid()
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            grid[i][j] = (random(100) < 20); // 20%的概率为活细胞
        }
    }
}

/**
 * @brief 绘制康威生命游戏的网格
 */
void drawConwayGrid()
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (grid[i][j])
            {
                tft.fillRect(j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE, TFT_GREEN);
            }
            else
            {
                tft.fillRect(j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE, TFT_BLACK);
            }
        }
    }
}

/**
 * @brief 更新康威生命游戏的网格到下一代
 */
void updateConwayGrid()
{
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            int liveNeighbors = 0;
            // 检查8个邻居
            for (int x = -1; x <= 1; x++)
            {
                for (int y = -1; y <= 1; y++)
                {
                    if (x == 0 && y == 0) continue; // 跳过自身
                    int ni = (i + x + GRID_SIZE) % GRID_SIZE; // 环形边界
                    int nj = (j + y + GRID_SIZE) % GRID_SIZE; // 环形边界
                    if (grid[ni][nj])
                    {
                        liveNeighbors++;
                    }
                }
            }

            // 应用生命游戏规则
            if (grid[i][j])
            { // 活细胞
                if (liveNeighbors < 2 || liveNeighbors > 3)
                {
                    nextGrid[i][j] = false; // 因孤单或拥挤而死
                }
                else
                {
                    nextGrid[i][j] = true; // 继续存活
                }
            }
            else
            { // 死细胞
                if (liveNeighbors == 3)
                {
                    nextGrid[i][j] = true; // 复活
                }
                else
                {
                    nextGrid[i][j] = false; // 保持死亡
                }
            }
        }
    }
    // 将下一代状态复制到当前网格
    memcpy(grid, nextGrid, sizeof(grid));
}

/**
 * @brief 康威生命游戏主函数
 */
void ConwayGame()
{
    tft.fillScreen(TFT_BLACK);
    initConwayGrid();
    drawConwayGrid();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(35, 220);
    tft.print("Auto-running, Double-click to exit");

    unsigned long lastUpdateTime = millis();
    const unsigned long UPDATE_INTERVAL = 200; // 每200ms更新一代

    while (true)
    {
        if (exitSubMenu || g_alarm_is_ringing) { return; }

        // 自动演化
        if (millis() - lastUpdateTime > UPDATE_INTERVAL)
        {
            updateConwayGrid();
            drawConwayGrid();
            lastUpdateTime = millis();
        }

        // 按钮处理，双击退出
        if (readButton())
        {
            if (gamesDetectDoubleClick())
            {
                return;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 辅助函数：设置反应力测试游戏的初始状态
void setupBuzzerTapInitialState(TFT_eSPI &tft_obj, GameState &state_ref, unsigned long &waitStartTime_ref, long &randomDelay_ref)
{
    tft_obj.fillScreen(TFT_GREEN);
    tft_obj.setTextColor(TFT_WHITE, TFT_GREEN);
    tft_obj.setTextDatum(MC_DATUM);
    tft_obj.setTextFont(4);
    tft_obj.setTextSize(1);
    tft_obj.drawString("Wait for Red", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    tft_obj.setTextFont(1);
    tft_obj.setTextSize(1);
    tft_obj.setTextDatum(BC_DATUM); // Bottom Center
    tft_obj.drawString("Double-click to exit", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 10);
    waitStartTime_ref = millis();
    randomDelay_ref = random(2000, 4001);
    state_ref = STATE_INITIAL;
    gamesDetectDoubleClick(true); // 重置双击检测
}

/**
 * @brief 反应力测试游戏
 * @details 模仿 humanbenchmark.com/tests/reactiontime。
 *          屏幕开始为绿色，随机2-4秒后变为红色，此时计时开始。
 *          玩家需尽快按下按钮，屏幕会显示反应时间。
 *          再次按下可重新开始。双击可退出。
 */
void BuzzerTapGame()
{
    GameState state = STATE_INITIAL;
    unsigned long waitStartTime = 0;
    unsigned long reactionStartTime = 0;
    long randomDelay = 0;

    setupBuzzerTapInitialState(tft, state, waitStartTime, randomDelay);

    while (true)
    {
        if (exitSubMenu || g_alarm_is_ringing) { return; }

        unsigned long currentTime = millis();

        // 状态机主体
        if (state == STATE_INITIAL)
        {
            // 检查是否到达变红时间
            if (currentTime - waitStartTime >= randomDelay)
            {
                tft.fillScreen(TFT_RED);
                reactionStartTime = millis();
                state = STATE_TIMING;
            }
        }

        // 统一处理按钮输入
        if (readButton())
        {
            if (gamesDetectDoubleClick())
            {
                return; // 双击退出
            }

            // 单击逻辑
            switch (state)
            {
            case STATE_INITIAL: // 在变红前点击
                tft.fillScreen(TFT_BLUE);
                tft.setTextColor(TFT_WHITE, TFT_BLUE);
                tft.setTextDatum(MC_DATUM);
                tft.setTextFont(4);
                tft.setTextSize(1);
                tft.drawString("Too Soon!", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20);
                tft.drawString("Click to restart", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20);
                state = STATE_TOO_SOON;
                break;

            case STATE_TIMING: // 成功点击
            {
                unsigned long reactionTime = millis() - reactionStartTime;
                tft.fillScreen(TFT_BLACK);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.setTextDatum(MC_DATUM);

                // 用数码管字体显示时长
                tft.setTextFont(7);
                tft.setTextSize(1);
                char timeStr[20];
                sprintf(timeStr, "%lu", reactionTime);
                tft.drawString(timeStr, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 10);

                // 显示提示文字
                tft.setTextFont(4);
                tft.setTextSize(1);
                tft.drawString("Click to restart", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 40);
                state = STATE_RESULT;
            }
            break;

            case STATE_RESULT:
            case STATE_TOO_SOON: // 在结果界面点击，重新开始
                setupBuzzerTapInitialState(tft, state, waitStartTime, randomDelay);
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// --- 时间挑战游戏常量 ---
static const int PROGRESS_BAR_X = 20;
static const int PROGRESS_BAR_Y = 180;
static const int PROGRESS_BAR_WIDTH = 200;
static const int PROGRESS_BAR_HEIGHT = 10;
static const uint16_t PROGRESS_BAR_COLOR = TFT_GREEN;
static const uint16_t PROGRESS_BAR_BG_COLOR = TFT_DARKGREY;

/**
 * @brief 时间挑战游戏
 */
void TimeChallengeGame()
{
    menuSprite.fillSprite(TFT_BLACK);
    menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    menuSprite.setTextSize(2);

    unsigned long lastBuzzerTime = 0;
    const unsigned long BUZZER_INTERVAL_MS = 1000;

    unsigned long targetTimeMs = random(8000, 12001); // 随机目标时间 8-12秒
    float targetTimeSec = targetTimeMs / 1000.0;

    menuSprite.setCursor(20, 30);
    menuSprite.printf("Target: %.1f s", targetTimeSec);
    menuSprite.setCursor(20, 80);
    menuSprite.print("Timer: 0.0 s");

    // 绘制进度条背景
    menuSprite.drawRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, TFT_WHITE);
    menuSprite.fillRect(PROGRESS_BAR_X + 1, PROGRESS_BAR_Y + 1, PROGRESS_BAR_WIDTH - 2, PROGRESS_BAR_HEIGHT - 2, PROGRESS_BAR_BG_COLOR);

    unsigned long startTime = millis();
    unsigned long pressTime = 0;
    bool gameEnded = false;

    while (true)
    {
        if (exitSubMenu || g_alarm_is_ringing) { return; }

        unsigned long currentTime = millis();

        if (!gameEnded)
        { // 如果游戏未结束
            float elapsedSec = (currentTime - startTime) / 1000.0;
            menuSprite.fillRect(100, 80, 120, 20, TFT_BLACK);
            menuSprite.setTextSize(3);
            menuSprite.setCursor(100, 80);
            menuSprite.printf("%.1f s", elapsedSec); // 更新计时器显示
            menuSprite.setTextSize(2);

            // 更新进度条
            float progressRatio = (float) (currentTime - startTime) / targetTimeMs;
            int filledWidth = (int) (progressRatio * (PROGRESS_BAR_WIDTH - 2));
            if (filledWidth > PROGRESS_BAR_WIDTH - 2) filledWidth = PROGRESS_BAR_WIDTH - 2;
            menuSprite.fillRect(PROGRESS_BAR_X + 1, PROGRESS_BAR_Y + 1, filledWidth, PROGRESS_BAR_HEIGHT - 2, PROGRESS_BAR_COLOR);

            // 每秒蜂鸣一次
            if (currentTime - lastBuzzerTime >= BUZZER_INTERVAL_MS)
            {
                tone(BUZZER_PIN, 1000, 50);
                lastBuzzerTime = currentTime;
            }
        }
        menuSprite.pushSprite(0, 0); // 推送到屏幕

        if (readButton())
        {
            if (gamesDetectDoubleClick()) { return; } // 双击退出
            if (!gameEnded)
            { // 首次单击结束游戏
                pressTime = currentTime;
                gameEnded = true;
                tone(BUZZER_PIN, 1500, 100); // 确认音

                // 计算并显示时间差
                float diffSec = (float) ((long) (pressTime - startTime) - (long) targetTimeMs) / 1000.0;
                menuSprite.setTextSize(2);
                menuSprite.setCursor(20, 130);
                menuSprite.printf("Diff: %.2f s", diffSec);
                menuSprite.pushSprite(0, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// --- Flappy Bird 游戏实现 ---
#define BIRD_X 40
#define BIRD_RADIUS 5
#define GRAVITY 0.3
#define JUMP_FORCE -4.5
#define PIPE_WIDTH 20
#define PIPE_GAP 80
#define PIPE_SPEED 2
#define PIPE_INTERVAL 120 // 管道之间的水平距离

/**
 * @brief Flappy Bird 游戏主函数
 */
void flappy_bird_game()
{
    float bird_y = SCREEN_HEIGHT / 2;
    float bird_vy = 0;
    int pipes_x[2], pipes_y[2];
    int score = 0;
    bool game_over = false;
    bool start_game = false;

    // 初始化管道
    pipes_x[0] = SCREEN_WIDTH;
    pipes_y[0] = random(40, SCREEN_HEIGHT - 40 - PIPE_GAP);
    pipes_x[1] = SCREEN_WIDTH + PIPE_INTERVAL + (PIPE_WIDTH / 2);
    pipes_y[1] = random(40, SCREEN_HEIGHT - 40 - PIPE_GAP);

    unsigned long lastFrameTime = millis();
    unsigned long lastClickTime = 0;

    while (true)
    {
        if (exitSubMenu || g_alarm_is_ringing) { return; }
        if (readButtonLongPress()) { return; }
        if (readButton())
        {
            if (gamesDetectDoubleClick()) { return; }
            lastClickTime = millis();
        }

        // 游戏逻辑
        if (!start_game)
        { // 等待开始
            if (millis() - lastClickTime < 500 && lastClickTime != 0)
            {
                start_game = true;
                lastClickTime = 0;
            }
        }
        else if (!game_over)
        { // 游戏进行中
// 帧率控制
            unsigned long currentTime = millis();
            if (currentTime - lastFrameTime < 30) { vTaskDelay(pdMS_TO_TICKS(1)); continue; }
            lastFrameTime = currentTime;

            // 输入，让小鸟跳跃
            if (millis() - lastClickTime < 500 && lastClickTime != 0)
            {
                bird_vy = JUMP_FORCE;
                tone(BUZZER_PIN, 1500, 20);
                lastClickTime = 0;
            }

            // 物理模拟
            bird_vy += GRAVITY;
            bird_y += bird_vy;

            // 管道逻辑
            for (int i = 0; i < 2; i++)
            {
                pipes_x[i] -= PIPE_SPEED;
                // 管道移出屏幕后重置
                if (pipes_x[i] < -PIPE_WIDTH)
                {
                    pipes_x[i] = SCREEN_WIDTH;
                    pipes_y[i] = random(40, SCREEN_HEIGHT - 40 - PIPE_GAP);
                }
                // 通过管道得分
                if (pipes_x[i] + PIPE_WIDTH < BIRD_X && pipes_x[i] + PIPE_WIDTH + PIPE_SPEED >= BIRD_X)
                {
                    score++;
                    tone(BUZZER_PIN, 2500, 20);
                }
            }

            // 碰撞检测
            if (bird_y + BIRD_RADIUS > SCREEN_HEIGHT || bird_y - BIRD_RADIUS < 0) game_over = true; // 撞到上下边界
            for (int i = 0; i < 2; i++)
            {
                if (BIRD_X + BIRD_RADIUS > pipes_x[i] && BIRD_X - BIRD_RADIUS < pipes_x[i] + PIPE_WIDTH)
                {
                    if (bird_y - BIRD_RADIUS < pipes_y[i] || bird_y + BIRD_RADIUS > pipes_y[i] + PIPE_GAP)
                    {
                        game_over = true; // 撞到管道
                    }
                }
            }
            if (game_over) tone(BUZZER_PIN, 500, 200);

        }
        else
        { // 游戏结束
            if (millis() - lastClickTime < 500 && lastClickTime != 0)
            {
                // 单击以重置游戏
                bird_y = SCREEN_HEIGHT / 2; bird_vy = 0; score = 0;
                pipes_x[0] = SCREEN_WIDTH; pipes_y[0] = random(40, SCREEN_HEIGHT - 40 - PIPE_GAP);
                pipes_x[1] = SCREEN_WIDTH + PIPE_INTERVAL + (PIPE_WIDTH / 2); pipes_y[1] = random(40, SCREEN_HEIGHT - 40 - PIPE_GAP);
                game_over = false; start_game = false; lastClickTime = 0;
            }
        }

        // --- 绘图 ---
        menuSprite.fillSprite(TFT_BLACK);

        if (!start_game)
        {
            menuSprite.setTextSize(2);
            menuSprite.setCursor(50, SCREEN_HEIGHT / 2 + 10);
            menuSprite.print("Click to start");
        }
        else
        {
            // 画小鸟
            menuSprite.pushImage(BIRD_X - 5, (int) bird_y - 4, 11, 8, bird);
            // 画管道
            for (int i = 0; i < 2; i++)
            {
                menuSprite.fillRect(pipes_x[i], 0, PIPE_WIDTH, pipes_y[i], TFT_GREEN);
                menuSprite.fillRect(pipes_x[i] - 2, pipes_y[i] - 10, PIPE_WIDTH + 4, 10, TFT_GREEN);
                menuSprite.fillRect(pipes_x[i], pipes_y[i] + PIPE_GAP, PIPE_WIDTH, SCREEN_HEIGHT - (pipes_y[i] + PIPE_GAP), TFT_GREEN);
                menuSprite.fillRect(pipes_x[i] - 2, pipes_y[i] + PIPE_GAP, PIPE_WIDTH + 4, 10, TFT_GREEN);
            }
            // 画分数
            menuSprite.setTextSize(2);
            menuSprite.setCursor(10, 10);
            menuSprite.printf("Score: %d", score);

            if (game_over)
            {
                menuSprite.setTextSize(3); menuSprite.setCursor(40, SCREEN_HEIGHT / 2 - 30); menuSprite.print("Game Over");
                menuSprite.setTextSize(2); menuSprite.setCursor(50, SCREEN_HEIGHT / 2 + 10); menuSprite.print("Click to restart");
            }
        }
        menuSprite.setTextSize(1);
        menuSprite.setTextColor(TFT_WHITE, TFT_BLACK);
        menuSprite.setCursor(20, 220);
        menuSprite.print("Click to jump, Double-click to exit");

        menuSprite.pushSprite(0, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}