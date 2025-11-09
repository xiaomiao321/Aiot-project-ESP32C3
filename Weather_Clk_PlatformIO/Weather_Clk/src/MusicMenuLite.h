#ifndef MUSICMENULITE_H
#define MUSICMENULITE_H

/**
 * @brief 轻量级音乐播放器菜单的入口函数。
 * @details 此函数提供一个双层菜单结构。第一层是歌曲列表，用户可以选择歌曲。
 *          选择后进入第二层，即播放界面。在播放界面中，用户可以暂停/继续播放，
 *          切换播放模式（列表循环、单曲循环、随机播放），并查看播放进度、
 *          音符信息和系统时间。长按按钮可以从播放界面返回到歌曲列表。
 */
void MusicMenuLite();

#endif
