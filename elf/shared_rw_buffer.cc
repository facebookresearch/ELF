#include "shared_rw_buffer.h"

namespace elf {

using namespace std;

int read_callback(void *handle, int num_columns, char **column_texts, char **column_names) {
    SharedRWBuffer *h = reinterpret_cast<SharedRWBuffer *>(handle);
    SharedRWBuffer::Record r;

    r.timestamp = stoi(column_texts[0]);
    r.game_id = stoi(column_texts[1]);
    r.machine = column_texts[2];
    r.seq = stoi(column_texts[3]);
    r.pri = stof(column_texts[4]);
    r.reward = stof(column_texts[5]);
    r.content = column_texts[6];

    h->cb_save(r);
    return 0;
}

}  // namespace elf
