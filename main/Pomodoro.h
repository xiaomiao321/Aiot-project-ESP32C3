#ifndef POMODORO_H
#define POMODORO_H

/**
 * @brief 番茄钟功能的主菜单函数。
 * @details 此函数实现了一个完整的番茄钟工作法定时器。它管理着“工作”、“短休息”、
 *          “长休息”和“暂停”等状态。用户可以通过单击按钮来启动、暂停和恢复计时器。
 *          长按按钮可以退出此功能。界面会通过一个环形进度条和数字来显示剩余时间，
 *          并用标记点来追踪已完成的工作会话数。
 */
void PomodoroMenu();

#endif // POMODORO_H
