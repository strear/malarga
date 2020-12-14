#include "rnvntn.h"

#include <set>

#define LOCK() { if (threadSafe) widgetMutex.lock(); }

#define UNLOCK() { if (threadSafe) widgetMutex.unlock(); }

using namespace Rnvntn;

RnWindow* RnWindow::VisibleOne;

RnWindow::RnWindow(int width, int height, const wchar_t* title)
	: threadSafe(false), title(title) {
	this->setSize(width, height);
}

void RnWindow::setLoopHook(std::function<void()>* function) {
	if (loophook != nullptr) delete loophook;
	if (function == nullptr)
		loophook = nullptr;
	else
		loophook = new std::function<void()>(*function);
}

clock_t RnWindow::tick() { return tic; }

void RnWindow::animateBegin(RnWidget& w) {
	if (w.getParent() != this) return;

	LOCK();

	if (std::find(animators.begin(), animators.end(), &w) != animators.end()) {
		UNLOCK();
		return;
	}

	animators.push_back(&w);
	UNLOCK();
}

void RnWindow::animateEnd(RnWidget& w) {
	LOCK();

	animators.remove(&w);

	UNLOCK();
}

void RnWindow::loop() {
	resume();

	clock_t tick = clock();

	MouseEvent m;
	KeyEvent k;
	bool mousehit = false, kbhit = false;

	while (VisibleOne == this) {
		if (loophook != nullptr && *loophook != nullptr) {
			(*loophook)();
			if (VisibleOne != this) break;
		}

		if (react) {
			mousehit = PopMouseEvent(m);
			if (navigate) kbhit = PopKeyEvent(k);

			while (mousehit || kbhit) {
				if (mousehit) {
					this->mousehit(m);
					mousehit = PopMouseEvent(m);
				}
				if (kbhit) {
					this->kbhit(k);
					kbhit = PopKeyEvent(k);
				}
			}
		}
		if (VisibleOne != this) break;

		LOCK();
		EnterBuffer();
		for (RnWidget* w : animators) {
			if (w->getParent() != nullptr) w->getParent()->drawPart(*w);
		}
		ExitBuffer();
		UNLOCK();

		Sleep(CLOCKS_PER_SEC / framerate - (clock() - tick));
		tick = clock();
		this->tic++;
	}
}

void RnWindow::hide() {
	LOCK();

	kbdListener = nullptr;
	animators.clear();

	UNLOCK();
}

void RnWindow::close() {
	hide();
	VisibleOne = prevVisibleOne;

	if (VisibleOne != nullptr) {
		VisibleOne->resume();
	} else {
		CloseGraphics();
	}
}

float RnWindow::getScale() { return scale; }

void RnWindow::resume() {
	if (VisibleOne != this && prevVisibleOne != this) {
		prevVisibleOne = VisibleOne;
		if (prevVisibleOne != nullptr) prevVisibleOne->hide();
		VisibleOne = this;
	}

	SetTarget();

	scale = GraphicsScale();

	int width = (int)(this->getWidth() * scale);
	int height = (int)(this->getHeight() * scale);

	if (GraphicsWidth() == 0 || GraphicsHeight() == 0) {
		OpenGraphics(width, height);
		SetGraphicsTitle(title);
	} else {
		SetGraphicsTitle(title);
		ResizeGraphics(width, height);
	}

	Reset();
	SetGraphicsScale(scale);
	SetBk(this->bk);
	Clear();
	SetCursor(Cursor::arrow);

	LOCK();
	for (RnWidget* it : widgetOrder) {
		it->updateParent(it->getParent());
	}
	UNLOCK();

	this->update(0, 0);
}

RnWindow* RnWindow::GetVisibleOne() { return VisibleOne; }

RnWindow::~RnWindow() {
	if (loophook != nullptr) delete loophook;
}

bool RnWindow::contain(RnWidget& w) {
	LOCK();

	bool b = std::find(widgetOrder.rbegin(), widgetOrder.rend(), &w) !=
		widgetOrder.rend();

	UNLOCK();

	return b;
}

void RnWindow::findpos(
	int& x, int& y, RnWidget& w, RnWidget::Signpost& place,
	int margin, int shift) {
	if (typeid(w) == typeid(*this)) return;

	x = 35, y = 20;
	int refwidth = 0, refheight = 0;
	bool shiftDefined = shift != INT_MIN;

	if (place.ref != nullptr && place.ref->getParent() != this &&
		place.ref != this) {
		place.ref = nullptr;
	}

	LOCK();

	if (place.ref == nullptr && widgetOrder.rbegin() != widgetOrder.rend()) {
		place.ref = *widgetOrder.rbegin();
	}

	UNLOCK();

	if (place.ref != nullptr) {
		if (place.ref->getParent() != nullptr) {
			x = getX(*(place.ref));
			y = getY(*(place.ref));
		} else {
			x = 0, y = 0;
		}

		refwidth = place.ref->getWidth();
		refheight = place.ref->getHeight();
	}

	switch (place.direction) {
	case 1:
		x += refwidth;

	case 0:
		x += margin;

		if (shiftDefined)
			y += shift;
		else
			y += (refheight - w.getHeight()) / 2;
		break;

	case 3:
		y += refheight;

	case 2:
		y += margin;

		if (shiftDefined) x += shift;
		break;

	default:
		return;
	}

	x = std::max(0, x), y = std::max(0, y);
}

void RnWindow::put(RnWidget& w, RnWidget::Signpost place, int margin,
	int shift) {
	if (typeid(w) == typeid(*this)) return;

	int x, y;
	findpos(x, y, w, place, margin, shift);

	if (this->getWidth() < x + w.getWidth()) this->setWidth(x + w.getWidth());
	if (this->getHeight() < y + w.getHeight()) this->setHeight(y + w.getHeight());

	put(w, x, y);
}

void RnWindow::put(RnWidget& w, int x, int y) {
	if (typeid(w) == typeid(*this)) return;

	if (!contain(w)) {
		LOCK();
		widgetOrder.push_back(&w);
		UNLOCK();
	}

	clarify(w);
	widgetPos[&w] = { x, y };

	w.updateParent(this);
	freshPaint(x, y, w.getWidth(), w.getHeight());
}

bool RnWindow::fit(RnWidget& w, RnWidget::Signpost place, int margin,
	int shift) {
	if (typeid(w) == typeid(*this)) return false;

	int x, y;
	findpos(x, y, w, place, margin, shift);

	return (this->getWidth() >= x + w.getWidth() &&
		this->getHeight() >= y + w.getHeight());
}

void RnWindow::drawPart(RnWidget& w) {
	if (!windowVisible() || !contain(w)) return;

	w.update(activeOne == &w, pressedOne == &w);
}

int RnWindow::getX(RnWidget& w) {
	return widgetPos[&w].x;
}

int RnWindow::getY(RnWidget& w) {
	return widgetPos[&w].y;
}

void RnWindow::clarify(RnWidget& w) {
	if (!contain(w)) return;

	std::list<RnWidget*> ignore = { &w };
	freshPaint(widgetPos[&w].x - 5, widgetPos[&w].y - 5,
		w.getWidth() + 10, w.getHeight() + 10, &ignore);
}

void RnWindow::freshPaint(int x, int y, int w, int h,
	std::list<RnWidget*>* ignores) {
	if (!windowVisible()) return;

	struct box {
		int x, y, w, h;
		bool operator<(const box& b) const {
			return (x < b.x) || (x == b.x && (y < b.y || (y == b.y &&
				(w < b.w || (w == b.w && h < b.h)))));
		}
	};
	std::set<box> refreshAreas;
	refreshAreas.emplace(box{ x, y, w, h });

	int xi, yi, wi, hi;

	LOCK();

	auto bi = refreshAreas.begin();
	while (bi != refreshAreas.end()) {
		bool modified = false;

		for (auto& it : widgetPos) {
			if (ignores != nullptr && std::find(ignores->begin(),
				ignores->end(), it.first) != ignores->end())
				continue;

			xi = it.second.x, yi = it.second.y;
			wi = it.first->getWidth(), hi = it.first->getHeight();

			if (xi < bi->x && yi < bi->y &&
				xi + wi > bi->x + bi->w && yi + hi > bi->y + bi->h) {
				refreshAreas.emplace(box{ xi, yi, wi, hi });
				bi = refreshAreas.erase(bi);
				modified = true;
				break;
			}

			if (xi >= bi->x && yi >= bi->y &&
				xi + wi <= bi->x + bi->w && yi + hi <= bi->y + bi->h) {
				continue;
			}

			if (bi->x + bi->w > xi && xi + wi > bi->x &&
				bi->y + bi->h > yi && yi + hi > bi->y &&
				(bi->x < xi || bi->y < yi ||
					bi->x + bi->w > xi + wi || bi->y + bi->h > yi + hi)) {
				refreshAreas.emplace(box{ xi, yi, wi, hi });
			}
		}

		if (!modified) ++bi;
	};

	SetBk(bk);
	for (const box& bi : refreshAreas) {
		Clear(bi.x, bi.y, bi.x + bi.w, bi.y + bi.h);
	}

	for (const box& bi : refreshAreas) {
		for (auto& it : widgetOrder) {
			if (ignores != nullptr &&
				std::find(ignores->begin(), ignores->end(), it) != ignores->end())
				continue;

			xi = widgetPos[it].x, yi = widgetPos[it].y;
			wi = it->getWidth(), hi = it->getHeight();

			if (xi >= bi.x && yi >= bi.y &&
				xi + wi <= bi.x + bi.w && yi + hi <= bi.y + bi.h) {
				it->updateParent(this);
				drawPart(*it);
			}
		}
	}

	UNLOCK();
}

void RnWindow::remove(RnWidget& w) {
	if (!contain(w)) return;
	clarify(w);

	LOCK();

	if (activeOne == &w) activeOne = nullptr;
	if (pressedOne == &w) pressedOne = nullptr;
	if (kbdListener == &w) kbdListener = nullptr;
	animators.remove(&w);

	widgetPos.erase(&w);
	widgetOrder.remove(&w);

	UNLOCK();

	w.updateParent(nullptr);
}

void RnWindow::mousehit(MouseEvent& m) {
	if (pressedOne != nullptr) {
		pressedOne->mousehit(m);
		if (m.type == MouseEventType::move) return;
	}

	RnWidget* prevActiveOne = activeOne, * prevPressedOne = pressedOne;
	activeOne = nullptr, pressedOne = nullptr;

	LOCK();

	for (auto it : widgetPos) {
		if (!it.first->interactable()) continue;

		int x = it.second.x, y = it.second.y;
		int w = it.first->getWidth(), h = it.first->getHeight();

		if (m.x > x && m.y > y && m.x < x + w && m.y < y + h) {
			if (navigate && it.first->needFocus()) {
				if (m.type == MouseEventType::leftDown) {
					pressedOne = &(*it.first);
				} else {
					activeOne = &(*it.first);
				}
			}

			it.first->mousehit(m);
			break;
		}
	}

	UNLOCK();

	checkfocus(prevActiveOne, prevPressedOne);
}

void RnWindow::kbhit(KeyEvent& k) {
	LOCK();
	if (kbdListener != nullptr) kbdListener->kbhit(k);
	UNLOCK();

	if (!navigate || !k.pressed) return;

	RnWidget* prevActiveOne = nullptr, * prevPressedOne = nullptr;

	if (k.code == KeyCode::enter) {
		prevPressedOne = activeOne;
		pressedOne = nullptr;

	} else if ((k.code == KeyCode::tab &&
		!((code)k.controlKeys & (code)KeyScancode::shift)) ||
		(k.code == KeyCode::down)) {

		RnWidget* newActiveOne = nullptr;
		bool found = activeOne == nullptr;

		LOCK();

		for (RnWidget* it : widgetOrder) {
			if (!found) {
				if (it == activeOne) found = true;
				continue;
			}
			if (!it->needFocus()) continue;

			newActiveOne = it;
			break;
		}

		UNLOCK();

		prevActiveOne = activeOne;
		activeOne = newActiveOne;
		pressedOne = nullptr;

	} else if ((k.code == KeyCode::tab &&
		!((code)k.controlKeys & (code)KeyScancode::shift)) ||
		(k.code == KeyCode::up)) {

		RnWidget* newActiveOne = nullptr;
		bool found = activeOne == nullptr;

		LOCK();

		for (auto it = widgetOrder.rbegin(); it != widgetOrder.rend(); ++it) {
			if (!found) {
				if (*it == activeOne) found = true;
				continue;
			}
			if (!(*it)->needFocus()) continue;

			newActiveOne = *it;
			break;
		}

		UNLOCK();

		prevActiveOne = activeOne;
		activeOne = newActiveOne;
		pressedOne = nullptr;
	}

	checkfocus(prevActiveOne, prevPressedOne);
}

void RnWindow::checkfocus(RnWidget* prevActiveOne, RnWidget* prevPressedOne) {
	LOCK();
	EnterBuffer();

	if (prevPressedOne != pressedOne) {
		if (prevPressedOne != nullptr) {
			drawPart(*prevPressedOne);

			if (prevPressedOne == activeOne) {
				prevPressedOne->clicked();
			}
		}
		if (pressedOne != nullptr) drawPart(*pressedOne);
	} else if (prevActiveOne != activeOne) {
		if (prevActiveOne != nullptr) drawPart(*prevActiveOne);
		if (activeOne != nullptr) drawPart(*activeOne);
	}

	ExitBuffer();
	UNLOCK();
}

void RnWindow::update(bool active, bool pressed) {
	LOCK();
	EnterBuffer();

	for (RnWidget* it : widgetOrder) drawPart(*it);

	ExitBuffer();
	UNLOCK();
}

std::recursive_mutex& RnWindow::getMutex() {
	return widgetMutex;
}

std::list<RnWidget*>::const_iterator RnWindow::widgetsBegin() {
	return widgetOrder.cbegin();
}

std::list<RnWidget*>::const_iterator RnWindow::widgetsEnd() {
	return widgetOrder.cend();
}

std::list<RnWidget*>::const_iterator RnWindow::animatorsBegin() {
	return animators.cbegin();
}

std::list<RnWidget*>::const_iterator RnWindow::animatorsEnd() {
	return animators.cend();
}

bool RnWindow::isActive(RnWidget* w) { return w == activeOne; }

bool RnWindow::isPressed(RnWidget* w) { return w == pressedOne; }

int RnWindow::getFrameRate() { return framerate; }

void RnWindow::setFrameRate(unsigned int r) {
	if (r <= 10) return;

	int sup = GraphicsFramerate();
	framerate = sup != -1 ? std::min((unsigned)sup, r) : r;
}

bool RnWindow::windowVisible() { return this == RnWindow::VisibleOne; }