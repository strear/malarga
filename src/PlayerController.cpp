#include "malarga.h"

using namespace Malarga;

int PlayerController::aiLastId = 0;
const wchar_t PlayerController::ai_names[][max_name_length + 1] = {
	L"詹姆士 %d 号", L"约瑟夫 %d 号", L"亚力山大 %d 号", L"杰克 %d 号",
	L"托马斯 %d 号", L"约翰 %d 号",   L"迈克尔 %d 号",   L"马修 %d 号",
	L"威廉 %d 号",   L"丹尼尔 %d 号" };

PlayerController::PlayerController(PlayerType type, const wchar_t* name)
	: type(type), currentToward(2) {

	if (type == PlayerType::local) {
		EnvProvider::WhoAmI(this->name, max_name_length);
	} else if (type == PlayerType::ai) {
		aiLastId += 1 + rand() % 10;
		swprintf(this->name, max_name_length, ai_names[aiLastId % 10],
			aiLastId / 10 + 1);
		aiFootprint = new std::list<position>;
	} else {
		wcscpy(this->name, name);
	}
}

PlayerController::~PlayerController() {
	if (aiFootprint != nullptr) delete aiFootprint;
}

PlayerController::PlayerType PlayerController::getType() { return type; }

const wchar_t* PlayerController::getName() { return this->name; }

void PlayerController::setName(const wchar_t* name) {
	wcsncpy(this->name, name, max_name_length);
}

void PlayerController::init(MapObject(*map)[16][9], std::vector<position>* segments,
	std::vector<PlayerController*>* others, ArenaSyncProvider* net) {

	this->dead = false;
	this->map = map;
	this->segments = segments;
	this->others = others;
	this->net = net;

	if (type == PlayerType::ai) {
		aiFootprint->clear();
	}
}

int PlayerController::toward() { return currentToward; }

bool PlayerController::valid() {
	if (segments == nullptr) return false;

	if (type == PlayerType::ai && !segments->empty()) {
		if (map == nullptr) return false;

		NavigatorProvider::evaluate(this, *segments->rbegin(),
			currentToward, *map, *aiFootprint, others);
	}

	if (type != PlayerType::remote && net != nullptr) {
		net->tellToward(currentToward, getName());
	}

	if (type == PlayerType::remote) {
		if (net == nullptr) {
			return false;
		} else if (net->readToward(currentToward, name)) {
			remoteLastFail = 0;
		} else if (remoteLastFail == 0) {
			remoteLastFail = clock();
		} else if (clock() - remoteLastFail > CLOCKS_PER_SEC * 3) {
			return false;
		}
	}

	return true;
}

void PlayerController::operate(int direction) {
	if (type == PlayerType::remote) return;
	if (direction < 0 || direction > 3) return;

	this->currentToward = direction;

	if (net != nullptr) {
		net->tellToward(currentToward, getName());
	}
}

position PlayerController::next() {
	position p = *segments->rbegin();
	int t = toward();

	if (t == 0) {
		p.y--;
	} else if (t == 1) {
		p.y++;
	} else if (t == 2) {
		p.x--;
	} else {
		p.x++;
	}

	return p;
}

void PlayerController::kill() { this->dead = true; }

bool PlayerController::isDead() { return this->dead; }