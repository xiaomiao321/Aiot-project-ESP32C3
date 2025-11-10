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

/**
 * @brief 直接进入指定歌曲的精简版播放UI。
 * @param songIndex 要播放的歌曲的索引。
 * @details 此函数会启动播放任务并直接显示“正在播放”界面，
 *          允许用户控制播放，而不是从歌曲列表开始。
 */
void play_song_lite_ui(int songIndex);

#endif
