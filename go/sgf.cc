#include "sgf.h"
#include "board.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

// [start, end)
typedef pair<int, int> seg;

void Sgf::Reset() {
    _move_idx = 0;
    _curr = _root.get();
}

bool Sgf::Load(const string& filename) {
    // Load the game.
    ifstream iFile(filename);
    stringstream ss;
    ss << iFile.rdbuf();
    string s = ss.str();

    const char *str = s.c_str();
    int len = s.size();

    _header.Reset();
    int next_offset = 0;
    if (load_header(str, seg(0, len), &next_offset)) {
        _root.reset(load(str, seg(next_offset, len), &next_offset));
        Reset();

        // Compute the length of the move.
        _num_moves = 0;
        do {
          _num_moves ++;
        } while (Next());
        Reset();
        return true;
    }
    return false;
    // cout << "Load complete, next_offset = " << next_offset << endl;
}

#define STATE_KEY 0
#define STATE_VALUE 1

// return next offset.
static int get_key_values(const char *s, const seg& range, std::function<void (const char *, const seg&, const seg&)> cb) {
    int i;
    seg key, value;
    int state = STATE_KEY;
    int start_idx = range.first;
    bool done = false;
    bool backslash = false;

    // cout << "Begin calling get_key_values with [" << range.first << ", " << range.second << ")" << endl;

    for (i = range.first; i < range.second && ! done; ++i) {
        if (s[i] == '\\') { backslash = ! backslash; continue; }
        if (backslash) { backslash = false; continue; }

        char c = s[i];
        switch (state) {
            case STATE_KEY:
                if (c == '[') {
                    // Finish a key and start a value.
                    key = seg(start_idx, i);
                    start_idx = i + 1;
                    state = STATE_VALUE;
                } else if (c == ';' || c == ')') {
                    --i;
                    done = true;
                }
                break;
            case STATE_VALUE:
                if (c == ']') {
                    // Finish a value and start a key
                    value = seg(start_idx, i);
                    // Now we have complete key/value pairs.
                    cb(s, key, value);
                    start_idx = i + 1;
                    state = STATE_KEY;
                }
                break;
        }
    }

    // cout << "End calling get_key_values with [" << range.first << ", " << range.second << "). next_offset = " << i << endl;
    return i;
}

static string make_str(const char *s, const seg &g) {
    return string(s + g.first, g.second - g.first);
}

static void save_sgf_header(SgfHeader *header, const char *s, const seg &key, const seg &value) {
    string v = make_str(s, value);
    string k = make_str(s, key);

    if (k == "RE") {
        header->winner = (v[0] == 'B' || v[0] == 'b') ? S_BLACK : S_WHITE;
        header->win_margin = stof(v.substr(2));
    } else if (k == "SZ") {
        header->size = stoi(v);
    } else if (k == "PW") {
        header->white_name = v;
    } else if (k == "PB") {
        header->black_name = v;
    } else if (k == "WR") {
        header->white_rank = v;
    } else if (k == "BR") {
        header->black_rank = v;
    } else if (k == "C") {
        header->comment = v;
    } else if (k == "KM") {
        header->komi = stof(v);
    } else if (k == "HA") {
        header->handi = stoi(v);
    }
}

bool Sgf::load_header(const char *s, const seg& range, int *next_offset) {
    // Load the header.
    int i = range.first;
    while (s[i] != ';') i++;
    i ++;
    // Now we have header.
    *next_offset = get_key_values(s, seg(i, range.second), [&](const char *_s, const seg& key, const seg& value) { save_sgf_header(&_header, _s, key, value); });
    return true;
}

// Load the remaining part.
static Coord str2coord(const string &s) {
    int x = s[0] - 'a';
    //if (x >= 9) x --;
    int y = s[1] - 'a';
    //if (y >= 9) y --;
    return OFFSETXY(x, y);
}

static string coord2str(Coord c) {
    int x = X(c);
    //if (x >= 8) x ++;
    int y = Y(c);
    //if (y >= 8) y ++;

    string s;
    s.resize(3);
    s[0] = 'a' + x;
    s[1] = 'a' + y;
    s[2] = 0;
    return s;
}

static void save_sgf_entry(SgfEntry *entry, const char *s, const seg &key, const seg &value) {
    string v = make_str(s, value);
    if (key.second - key.first == 1) {
        char c = s[key.first];
        if (c == 'B') {
            entry->player = S_BLACK;
            entry->move = str2coord(v);
        } else if (c == 'W') {
            entry->player = S_WHITE;
            entry->move = str2coord(v);
        } else if (c == 'C') {
            entry->comment = v;
        }
    } else {
        // Default key/value pairs.
        entry->kv.insert(make_pair(make_str(s, key), v));
    }
}

SgfEntry *Sgf::load(const char *s, const seg &range, int *next_offset) {
    // Build the tree recursively.
    *next_offset = 0;
    int i = range.first;
    const int e = range.second;

    while (i < e && s[i] != ';') ++i;
    if (i >= e) return nullptr;
    ++i;

    SgfEntry *entry = new SgfEntry;
    if (s[i] == '(') {
        ++i;
        // Recursion.
        entry->child.reset(load(s, seg(i, e), next_offset));
        if (s[*next_offset] != ')') {
            // Corrupted file.
            return nullptr;
        }
        (*next_offset) ++;
        entry->sibling.reset(load(s, seg(*next_offset, e), next_offset));
    } else {
        *next_offset = get_key_values(s, seg(i, e), [&](const char *_s, const seg& key, const seg& value) { save_sgf_entry(entry, _s, key, value); });
        entry->sibling.reset(load(s, seg(*next_offset, e), next_offset));
    }
    return entry;
}

bool Sgf::Next() {
    // Only move at siblings (main variation).
    const SgfEntry *next = get_next(_curr);
    if (next == nullptr) return false;
    _curr = next;
    _move_idx ++;
    return true;
}

string Sgf::PrintHeader() const {
    stringstream ss;

    ss << "Win: " << STR_STONE(_header.winner) << " by " << _header.win_margin << endl;
    ss << "Komi: " << _header.komi << endl;
    ss << "Handi: " << _header.handi << endl;
    ss << "Size: " << _header.size << endl;
    ss << "White: " << _header.white_name << "[" << _header.white_rank << "]" << endl;
    ss << "Black: " << _header.black_name << "[" << _header.black_rank << "]" << endl;
    ss << "Comment: " << _header.comment << endl;
    return ss.str();
}

string Sgf::PrintMainVariation() {
    stringstream ss;
    Reset();
    do {
        pair<Stone, Coord> curr = GetCurr();
        ss << "[" << _move_idx << "]: " << STR_STONE(curr.first) << " " << coord2str(curr.second);
        string s = GetCurrComment();
        if (! s.empty()) ss << " Comment: " << s;
        ss << endl;
    } while (Next());
    return ss.str();
}
