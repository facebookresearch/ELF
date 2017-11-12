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

bool SharedRWBuffer::table_read_recent(int max_num_records) {
    // Read things into a buffer.
    const string sql = "SELECT * FROM " + table_name_ + " ORDER BY TIME DESC LIMIT " + to_string(max_num_records) + ";";
    cb_save_start();
    int ret = exec(sql, read_callback);
    cb_save_end();
    return ret == 0;
}

}  // namespace elf
