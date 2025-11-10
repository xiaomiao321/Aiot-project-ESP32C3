#ifndef ANIMATION_H
#define ANIMATION_H

/**
 * @brief 动画演示的入口函数。
 * @details 此函数清空屏幕并启动一个FreeRTOS任务(`Animation_task`)，
 *          该任务会持续在TFT屏幕上绘制随机的平滑弧形，
 *          同时驱动NeoPixel灯带显示匹配的颜色，并播放随机的蜂鸣器音效，
 *          形成一个视听联动的动画效果。
 *          函数会监听退出信号（如按钮按下或闹钟触发），并负责安全地停止和清理相关任务。
 */
void AnimationMenu();

#endif