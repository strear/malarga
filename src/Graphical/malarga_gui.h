#ifndef MALARGA_GUI_H
#define MALARGA_GUI_H

#include <array>
#include <queue>

#include "../malarga_model.h"
#include "../Rnvntn/rnvntn.h"

using namespace Rnvntn;

namespace Malarga {
	class __ToolkitHelper {
		static bool _toolkitNeedInit;
	public:
		static bool _initToolkit();
	};

#define _NeedToolkitReady \
	bool __toolkitInitMark = __ToolkitHelper::_initToolkit()

	class MsgViewProvider {
	public:
		static const wchar_t* const Msg(const char* query);
	};

	class RecordView {
		_NeedToolkitReady;

		bool windowLoaded = false;
		RnWindow winqry;
		RnButton btnback;
		RnLabel lblhsty, lblpath, lblfind;
		RnTextbox txtfind;
		class ViewItem {
		public:
			RecordView* view;
			const RecordManager::record* record;
			wchar_t label[PlayerController::max_name_length + 70];
			RnLabel lblchk, lblrcd;
			RnButton btnrmv, btnccl;
			ViewItem(const RecordManager::record*, RecordView*);
			~ViewItem();
			void remove();
			void confirmRemove();
			void cancelRemove();
		};
		std::list<ViewItem*> listView;
		void clearListView();
		void updateListView();

	private:
		RecordManager& manager;

	public:
		RecordView(RecordManager&);

		void show();
		void tell(wchar_t*, int lenlim);
	};


	class ArenaView {
		_NeedToolkitReady;

		static const wchar_t* const fruit;
		static const Color fruit_colors[6];
		static const Color bk_colors[5];
		static const Color player_colors[5];
		static const Color player_colors_light[5];

		ArenaModel& model;
		bool inited = false, playing = false;
		bool over = false, quitted = false;
		int highscore = 0;

		RnWindow wui;
		Image doublebuf;
		wchar_t title[41]{};
		RnButton btnpause, btnend;

		std::vector<int> scorepos;
		bool newscore = false;
		std::vector<clock_t> scoretick;

		clock_t msgtick = 0;
		std::queue<std::array<wchar_t, 42>> msgpend;
		Image msgbuf, msgbufprev;

		clock_t lightentick = 0;
		bool lightens[16][9] = {};

		int _mousedownx = -1, _mousedowny = -1;

		friend class ArenaModel;
		bool pauseToggled = false;

		void _lightenbk(Color, float);
		void _paintmsgimg(Image*, const wchar_t* msg);
		void _putshadow(int x, int y, int w, int h);
		void paintbg();
		void paintSnakes(int frameoffset);
		void paintFruits();

	private:
		void paint();
		void putmsg(const wchar_t* msg);
		void togglePause();
		void newFrame();
		void lighten(int i, int j);
		void newScore(int id);
		void tellReady();
		void tellDeath(const wchar_t* name, int type);
		void tellConnectionLost();

	public:
		ArenaView(ArenaModel&);

		void start(int mode, RecordManager* record);
		void quit();
		void reset();
	};

	class AddView {
		_NeedToolkitReady;

		static const int max_players = 5;
		static const int max_online_players = 8;

		bool windowLoaded = false;

		RnWindow wadd;
		RnButton btnback;
		RnLabel lbladd, lblmsg, lbloln, lblnet;
		RnButton btnaddai;
		RnLabel lblchkme, lblme;

		class ItemAdded {
		public:
			AddView* view;
			PlayerController* player;
			RnLabel lblchk, lblrcd;
			RnButton btnrmv;
			ItemAdded(PlayerController*, AddView*);
			~ItemAdded();
			void remove();
		};
		class ItemPend {
		public:
			AddView* view;
			RnLabel lblrcd;
			RnButton btnadd;
			ItemPend(const wchar_t* name, AddView*);
			~ItemPend();
			void add();
		};

		std::list<ItemAdded*> addedItems;
		const RnWidget::Signpost* addedRef;

		std::list<ItemPend*> pendItems;
		bool pendUpdating = false;
		clock_t pendNextUpdate = 0;

		wchar_t onlinePlayers[max_online_players]
			[PlayerController::max_name_length + 1] = {};

		void addAI();
		void add(PlayerController*);
		void disableForFull();
		void updateAdded();
		void updatePend(bool fetchList);
		void clearAdded();
		void clearPend();
		void timelyRefresh();

		ArenaModel* arena;

	public:
		AddView(ArenaModel&);
		~AddView();

		void start();
	};

	class GameInstance {
		_NeedToolkitReady;

		RnWindow w;

		RnImage ico;
		RnLabel lblwtbk;
		RnLabel lbltitle;
		RnLabel lblusr;
		RnButton btnedtusr;
		RnTextbox txtusr;
		RnButton btnabout;
		RnLabel lblrcd, lblrcds, lblstrt;
		RnButton btnrcd;

		std::list<RnCombo*> lvlsel;
		RnCombo cmbsttl, cmbadvl, cmbprml;

		wchar_t msgplys[255];
		std::map<PlayerController*, std::pair<RnLabel*, RnLabel*>> lblplys;
		RnButton btnaddp, btnstrt;
		RnHourglass glzwait;
		RnLabel lblor;
		RnButton btnwait;

		RnWindow wabout;
		RnButton btnaboutbk;
		RnLabel lblabout, lblsbl, lblctdt, lblabtblw;
		wchar_t msgabtblw[255];

		bool name_editing = false;
		bool join_waiting = false;

		void editName();
		void about();
		void editRecords();
		void addPlayer();
		void refreshRecords();
		void refreshPlayers();
		void startGame();
		void checkJoined();
		void waitJoinToggle();
		void lockConfig(bool);

		RecordManager records;
		RecordView recordView;
		ArenaModel arena;
		ArenaView arenaView;
		AddView adder;

	public:
		GameInstance();
		~GameInstance();

		void start();
	};
}

#endif // MALARGA_GUI_H