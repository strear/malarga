#include "malarga_gui.h"

using namespace Malarga;

bool __ToolkitHelper::_toolkitNeedInit = true;

bool __ToolkitHelper::_initToolkit() {
	if (_toolkitNeedInit) {
		RnImplement::InitGraphics();
		_toolkitNeedInit = false;
	}
	return true;
}

GameInstance::GameInstance()
	: w(380, 510, MsgViewProvider::Msg("name")),
	ico(nullptr, 64, 64),
	lblwtbk(),
	lbltitle(MsgViewProvider::Msg("title-hello"), true, 0xc47244),
	lblusr(nullptr, true),
	btnedtusr(MsgViewProvider::Msg("pen"), true),
	txtusr(nullptr, PlayerController::max_name_length),
	btnabout(MsgViewProvider::Msg("questionmark"), true),
	lblrcd(MsgViewProvider::Msg("title-record"), true),
	lblrcds(msgplys),
	lblstrt(MsgViewProvider::Msg("title-start"), true),
	btnrcd(MsgViewProvider::Msg("magnifier"), true),
	cmbsttl(MsgViewProvider::Msg("starter-level"), &lvlsel),
	cmbadvl(MsgViewProvider::Msg("advanced-level"), &lvlsel),
	cmbprml(MsgViewProvider::Msg("premium-level"), &lvlsel),
	btnaddp(MsgViewProvider::Msg("add-player")),
	btnstrt(MsgViewProvider::Msg("start")),
	glzwait(),
	lblor(MsgViewProvider::Msg("or")),
	btnwait(MsgViewProvider::Msg("wait-for-join")),

	wabout(480, 640, MsgViewProvider::Msg("title-about")),
	btnaboutbk(MsgViewProvider::Msg("left-arrow"), true),
	lblabout(MsgViewProvider::Msg("title-about"), true),
	lblsbl(MsgViewProvider::Msg("about-item-symbol"), false, 0xdcaa8f),
	lblctdt(MsgViewProvider::Msg("about")),
	lblabtblw(nullptr, false, 0xdcaa8f),

	records(),
	recordView(records),
	arena(),
	arenaView(arena),
	adder(arena)

{
	Image icoimg;
	LoadIco(icoimg, L"#101");
	ico.setImage(icoimg);

	lblusr.setLabel((**arena.playerIterator()).getName());

	std::function<void()> hook;

	hook = std::bind(&GameInstance::checkJoined, this);
	w.setLoopHook(&hook);

	hook = std::bind(&GameInstance::editName, this);
	btnedtusr.setClickHook(&hook);
	txtusr.setEnterHook(&hook);

	hook = std::bind(&GameInstance::about, this);
	btnabout.setClickHook(&hook);

	hook = std::bind(&GameInstance::editRecords, this);
	btnrcd.setClickHook(&hook);

	hook = std::bind(&GameInstance::addPlayer, this);
	btnaddp.setClickHook(&hook);

	hook = std::bind(&GameInstance::startGame, this);
	btnstrt.setClickHook(&hook);

	hook = std::bind(&GameInstance::waitJoinToggle, this);
	btnwait.setClickHook(&hook);

	w.bk = 0xffffff;
	lblrcds.bk = w.bk;
	for (RnCombo* combo : lvlsel) {
		combo->bk = w.bk;
	}
	glzwait.bk = w.bk;

	lblwtbk.setSize(w.getWidth(), 115);
	w.put(lblwtbk, w.up, 0);

	w.put(ico, lblwtbk.left, 30, 30);
	w.put(lbltitle, ico.right, -10);
	w.put(btnabout, ico.right, lblwtbk.getWidth() - 170);
	w.put(lblusr, lbltitle.right, 5);
	w.put(btnedtusr, lblusr.right, 5);

	w.put(lblrcd, lblwtbk.down, 20, 40);
	w.put(btnrcd, lblrcd.down, 15);
	w.put(lblrcds, btnrcd.right, 15, -2);
	refreshRecords();

	w.put(lblstrt, lblrcd.down, 90);
	w.put(cmbsttl, lblstrt.down, 15);
	w.put(cmbadvl, cmbsttl.right);
	w.put(cmbprml, cmbadvl.right);
	w.put(btnaddp, cmbsttl.down, 45);
	w.put(btnstrt, btnaddp.right);
	refreshPlayers();

	w.put(lblor, btnaddp.down);
	w.put(btnwait, lblor.down);

	hook = RnButton::CloseWindow;
	btnaboutbk.setClickHook(&hook);

	wchar_t compile_date[15], compile_time[10];
	mbstowcs(compile_date, __DATE__, 14);
	mbstowcs(compile_time, __TIME__, 10);
	swprintf(msgabtblw, 254, MsgViewProvider::Msg("about-below"),
		RnBackendInfo(), compile_date, compile_time);
	lblabtblw.setLabel(msgabtblw);

	wabout.put(btnaboutbk);
	wabout.put(lblabout, btnaboutbk.right);
	wabout.put(lblctdt, btnaboutbk.down, 20);
	wabout.put(lblsbl, lblctdt.left, 0, 0);
	wabout.put(lblabtblw, lblctdt.down, 25);
	wabout.setSize(wabout.getX(lblctdt) + lblctdt.getWidth() + 36,
		wabout.getY(lblabtblw) + lblabtblw.getHeight() + 36);
}

GameInstance::~GameInstance() {
	w.close();
	for (auto lblply = lblplys.begin(); lblply != lblplys.end();) {
		delete lblply->second.first;
		delete lblply->second.second;
		lblply = lblplys.erase(lblply);
	}
}

void GameInstance::start() { w.loop(); }

void GameInstance::editName() {
	if (name_editing) {
		wchar_t res[PlayerController::max_name_length + 1];
		txtusr.getText(res);
		RnTextbox::trim(res);
		if (res[0] != '\0') arena.playerOf(0)->setName(res);
		lblusr.setLabel(arena.playerOf(0)->getName());
		btnedtusr.setLabel(MsgViewProvider::Msg("pen"));

		w.remove(txtusr);
		w.put(lblusr, lbltitle.right, 5);
		w.put(btnedtusr, lblusr.right, 5);
	} else {
		txtusr.setText(arena.playerOf(0)->getName());
		btnedtusr.setLabel(MsgViewProvider::Msg("checkmark"));

		w.remove(lblusr);
		w.put(txtusr, lbltitle.right, 8);
		w.put(btnedtusr, txtusr.right, 5);
	}
	name_editing = !name_editing;
}

void GameInstance::about() {
	if (join_waiting) this->waitJoinToggle();
	wabout.loop();
}

void GameInstance::editRecords() {
	if (join_waiting) this->waitJoinToggle();
	recordView.show();
	refreshRecords();
}

void GameInstance::addPlayer() {
	adder.start();
	refreshPlayers();
}

void GameInstance::refreshRecords() {
	lblrcds.setLabel(msgplys + wcslen(msgplys));
	recordView.tell(msgplys, 255);

	bool empty = msgplys[0] == '\0';
	if (empty) {
		wcscpy(msgplys, MsgViewProvider::Msg("record-empty"));
	}

	lblrcds.setLabel(msgplys);
	lblrcds.setEnabled(!empty);
	btnrcd.setEnabled(!empty);
}

void GameInstance::refreshPlayers() {
	const RnWidget::Signpost* referpos = &cmbsttl.down;

	for (auto lblply = lblplys.begin(); lblply != lblplys.end();) {
		w.remove(*lblply->second.first);
		w.remove(*lblply->second.second);
		delete lblply->second.first;
		delete lblply->second.second;
		lblply = lblplys.erase(lblply);
	}

	for (auto plyi = arena.playerIterator(); plyi != arena.playerIteratorEnd();
		++plyi) {
		if (*plyi == nullptr) continue;

		lblplys.emplace(*plyi, std::make_pair(
			new RnLabel(MsgViewProvider::Msg("checkmark"), false, 0x47ad70),
			new RnLabel(plyi == arena.playerIterator()
				? MsgViewProvider::Msg("me")
				: (*plyi)->getName())));

		lblplys[*plyi].first->bk = w.bk;
		w.put(*lblplys[*plyi].first, *referpos, 10);

		lblplys[*plyi].second->bk = w.bk;
		lblplys[*plyi].second->setWidth(lblplys[*plyi].second->getWidth() + 50);

		if (!w.fit(*lblplys[*plyi].second, lblplys[*plyi].first->right, 5)) {
			lblplys[*plyi].second->setWidth(lblplys[*plyi].second->getWidth() - 50);
			lblplys[*plyi].second->setLabel(MsgViewProvider::Msg("ellipsis-dots"));

			w.put(*lblplys[*plyi].second, lblplys[*plyi].first->right, 5);
			break;
		}

		lblplys[*plyi].second->setWidth(lblplys[*plyi].second->getWidth() - 50);
		w.put(*lblplys[*plyi].second, lblplys[*plyi].first->right, 5);

		referpos = &lblplys[*plyi].second->right;
	}
}

void GameInstance::startGame() {
	if (name_editing) editName();

	arenaView.start(cmbadvl.selected ? 1 : cmbprml.selected ? 2 : 0, &records);

	arena.clearPlayers();

	lblusr.setLabel(arena.playerOf(0)->getName());
	w.put(btnedtusr, lblusr.right, 5);
	if (join_waiting) waitJoinToggle();

	refreshRecords();
	refreshPlayers();
}

void GameInstance::checkJoined() {
	if (!join_waiting || !arena.checkAccepted()) return;

	if (arena.tryJoin()) {
		startGame();
	} else {
		lblor.setLabel(MsgViewProvider::Msg("connect-failed"));
		btnwait.setLabel(MsgViewProvider::Msg("cancel"));

		w.remove(glzwait);
		w.put(lblor, btnaddp.down, 20);
		w.put(btnwait, lblor.right);
	}
}

void GameInstance::waitJoinToggle() {
	lockConfig(!join_waiting);

	bool success = arena.setHost(join_waiting);

	if (join_waiting) {
		lblor.setLabel(MsgViewProvider::Msg("or"));
		btnwait.setLabel(MsgViewProvider::Msg("wait-for-join"));

		w.remove(glzwait);
		w.put(lblor, btnaddp.down, 20);
		w.put(btnwait, lblor.down);
	} else {
		if (success) {
			lblor.setLabel(MsgViewProvider::Msg("waiting-receive"));
			btnwait.setLabel(MsgViewProvider::Msg("cancel"));

			w.put(glzwait, btnaddp.down, 20);
			w.put(lblor, glzwait.right, 10);
			w.put(btnwait, lblor.right);
		} else {
			lblor.setLabel(MsgViewProvider::Msg("connect-failed"));
			btnwait.setLabel(MsgViewProvider::Msg("cancel"));

			w.remove(glzwait);
			w.put(lblor, btnaddp.down, 20);
			w.put(btnwait, lblor.right);
		}
	}
	join_waiting = !join_waiting;
}

void GameInstance::lockConfig(bool lock) {
	for (auto lblply = lblplys.begin(); lblply != lblplys.end(); ++lblply) {
		lblply->second.first->setEnabled(!lock);
		lblply->second.second->setEnabled(!lock);
	}

	if (name_editing) this->editName();

	RnWidget* tonotify[]{ &cmbsttl, &cmbadvl, &cmbprml,
						 &btnaddp, &btnstrt, &btnedtusr };
	for (RnWidget* widget : tonotify) widget->setEnabled(!lock);
}