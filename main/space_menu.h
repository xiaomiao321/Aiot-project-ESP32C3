#ifndef SPACE_MENU_H
#define SPACE_MENU_H

/**
 * @brief 太空信息中心的主屏幕函数。
 * @details 这是太空信息功能的入口点。它首先初始化并获取一次数据，
 *          然后进入一个循环，处理用户的旋转编码器和按钮输入，
 *          以实现页面切换和退出功能。它还会定时自动更新太空数据。
 */
void SpaceMenuScreen();

/**
 * @brief 切换到下一个信息页面。
 * @details 将内部的页面索引加一，如果到达末尾则循环回第一页。
 */
void space_menu_next_page();

/**
 * @brief 切换到上一个信息页面。
 * @details 将内部的页面索引减一，如果到达开头则循环到最后一页。
 */
void space_menu_prev_page();

/**
 * @brief 处理返回按钮的逻辑。
 * @details 用于退出当前菜单。
 * @return 总是返回 `true`，表示应当退出。
 */
bool space_menu_back_press();

/**
 * @brief 绘制当前页面的内容。
 * @details 根据当前的页面索引，调用相应的内部绘图函数来显示
 *          宇航员信息或国际空间站的实时位置。
 */
void space_menu_draw();

#endif // SPACE_MENU_H
