#pragma once

#include "board.h"
#include <vector>
#define MAX_NUM_FEATURE 25

class BoardFeature {
public:
    enum Rot { NONE = 0, CCW90, CCW180, CCW270 };

    BoardFeature(const Board &b, Rot rot, bool flip) : _board(&b), _rot(rot), _flip(flip) { }
    BoardFeature(const Board &b) : _board(&b), _rot(NONE), _flip(false) { }
    void SetD4Group(Rot new_rot, bool new_flip) { _rot = new_rot; _flip = new_flip; }

    std::pair<int, int> Transform(const std::pair<int, int> &p) const {
        std::pair<int, int> output;

        if (_rot == CCW90) output = std::make_pair(p.second, BOARD_SIZE - p.first - 1);
        else if (_rot == CCW180) output = std::make_pair(BOARD_SIZE - p.first - 1, BOARD_SIZE - p.second - 1);
        else if (_rot == CCW270) output = std::make_pair(BOARD_SIZE - p.second - 1, p.first);
        else output = p;

        if (_flip) std::swap(output.first, output.second);
        return output;
    }

    std::pair<int, int> InvTransform(const std::pair<int, int> &p) const {
        std::pair<int, int> output(p);

        if (_flip) std::swap(output.first, output.second);

        if (_rot == CCW90) output = std::make_pair(BOARD_SIZE - output.second - 1, output.first); 
        else if (_rot == CCW180) output = std::make_pair(BOARD_SIZE - output.first - 1, BOARD_SIZE - output.second - 1);
        else if (_rot == CCW270) output = std::make_pair(output.second, BOARD_SIZE - output.first - 1);
        else output = p;

        return output;
    }

    int64_t Coord2Action(Coord m) const {
        auto p = Transform(std::make_pair(X(m), Y(m)));
        return EXPORT_OFFSET_XY(p.first, p.second);
    }

    Coord Action2Coord(int64_t action) const {
        if (action == -1) return M_PASS;
        auto p = InvTransform(std::make_pair(EXPORT_X(action), EXPORT_Y(action)));
        return OFFSETXY(p.first, p.second);
    }

    void Extract(std::vector<float> *features) const;

private:
    const Board *_board;
    Rot _rot;
    bool _flip;

    int transform(int x, int y) const {
        auto p = Transform(std::make_pair(x, y));
        return EXPORT_OFFSET_XY(p.first, p.second);
    }

    int transform(Coord m) const {
        return transform(X(m), Y(m));
    }

    int transform(Coord m, int c) const {
        return transform(X(m), Y(m)) + c * BOARD_SIZE * BOARD_SIZE;
    }

    // Compute features.
    bool GetLibertyMap3(Stone player, float* data) const;
    bool GetLibertyMap(Stone player, float* data) const;
    bool GetLibertyMap3binary(Stone player, float* data) const;
    bool GetStones(Stone player, float *data) const;
    bool GetSimpleKo(Stone player, float *data) const;
    bool GetHistory(Stone player, float *data) const;
    bool GetHistoryExp(Stone player, float *data) const;
    bool GetDistanceMap(Stone player, float *data) const;
};
