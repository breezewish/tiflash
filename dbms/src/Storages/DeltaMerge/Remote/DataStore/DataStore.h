// Copyright 2022 PingCAP, Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <Storages/DeltaMerge/File/DMFile.h>
#include <Storages/DeltaMerge/Remote/DataStore/DataStore_fwd.h>
#include <Storages/Page/V3/CheckpointFile/CheckpointFiles.h>
#include <Storages/S3/S3Filename.h>

#include <boost/core/noncopyable.hpp>

namespace DB::DM::Remote
{

class IPreparedDMFileToken : boost::noncopyable
{
public:
    virtual ~IPreparedDMFileToken() = default;

    /**
     * Restores into a DMFile object. This token will be kept valid when DMFile is valid.
     */
    virtual DMFilePtr restore(DMFile::ReadMetaMode read_mode) = 0;

protected:
    // These should be the required information for any kind of DataStore.
    const FileProviderPtr file_provider;
    const S3::DMFileOID oid;
    UInt64 page_id;

    IPreparedDMFileToken(const FileProviderPtr & file_provider_, const S3::DMFileOID & oid_, UInt64 page_id_)
        : file_provider(file_provider_)
        , oid(oid_)
        , page_id(page_id_ == 0 ? oid.file_id : page_id_)
    {}
};

struct RemoteGCThreshold
{
    // The file with valid rate less than `valid_rate` will be compact
    double valid_rate;
    // The file size less than `min_file_threshold` will be compact
    size_t min_file_threshold;
};

class IDataStore : boost::noncopyable
{
public:
    virtual ~IDataStore() = default;

    /**
     * Blocks until a local DMFile is successfully put in the remote data store.
     * Should be used by a write node.
     *
     *
     * `remove_local` When it is true, after put is success, the `local_dm_file`
     * will turn to an instance pointing to remote location
     * (`DMFile::switchToRemote`).
     */
    virtual void putDMFile(DMFilePtr local_dm_file, const S3::DMFileOID & oid, bool remove_local) = 0;

    /**
     * Blocks until a DMFile in the remote data store is successfully prepared in a local cache.
     * If the DMFile exists in the local cache, it will not be prepared again.
     *
     * Returns a "token", which can be used to rebuild the `DMFile` object.
     * The DMFile in the local cache may be invalidated if you deconstructs the token.
     *
     * When page_id is 0, will use its file_id as page_id.(Used by WN, RN can just use default value)
     */
    virtual IPreparedDMFileTokenPtr prepareDMFile(const S3::DMFileOID & oid, UInt64 page_id = 0) = 0;

    virtual IPreparedDMFileTokenPtr prepareDMFileByKey(const String & remote_key) = 0;

    /**
     * Blocks until all checkpoint files are successfully put in the remote data store.
     * Returns true if all files are successfully uploaded.
     * Should be used by a write node.
     *
     * Note that this function ensure CheckpointManifest is the last file to be seen in the
     * remote data source for a given `upload_seq`.
     */
    virtual bool putCheckpointFiles(const PS::V3::LocalCheckpointFiles & local_files, StoreID store_id, UInt64 upload_seq) = 0;

    virtual std::unordered_map<String, Int64> getDataFileSizes(const std::unordered_set<String> & lock_keys) = 0;
};


} // namespace DB::DM::Remote
