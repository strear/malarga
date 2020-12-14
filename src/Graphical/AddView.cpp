#include <thread>

#include "malarga_gui.h"

using namespace Malarga;

AddView::AddView(ArenaModel& arena)
	: wadd(500, 500, MsgViewProvider::Msg("add-player")),
	btnback(MsgViewProvider::Msg("left-arrow"), true),
	lbladd(MsgViewProvider::Msg("add-player"), true),
	lblmsg(MsgViewProvider::Msg("add-prompt")),
	lbloln(MsgViewProvider::Msg("online-prompt")),
	lblnet(),
	btnaddai(MsgViewProvider::Msg("add-ai")),
	lblchkme(MsgViewProvider::Msg("checkmark"), false, 0x47ad70),
	lblme(MsgViewProvider::Msg("me")),
	addedRef(&lblchkme.down) {
	this->arena = &arena;
}

AddView::~AddView() {
	clearAdded();
	clearPend();
}

void AddView::addAI() {
	srand(clock());
	add(new PlayerController(PlayerController::PlayerType::ai, nullptr));
}

void AddView::add(PlayerController* player) {
	arena->addPlayer(*player);

	ItemAdded* pend = new ItemAdded(player, this);
	addedItems.push_back(pend);

	wadd.put(pend->lblchk, *addedRef, 15);
	wadd.put(pend->lblrcd, pend->lblchk.right, 5);
	wadd.put(pend->btnrmv, pend->lblrcd.right, 5);

	addedRef = &pend->lblchk.down;

	disableForFull();
}

AddView::ItemAdded::ItemAdded(PlayerController* player, AddView* view)
	: lblchk(MsgViewProvider::Msg("checkmark"), false, 0x47ad70),
	lblrcd(),
	btnrmv(MsgViewProvider::Msg("minus"), true) {
	this->player = player;
	this->view = view;
	btnrmv.fg = 0x3171ed;
	lblrcd.setLabel(player->getName());

	std::function<void()> hook = std::bind(&AddView::ItemAdded::remove, this);
	btnrmv.setClickHook(&hook);
}

AddView::ItemAdded::~ItemAdded() {
	view->wadd.remove(lblchk);
	view->wadd.remove(lblrcd);
	view->wadd.remove(btnrmv);

	const RnWidget::Signpost* prevRef = &view->lblchkme.down;
	auto it = view->addedItems.begin();
	while (it != view->addedItems.end()) {
		if (*it == this) break;
		prevRef = &(*it)->lblchk.down;
		++it;
	}

	if (it != view->addedItems.end()) ++it;

	while (it != view->addedItems.end()) {
		view->wadd.put((*it)->lblchk, *prevRef, 15);
		view->wadd.put((*it)->lblrcd, (*it)->lblchk.right, 5);
		view->wadd.put((*it)->btnrmv, (*it)->lblrcd.right, 10);
		prevRef = &(*it)->lblchk.down;
		++it;
	}

	if (std::find(view->addedItems.begin(), view->addedItems.end(), this) !=
		view->addedItems.end())
		view->addedItems.remove(this);

	if (view->addedRef == &lblchk.down) {
		if (view->addedItems.empty()) {
			view->addedRef = &view->lblchkme.down;
		} else {
			view->addedRef = &view->addedItems.back()->lblchk.down;
		}
	}
}

void AddView::ItemAdded::remove() {
	view->arena->removePlayer(*player);
	delete player;

	AddView* v = view;
	delete this;
	v->updatePend(false);
}

void AddView::disableForFull() {
	bool allow_to_add = addedItems.size() + 1 < max_players;
	btnaddai.setEnabled(allow_to_add);

	for (ItemPend* it : pendItems) {
		it->btnadd.setEnabled(allow_to_add);
	}
}

AddView::ItemPend::ItemPend(const wchar_t* name, AddView* view)
	: lblrcd(), btnadd(MsgViewProvider::Msg("plus"), true) {
	this->view = view;
	btnadd.fg = 0x47ad70;
	lblrcd.setLabel(name);

	std::function<void()> hook = std::bind(&AddView::ItemPend::add, this);
	btnadd.setClickHook(&hook);
}

AddView::ItemPend::~ItemPend() {
	view->wadd.remove(this->lblrcd);
	view->wadd.remove(this->btnadd);
}

void AddView::ItemPend::add() {
	if (view->pendUpdating) return;

	view->add(new PlayerController(PlayerController::PlayerType::remote,
		this->lblrcd.getLabel()));
	view->updatePend(false);
}

void AddView::updateAdded() {
	clearAdded();

	addedRef = &lblchkme.down;
	auto it = ++arena->playerIterator();

	while (it != arena->playerIteratorEnd()) {
		ItemAdded* item = new ItemAdded(*it, this);

		wadd.put(item->lblchk, *addedRef, 15);
		wadd.put(item->lblrcd, item->lblchk.right, 5);
		wadd.put(item->btnrmv, item->lblrcd.right, 5);

		addedRef = &item->lblchk.down;
		addedItems.push_back(item);

		++it;
	}

	disableForFull();
}

void AddView::updatePend(bool fetchList) {
	if (pendUpdating) return;
	pendUpdating = true;

	int sameCount = 0;
	if (fetchList) {
		wchar_t prevOnlines[max_online_players]
			[PlayerController::max_name_length + 1]{};

		for (int i = 0; i < max_online_players; i++) {
			wcsncpy(prevOnlines[i], onlinePlayers[i],
				PlayerController::max_name_length);
		}

		bool checked = arena->getNet()
			.FindOnlinePlayers(&(*onlinePlayers), max_online_players);

		if (!wadd.contain(lblnet))
			wadd.put(lblnet, btnaddai.down, 15);
		if (lblnet.isEnabled() != checked) {
			lblnet.setEnabled(checked);

			if (checked) {
				lblnet.setLabel(arena->getNet().serverAddress());
			} else {
				lblnet.setLabel(MsgViewProvider::Msg("server-unavaliable"));
				clearPend();

				pendUpdating = false;
				return;
			}
		}

		bool allSame = false;

		for (; sameCount < max_online_players; sameCount++) {
			if (sameCount != 0 && onlinePlayers[sameCount][0] == '\0') {
				allSame = true;
				break;
			}
			if (wcscmp(prevOnlines[sameCount], onlinePlayers[sameCount]) != 0) {
				break;
			}
		}

		if (allSame) {
			pendUpdating = false;
			return;
		}
	}

	std::recursive_mutex& mtx = wadd.getMutex();
	std::lock_guard<std::recursive_mutex> lck(mtx);

	auto it = pendItems.begin();
	ItemPend* is = nullptr;
	if (it != pendItems.end()) {
		if (sameCount > 0) {
			std::advance(is, sameCount - 1);
			is = *it;
			++it;
		}

		ItemPend* i;
		while (it != pendItems.end()) {
			i = *it;
			it = pendItems.erase(it);
			delete i;
		}
	}

	const RnWidget::Signpost* prevRef;
	int margin;

	if (is == nullptr) {
		prevRef = &lblnet.down;
		margin = 20;
	} else {
		prevRef = &is->lblrcd.down;
		margin = 15;
	}

	for (auto* nameptr = &onlinePlayers[sameCount];
		nameptr < onlinePlayers + max_online_players; nameptr++) {

		if ((*nameptr)[0] == '\0') break;
		bool chk_added = false;

		for (ItemAdded* iadded : addedItems) {
			if (wcscmp(iadded->player->getName(), *nameptr) == 0 &&
				iadded->player->getType() == PlayerController::PlayerType::remote) {
				chk_added = true;
				break;
			}
		}
		if (chk_added) continue;

		ItemPend* item = new ItemPend(*nameptr, this);
		wadd.put(item->lblrcd, *prevRef, margin);
		wadd.put(item->btnadd, item->lblrcd.right, 10);
		prevRef = &item->lblrcd.down;
		pendItems.push_back(item);

		margin = 15;
	}

	disableForFull();
	pendUpdating = false;
	EnterBuffer();
}

void AddView::clearAdded() {
	auto it = addedItems.begin();
	ItemAdded* i;

	while (it != addedItems.end()) {
		i = *it;
		it = addedItems.erase(it);
		delete i;
	}
}

void AddView::clearPend() {
	auto it = pendItems.begin();
	ItemPend* i;

	while (it != pendItems.end()) {
		i = *it;
		it = pendItems.erase(it);
		delete i;
	}
}

void AddView::start() {
	if (!windowLoaded) {
		std::function<void()> hook(RnButton::CloseWindow);
		btnback.setClickHook(&hook);

		hook = std::bind(&AddView::addAI, this);
		btnaddai.setClickHook(&hook);

		hook = std::bind(&AddView::timelyRefresh, this);
		wadd.setLoopHook(&hook);

		lblnet.setLabel(arena->getNet().serverAddress());

		lblnet.fg = 0xa5a5a5;

		wadd.put(btnback);
		wadd.put(lbladd, btnback.right);
		wadd.put(lblmsg, btnback.down, 20);
		wadd.put(lblchkme, lblmsg.down, 20);
		wadd.put(lblme, lblchkme.right, 5);
		wadd.put(lbloln, lblmsg.right, 40, lblmsg.getHeight() - lbloln.getHeight());
		wadd.put(btnaddai, lbloln.down, 20);

		wadd.threadSafe = true;
		windowLoaded = true;
	}

	updateAdded();
	pendNextUpdate = wadd.tick();
	wadd.loop();
}

void AddView::timelyRefresh() {
	if (wadd.tick() > pendNextUpdate) {
		std::thread t([this]() { updatePend(true); });
		t.detach();

		pendNextUpdate = wadd.tick() + 200;
	}
}