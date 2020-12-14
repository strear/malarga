#include <typeinfo>
#include <cwctype>

#include "rnvntn.h"

using namespace Rnvntn;

RnTextbox::RnTextbox(const wchar_t* text, const int lenlim) {
	this->lenlim = lenlim;
	this->setText(text);
	setSize(120, 24);
}

RnTextbox::~RnTextbox() {
	delete[] text;
	delete[] offsets;
	if (changehook != nullptr) delete changehook;
	if (enterhook != nullptr) delete enterhook;
}

void RnTextbox::trim(wchar_t* str) {
	if (str == nullptr) return;

	wchar_t* p = str + wcslen(str) - 1;
	while (p >= str && iswspace(*p)) {
		*p = '\0';
		p--;
	}

	p = str;
	while (*p != '\0' && iswspace(*p)) p++;

	wcscpy_s(str, wcslen(p) + 1, p);
}

void RnTextbox::getText(wchar_t* t) { wcscpy_s(t, lenlim + 1, text); }

void RnTextbox::setText(const wchar_t* t) {
	if (text == nullptr) text = new wchar_t[lenlim + 1]{};
	if (offsets == nullptr) offsets = new float[lenlim + 1]{};
	if (t == nullptr) return;

	memset(text, 0, sizeof(wchar_t) * (lenlim + 1));
	memset(offsets, 0, sizeof(int) * (lenlim + 1));
	wcscpy_s(text, lenlim + 1, t);

	caret = (int)wcslen(t);

	if (windowVisible()) {
		changed = true;
		parent->drawPart(*this);
	}
}

void RnTextbox::setChangeHook(std::function<void()>* function) {
	if (changehook != nullptr) delete changehook;
	if (function == nullptr)
		changehook = nullptr;
	else
		changehook = new std::function<void()>(*function);
}

void RnTextbox::setEnterHook(std::function<void()>* function) {
	if (enterhook != nullptr) delete enterhook;
	if (function == nullptr)
		enterhook = nullptr;
	else
		enterhook = new std::function<void()>(*function);
}

bool RnTextbox::interactable() { return isEnabled(); }

void RnTextbox::updateParent(RnWindow* parent) {
	changed = true;
	selecting = false;
	select = 0;
	selectLength = 0;

	RnWidget::updateParent(parent);

	if (parent == nullptr) {
		SetCursor(Cursor::arrow, true);
	}

	else if (parent != nullptr && parent->kbdListener == nullptr) {
		parent->kbdListener = this;
		parent->animateBegin(*this);
	}
}

void RnTextbox::update(bool active, bool pressed) {
	SetCursor((active || pressed) ? Cursor::ibeam : Cursor::arrow,
		(!pressed) && selecting);

	if ((active || pressed) && parent->kbdListener != this) {
		if (typeid(*parent->kbdListener) == typeid(RnTextbox)) {
			RnTextbox* oldbox = (RnTextbox*)parent->kbdListener;
			oldbox->paintCaret(
				oldbox->caret - oldbox->choffset,
				oldbox->getParent()->getX(*oldbox) + 6,
				oldbox->getParent()->getY(*oldbox) + 1, oldbox->getWidth() - 12,
				oldbox->getHeight() - 2,
				(int)(oldbox->offsets[oldbox->caret - oldbox->choffset + 1] -
					oldbox->pxoffset),
				0,
				(int)(oldbox->offsets[oldbox->caret - oldbox->choffset + 1] -
					oldbox->offsets[oldbox->caret - oldbox->choffset]),
				false);

			parent->animateEnd(*parent->kbdListener);
		}

		parent->kbdListener = this;
		parent->animateBegin(*this);
		changed = true;
	}

	static bool inited = false, caretOn = false;
	if (changed) {
		moveView(selecting ? select + selectLength : caret);
		refresh();
		if (inited) caretOn = true;
	}
	inited = true;

	if (RnWindow::GetVisibleOne()->tick() % (CLOCKS_PER_SEC / 40) == 0 &&
		selectLength == 0) {
		int x = parent->getX(*this), y = parent->getY(*this);
		paintCaret(
			caret, x + 6, y + 1, getWidth() - 12, getHeight() - 2,
			(int)(offsets[caret - choffset] - pxoffset), 0,
			(int)(offsets[caret - choffset + 1] - offsets[caret - choffset]),
			caretOn || pressed);

		caretOn = !caretOn;
	}
}

void RnTextbox::paintCaret(int caret, int x0, int y0, int w0, int h0, int x,
	int y, int w, bool on) {
	int ltx = std::max(x0, std::min(x0 + w0, x0 + x)),
		lty = std::max(y0, std::min(y0 + h0, y0 + y)),
		rbx = std::max(x0, std::min(x0 + w0 + 3, x0 + x + 3)),
		rby = std::max(y0, std::min(y0 + h0, y0 + y + 8));

	int tlty = std::max(y0, std::min(y0 + h0, y0 + y + 2)),
		tw = std::max(x0, std::min(x0 + w0, x0 + x + w)) - ltx;

	SetFg(0xffffff);
	FillRect(ltx, lty, rbx, rby);

	if (text[caret] != '\0') {
		PutImage(ltx, tlty, canvas, tw, 18, x + pxoffset, y);
	}

	if (on) {
		SetFg(0xdcaa8f);
		FillRect(ltx, lty, rbx, rby);
	}
}

void RnTextbox::refreshCharBg(int x0, int y0, int chw, bool reversed) {
	SetBk(reversed ? 0x97552f : 0xffffff);
	Clear(x0, y0, x0 + chw, y0 + 16);
}

void RnTextbox::refreshChar(wchar_t ch, int x0, int y0, bool reversed) {
	SetFont(this->font);
	SetTextColor(reversed ? 0xffffff : isEnabled() ? 0x0 : 0xa5a5a5);
	PutText(x0, y0, ch);
}

void RnTextbox::refreshCaret(int newCaret) {
	if (newCaret == caret || selectLength != 0) return;

	int x = parent->getX(*this), y = parent->getY(*this);

	paintCaret(
		caret, x + 6, y + 1, getWidth() - 12, getHeight() - 2,
		(int)offsets[caret - choffset] - pxoffset, 0,
		(int)(offsets[caret - choffset + 1] - offsets[caret - choffset]),
		false);

	paintCaret(
		newCaret, x + 6, y + 1, getWidth() - 12, getHeight() - 2,
		(int)offsets[newCaret - choffset] - pxoffset, 0,
		(int)(offsets[newCaret - choffset + 1] - offsets[newCaret - choffset]),
		true);

	moveView(newCaret);
}

void RnTextbox::refresh() {
	int x = parent->getX(*this), y = parent->getY(*this);
	SetFg(0xffffff);
	FillRect(x, y, x + getWidth(), y + getHeight());

	SetLineStyle(2);
	SetLineColor(0xaba9a9);
	Rectangle(x, y, x + getWidth(), y + getHeight());

	float scale = getParent()->getScale();
	SetTarget(&canvas);
	SetBk(0xffffff);
	SetGraphicsScale(scale);
	Clear();

	int pos = 0;
	int selectbeg = std::min(select, select + selectLength);
	int selectend = std::max(select, select + selectLength);
	bool reversed;

	while (text[pos + choffset] != '\0' &&
		offsets[pos] < pxoffset + getWidth() - 12) {
		reversed = selectLength != 0 && pos + choffset >= selectbeg &&
			pos + choffset < selectend;

		refreshCharBg((int)offsets[pos], 0, (int)(offsets[pos + 1] - offsets[pos]),
			reversed);
		refreshChar(text[pos + choffset], (int)offsets[pos], 0, reversed);

		pos++;
	}

	SetTarget();
	PutImage(x + 6, y + 3, canvas, getWidth() - 12, getHeight() - 6, pxoffset, 0);

	changed = false;
}

void RnTextbox::checkOffsets(int end) {
	int pos = choffset;
	float* offset0 = this->offsets, x0 = 0, w = 0;

	float scale = getParent()->getScale();
	int widthexp = (int)(canvas.width() / scale);
	bool needexpand = false;

	while (text[pos] != '\0' &&
		((end != 0 && pos <= end) ||
			(end == 0 && x0 - w < pxoffset + getWidth() - 12))) {
		w = (float)TextWidth(text[pos]);

		if (widthexp < (int)(x0 + w)) {
			needexpand = true;
			widthexp = (int)(x0 + w);
		}

		x0 += w;
		*(++offset0) = x0;
		pos++;
	}

	if (needexpand) {
		canvas.resize((int)(scale * widthexp + 1), (int)(scale * 18 + 1));
		changed = true;
	}
}

void RnTextbox::moveView(int newCaret) {
	checkOffsets(newCaret);

	if (newCaret <= choffset) {
		if (newCaret != 0 && text[newCaret] == '\0') {
			choffset = newCaret - 1;
		} else {
			choffset = newCaret;
		}
		pxoffset = 0;

		changed = true;
	} else if (offsets[newCaret - choffset] + TextWidth(text[newCaret]) >=
		pxoffset + getWidth() - 12) {
		pxoffset = (int)offsets[newCaret - choffset] + TextWidth(text[newCaret]) -
			getWidth() + 12;

		int i = 0;
		while (i < newCaret && offsets[i + 1] <= pxoffset) {
			choffset++;
			i++;
		}
		pxoffset -= (int)offsets[i];

		changed = true;
	}

	checkOffsets();
}

void RnTextbox::mousehit(MouseEvent& m) {
	if (!isEnabled()) return;

	static bool mouseOn = false;
	static clock_t tdn, tup;

	if (!mouseOn && m.type == MouseEventType::leftDown) {
		selecting = false;
		selectLength = 0;
		refresh();
		mouseOn = true;
		tdn = parent->tick();
	}
	if (mouseOn && m.type == MouseEventType::leftUp) {
		if (parent->tick() - tup < 30) {
			select = 0;
			selectLength = (int)wcslen(text);
			refresh();
		}
		mouseOn = false;
		tup = tdn;
	}

	int x0 = m.x - parent->getX(*this) - 7 + pxoffset;

	const wchar_t* ptr = text + choffset;
	float* offset0 = this->offsets;
	int newCaret = caret;

	while (*ptr != '\0') {
		if (x0 < (*offset0 + *(offset0 + 1)) / 2) {
			newCaret = (int)(ptr - text);
			break;
		}
		ptr++, offset0++;
	}

	if (*ptr == '\0') {
		newCaret = (int)(ptr - text);
	}

	if (mouseOn) {
		if (m.x < parent->getX(*this) + 7 && choffset > 0) {
			newCaret--;
		} else if (m.x > parent->getX(*this) + getWidth() - 7 && *ptr != '\0') {
			newCaret++;
		}

		newSelect(newCaret);
	}

	if (caret != newCaret) {
		refreshCaret(newCaret);
		caret = newCaret;
	}
}

void RnTextbox::newSelect(int newCaret) {
	if (!selecting && selectLength == 0) {
		select = caret;
		selecting = true;
	}

	if ((newCaret - select != selectLength) && (newCaret >= 0) &&
		((size_t)newCaret <= wcslen(text))) {
		selectLength = newCaret - select;
		changed = true;
	}
}

void RnTextbox::copy() {
	checkSelection();
	SetClipboardText(text + select, selectLength);
}

void RnTextbox::paste() {
	wchar_t* buffer = new wchar_t[lenlim + 1]{};

	ReadClipboardText(buffer, lenlim + 1);

	wchar_t* pmem = buffer;
	while (*pmem != '\0') {
		if (wcslen(text) == lenlim && selectLength == 0) break;
		this->putChar(*pmem);
		pmem++;
	}

	delete[] buffer;
}

void RnTextbox::checkSelection() {
	if (selectLength == 0) return;

	if (selecting) {
		selecting = false;
		changed = true;

		if (selectLength < 0) {
			select += selectLength;
			selectLength = -selectLength;
		}
	}
}

void RnTextbox::delSelection() {
	checkSelection();

	wcscpy_s(text + select, lenlim + 1 - select - selectLength,
		text + select + selectLength);
	text[lenlim] = '\0';

	caret = select;
	selecting = false;
	selectLength = 0;

	if (changehook != nullptr) (*changehook)();
	changed = true;
}

void RnTextbox::putCaret(int newCaret) {
	if (selecting) {
		selecting = false;
		changed = true;
	}
	if (selectLength != 0) {
		selectLength = 0;
		changed = true;
	}

	newCaret = std::min((int)wcslen(text), std::max(0, newCaret));
	if (caret != newCaret) {
		refreshCaret(newCaret);
		caret = newCaret;
	}
}

void RnTextbox::putChar(wchar_t c) {
	if (iswcntrl(c)) return;

	if (selectLength != 0) delSelection();

	if (wcslen(text) < lenlim) {
		for (wchar_t* ptr = text + lenlim - 1; ptr > text + caret; ptr--) {
			*ptr = *(ptr - 1);
		}
		*(text + caret) = c;

		text[lenlim] = '\0';
		caret++;

		if (changehook != nullptr) (*changehook)();
		changed = true;
	}
}

void RnTextbox::kbhit(KeyEvent& k) {
	if (!isEnabled() || !k.pressed) return;

	if (k.ch == '\0') {
		if (k.code == KeyCode::del) {
			if (selectLength != 0) {
				delSelection();
			} else if (*(text + caret) != '\0') {
				wcscpy_s(text + caret, lenlim + 1 - caret,
					text + caret + 1);
				text[lenlim] = '\0';

				if (changehook != nullptr) (*changehook)();
				changed = true;
			}
		} else if (k.code == KeyCode::left) {
			if (((code)k.controlKeys & (code)KeyScancode::leftCtrl) ||
				((code)k.controlKeys & (code)KeyScancode::rightCtrl)) {
				if ((code)k.controlKeys & (code)KeyScancode::shift) {
					newSelect(0);
				} else {
					putCaret(0);
				}
			} else {
				if ((code)k.controlKeys & (code)KeyScancode::shift) {
					if (selectLength == 0)
						newSelect(caret - 1);
					else
						newSelect(select + selectLength - 1);
				} else {
					if (selectLength == 0)
						putCaret(caret - 1);
					else
						putCaret(std::min(select, select + selectLength));
				}
			}
		} else if (k.code == KeyCode::right) {
			if (((code)k.controlKeys & (code)KeyScancode::leftCtrl) ||
				((code)k.controlKeys & (code)KeyScancode::rightCtrl)) {
				if ((code)k.controlKeys & (code)KeyScancode::shift) {
					newSelect((int)wcslen(text));
				} else {
					putCaret((int)wcslen(text));
				}
			} else {
				if ((code)k.controlKeys & (code)KeyScancode::shift) {
					if (selectLength == 0)
						newSelect(caret + 1);
					else
						newSelect(select + selectLength + 1);
				} else {
					if (selectLength == 0)
						putCaret(caret + 1);
					else
						putCaret(std::max(select, select + selectLength));
				}
			}
		} else if (k.code == KeyCode::home) {
			if ((code)k.controlKeys & (code)KeyScancode::shift) {
				newSelect(0);
			} else {
				putCaret(0);
			}
		} else if (k.code == KeyCode::end) {
			if ((code)k.controlKeys & (code)KeyScancode::shift) {
				newSelect((int)wcslen(text));
			} else {
				putCaret((int)wcslen(text));
			}
		}
	} else if (k.ch == '\b') {
		if (selectLength != 0) {
			delSelection();
		} else if (caret > 0) {
			wcscpy_s(text + caret - 1, lenlim + 1 - caret + 1,
				text + caret);
			text[lenlim] = '\0';

			caret--;
			if (changehook != nullptr) (*changehook)();
			changed = true;
		}
	} else if (k.ch == '\r' && enterhook != nullptr) {
		(*enterhook)();
		k.code = (KeyCode)0;
	} else if (((code)k.controlKeys & (code)KeyScancode::leftCtrl) ||
		((code)k.controlKeys & (code)KeyScancode::rightCtrl)) {
		if (k.ch == 1) {
			select = 0;
			selectLength = (int)wcslen(text);
			changed = true;
		} else if (k.ch == 3) {
			copy();
		} else if (k.ch == 22) {
			paste();
		} else if (k.ch == 24) {
			copy();
			delSelection();
		}
	} else {
		this->putChar(k.ch);
	}
}