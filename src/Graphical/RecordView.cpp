#include "malarga_gui.h"
#include <cwctype>

using namespace Malarga;

RecordView::RecordView(RecordManager& m)
	: winqry(420, 540, MsgViewProvider::Msg("title-history")), 
	btnback(MsgViewProvider::Msg("left-arrow"), true),
	lblhsty(MsgViewProvider::Msg("title-history"), true),
	lblpath(m.getPath()),
	lblfind(MsgViewProvider::Msg("find-for")),
	txtfind(nullptr, PlayerController::max_name_length),
	manager(m) {}

void RecordView::tell(wchar_t* w, int lenlim) {
	if (lenlim > 0) *w = '\0';

	wchar_t name[3][PlayerController::max_name_length + 1] = {};
	int mode[3] = {};
	char score[3][45] = {};
	int count = 0;
	bool full = false, found;

	for (auto it = manager.recordsIterator(); it != manager.recordsIteratorEnd();
		++it) {
		found = false;
		for (int i = 0; i < 3; i++) {
			if (strlen(score[i]) > 25) break;

			if (wcscmp(name[i], it->name) == 0 && it->mode == mode[i]) {
				strcat(score[i], ", ");
				snprintf(score[i] + strlen(score[i]),
					45 - strlen(score[i]), "%d", it->score);

				found = true;
				break;
			}
		}
		if (!found) {
			if (count < 3) {
				wcscat_s(name[count], PlayerController::max_name_length + 1, it->name);

				mode[count] = it->mode;
				snprintf(score[count], 45, "%d", it->score);

				count++;
			} else {
				full = true;
			}
		}
	}

	for (int i = 0; i < count; i++) {
		wcscat_s(w, lenlim, L"> ");
		wcscat_s(w, lenlim, name[i]);
		wcscat_s(w, lenlim, L": ");

		mbstowcs_s(nullptr, w + wcslen(w), lenlim - wcslen(w), score[i], 50);

		wcscat_s(w, lenlim, L" (");
		wcscat_s(w, lenlim,
			mode[i] == 3 ? MsgViewProvider::Msg("online")
			: mode[i] == 0 ? MsgViewProvider::Msg("starter-level")
			: mode[i] == 1 ? MsgViewProvider::Msg("advanced-level")
			: MsgViewProvider::Msg("premium-level"));
		wcscat_s(w, lenlim, L")\n");
	}

	w[wcslen(w) - 1] = '\0';
	if (full) {
		wcscat_s(w, lenlim, L" ");
		wcscat_s(w, lenlim, MsgViewProvider::Msg("ellipsis-dots"));
	}
}

void RecordView::show() {
	if (!windowLoaded) {
		std::function<void()> hook = RnButton::CloseWindow;
		btnback.setClickHook(&hook);
		hook = std::bind(&RecordView::updateListView, this);
		txtfind.setChangeHook(&hook);

		lblpath.fg = 0xa5a5a5;

		winqry.put(btnback);
		winqry.put(lblhsty, btnback.right);
		winqry.put(lblpath, btnback.down);
		winqry.put(lblfind, lblpath.down, 20);
		winqry.put(txtfind, lblfind.right, 10);
		windowLoaded = true;
	}

	manager.load();
	updateListView();
	winqry.loop();
	clearListView();
	manager.save();
}

void RecordView::updateListView() {
	clearListView();

	wchar_t query[PlayerController::max_name_length + 1],
		compare[PlayerController::max_name_length + 1];
	txtfind.getText(query);
	RnTextbox::trim(query);
	for (wchar_t& c : query) c = tolower(c);

	const RnWidget::Signpost* ref = &lblfind.down;
	int margin = 25, count = 0;

	for (auto it = manager.recordsIterator(); it != manager.recordsIteratorEnd();
		++it) {
		if (count > RecordManager::max_records) break;
		int i = 0;
		while (it->name[i] != '\0') {
			compare[i] = towlower(it->name[i]);
			i++;
		}
		compare[i] = '\0';

		if (wcsstr(compare, query) == nullptr) continue;

		ViewItem* item = new ViewItem(&(*it), this);

		winqry.put(item->lblrcd, *ref, margin);
		winqry.put(item->btnrmv, item->lblrcd.right, 10);
		listView.push_back(item);

		count++;
		ref = &item->lblrcd.down;
		margin = 15;
	}
}

void RecordView::clearListView() {
	auto it = listView.begin();
	while (it != listView.end()) {
		delete* it;
		it = listView.erase(it);
	}
}

RecordView::ViewItem::ViewItem(const RecordManager::record* r, RecordView* v)
	: view(v), record(r), lblrcd(),
	btnrmv(MsgViewProvider::Msg("trashbin"), true),
	btnccl(MsgViewProvider::Msg("cancel")) {
	btnrmv.fg = 0x3171ed;

	swprintf(label, sizeof(label) / sizeof(*label),
		MsgViewProvider::Msg("record-format"), record->date / 10000,
		record->date % 10000 / 100, record->date % 100, record->name, record->score,
		record->mode == 3 ? MsgViewProvider::Msg("online")
		: record->mode == 0 ? MsgViewProvider::Msg("starter-level")
		: record->mode == 1 ? MsgViewProvider::Msg("advanced-level")
		: MsgViewProvider::Msg("premium-level"),
		record->time / 60, record->time % 60);
	lblrcd.setLabel(label);

	std::function<void()> hook = std::bind(&RecordView::ViewItem::remove, this);
	btnrmv.setClickHook(&hook);
	hook = std::bind(&RecordView::ViewItem::cancelRemove, this);
	btnccl.setClickHook(&hook);
}

RecordView::ViewItem::~ViewItem() {
	view->winqry.remove(lblrcd);
	view->winqry.remove(btnrmv);
	view->winqry.remove(btnccl);
}

void RecordView::ViewItem::remove() {
	std::function<void()> hook =
		std::bind(&RecordView::ViewItem::confirmRemove, this);
	btnrmv.setClickHook(&hook);
	btnrmv.setLabel(MsgViewProvider::Msg("delete"));
	view->winqry.put(btnccl, btnrmv.right, 10);
}

void RecordView::ViewItem::confirmRemove() {
	view->manager.remove(*record);
	view->updateListView();
}

void RecordView::ViewItem::cancelRemove() {
	std::function<void()> hook(std::bind(&RecordView::ViewItem::remove, this));
	btnrmv.setClickHook(&hook);
	btnrmv.setLabel(MsgViewProvider::Msg("trashbin"));
	view->winqry.remove(btnccl);
}