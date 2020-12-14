#ifndef MALARGA_H
#define MALARGA_H

#include "malarga_model.h"

namespace Malarga {
	class RecordView {
		RecordManager& manager;

	public:
		RecordView(RecordManager&);

		void show();
		void tell(wchar_t*, int lenlim);
	};

	class ArenaView {
		friend class ArenaModel;
		bool pauseToggled = false;

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
		ArenaModel* arena;

	public:
		AddView(ArenaModel&);
		~AddView();

		void start();
	};

	class GameInstance {
		RecordManager records;
		RecordView recordView;
		ArenaModel arena;
		ArenaView arenaView;
		AddView adder;

	public:
		static void Prepare();

		GameInstance();
		~GameInstance();

		void start();
	};
}

#endif // MALARGA_H