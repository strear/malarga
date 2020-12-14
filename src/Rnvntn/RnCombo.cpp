#include "rnvntn.h"

using namespace Rnvntn;

RnCombo::RnCombo(const wchar_t* label, std::list<RnCombo*>* excludes) {
	this->setLabel(label);
	this->setExclude(excludes);
}

const wchar_t* RnCombo::getLabel() { return this->label; }

void RnCombo::setLabel(const wchar_t* label) {
	if (windowVisible()) parent->clarify(*this);

	this->label = label;
	if (label == nullptr) return;

	SetFont(this->font);
	this->setWidth(TextWidth(label) + 24);
	this->setHeight(std::max(24, TextHeight(label)));
}

bool RnCombo::interactable() { return isEnabled() && !selected; }

void RnCombo::clicked() {
	if (excludes != nullptr) {
		for (RnCombo* c : *excludes) {
			if (c != this) c->selected = false;
			c->parent->drawPart(*c);
		}
	}

	this->selected = true;
	parent->drawPart(*this);
}

void RnCombo::setExclude(std::list<RnCombo*>* excludes) {
	if (this->excludes == excludes) return;

	if (this->excludes != nullptr) this->excludes->remove(this);

	this->selected = false;

	this->excludes = excludes;

	if (excludes != nullptr) {
		if (excludes->begin() == excludes->end()) this->selected = true;

		excludes->push_back(this);
	}
}

void RnCombo::update(bool active, bool pressed) {
	Color border = isEnabled() && (active || selected) ? 0xc47244 : 0xaba9a9;
	Color fill = isEnabled() && selected ? 0xc47244 : GetBk();

	int x = parent->getX(*this), y = parent->getY(*this);

	if (this->compositeBk)
		parent->clarify(*this);
	else {
		SetBk(this->bk);
		Clear(x + 24, y, x + getWidth(), y + getHeight());
	}

	SetFg(fill);
	FillCircle(x + 10, y + 10, 5);

	SetLineStyle(3);
	SetLineColor(border);
	Circle(x + 10, y + 10, 8);

	if (label == nullptr) return;

	SetTextColor(isEnabled() ? 0x0 : 0xa5a5a5);
	SetFont(this->font);
	PutText(x + 24, y, label);
}
