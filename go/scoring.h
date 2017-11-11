//
// Copyright (c) 2016-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant 
// of patent rights can be found in the PATENTS file in the same directory.
// 
#pragma once

#include "board.h"

class OwnerMap {
public:
    OwnerMap();

	// Accumulating Ownermap
	void Clear();
	void Accumulate(const Board *board);

	void GetDeadStones(const Board *board, float ratio, Stone *livedead, Stone *group_stats) const;

	// Get Trompy-Taylor score directly.
	// If livedead != NULL, then livedead is a BOARD_SIZE * BOARD_SIZE array. Otherwise this output is ignored.
	// If territory != NULL, then it is also a BOARD_SIZE * BOARD_SIZE array. Otherwise this output is ignored.
	float GetTTScore(const Board *board, Stone *livedead, Stone *territory) const;
	void ShowStonesProb(Stone player) const;

	// Visulize DeadStones
	static void ShowDeadStones(const Board *board, const Stone *stones);

private:
	// Ownermap
	int total_ownermap_count_;

	// Histogram. S_EMPTY (S_UNKNOWN), S_BLACK, S_WHITE, S_OFF_BOARD (S_DAME)
	int ownermap_[BOARD_SIZE][BOARD_SIZE][4];

	void get(float ratio, Stone *ownermap) const;
	void get_float(Stone player, float *ownermap) const;
};
