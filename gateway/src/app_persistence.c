#include "../include/app_persistence.h"
#include "../thirdparty/log.c/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

static const char *SQL_CREATE_TABLE = 
    "CREATE TABLE IF NOT EXISTS messages ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "topic TEXT NOT NULL,"
    "payload BLOB NOT NULL,"
    "payload_len INTEGER NOT NULL,"
    "qos INTEGER DEFAULT 0,"
    "status INTEGER DEFAULT 0,"
    "retry_count INTEGER DEFAULT 0,"
    "create_time INTEGER NOT NULL,"
    "update_time INTEGER NOT NULL"
    ");";

static const char *SQL_CREATE_INDEX = 
    "CREATE INDEX IF NOT EXISTS idx_status ON messages(status);";

int persistence_init(PersistenceManager *manager, const PersistenceConfig *config)
{
    if (!manager) return -1;
    
    memset(manager, 0, sizeof(PersistenceManager));
    
    if (config) {
        memcpy(&manager->config, config, sizeof(PersistenceConfig));
    } else {
        strcpy(manager->config.db_path, "/tmp/gateway_messages.db");
        manager->config.max_retry_count = 3;
        manager->config.message_expire_hours = 24;
        manager->config.max_queue_size = 10000;
    }
    
    sqlite3 *db;
    int rc = sqlite3_open(manager->config.db_path, &db);
    if (rc != SQLITE_OK) {
        log_error("Failed to open database: %s", manager->config.db_path);
        return -1;
    }
    
    manager->db = db;
    
    char *err_msg = NULL;
    rc = sqlite3_exec(db, SQL_CREATE_TABLE, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        log_error("Failed to create table: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }
    
    rc = sqlite3_exec(db, SQL_CREATE_INDEX, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
    }
    
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    
    manager->is_initialized = 1;
    
    log_info("Persistence initialized: %s", manager->config.db_path);
    return 0;
}

int persistence_init_default(PersistenceManager *manager, const char *db_path)
{
    PersistenceConfig config = {0};
    
    if (db_path) {
        strncpy(config.db_path, db_path, sizeof(config.db_path) - 1);
    } else {
        strcpy(config.db_path, "/tmp/gateway_messages.db");
    }
    config.max_retry_count = 3;
    config.message_expire_hours = 24;
    config.max_queue_size = 10000;
    
    return persistence_init(manager, &config);
}

void persistence_close(PersistenceManager *manager)
{
    if (!manager) return;
    
    if (manager->db) {
        sqlite3_close((sqlite3 *)manager->db);
        manager->db = NULL;
    }
    
    manager->is_initialized = 0;
}

int persistence_save(PersistenceManager *manager, const char *topic,
                     const void *payload, size_t len, int qos, uint64_t *out_id)
{
    if (!manager || !manager->is_initialized || !topic || !payload) {
        return -1;
    }
    
    sqlite3 *db = (sqlite3 *)manager->db;
    time_t now = time(NULL);
    
    const char *sql = "INSERT INTO messages (topic, payload, payload_len, qos, "
                      "status, retry_count, create_time, update_time) "
                      "VALUES (?, ?, ?, ?, 0, 0, ?, ?);";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, topic, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, payload, len, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, len);
    sqlite3_bind_int(stmt, 4, qos);
    sqlite3_bind_int64(stmt, 5, now);
    sqlite3_bind_int64(stmt, 6, now);
    
    rc = sqlite3_step(stmt);
    
    if (rc != SQLITE_DONE) {
        log_error("Failed to insert message: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }
    
    uint64_t id = (uint64_t)sqlite3_last_insert_rowid(db);
    
    if (out_id) {
        *out_id = id;
    }
    
    sqlite3_finalize(stmt);
    
    log_debug("Message saved: id=%llu, topic=%s, len=%zu", 
              (unsigned long long)id, topic, len);
    return 0;
}

int persistence_get_next(PersistenceManager *manager, PersistMessage *out_message)
{
    if (!manager || !manager->is_initialized || !out_message) {
        return -1;
    }
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    const char *sql = "SELECT id, topic, payload, payload_len, qos, status, "
                      "retry_count, create_time, update_time "
                      "FROM messages WHERE status = 0 "
                      "AND retry_count < ? "
                      "ORDER BY create_time ASC LIMIT 1;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, manager->config.max_retry_count);
    
    rc = sqlite3_step(stmt);
    
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    memset(out_message, 0, sizeof(PersistMessage));
    
    out_message->id = sqlite3_column_int64(stmt, 0);
    strncpy(out_message->topic, (const char *)sqlite3_column_text(stmt, 1), 
            sizeof(out_message->topic) - 1);
    
    const void *payload = sqlite3_column_blob(stmt, 2);
    int payload_len = sqlite3_column_int(stmt, 3);
    
    out_message->payload = malloc(payload_len);
    if (!out_message->payload) {
        log_error("Failed to allocate memory for payload");
        sqlite3_finalize(stmt);
        return -1;
    }
    else {
        memcpy(out_message->payload, payload, payload_len);
        out_message->payload_len = payload_len;
    }
    
    out_message->qos = sqlite3_column_int(stmt, 4);
    out_message->status = sqlite3_column_int(stmt, 5);
    out_message->retry_count = sqlite3_column_int(stmt, 6);
    out_message->create_time = sqlite3_column_int64(stmt, 7);
    out_message->update_time = sqlite3_column_int64(stmt, 8);
    
    sqlite3_finalize(stmt);
    
    persistence_update_status(manager, out_message->id, MSG_STATUS_SENDING);
    
    return 0;
}

int persistence_update_status(PersistenceManager *manager, uint64_t id, 
                              MessageStatus status)
{
    if (!manager || !manager->is_initialized) return -1;
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    const char *sql = "UPDATE messages SET status = ?, update_time = ? WHERE id = ?;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_int64(stmt, 2, time(NULL));
    sqlite3_bind_int64(stmt, 3, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int persistence_mark_sent(PersistenceManager *manager, uint64_t id)
{
    return persistence_update_status(manager, id, MSG_STATUS_SENT);
}

int persistence_mark_failed(PersistenceManager *manager, uint64_t id)
{
    if (!manager || !manager->is_initialized) return -1;
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    const char *check_sql = "SELECT retry_count FROM messages WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_int64(stmt, 1, id);
    rc = sqlite3_step(stmt);
    
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    int retry_count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    
    if (retry_count >= manager->config.max_retry_count) {
        persistence_update_status(manager, id, MSG_STATUS_FAILED);
        return -1;
    }
    
    const char *update_sql = "UPDATE messages SET status = 0, retry_count = ?, "
                             "update_time = ? WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, retry_count + 1);
    sqlite3_bind_int64(stmt, 2, time(NULL));
    sqlite3_bind_int64(stmt, 3, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int persistence_delete(PersistenceManager *manager, uint64_t id)
{
    if (!manager || !manager->is_initialized) return -1;
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    const char *sql = "DELETE FROM messages WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_int64(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int persistence_get_pending_count(PersistenceManager *manager)
{
    if (!manager || !manager->is_initialized) return 0;
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    const char *sql = "SELECT COUNT(*) FROM messages WHERE status = 0 "
                      "AND retry_count < ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return 0;
    
    sqlite3_bind_int(stmt, 1, manager->config.max_retry_count);
    
    rc = sqlite3_step(stmt);
    int count = 0;
    
    if (rc == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int persistence_get_all_pending(PersistenceManager *manager, 
                                PersistMessage *messages, int max_count)
{
    if (!manager || !manager->is_initialized || !messages || max_count <= 0) {
        return -1;
    }
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    const char *sql = "SELECT id, topic, payload, payload_len, qos, status, "
                      "retry_count, create_time, update_time "
                      "FROM messages WHERE status = 0 "
                      "AND retry_count < ? "
                      "ORDER BY create_time ASC LIMIT ?;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, manager->config.max_retry_count);
    sqlite3_bind_int(stmt, 2, max_count);
    
    int count = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && count < max_count) {
        PersistMessage *msg = &messages[count];
        memset(msg, 0, sizeof(PersistMessage));
        
        msg->id = sqlite3_column_int64(stmt, 0);
        strncpy(msg->topic, (const char *)sqlite3_column_text(stmt, 1), 
                sizeof(msg->topic) - 1);
        
        const void *payload = sqlite3_column_blob(stmt, 2);
        int payload_len = sqlite3_column_int(stmt, 3);
        
        msg->payload = malloc(payload_len);
        if (msg->payload) {
            memcpy(msg->payload, payload, payload_len);
            msg->payload_len = payload_len;
        }
        else {
            log_error("Failed to allocate memory for message payload");
            break;
        }
        
        msg->qos = sqlite3_column_int(stmt, 4);
        msg->status = sqlite3_column_int(stmt, 5);
        msg->retry_count = sqlite3_column_int(stmt, 6);
        msg->create_time = sqlite3_column_int64(stmt, 7);
        msg->update_time = sqlite3_column_int64(stmt, 8);
        
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int persistence_cleanup_expired(PersistenceManager *manager)
{
    if (!manager || !manager->is_initialized) return 0;
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    time_t expire_time = time(NULL) - manager->config.message_expire_hours * 3600;
    
    const char *sql = "DELETE FROM messages WHERE create_time < ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_int64(stmt, 1, expire_time);
    rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    
    log_debug("Cleaned up %d expired messages", changes);
    return changes;
}

int persistence_cleanup_sent(PersistenceManager *manager)
{
    if (!manager || !manager->is_initialized) return 0;
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    const char *sql = "DELETE FROM messages WHERE status = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, MSG_STATUS_SENT);
    rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    
    log_debug("Cleaned up %d sent messages", changes);
    return changes;
}

int persistence_clear_all(PersistenceManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    sqlite3 *db = (sqlite3 *)manager->db;
    
    const char *sql = "DELETE FROM messages;";
    int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    
    return (rc == SQLITE_OK) ? 0 : -1;
}

void persistence_free_message(PersistMessage *message)
{
    if (message && message->payload) {
        free(message->payload);
        message->payload = NULL;
        message->payload_len = 0;
    }
}
