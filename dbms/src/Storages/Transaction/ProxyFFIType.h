#pragma once

#include <Storages/Transaction/ColumnFamily.h>
#include <Storages/Transaction/FileEncryption.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>

namespace DB
{

using RegionId = uint64_t;
using AppliedIndex = uint64_t;
using TiFlashRaftProxyPtr = void *;

class TMTContext;
struct TiFlashServer;

enum class TiFlashApplyRes : uint32_t
{
    None = 0,
    Persist,
    NotFound,
};

enum class WriteCmdType : uint8_t
{
    Put = 0,
    Del,
};

extern "C" {

using TiFlashRawString = std::string *;

struct BaseBuffView
{
    const char * data;
    const uint64_t len;

    BaseBuffView(const std::string & s) : data(s.data()), len(s.size()) {}
    BaseBuffView(const char * data_, const uint64_t len_) : data(data_), len(len_) {}
    BaseBuffView(std::string_view view) : data(view.data()), len(view.size()) {}
};

struct SnapshotView
{
    const BaseBuffView * keys;
    const BaseBuffView * vals;
    const ColumnFamilyType cf;
    const uint64_t len = 0;
};

struct SnapshotViewArray
{
    const SnapshotView * views;
    const uint64_t len = 0;
};

struct RaftCmdHeader
{
    uint64_t region_id;
    uint64_t index;
    uint64_t term;
};

struct WriteCmdsView
{
    const BaseBuffView * keys;
    const BaseBuffView * vals;
    const WriteCmdType * cmd_types;
    const ColumnFamilyType * cmd_cf;
    const uint64_t len;
};

struct FsStats
{
    uint64_t used_size;
    uint64_t avail_size;
    uint64_t capacity_size;

    uint8_t ok;

    FsStats() { memset(this, 0, sizeof(*this)); }
};

struct TiFlashRaftProxyHelper
{
public:
    bool checkServiceStopped() const;
    bool checkEncryptionEnabled() const;
    EncryptionMethod getEncryptionMethod() const;
    FileEncryptionInfo getFile(std::string_view) const;
    FileEncryptionInfo newFile(std::string_view) const;
    FileEncryptionInfo deleteFile(std::string_view) const;
    FileEncryptionInfo linkFile(std::string_view, std::string_view) const;
    FileEncryptionInfo renameFile(std::string_view, std::string_view) const;

private:
    TiFlashRaftProxyPtr proxy_ptr;
    uint8_t (*fn_handle_check_service_stopped)(TiFlashRaftProxyPtr);
    uint8_t (*fn_is_encryption_enabled)(TiFlashRaftProxyPtr);
    EncryptionMethod (*fn_encryption_method)(TiFlashRaftProxyPtr);
    FileEncryptionInfoRaw (*fn_handle_get_file)(TiFlashRaftProxyPtr, BaseBuffView);
    FileEncryptionInfoRaw (*fn_handle_new_file)(TiFlashRaftProxyPtr, BaseBuffView);
    FileEncryptionInfoRaw (*fn_handle_delete_file)(TiFlashRaftProxyPtr, BaseBuffView);
    FileEncryptionInfoRaw (*fn_handle_link_file)(TiFlashRaftProxyPtr, BaseBuffView, BaseBuffView);
    FileEncryptionInfoRaw (*fn_handle_rename_file)(TiFlashRaftProxyPtr, BaseBuffView, BaseBuffView);
};

enum class TiFlashStatus : uint8_t
{
    IDLE = 0,
    Running,
    Stopped,
};

struct CppStrWithView
{
    TiFlashRawString inner{nullptr};
    BaseBuffView view;

    CppStrWithView(std::string && v) : inner(new std::string(std::move(v))), view(*inner) {}
};

struct TiFlashServerHelper
{
    uint32_t magic_number; // use a very special number to check whether this struct is legal
    uint32_t version;      // version of function interface
    //

    TiFlashServer * inner;
    TiFlashRawString (*fn_gen_cpp_string)(BaseBuffView);
    TiFlashApplyRes (*fn_handle_write_raft_cmd)(const TiFlashServer *, WriteCmdsView, RaftCmdHeader);
    TiFlashApplyRes (*fn_handle_admin_raft_cmd)(const TiFlashServer *, BaseBuffView, BaseBuffView, RaftCmdHeader);
    void (*fn_handle_apply_snapshot)(const TiFlashServer *, BaseBuffView, uint64_t, SnapshotViewArray, uint64_t, uint64_t);
    void (*fn_atomic_update_proxy)(TiFlashServer *, TiFlashRaftProxyHelper *);
    void (*fn_handle_destroy)(TiFlashServer *, RegionId);
    TiFlashApplyRes (*fn_handle_ingest_sst)(TiFlashServer *, SnapshotViewArray, RaftCmdHeader);
    uint8_t (*fn_handle_check_terminated)(TiFlashServer *);
    FsStats (*fn_handle_compute_fs_stats)(TiFlashServer *);
    TiFlashStatus (*fn_handle_get_tiflash_status)(TiFlashServer *);
    void * (*fn_pre_handle_snapshot)(TiFlashServer *, BaseBuffView, uint64_t, SnapshotViewArray, uint64_t, uint64_t);
    void (*fn_apply_pre_handled_snapshot)(TiFlashServer *, void *);
    void (*fn_gc_pre_handled_snapshot)(TiFlashServer *, void *);
    CppStrWithView (*fn_handle_get_table_sync_status)(TiFlashServer *, uint64_t);
    void (*gc_cpp_string)(TiFlashServer *, TiFlashRawString);
};

void run_tiflash_proxy_ffi(int argc, const char ** argv, const TiFlashServerHelper *);
}

struct TiFlashServer
{
    TMTContext * tmt{nullptr};
    TiFlashRaftProxyHelper * proxy_helper{nullptr};
    std::atomic<TiFlashStatus> status{TiFlashStatus::IDLE};
};

TiFlashRawString GenCppRawString(BaseBuffView);
TiFlashApplyRes HandleAdminRaftCmd(const TiFlashServer * server, BaseBuffView req_buff, BaseBuffView resp_buff, RaftCmdHeader header);
void HandleApplySnapshot(
    const TiFlashServer * server, BaseBuffView region_buff, uint64_t peer_id, SnapshotViewArray snaps, uint64_t index, uint64_t term);
TiFlashApplyRes HandleWriteRaftCmd(const TiFlashServer * server, WriteCmdsView req_buff, RaftCmdHeader header);
void AtomicUpdateProxy(TiFlashServer * server, TiFlashRaftProxyHelper * proxy);
void HandleDestroy(TiFlashServer * server, RegionId region_id);
TiFlashApplyRes HandleIngestSST(TiFlashServer * server, SnapshotViewArray snaps, RaftCmdHeader header);
uint8_t HandleCheckTerminated(TiFlashServer * server);
FsStats HandleComputeFsStats(TiFlashServer * server);
TiFlashStatus HandleGetTiFlashStatus(TiFlashServer * server);
void * PreHandleSnapshot(
    TiFlashServer * server, BaseBuffView region_buff, uint64_t peer_id, SnapshotViewArray snaps, uint64_t index, uint64_t term);
void ApplyPreHandledSnapshot(TiFlashServer * server, void * res);
void GcPreHandledSnapshot(TiFlashServer * server, void * res);
CppStrWithView HandleGetTableSyncStatus(TiFlashServer *, uint64_t);
void GcCppString(TiFlashServer *, TiFlashRawString);

} // namespace DB