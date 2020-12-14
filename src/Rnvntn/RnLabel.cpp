#include "rnvntn.h"

using namespace Rnvntn;

RnLabel::RnLabel(const wchar_t* label, bool caption, Color color) {
	if (caption) this->font.setHeight(28);

	if (label != nullptr) this->setLabel(label);
	this->fg = color;
}

const wchar_t* RnLabel::getLabel() { return this->label; }

void RnLabel::setLabel(const wchar_t* label) {
	if (windowVisible()) parent->clarify(*this);

	this->label = label;
	if (label == nullptr) return;

	int width = 0, height = 0;

	SetFont(this->font);

	wchar_t* chars = new wchar_t[wcslen(label) + 1];
	wcscpy_s(chars, wcslen(label) + 1, label);
	wchar_t* nextsplit = nullptr;

	wchar_t* p = wcstok_s(chars, L"\n", &nextsplit);
	while (p) {
		if (width < TextWidth(p)) width = TextWidth(p);
		height += TextHeight(p);
		p = wcstok_s(nullptr, L"\n", &nextsplit);
	}

	this->setSize(width, height);

	delete[] chars;
}

void RnLabel::update(bool active, bool pressed) {
	int x = parent->getX(*this), y = parent->getY(*this);

	if (!isEnabled() || !label) {
		if (this->compositeBk)
			parent->clarify(*this);
		else {
			SetBk(this->bk);
			Clear(x, y, x + getWidth(), y + getHeight());
		}
	}

	if (label == nullptr) return;

	SetTextColor(isEnabled() ? fg : 0xa5a5a5);
	SetFont(this->font);

	wchar_t* chars = new wchar_t[wcslen(label) + 1];
	wcscpy_s(chars, wcslen(label) + 1, label);
	wchar_t* nextsplit = nullptr;
	int y0 = y;

	wchar_t* p = wcstok_s(chars, L"\n", &nextsplit);
	while (p) {
		PutText(x, y0, p);
		y0 += TextHeight(p);
		p = wcstok_s(nullptr, L"\n", &nextsplit);
	}

	delete[] chars;
}