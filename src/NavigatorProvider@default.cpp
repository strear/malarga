#include "malarga.h"

using namespace Malarga;

void NavigatorProvider::evaluate(PlayerController* client, position& ref,
	int& currentToward, MapObject(&map)[16][9],
	std::list<position>& steps,
	std::vector<PlayerController*>* others) {
	bool capable = !steps.empty();

	if (capable) {
		steps.erase(steps.begin());
	}

	position towards[4] = { {ref.x, ref.y - 1},
						{ref.x, ref.y + 1},
						{ref.x - 1, ref.y},
						{ref.x + 1, ref.y} };
	position p = towards[currentToward];

	int towardSafetyRank[4]{};

	static const int fruitsups[3] = { (int)MapObject::chili, (int)MapObject::deadSeg,
									 (int)MapObject::body };

	for (int fruitsup : fruitsups) {
		capable =
			NavigatorProvider::Stepoff(ref, map, fruitsup, towardSafetyRank, steps);

		if (capable && !steps.empty()) {
			p = *(steps.begin());
			currentToward = p.y < ref.y ? 0 : p.y > ref.y ? 1 : p.x < ref.x ? 2 : 3;
		} else if (rand() % 64 != 0) {
			currentToward = towardSafetyRank[0];
		}

		for (int t = -1;;) {
			p = towards[currentToward];

			bool inDanger =
				p.x < 0 || p.y < 0 || p.x >= 16 || p.y >= 9 ||
				(map[p.x][p.y] != MapObject::air && (int)map[p.x][p.y] <= fruitsup);

			if (!inDanger && others != nullptr) {
				for (PlayerController* player : *others) {
					if (player == client || player->isDead()) continue;
					position nextstep = player->next();

					if (nextstep.x == p.x && nextstep.y == p.y) {
						inDanger = true;
						break;
					}
				}
			}

			if (!inDanger) {
				position nextsteps[4] = {
					{p.x, p.y - 1}, {p.x - 1, p.y}, {p.x, p.y + 1}, {p.x + 1, p.y} };
				int barrierCount = 0;

				for (position nextstep : nextsteps) {
					if (nextstep.x == ref.x && nextstep.y == ref.y) continue;

					if (nextstep.x >= 0 && nextstep.y >= 0 && nextstep.x < 16 &&
						nextstep.y < 9) {
						if (map[nextstep.x][nextstep.y] != MapObject::air &&
							(int)map[nextstep.x][nextstep.y] <= fruitsup) {
							barrierCount++;
						}
					}
				}

				inDanger = barrierCount > 2;
			}

			if (inDanger) {
				if (++t > 4) break;
				currentToward = towardSafetyRank[t % 4];
			} else {
				return;
			}
		}
	}
}

bool NavigatorProvider::Stepoff(position& p, MapObject(&map)[16][9], int& fruitsup,
	int(&towardSafetyRank)[4],
	std::list<position>& steps) {
	position nextsteps[4] = {
		{p.x, p.y - 1}, {p.x, p.y + 1}, {p.x - 1, p.y}, {p.x + 1, p.y} };
	int limits[4][4] = { {0, 15, 0, p.y - 1},
						{0, 15, p.y + 1, 8},
						{0, p.x - 1, 0, 8},
						{p.x + 1, 15, 0, 8} };

	bool found = false;
	int dangers[4]{};
	int fruitDistance = 0;

	for (int i = 0; i < 4; i++) {
		int distances[16][9]{};
		int space = 1;

		found = Stepoff(p, nextsteps[i], map, limits[i][0], limits[i][1],
			limits[i][2], limits[i][3], distances, dangers[i], space,
			fruitDistance, fruitsup, 1, steps) ||
			found;

		dangers[i] = dangers[i] / space;
	}

	if (!found) steps.clear();

	for (int k = 0; k < 4; k++) {
		int minDanger = INT_MAX;

		for (int i = 0; i < 4; i++) {
			if (dangers[i] < minDanger) {
				towardSafetyRank[k] = i;
				minDanger = dangers[i];
			}
		}
		dangers[towardSafetyRank[k]] = INT_MAX;
	}

	return found;
}

bool NavigatorProvider::Stepoff(position& origin, position& p, MapObject(&map)[16][9],
	int& xmin, int& xmax, int& ymin, int& ymax,
	int(&distances)[16][9], int& danger,
	int& space, int& fruitDistance, int& fruitsup,
	int stepCount, std::list<position>& steps) {
	if (stepCount > 18) {
		return false;
	} else if (p.x < xmin || p.x > xmax || p.y < ymin || p.y > ymax ||
		map[p.x][p.y] == MapObject::body) {
		danger += 8;
		return false;
	} else if (map[p.x][p.y] != MapObject::air && (int)map[p.x][p.y] <= fruitsup) {
		danger += 4;
		if ((int)map[p.x][p.y] < fruitsup) return false;
	} else {
		space += 2;
	}

	if (distances[p.x][p.y] != 0 && stepCount + danger >= distances[p.x][p.y]) {
		return false;
	}
	distances[p.x][p.y] = stepCount + danger;

	position nextsteps[4] = {
		{p.x, p.y - 1}, {p.x - 1, p.y}, {p.x, p.y + 1}, {p.x + 1, p.y} };
	bool found = false;

	if (map[p.x][p.y] > MapObject::chili &&
		(fruitDistance == 0 || stepCount < fruitDistance)) {
		bool seg = false;

		if (fruitsup == (int)MapObject::chili) {
			bool barriers[4]{};

			for (int k = 0; k < 4; k++) {
				if (nextsteps[k].x == origin.x && nextsteps[k].y == origin.y) continue;

				if (nextsteps[k].x < 0 || nextsteps[k].x >= 16 || nextsteps[k].y < 0 ||
					nextsteps[k].y >= 9 ||
					(map[nextsteps[k].x][nextsteps[k].y] != MapObject::air &&
						(int)map[nextsteps[k].x][nextsteps[k].y] < fruitsup)) {
					barriers[k] = true;
				}
			}

			seg = seg || (barriers[0] && barriers[2]) || (barriers[1] && barriers[3]);
		}

		if (!seg) {
			steps.clear();
			fruitDistance = stepCount;
			found = true;
			space += 2;
		}
	}

	int d[] = { danger, danger, danger, danger };
	for (int i = 0; i < 4; i++) {
		found =
			Stepoff(origin, nextsteps[i], map, xmin, xmax, ymin, ymax, distances,
				d[i], space, fruitDistance, fruitsup, stepCount + 1, steps) ||
			found;
	}
	danger = d[0] + d[1] + d[2] + d[3];

	if (found) {
		steps.push_front(p);
	}
	return found;
}