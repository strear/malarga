#include <Client.h>

#include "malarga.h"

using namespace Malarga;
using namespace NetworkHelper;

#define impl(client) ((ClientHelper*)client)

// const char* ArenaSyncProvider::address = "192.168.1.103";
const char* ArenaSyncProvider::address = "111.231.112.136";

const int ArenaSyncProvider::port = 10001;
const int ArenaSyncProvider::timeout = 1000;

const wchar_t* const ArenaSyncProvider::serverAddress() {
	static wchar_t wcaddr[50]{};

	if (wcaddr[0] == '\0') {
		char chaddr[50];
		_snprintf_s(chaddr, 49, "%s:%d", ArenaSyncProvider::address,
			ArenaSyncProvider::port);
		mbstowcs(wcaddr, chaddr, 49);
	}

	return wcaddr;
}

namespace {
	void newGUID(char* ch, int len) {
		(void)CoInitialize(nullptr);

		GUID guid;
		if (S_OK == ::CoCreateGuid(&guid)) {
			_snprintf_s(ch, len, (size_t)len - 1,
				"{%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}",
				guid.Data1, guid.Data2, guid.Data3, guid.Data4[0],
				guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4],
				guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		}

		CoUninitialize();
	}
}

ArenaSyncProvider::ArenaSyncProvider() { newGUID(guid, 64); }

ArenaSyncProvider::~ArenaSyncProvider() {
	disconnect();
	if (impl(this->client) != nullptr) delete impl(this->client);
	WSACleanup();
}

bool ArenaSyncProvider::tryConnect(int attemptCount) {
	int i = 0;
	bool connected;

	do {
		if (client != nullptr) disconnect();
		client = new ClientHelper;
		connected = impl(client)->connectToServer(address, port, guid);
	} while (!connected && ++i < attemptCount);

	return connected;
}

void ArenaSyncProvider::disconnect() {
	if (client == nullptr) return;

	if (putInBuffer) tellUpdated();

	recvList.clear();
	if (impl(client)->checkRoomStatus()) {
		impl(client)->clearSend();
		impl(client)->disconnectFromServer();
	}

	delete impl(client);
	client = nullptr;
}

void ArenaSyncProvider::_sendWith(char* msg, int repeat, void* c) {
	if (c == nullptr) c = this->client;
	if (!impl(c)->checkRoomStatus()) return;

	bool sent = false;

	for (int i = 0; !sent || i < repeat; i++) {
		sent = impl(c)->sendMsg(msg) && impl(c)->checkSend();
	}
}

bool ArenaSyncProvider::waitForJoin(const wchar_t* name) {
	disconnect();
	bool success = tryConnect(1);
	if (!success) return false;

	char n_bytes[RecordManager::max_name_bytes + 5] = "WthW/";
	WideCharToMultiByte(CP_UTF8, 0, name, -1, n_bytes + 5,
		RecordManager::max_name_bytes, 0, nullptr);

	uint32_t roomId;
	success = impl(client)->hostNewRoom(n_bytes, roomId);

	if (success) {
		meHost = false;
	} else {
		endWaitForJoin();
	}

	return success;
}

bool ArenaSyncProvider::checkAccepted(wchar_t* exactName) {
	if (meHost) return false;

	msg_package_t m;
	bool success = false;

	wchar_t newHost[PlayerController::max_name_length + 1]{};
	wchar_t newName[PlayerController::max_name_length + 1]{};

	while (impl(client)->recvMsg(m)) {
		for (uint32_t i = 0; i < m.msgNum; ++i) {
			if (newHost[0] == '\0' && m.msgs[i]->msgContent[0] == '/') {
				MultiByteToWideChar(CP_UTF8, 0, m.msgs[i]->msgContent + 1, -1, newHost,
					PlayerController::max_name_length + 1);
			} else if (newName[0] == '\0' && m.msgs[i]->msgContent[0] == ':') {
				MultiByteToWideChar(CP_UTF8, 0, m.msgs[i]->msgContent + 1, -1, newName,
					PlayerController::max_name_length + 1);
			}

			success = newHost[0] != '\0' && newName[0] != '\0';
			if (success) break;
		}

		if (success) {
			wcsncpy(host, newHost, PlayerController::max_name_length);
			wcsncpy_s(exactName, PlayerController::max_name_length + 1, newName,
				PlayerController::max_name_length);
			break;
		}
	}

	return success;
}

bool ArenaSyncProvider::tryJoin() {
	impl(client)->disconnectFromRoom();

	char n_bytes[RecordManager::max_name_bytes + 5] = "WthG/";
	WideCharToMultiByte(CP_UTF8, 0, host, -1, n_bytes + 5,
		RecordManager::max_name_bytes, 0, nullptr);

	bool success;

	room_list_t rlt;
	success = impl(client)->getRoomList(rlt);

	if (success) {
		success = false;

		for (uint32_t i = 0; i < rlt.roomNum; ++i) {
			if (strcmp(n_bytes, rlt.rooms[i].roomName) == 0) {
				targetId = rlt.rooms[i].roomId;

				success = true;
				break;
			}
		}
	}

	if (success) {
		success = impl(client)->connectToRoom(targetId);
	}

	if (!success) {
		meHost = true;
	}

	return success;
}

void ArenaSyncProvider::endWaitForJoin() {
	meHost = true;
	disconnect();
}

bool ArenaSyncProvider::newGame(const wchar_t* name) {
	if (!meHost) return false;

	disconnect();
	if (!tryConnect(3)) return false;

	wcsncpy_s(host, PlayerController::max_name_length + 1, name,
		PlayerController::max_name_length);

	char n_bytes[RecordManager::max_name_bytes + 5] = "WthG/";
	WideCharToMultiByte(CP_UTF8, 0, name, -1, n_bytes + 5,
		RecordManager::max_name_bytes - 1, 0, nullptr);

	return impl(client)->hostNewRoom(n_bytes, targetId);
}

void ArenaSyncProvider::tellJoin(const wchar_t* name, const wchar_t* newName) {
	if (!meHost) return;

	char n_bytes[RecordManager::max_name_bytes + 5] = "WthW/";
	WideCharToMultiByte(CP_UTF8, 0, name, -1, n_bytes + 5,
		RecordManager::max_name_bytes - 1, 0, nullptr);

	bool success;
	int notifyId;

	room_list_t rlt;
	success = impl(client)->getRoomList(rlt);

	if (success) {
		success = false;

		for (uint32_t i = 0; i < rlt.roomNum; ++i) {
			if (strcmp(n_bytes, rlt.rooms[i].roomName) == 0) {
				notifyId = rlt.rooms[i].roomId;

				success = true;
				break;
			}
		}
	}

	if (success) {
		char n_bytes[RecordManager::max_name_bytes + 1] = "/";
		char nw_bytes[RecordManager::max_name_bytes + 1] = ":";

		WideCharToMultiByte(CP_UTF8, 0, host, -1, n_bytes + 1,
			RecordManager::max_name_bytes - 1, 0, nullptr);
		WideCharToMultiByte(CP_UTF8, 0, newName, -1, nw_bytes + 1,
			RecordManager::max_name_bytes - 1, 0, nullptr);

		ClientHelper tmpClient;
		tmpClient.connectToServer(address, port, guid);

		success = tmpClient.connectToRoom(notifyId);
		if (success) {
			_sendWith(n_bytes, 2, &tmpClient);
			_sendWith(nw_bytes, 2, &tmpClient);
			tmpClient.disconnectFromRoom();
		}
	}
}

bool ArenaSyncProvider::FindOnlinePlayers(
	wchar_t(*names)[(size_t)PlayerController::max_name_length + 1],
	int capacity) {

	memset(names, 0,
		capacity * sizeof(wchar_t) *
		((size_t)PlayerController::max_name_length + 1));

	bool success = tryConnect(1);
	if (!success) return false;

	room_list_t rlt;
	success = impl(client)->getRoomList(rlt);
	int chroff = 0;

	if (success) {
		for (uint32_t i = 0; i < rlt.roomNum; ++i) {
			if (strncmp(rlt.rooms[i].roomName, "WthW/", 5) == 0) {
				MultiByteToWideChar(CP_UTF8, 0, rlt.rooms[i].roomName + 5, -1,
					&(*names)[chroff],
					PlayerController::max_name_length + 1);

				chroff += (size_t)PlayerController::max_name_length + 1;
			}
		}
	}

	return success;
}

bool ArenaSyncProvider::_pack(char(*s)[msg_length], int& order) {
	if (sendList.empty()) return false;

	char* ptr = *s, * end = *s + msg_length;

	int wrotelen = 0;
	for (auto it = sendList.begin(); it != sendList.end() && ptr < end;) {
		wrotelen = _snprintf_s(ptr, end - ptr, end - ptr - 1,
			"%d:%c%d,%d,%d/%s\x1e", ++order, it->sign,
			it->typeData, it->pos.x, it->pos.y, it->name);

		if (wrotelen == -1) {
			*ptr = '\0';
			memset(ptr, 0, (int)strlen(ptr));
			--order;
			break;
		} else {
			ptr += wrotelen;
			it = sendList.erase(it);
		}
	}

	return true;
}

void ArenaSyncProvider::_unpack(char* str, uint64_t time, bool forward) {
	char* cptr = nullptr, * context = nullptr;
	cptr = strtok_s(str, "\x1e", &context);

	while (cptr) {
		message m;

		m.order = strtol(cptr, &cptr, 10);
		cptr += 1;
		m.sign = *cptr;
		cptr += 1;
		m.typeData = strtol(cptr, &cptr, 10);
		cptr += 1;
		m.pos.x = strtol(cptr, &cptr, 10);
		cptr += 1;
		m.pos.y = strtol(cptr, &cptr, 10);
		cptr += 1;
		strncpy_s(m.name, cptr, RecordManager::max_name_bytes - 1);

		m.time = time;
		recvList.emplace_back(m);

		if (forward) {
			if (putInBuffer) {
				sendList.emplace_back(m);
			} else {
				char bytes[msg_length]{};
				encode(m, &bytes);
				send(bytes);
			}
		}

		cptr = strtok_s(nullptr, "\x1e", &context);
	}
}

void ArenaSyncProvider::encode(message& m, char(*s)[msg_length]) {
	sprintf_s(*s, "%d:%c%d,%d,%d/%s", m.order, m.sign, m.typeData, m.pos.x,
		m.pos.y, m.name);
}

const char* ArenaSyncProvider::_wcmb(const wchar_t* wch) {
	if (wch == nullptr) return nullptr;
	if (wch[0] == '\0') return "\0";

	auto entry = wcmbdict.find(wch);

	if (entry != wcmbdict.end()) {
		return entry->second.c_str();
	} else {
		char n_bytes[RecordManager::max_name_bytes]{};
		WideCharToMultiByte(CP_UTF8, 0, wch, -1, n_bytes,
			RecordManager::max_name_bytes, 0, nullptr);
		wcmbdict.emplace(wch, n_bytes);

		return wcmbdict[wch].c_str();
	}
}

bool ArenaSyncProvider::_comply(const ArenaSyncProvider::message& m, char sign,
	const char* n_bytes, int typeData, position pos) {
	return m.sign == sign &&
		(n_bytes == nullptr || strcmp(m.name, n_bytes) == 0) &&
		(typeData == -1 || m.typeData == typeData) &&
		((pos.x == -1 && pos.y == -1) ||
			(m.pos.x == pos.x && m.pos.y == pos.y));
}

void ArenaSyncProvider::fetchMessages() {
	msg_package_t m;

	while (impl(client)->recvMsg(m)) {
		for (uint32_t i = 0; i < m.msgNum; i++) {
			_unpack(m.msgs[i]->msgContent, m.msgs[i]->time,
				meHost && strcmp(guid, m.msgs[i]->name) != 0);
		}
	}

	uint64_t freshlimit = getMsTimestamp() - timeout * 2;

	for (auto it = recvList.begin(); it != recvList.end();) {
		if (it->time < freshlimit) {
			it = recvList.erase(it);
		} else {
			++it;
		}
	}
}

std::list<ArenaSyncProvider::message>::iterator ArenaSyncProvider::get(
	char sign, bool dedup, const wchar_t* name, int typeData, position pos) {
	if (readOver()) return recvList.end();

	const char* n_bytes = _wcmb(name);

	if (dedup) {
		auto ir = recvList.end();
		auto selection = ir;

		while (ir != recvList.begin()) {
			--ir;

			if (_comply(*ir, sign, n_bytes, typeData, pos)) {
				selection = ir;
				break;
			}
		}

		if (selection != recvList.end()) {
			for (auto it = recvList.begin(); it != recvList.end();) {
				if (it != selection &&
					_comply(*it, sign, name != nullptr ? n_bytes : nullptr, typeData,
						pos)) {
					it = recvList.erase(it);
				} else {
					++it;
				}
			}
		}

		return selection;
	} else {
		auto ir = recvList.begin();

		while (ir != recvList.end()) {
			if (_comply(*ir, sign, n_bytes, typeData, pos)) {
				return ir;
			}
			++ir;
		}

		return recvList.end();
	}
}

void ArenaSyncProvider::put(char sign, const wchar_t* name, int typeData,
	position pos) {
	if (readOver()) return;

	message msg;

	strcpy_s(msg.name, _wcmb(name));
	msg.sign = sign;
	msg.typeData = typeData;
	msg.pos = pos;

	if (putInBuffer) {
		sendList.emplace_back(msg);
	} else {
		fetchMessages();

		char bytes[msg_length]{};
		encode(msg, &bytes);
		send(bytes);
	}
}

void ArenaSyncProvider::send(char* msg) {
	bool sent = false;

	for (int i = 0; i < 10 && !sent; i++) {
		sent = impl(client)->sendMsg(msg) && impl(client)->checkSend();
	}
}

void ArenaSyncProvider::openBuffer() {
	if (!sendList.empty()) tellUpdated();

	putInBuffer = true;
}

void ArenaSyncProvider::tellUpdated() {
	if (client == nullptr || !impl(client)->checkRoomStatus()) return;
	fetchMessages();

	char bytes[msg_length]{};
	int c = 0;

	while (_pack(&bytes, c)) send(bytes);

	message delim;
	delim.sign = '.';
	delim.order = ++c;
	encode(delim, &bytes);
	send(bytes);

	sendList.clear();
	putInBuffer = false;
	impl(client)->clearSend();
}

void ArenaSyncProvider::tellPlayerName(const wchar_t* name) {
	put('~', name, 2);
}

void ArenaSyncProvider::tellNewFruit(int fruit, int x, int y) {
	put('+', L"\0", fruit, { x, y });
}

void ArenaSyncProvider::tellLength(int len, const wchar_t* name) {
	put('&', name, len);
}

void ArenaSyncProvider::tellSegment(int seg, int x, int y,
	const wchar_t* name) {
	put('$', name, seg, { x, y });
}

void ArenaSyncProvider::tellToward(int toward, const wchar_t* name) {
	put(':', name, toward);
}

void ArenaSyncProvider::tellScore(int score, const wchar_t* name) {
	put('\'', name, score);
}

void ArenaSyncProvider::tellDeath(int reason, const wchar_t* name) {
	put('X', name, reason);
}

void ArenaSyncProvider::tellOver() { disconnect(); }

void ArenaSyncProvider::clearUnread() { recvList.clear(); }

bool ArenaSyncProvider::readUpdated() {
	if (readOver()) return true;

	fetchMessages();
	auto msg = get('.');

	if (msg != recvList.end() && recvList.size() >= (size_t)msg->order) {
		bool complete = true;
		int i = 1;
		for (; complete && i < msg->order; i++) {
			complete = std::find_if(recvList.begin(), recvList.end(),
				[i](const message& m) { return m.order == i; }) !=
				recvList.end();
		}

		if (complete) {
			recvList.erase(msg);
			return true;
		}
	}

	return false;
}

bool ArenaSyncProvider::readPlayerName(wchar_t* name) {
	auto msg = get('~');

	if (msg != recvList.end()) {
		MultiByteToWideChar(CP_UTF8, 0, msg->name, -1, name,
			PlayerController::max_name_length + 1);

		recvList.erase(msg);
		return true;
	}

	return false;
}

bool ArenaSyncProvider::readNewFruit(int& fruit, int& x, int& y) {
	auto msg = get('+', false, nullptr);

	if (msg != recvList.end()) {
		fruit = msg->typeData;
		x = msg->pos.x;
		y = msg->pos.y;

		recvList.erase(msg);
		return true;
	}

	return false;
}

bool ArenaSyncProvider::readLength(int& len, const wchar_t* name) {
	auto msg = get('&', true, name);

	if (msg != recvList.end()) {
		len = msg->typeData;

		recvList.erase(msg);
		return true;
	}

	return false;
}

bool ArenaSyncProvider::readSegment(int seg, int& x, int& y,
	const wchar_t* name) {
	for (int attempt = 0; attempt < 96; attempt++) {
		auto msg = get('$', true, name, seg);

		if (msg != recvList.end()) {
			x = msg->pos.x;
			y = msg->pos.y;

			recvList.erase(msg);
			return true;
		}
	}

	return false;
}

bool ArenaSyncProvider::readToward(int& toward, const wchar_t* name) {
	auto msg = get(':', true, name);

	if (msg != recvList.end()) {
		toward = msg->typeData;

		recvList.erase(msg);
		return true;
	}

	return false;
}

bool ArenaSyncProvider::readScore(int& score, const wchar_t* name) {
	auto msg = get('\'', true, name);

	if (msg != recvList.end()) {
		score = msg->typeData;

		recvList.erase(msg);
		return true;
	}

	return false;
}

bool ArenaSyncProvider::readDeath(int& reason, const wchar_t* name) {
	auto msg = get('X', true, name);

	if (msg != recvList.end()) {
		reason = msg->typeData;

		recvList.erase(msg);
		return true;
	}

	return false;
}

bool ArenaSyncProvider::readOver() {
	if (client == nullptr) return true;

	bool over = !impl(client)->checkRoomStatus();
	if (over) {
		disconnect();
		meHost = true;
	}

	return over;
}