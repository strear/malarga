#include <cmath>

#include "rnvntn.h"

using namespace Rnvntn;

void RnHourglass::setWidth(int) {}

void RnHourglass::setHeight(int) {}

int RnHourglass::getWidth() { return 24; }

int RnHourglass::getHeight() { return 24; }

void RnHourglass::setEnabled(bool enabled) {
	RnWidget::setEnabled(enabled);
	if (getParent() == nullptr) return;

	if (enabled)
		getParent()->animateBegin(*this);
	else
		getParent()->animateEnd(*this);
}

void RnHourglass::updateParent(RnWindow* newparent) {
	if (parent != nullptr) parent->animateEnd(*this);

	RnWidget::updateParent(newparent);
	if (parent != nullptr && isEnabled()) {
		parent->animateBegin(*this);
	}
}

void RnHourglass::update(bool, bool) {
	int x = parent->getX(*this), y = parent->getY(*this);

	if (this->compositeBk)
		parent->clarify(*this);
	else {
		SetBk(this->bk);
		Clear(x, y, x + 24, y + 24);
	}

	double tick = parent->tick() / 30.0;
	double angles[] = { 3 * (tick - sin(tick)), 3 * (tick - sin(tick + 2.094395)),
					   3 * (tick - sin(tick + 4.18879)) };

	SetFg(0x00c0ff);
	for (double angle : angles)
		FillCircle((int)(x + 12 + 9 * cos(angle)), (int)(y + 12 + 9 * sin(angle)), 2);
}