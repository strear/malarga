#include "malarga_gui.h"

using namespace Malarga;

namespace {
	std::map<const char*, const wchar_t*> msg = {
		{"name", L"Malarga"},
		{"title-hello", L"Hello!"},
		{"ellipsis-dots", L"..."},
		{"pen", L"🖊"},
		{"checkmark", L"✔"},
		{"questionmark", L"❓"},
		{"left-arrow", L"⇱"},
		{"magnifier", L"🔍"},
		{"plus", L"➕"},
		{"minus", L"➖"},
		{"trashbin", L"🗑"},
		{"message", L"ℹ"},
		{"stopwatch", L"⏱"},

		{"start", L"开始"},
		{"delete", L"删除"},
		{"cancel", L"取消"},
		{"pause", L"暂停"},
		{"continue", L"继续"},
		{"end", L"结束"},
		{"leave", L"离开"},

		{"title-record", L"排行 Ranking"},
		{"record-empty", L"目前没有记录。\n开始新游戏吧！您的成绩将显示在这里。"},
		{"title-history", L"历史记录"},
		{"find-for", L"按名称查找:"},
		{"record-format", L"%d-%d-%d: %s 得分 %d (%s), 用时 %d:%02d"},

		{"title-start", L"开始！Start"},
		{"starter-level", L"入门版"},
		{"advanced-level", L"进阶版"},
		{"premium-level", L"高级版"},
		{"online", L"联机"},
		{"add-player", L"添加玩家"},
		{"me", L"我"},
		{"or", L"或者："},
		{"wait-for-join", L"等待他人发起游戏"},
		{"waiting-receive", L"正在等待他人发起游戏。"},
		{"add-prompt",
		 L"同一场游戏最多参与5人。\n您可以将 AI 或在线玩家加入游戏。"},
		{"add-ai", L"加入 AI"},
		{"server-unavaliable", L"服务器不可用。"},
		{"connect-failed", L"无法连接服务器。"},
		{"online-prompt", L"以下玩家在线。"},

		{"title-gaming", L"%s 的%s游戏 - Malarga"},
		{"highscore", L"高分"},
		{"ready-start", L"%d名玩家参与游戏。准备开始！"},
		{"death-wall", L"%s 触壁而亡。"},
		{"death-hurt", L"%s 撞上蛇身而亡。"},
		{"death-noscore", L"%s 得分耗尽，不能继续游戏。"},
		{"death-exhaust", L"%s 重生次数用尽。"},
		{"quitted-player", L"%s 退出了游戏。"},
		{"player-lost", L"%s 连接已断开。"},
		{"game-end", L"游戏结束"},

		{"title-about", L"关于 Malarga"},
		{"about-below", L"\
图形库：%s\n编译时间：%s %s\n\
Malarga 即『無它乎』，来自《说文》，是上古时期的问侯语。\n\
意思是“你昨晚睡的好么？有没有被蛇骚扰？”。"},
	{"about-item-symbol",
	L" \n \n▍\n \n \n \n▍\n \n \n \n \n▍\n \n \n \n \n▍\n \n \n \n▍"},
	{"about", L"\
贪吃蛇小游戏 × 高程课大作业。\n \n\
　如何联机？\n\
　一位玩家需要进入“添加玩家”界面来选择参与者，其余玩\n\
　家选择“等待他人发起游戏”即可。\n \n\
　关于操作：\n\
　游戏过程中允许随时退出并观看其他玩家进行游戏，或者\n\
　离开以开始新游戏。可以随时暂停没有联机玩家参与的游戏。\n\
　通过触摸或键盘↑↓←→，WSAD 和 KJHL 都可以操控方向。\n \n\
　入门版：\n\
　让蛇吃掉面板随机位置上的食物。吃到食物小蛇加长。如果\n\
　吃到的是辣椒🌶，得分减少；吃到其他食物得分增加。当\n\
　小蛇撞到边界或者蛇头与蛇身相撞时，蛇将挂掉，游戏结束。\n \n\
　进阶版：\n\
　蛇挂掉后，蛇尸身变成边界，产生新蛇，游戏继续；直到剩\n\
　余空间不足以生成新的蛇和食物为止。其他与入门版相同。\n \n\
　高级版：\n\
　蛇挂掉后，蛇尸身变成食物；其他与进阶版相同。"} };
}

const wchar_t* const MsgViewProvider::Msg(const char* query) {
	return msg[query];
}