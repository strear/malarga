#include <algorithm>
#include <chrono>
#include <thread>

#include "malarga.h"

using namespace Malarga;

ArenaModel::ArenaModel() {
	PlayerController* me =
		new PlayerController(PlayerController::PlayerType::local, nullptr);
	this->players.push_back(me);
}

ArenaModel::~ArenaModel() {
	clearPlayers();
	delete players[0];
}

void ArenaModel::ready(int mode, RecordManager* record, ArenaView* view) {
	if (!cleared) return;

	this->cleared = false;
	this->started = false;
	this->mode = mode;
	this->record = record;
	this->view = view;
	if (view != nullptr) view->reset();
	pos.clear();

	srand(clock());

	if (host) {
		pos.resize(players.size());

		sync = false;
		for (PlayerController* p : players) {
			if (p->getType() == PlayerController::PlayerType::remote) {
				sync = true;
				break;
			}
		}

		sync = notifyJoin();
		if (sync) net.openBuffer();

		for (int i = 0; i < (int)players.size(); i++) {
			if (sync) net.tellPlayerName(players[i]->getName());

			players[i]->init(&map, &pos[i], &players, sync ? &net : nullptr);
			if (players[i]->getType() != PlayerController::PlayerType::remote) {
				players[i]->operate(rand() % 4);
			}

			scores.push_back(0);
			rebornCount.push_back(0);
		}

		initLand();
		initPos();

		if (view != nullptr) view->tellReady();

		if (sync) {
			std::this_thread::sleep_for(std::chrono::milliseconds(400));
			net.tellUpdated();
		}
	} else {
		clearPlayers();
		pos.resize(1);
		scores.resize(1);

		players[0]->init(&map, &pos[0], &players, &net);

		nextfetchtick = clock() + 400;
	}

	gametick = clock();
	frametick = gametick;
}

void ArenaModel::end() {
	if (playing) gameOver();

	if (record != nullptr && scores[0] > 0) {
		record->add(players[0]->getName(), scores[0], host ? mode : 3,
			gametick / CLOCKS_PER_SEC);
		record->save();
	}

	mode = 0;
	pos.clear();
	scores.clear();
	rebornCount.clear();

	playing = false;
	started = false;
	host = true;
	sync = false;
	accelerated = false;
	pausetick = 0;

	memset(map, 0, sizeof(map));

	cleared = true;
}

void ArenaModel::setPaused(bool p) {
	if (!host || sync || !started || p == isPaused()) return;

	if (!p) gametick += clock() - pausetick;
	pausetick = p ? clock() : 0;

	if (view != nullptr) view->pauseToggled = true;
}

void ArenaModel::accelerate() {
	if (!host || sync) return;
	accelerated = true;
}

void ArenaModel::frameLogic() {
	if ((started && !playing) || pausetick != 0) return;

	if (!started && (clock() > gametick + 3 * CLOCKS_PER_SEC)) {
		started = true;
		playing = true;

		gametick = clock();
	}

	if (!host && clock() > nextfetchtick && net.readUpdated()) {
		fetch();
	}

	if (playing) {
		if (host && (clock() > frametick + CLOCKS_PER_SEC / 2 || accelerated)) {
			accelerated = false;
			frametick = clock();

			if (sync) {
				net.openBuffer();
				net.fetchMessages();
			}

			judge();

			if (sync) {
				net.tellUpdated();
			}
		}

		if (playing &&
			std::find_if(players.begin(), players.end(), [](PlayerController* p) {
				return !p->isDead();
				}) == players.end()) {
			gameOver();
		}
	}
}

void ArenaModel::placePlayer(int id) {
	std::vector<position> points;
	int toward = players[id]->toward();
	int x0, y0, x1, y1, x2, y2;
	int attempt = 0;

	while (true) {
		x0 = rand() % 12 + 2, y0 = rand() % 5 + 2;

		if (toward == 0) {
			x1 = x0, y1 = y0 - 1;
			x2 = x1, y2 = y1 - 1;
		} else if (toward == 1) {
			x1 = x0, y1 = y0 + 1;
			x2 = x1, y2 = y1 + 1;
		} else if (toward == 2) {
			x1 = x0 - 1, y1 = y0;
			x2 = x1 - 1, y2 = y0;
		} else {
			x1 = x0 + 1, y1 = y0;
			x2 = x1 + 1, y2 = y0;
		}

		bool conflict = map[x0][y0] != MapObject::air || map[x1][y1] != MapObject::air ||
			(map[x2][y2] != MapObject::air && map[x2][y2] <= MapObject::chili);

		if (conflict && mode == 2 && map[x0][y0] == MapObject::deadSeg) {
			conflict = false;
		}

		for (int n = 0; !conflict && n < id; n++) {
			if (pos[n].empty()) continue;
			position head = players[n]->next();

			if (head.x == x2 && head.y == y2) {
				conflict = true;
				break;
			}
		}

		if (!conflict) {
			pos[id] = { {x0, y0}, {x1, y1} };
			map[x0][y0] = map[x1][y1] = MapObject::body;
			break;
		} else {
			attempt++;

			if (attempt >= 100) {
				gameOver();
				return;
			}
		}
	}

	broadcastPlayer(id);
}

void ArenaModel::putFruit(int x, int y, MapObject type) {
	map[x][y] = type;

	if (sync) {
		net.tellNewFruit((int)type, x, y);
	}
}

void ArenaModel::placeFruit() {
	bool full = true;
	for (int i = 0; i < 16 * 9; i++) {
		if (map[i / 9][i % 9] == MapObject::air) {
			full = false;
			break;
		}
	}
	if (full) {
		gameOver();
		return;
	}

	bool placed = false;
	int x, y;
	while (!placed) {
		x = rand() % 16;
		y = rand() % 9;

		if (map[x][y] == MapObject::air) {
			putFruit(x, y, MapObject(rand() % 6 + (int)MapObject::chili));

			if (view != nullptr) view->lighten(x, y);
			placed = true;
		}
	}

	fruitrefreshtick = clock();
}

void ArenaModel::replaceChili() {
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j] > MapObject::chili) {
				return;
			}
		}
	}

	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j] == MapObject::chili) {
				putFruit(i, j, MapObject(rand() % 5 + (int)MapObject::chili + 1));

				if (view != nullptr) view->lighten(i, j);
				return;
			}
		}
	}
}

void ArenaModel::removeFruit() {
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j] >= MapObject::chili) {
				putFruit(i, j, MapObject::air);
				return;
			}
		}
	}
}

void ArenaModel::initLand() {
	int n = (int)players.size() * (rand() % 5 + 1);
	for (int i = 0; i < n; i++) placeFruit();
	replaceChili();
}

void ArenaModel::initPos() {
	pos.resize(players.size());

	for (int i = 0; i < (int)players.size(); i++) {
		placePlayer(i);
	}
}

void ArenaModel::broadcastPlayer(int id) {
	if (!sync) return;

	net.tellLength((int)pos[id].size(), players[id]->getName());

	for (int i = 0; i < (int)pos[id].size(); i++) {
		net.tellSegment(i, pos[id][i].x, pos[id][i].y, players[id]->getName());
	}
}

void ArenaModel::quitMe() { fatalize(0, 8); }

void ArenaModel::fatalize(int id, int reason, bool kill) {
	kill = kill || mode == 0 || reason == 2 || reason == 8 || reason == 9;

	if (kill) {
		players[id]->kill();

		if (sync || !host) {
			net.tellDeath(reason, players[id]->getName());
		}

		for (position seg : pos[id]) {
			putFruit(seg.x, seg.y, MapObject::air);
		}
		pos[id].clear();

		if (view != nullptr) {
			view->newScore(id);
			view->tellDeath(players[id]->getName(), reason);
		}
	} else {
		for (position seg : pos[id]) {
			putFruit(seg.x, seg.y, MapObject::deadSeg);
		}

		if (mode == 1) {
			placePlayer(id);
		} else if (mode == 2) {
			rebornCount[id]++;
			if (rebornCount[id] >= 5) {
				players[id]->kill();
				pos[id].clear();

				if (sync) {
					net.tellDeath(3, players[id]->getName());
				}

				if (view != nullptr) {
					view->newScore(id);
					view->tellDeath(players[id]->getName(), 3);
				}
			} else {
				placePlayer(id);
			}
		}
	}
}

void ArenaModel::move(int id) {
	position head = players[id]->next();
	if (head.x >= 16 || head.y >= 9 || head.x < 0 || head.y < 0) return;

	putFruit(pos[id].begin()->x, pos[id].begin()->y, MapObject::air);
	putFruit(head.x, head.y, MapObject::body);

	pos[id].erase(pos[id].begin());
	pos[id].emplace_back(head);

	broadcastPlayer(id);
}

void ArenaModel::grow(int id) {
	position head = players[id]->next();
	putFruit(head.x, head.y, MapObject::body);
	pos[id].emplace_back(head);

	broadcastPlayer(id);
}

void ArenaModel::gameOver() {
	playing = false;
	gametick = clock() - gametick;

	for (PlayerController* player : players) {
		player->kill();
	}

	if (sync) {
		net.tellOver();
	}
}

void ArenaModel::addScore(int id, int add) {
	scores[id] += add;
	if (view != nullptr) view->newScore(id);

	if (sync) {
		net.tellScore(scores[id], players[id]->getName());
	}
}

void ArenaModel::judge() {
	checkValid();

	for (int i = 0; i < (int)players.size(); i++) {
		if (players[i]->isDead()) continue;

		position p = players[i]->next();

		if (p.x >= 16 || p.y >= 9 || p.x < 0 || p.y < 0) {
			move(i);
			fatalize(i, 0);
		} else if (map[p.x][p.y] == MapObject::air) {
			move(i);
		} else if (map[p.x][p.y] == MapObject::body) {
			move(i);
			fatalize(i, 1);
		} else if (map[p.x][p.y] == MapObject::deadSeg) {
			if (mode == 1) {
				move(i);
				fatalize(i, 0);
				continue;
			} else {
				grow(i);
			}
		} else if (map[p.x][p.y] == MapObject::chili) {
			addScore(i, -5);
			move(i);
			placeFruit();

			if (scores[i] < 0) fatalize(i, 2);
		} else {
			addScore(i, 2 * ((int)map[p.x][p.y] - (int)MapObject::chili));
			grow(i);

			removeFruit();
			placeFruit();
			if (rand() % 2) placeFruit();
		}
	}

	if (frametick > fruitrefreshtick + CLOCKS_PER_SEC * 10) {
		removeFruit();
		placeFruit();
	}
	replaceChili();
}

void ArenaModel::fetch() {
	frametick = clock();
	nextfetchtick += CLOCKS_PER_SEC / 8;

	if (!started) {
		wchar_t name[PlayerController::max_name_length + 1]{};

		while (net.readPlayerName(name)) {
			if (wcscmp(name, players[0]->getName()) == 0) continue;

			PlayerController* p =
				new PlayerController(PlayerController::PlayerType::remote, name);
			addPlayer(*p);
		}

		pos.resize(players.size());
		scores.clear();
		rebornCount.clear();

		for (int i = 0; i < (int)players.size(); i++) {
			players[i]->init(&map, &pos[i], &players, &net);

			if (host) net.tellPlayerName(players[i]->getName());

			scores.push_back(0);
			rebornCount.push_back(0);
		}

		if (view != nullptr) view->tellReady();

		gametick = clock() - 2 * CLOCKS_PER_SEC / 5;
		nextfetchtick = gametick + CLOCKS_PER_SEC * 3;
	}

	if (net.readOver()) {
		playing = false;

		for (PlayerController* player : players) {
			player->kill();
		}

		return;
	}

	checkValid();

	int f, x, y;
	bool pend = net.readNewFruit(f, x, y);
	while (pend) {
		map[x][y] = (MapObject)f;
		if (f > 2 && view != nullptr) view->lighten(x, y);

		pend = net.readNewFruit(f, x, y);
	}

	const wchar_t* name;

	for (int id = 0; id < (int)players.size(); id++) {
		if (players[id]->isDead()) continue;

		name = players[id]->getName();

		if (net.readScore(scores[id], name)) {
			view->newScore(id);
		}

		int l = (int)pos[id].size();
		net.readLength(l, name);

		std::vector<position> posi;
		posi.resize(l);

		int xi, yi;
		for (int i = 0; i < l; i++) {
			xi = -1, yi = -1;

			if (net.readSegment(i, xi, yi, name)) {
				posi[i].x = xi;
				posi[i].y = yi;
			}

			if (xi == -1 || yi == -1) {
				posi[i].x =
					std::min(15, std::max(0,
						2 * posi[std::max(0, i - 1)].x -
						posi[std::max(0, i - 2)].x));
				posi[i].y =
					std::min(15, std::min(0,
						2 * posi[std::max(0, i - 1)].y -
						posi[std::max(0, i - 2)].y));
			}
		}

		if (posi.size() > 1) {
			pos[id] = posi;
		}
	}

	net.clearUnread();
}

bool ArenaModel::setHost(bool h) {
	if (!cleared) return false;
	if (this->host == h) return false;
	host = h;

	if (host) {
		net.endWaitForJoin();
		return true;
	} else {
		bool success = net.waitForJoin(players[0]->getName());
		if (!success) host = true;
		return success;
	}
}

bool ArenaModel::checkAccepted() {
	if (host) return false;

	wchar_t name[PlayerController::max_name_length + 1];
	bool success = net.checkAccepted(name);

	if (success) players[0]->setName(name);
	return success;
}

bool ArenaModel::tryJoin() {
	if (host) return false;

	bool success = net.tryJoin();

	if (success) {
		players[0]->operate(2);
	} else {
		host = true;
	}

	return success;
}

bool ArenaModel::notifyJoin() {
	if (!sync) return false;

	bool success = net.newGame(players[0]->getName());
	if (success) {
		for (PlayerController* p : players) {
			if (p->getType() == PlayerController::PlayerType::remote) {
				wchar_t newName[PlayerController::max_name_length + 1]{};

				wcsncpy(newName, p->getName(), PlayerController::max_name_length);

				for (PlayerController* pj : players) {
					if (pj == p) break;

					if (wcscmp(pj->getName(), newName) == 0) {
						if (newName[0] > '0' && newName[0] < '9') {
							newName[0] += 1;
							continue;
						}

						wchar_t tmpName[PlayerController::max_name_length + 1]{};
						wcsncpy_s(tmpName, PlayerController::max_name_length + 1, newName,
							PlayerController::max_name_length);
						wcsncpy_s(newName + 1, PlayerController::max_name_length + 1, tmpName,
							PlayerController::max_name_length);
						newName[0] = '1';
					}
				}

				net.tellJoin(p->getName(), newName);
				p->setName(newName);
			}
		}
	}

	return success;
}

void ArenaModel::checkValid() {
	for (int i = 0; i < (int)players.size(); i++) {
		if (players[i]->isDead()) continue;

		int r;
		if (net.readDeath(r, players[i]->getName())) {
			fatalize(i, r, true);
		}

		if (!players[i]->valid() && host && playing) {
			fatalize(i, 9);
		}
	}
}

ArenaSyncProvider& ArenaModel::getNet() { return net; }

int ArenaModel::getGametick() { return gametick; }

int ArenaModel::getFrametick() { return frametick; }

int ArenaModel::getMode() { return mode; }

MapObject ArenaModel::at(int x, int y) { return map[x][y]; }

PlayerController* ArenaModel::playerOf(int id) { return players[id]; }

int ArenaModel::playerCount() { return (int)players.size(); }

bool ArenaModel::contain(PlayerController* p) {
	return std::find(players.begin(), players.end(), p) != players.end();
}

int ArenaModel::scoreOf(int id) { return scores[id]; }

std::vector<position>& ArenaModel::positionList(int id) { return pos[id]; }

bool ArenaModel::isHost() { return host; }

bool ArenaModel::isStarted() { return started; }

bool ArenaModel::isPlaying() { return playing; }

bool ArenaModel::isSyncing() { return sync; }

bool ArenaModel::isPaused() { return pausetick != 0; }

void ArenaModel::addPlayer(PlayerController& player) {
	if (player.getType() == PlayerController::PlayerType::local) return;

	players.push_back(&player);
}

std::vector<PlayerController*>::const_iterator ArenaModel::removePlayer(
	PlayerController& player) {
	if (player.getType() == PlayerController::PlayerType::local ||
		std::find(players.begin(), players.end(), &player) == players.end())
		return ++std::find(players.begin(), players.end(), &player);

	return players.erase(std::find(players.begin(), players.end(), &player));
}

std::vector<PlayerController*>::const_iterator ArenaModel::playerIterator() {
	return players.cbegin();
}

std::vector<PlayerController*>::const_iterator ArenaModel::playerIteratorEnd() {
	return players.cend();
}

void ArenaModel::clearPlayers() {
	auto it = players.begin();
	while (it != players.end()) {
		if (*it == players[0]) {
			++it;
		} else {
			delete* it;
			it = players.erase(it);
		}
	}
}