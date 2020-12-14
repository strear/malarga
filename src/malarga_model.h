#ifndef MALARGA_MODEL_H
#define MALARGA_MODEL_H

#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "Rnvntn/rncrtdef.h"

namespace Malarga {
	class EnvProvider {
	public:
		static const int path_len_limit = 255;

		static void WhoAmI(wchar_t* dest, size_t capacity);
		static void GetFolderForData(wchar_t* path);
		static void AppendPathSubitem(
			wchar_t* folder, const wchar_t* subitem);
		static bool MbsCastWch(
			const char* source, wchar_t* target, int capacity);
		static bool WchCastMbs(
			const wchar_t* source, char* target, int capacity);
		static void OpenStream(std::ifstream&,
			const wchar_t* path, std::ios::openmode = std::ios_base::in);
		static void OpenStream(std::ofstream&,
			const wchar_t* path, std::ios::openmode = std::ios_base::out);
	};

	struct position { int x, y; };

	enum class MapObject {
		air, body, deadSeg, chili,
		mushroom, cherries, apple, strawberry, watermelon
	};

	class NavigatorProvider;
	class ArenaSyncProvider;

	class PlayerController {
	public:
		static const int max_name_length = 10;
		enum class PlayerType { local, ai, remote };

	private:
		PlayerType type;

		wchar_t name[max_name_length + 1]{};
		bool dead = false;
		MapObject(*map)[16][9] = nullptr;
		std::vector<position>* segments = nullptr;
		std::vector<PlayerController*>* others = nullptr;
		ArenaSyncProvider* net = nullptr;
		int currentToward = 3;

		clock_t remoteLastFail = 0;

		static int aiLastId;
		static const wchar_t ai_names[10][max_name_length + 1];
		std::list<position>* aiFootprint = nullptr;

	public:
		PlayerController(PlayerType type, const wchar_t* name);
		~PlayerController();

		const wchar_t* getName();
		void setName(const wchar_t* name);

		PlayerType getType();

		void init(MapObject(*map)[16][9], std::vector<position>* segments,
			std::vector<PlayerController*>* others, ArenaSyncProvider*);
		bool valid();
		position next();
		void kill();
		bool isDead();

		int toward();
		void operate(int direction);
	};

	class NavigatorProvider {
		static bool Stepoff(
			position& origin, position& p, MapObject(&map)[16][9],
			int& xmin, int& xmax, int& ymin, int& ymax,
			int(&distances)[16][9], int& danger, int& space,
			int& fruitDistance, int& fruitsup, int stepCount,
			std::list<position>& steps);

	public:
		static bool Stepoff(position& p, MapObject(&map)[16][9], int& fruitsup,
			int(&towardSafetyRank)[4], std::list<position>& steps);

		static void evaluate(
			PlayerController* client,
			position& ref, int& currentToward,
			MapObject(&map)[16][9], std::list<position>& steps,
			std::vector<PlayerController*>* others);
	};

	class RecordManager {
	public:
		static const int max_records = 8;
		struct record {
			unsigned int date, time;
			wchar_t name[PlayerController::max_name_length + 1];
			int score;
			int mode;
		};

	private:
		static const wchar_t* const filename;

		wchar_t path[EnvProvider::path_len_limit];

		static bool compare(const record&, const record&);
		std::set<record, bool(*)(const record&, const record&)> records;

	public:
		static const int max_name_bytes = 42;

		RecordManager();
		~RecordManager();

		void load();
		void save();

		const wchar_t* const getPath();

		std::set<record>::const_iterator recordsIterator();
		std::set<record>::const_iterator recordsIteratorEnd();

		void add(const wchar_t* name, int score, int mode, int lastTime);
		void remove(const record&);
		int highestScore(const wchar_t* name, int mode);
	};

	class ArenaSyncProvider {
		static const int timeout;
		static const int msg_length = 512;

		std::map<const wchar_t*, std::basic_string<char>> wcmbdict;
		const char* _wcmb(const wchar_t*);

		void* client = nullptr;
		char guid[64]{};

		wchar_t host[PlayerController::max_name_length + 1]{};
		uint32_t targetId = -1;
		bool meHost = true;
		bool putInBuffer = false;

		struct message {
			int order;
			uint64_t time = 0;
			char name[RecordManager::max_name_bytes]{};
			char sign = '\0';
			int typeData = 0;
			position pos = {};
		};

		std::list<message> recvList, sendList;

		bool tryConnect(int attemptCount = 3);
		void disconnect();
		void _sendWith(char* msg, int repeat, void* c = nullptr);

		static void encode(message&, char(*)[msg_length]);
		bool _pack(char(*)[msg_length], int& order);
		void _unpack(char*, uint64_t time, bool forward);
		static inline bool _comply(const message& m, char sign,
			const char* n_bytes, int typeData, position pos);
		std::list<message>::iterator get(char sign, bool dedup = false,
			const wchar_t* name = nullptr,
			int typeData = -1, position pos = { -1, -1 });
		void put(char sign, const wchar_t* name = nullptr,
			int typeData = -1, position pos = { -1, -1 });
		void send(char*);

	public:
		static const char* address;
		static const int port;

		ArenaSyncProvider();
		~ArenaSyncProvider();

		const wchar_t* const serverAddress();

		bool waitForJoin(const wchar_t* name);
		bool checkAccepted(wchar_t* exactName);
		bool tryJoin();
		void endWaitForJoin();

		bool newGame(const wchar_t* name);
		void tellJoin(const wchar_t* name, const wchar_t* newName);

		bool FindOnlinePlayers(
			wchar_t(*names)[(size_t)PlayerController::max_name_length + 1],
			int capacity);

		void fetchMessages();
		void openBuffer();
		void tellUpdated();
		void tellPlayerName(const wchar_t* name);
		void tellNewFruit(int fruit, int x, int y);
		void tellLength(int len, const wchar_t* name);
		void tellSegment(int seg, int x, int y, const wchar_t* name);
		void tellToward(int toward, const wchar_t* name);
		void tellScore(int score, const wchar_t* name);
		void tellDeath(int reason, const wchar_t* name);
		void tellOver();

		void clearUnread();
		bool readUpdated();
		bool readPlayerName(wchar_t* name);
		bool readNewFruit(int& fruit, int& x, int& y);
		bool readLength(int& len, const wchar_t* name);
		bool readSegment(int seg, int& x, int& y, const wchar_t* name);
		bool readToward(int& toward, const wchar_t* name);
		bool readScore(int& score, const wchar_t* name);
		bool readDeath(int& reason, const wchar_t* name);
		bool readOver();
	};

	class ArenaView;

	class ArenaModel {
		std::vector<PlayerController*> players;
		bool playing = false, host = true, sync = false;
		bool started = false, cleared = true;
		bool accelerated = false;
		int mode = 0;

		std::vector<std::vector<position>> pos;
		std::vector<int> scores;
		std::vector<int> rebornCount;

		ArenaSyncProvider net;
		ArenaView* view = nullptr;
		RecordManager* record = nullptr;

		clock_t gametick = 0, frametick = 0;
		clock_t fruitrefreshtick = 0;
		clock_t pausetick = 0;
		clock_t nextfetchtick = 0;

		MapObject map[16][9]{};

		void checkValid();
		void initPos();
		void initLand();
		void broadcastPlayer(int id);
		void fatalize(int id, int reason, bool kill = false);
		void move(int id);
		void grow(int id);
		void addScore(int id, int add);
		void gameOver();
		void placePlayer(int id);
		void placeFruit();
		void putFruit(int x, int y, MapObject type);
		void replaceChili();
		void removeFruit();
		void judge();
		void fetch();

	public:
		ArenaModel();
		~ArenaModel();

		void addPlayer(PlayerController&);
		std::vector<PlayerController*>::const_iterator
			removePlayer(PlayerController&);
		std::vector<PlayerController*>::const_iterator
			playerIterator();
		std::vector<PlayerController*>::const_iterator
			playerIteratorEnd();

		bool setHost(bool);
		bool checkAccepted();
		bool tryJoin();
		bool notifyJoin();

		void ready(int mode, RecordManager*, ArenaView*);
		void setPaused(bool);
		void accelerate();
		void frameLogic();
		void quitMe();

		ArenaSyncProvider& getNet();
		int getGametick();
		int getFrametick();
		int getMode();
		MapObject at(int x, int y);
		PlayerController* playerOf(int id);
		int playerCount();
		bool contain(PlayerController*);
		int scoreOf(int id);
		std::vector<position>& positionList(int id);
		bool isHost();
		bool isStarted();
		bool isPlaying();
		bool isSyncing();
		bool isPaused();

		void end();
		void clearPlayers();
	};
}

#endif // MALARGA_MODEL_H