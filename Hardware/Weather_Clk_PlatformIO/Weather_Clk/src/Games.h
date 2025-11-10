#ifndef GAME_H
#define GAME_H
/**
 * @brief 游戏中心的主菜单函数。
 * @details 显示一个水平滚动的游戏选择界面，用户可以通过旋转编码器选择不同的游戏，
 *          并通过单击进入游戏。双击或长按可以退出游戏菜单。
 */
void GamesMenu();

/**
 * @brief “时间挑战”游戏的入口函数。
 * @details 游戏会给出一个随机的目标时间，玩家需要在计时开始后，
 *          凭感觉在最接近目标时间的时刻按下按钮，游戏会显示最终的时间差。
 */
void TimeChallengeGame();

/**
 * @brief “Flappy Bird”游戏的入口函数。
 * @details 一个经典的Flappy Bird游戏实现。玩家通过单击屏幕（或按键）来使小鸟跳跃，
 *          目标是穿过尽可能多的管道而不发生碰撞。
 */
void flappy_bird_game();

/**
 * @brief “打砖块”游戏的入口函数。
 * @details 玩家控制一个平台左右移动，反弹小球来消除屏幕上方的所有砖块。
 */
void breakoutGame();

/**
 * @brief “躲避汽车”游戏的入口函数。
 * @details 玩家控制自己的赛车在多条车道间切换，以躲避迎面而来的其他车辆。
 */
void carDodgerGame();
#endif