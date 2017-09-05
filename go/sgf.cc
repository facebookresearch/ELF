#include "sgf.h"
#include "board.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

static std::string trim(const std::string& str) {
    int l = 0;
    while (l < (int)str.size() && (str[l] == ' ' || str[l] == '\n')) l ++;
    int r = str.size() - 1;
    while (r >= 0 && (str[r] == ' ' || str[r] == '\n')) r --;

    return str.substr(l, r + 1);
}

// [start, end)
typedef pair<int, int> seg;

bool Sgf::Load(const string& filename) {
    // std::cout << "Loading SGF: " << filename << std::endl;
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
        // cout << "Next offset = " << next_offset << " len = " << len << endl;
        // cout << PrintHeader();
        // cout << PrintMainVariation();

        SgfIterator iter = begin();
        if (iter.done()) return false;

        // Compute the length of the move.
        _num_moves = 0;

        while (! iter.done()) {
          // Although two PASS means the ending of a game. In our training,
          // as long as we see one pass, the game is considered done.
          if (iter.GetCurrMove().move == M_PASS) break;
          _num_moves ++;
          ++ iter;
        }
        return true;
    } else {
        std::cout << "Failed to read the header of " << filename << std::endl;
    }
    return false;
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
        // std::cout << "Next c: " << c << endl;
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
    string v = trim(make_str(s, value));
    string k = trim(make_str(s, key));

    // std::cout << "SGF_Header: \"" << k << "\" = \"" <<  v << "\"" << std::endl;
    if (k == "RE") {
        if (! v.empty()) {
            header->winner = (v[0] == 'B' || v[0] == 'b') ? S_BLACK : S_WHITE;
            if (v.size() >= 3) {
                try {
                  header->win_margin = stof(v.substr(2));
                } catch (...) {
                  header->win_reason = v.substr(2);
                }
            }
        }
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
    // std::cout << "[" << range.first << ", " << range.second << ")" << std::endl;
    while (s[i] != ';' && i < 2) {
        // std::cout << "Char[" << i << "]: " << s[i] << std::endl;
        i++;
    }
    if (s[i] != ';') return false;
    i ++;
    // Now we have header.
    *next_offset = get_key_values(s, seg(i, range.second), [&](const char *_s, const seg& key, const seg& value) {
        save_sgf_header(&_header, _s, key, value);
    });
    return true;
}

static void save_sgf_entry(SgfEntry *entry, const char *s, const seg &key, const seg &value) {
    string v = trim(make_str(s, value));
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
        *next_offset =
            get_key_values(s, seg(i, e), [&](const char *_s, const seg& key, const seg& value) {
                save_sgf_entry(entry, _s, key, value);
            });
        entry->sibling.reset(load(s, seg(*next_offset, e), next_offset));
    }
    return entry;
}

string Sgf::PrintHeader() const {
    stringstream ss;

    ss << "Win: " << STR_STONE(_header.winner) << " by " << _header.win_margin;
    if (! _header.win_reason.empty()) ss << " Reason: " << _header.win_reason;
    ss << endl;
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
    SgfIterator iter = begin();
    while (! iter.done()) {
        auto curr = iter.GetCurrMove();
        ss << "[" << iter.GetCurrIdx() << "]: " << STR_STONE(curr.player) << " " << coord2str(curr.move);
        string s = iter.GetCurrComment();
        if (! s.empty()) ss << " Comment: " << s;
        ss << endl;
        ++ iter;
    }
    return ss.str();
}
