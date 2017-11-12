#include <sqlite3.h>
#include <time.h>
#include <iostream>
#include <functional>
#include <chrono>

namespace elf {

using namespace std;

using SqlCB = function<int (void*,int,char**,char**)>;

class SharedRWBuffer {
public:
    struct Record {
        uint64_t timestamp = 0;
        uint64_t game_id = 0;
        int seq = 0;
        float pri = 0.0;
        float reward = 0.0;
        string machine;
        string content;
    };

    SharedRWBuffer(const string &filename, const string& table_name)
      : table_name_(table_name), rng_(time(NULL)), recent_loaded_(2) {
        int rc = sqlite3_open(filename, &db_);
        if (rc) {
            cerr << "Can't open database. Filename: " << filename << ", ErrMsg: " << sqlite3_errmsg(db_) << endl;
            throw std::range_error("Cannot open database");
        }
        // Check whether table exists.
        exec("SELECT 1 FROM " + table_name + " LIMIT 1;")
    }

    void Sample(int n, vector<Record> *sample_records) const {
        shared_lock<shared_mutex> lock(write_mutex_);
        const auto &loaded = recent_loaded_[curr_idx_];

        // Sample with replacement.
        for (int i = 0; i < n; ++i) {
            int idx = rng_() % loaded.size();
            sample_records->push_back(loaded[idx]);
        }
    }

    bool Insert(const Record &r) {
        return table_insert(r);
    }

    ~SharedRWBuffer() {
        sqlite3_close(db_);
    }

    // Save and load
private:
   sqlite3 *db_ = nullptr;
   const string table_name_;

   mt19937 rng_;

   int curr_idx_ = 0;
   vector<vector<Record>> recent_loaded_;
   mutable mutex alt_mutex_;
   mutable shared_mutex write_mutex_;

   friend int read_callback(void *, int, char **, char **);

   int exec(const string &sql, SqlCB callback = nullptr) {
       char *zErrMsg;
       int rc = sqlite3_exec(db_, sql.c_str(), callback, this, &zErrMsg);
       sqlite3_free(zErrMsg);
       return rc;
   }

   bool table_exists() {
       const string sql = "SELECT 1 FROM " + table_name_ + " LIMIT 1;";
       return exec(sql) == 0;
   }

   bool table_create() {
       const string sql =
         "CREATE TABLE " + table_name + " ("  \
             "TIME           CHAR(20) PRIMARY KEY NOT NULL," \
             "GAME_ID        INT     NOT NULL," \
             "MACHINE        CHAR(80) NOT NULL," \
             "SEQ            INT     NOT NULL," \
             "PRI            REAL    NOT NULL," \
             "REWARD         REAL    NOT NULL," \
             "CONTENT        TEXT);";

       if (exec(sql) != 0) return false;

       const string sql_idx = "CREATE INDEX idx_pri ON " + table_name_ + "(PRI);";
       if (exec(sql_idx) != 0) return false;

       const string sql_reward = "CREATE INDEX idx_reward ON " + table_name_ + "(REWARD);";
       if (exec(sql_reward) != 0) return false;

       return true;
   }

   bool table_insert(const Record &r) {
       auto now = chrono::system_clock::now();
       uint64_t timestamp = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();

       const string sql = "INSERT INTO " + table_name_ + " VALUES (" \
          + to_string(r.timestamp == 0 ? timestamp : r.timestamp) + ", " \
          + to_string(r.game_id) + ", " \
          + r.machine + ", " \
          + to_string(r.seq) + ", " \
          + to_string(r.pri) + ", " \
          + to_string(r.reward) + ", " \
          + r.content + ");";
       return exec(sql) == 0;
   }

   bool table_read_recent(int max_num_records) {
       // Read things into a buffer.
       const string sql = "SELECT * FROM " + table_name_ + " ORDER BY TIME DESC LIMIT " + to_string(max_num_records) + ";";
       cb_save_start();
       int ret = exec(sql, read_callback);
       cb_save_end();
       return ret == 0;
   }

   void cb_save_start() {
       alt_mutex_.lock();
       int alt_idx = (curr_idx_ + 1) % recent_loaded_.size();
       recent_loaded_[alt_idx].clear();
   }

   bool cb_save(const Record &r) {
       int alt_idx = (curr_idx_ + 1) % recent_loaded_.size();
       // Assuming the locker is ready.
       recent_loaded_[alt_idx].push_back(r);
       return true;
   }

   void cb_save_end() {
       int alt_idx = (curr_idx_ + 1) % recent_loaded_.size();
       unique_lock<shared_mutex> lock(write_mutex_);
       curr_idx_ = alt_idx;
       lock.release();
       alt_mutex_.unlock();
   }
};

} // namespace elf
