/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <Common/GlutenConfig.h>
#include <Common/MergeTreeTool.h>
#include <Poco/LRUCache.h>
#include <Parser/SerializedPlanParser.h>
#include <Storages/CustomStorageMergeTree.h>
#include <Interpreters/MergeTreeTransaction.h>


namespace local_engine
{
using CustomStorageMergeTreePtr = std::shared_ptr<CustomStorageMergeTree>;

class DataPartStorageHolder
{
public:
    DataPartStorageHolder(const DataPartPtr& data_part, const CustomStorageMergeTreePtr& storage)
        : data_part_(data_part),
          storage_(storage)
    {
    }

    [[nodiscard]] DataPartPtr dataPart() const
    {
        return data_part_;
    }

    [[nodiscard]] CustomStorageMergeTreePtr storage() const
    {
        return storage_;
    }

    ~DataPartStorageHolder()
    {
        storage_->removePartFromMemory(*data_part_);
        // std::cerr << fmt::format("clean part {}", data_part_->name) << std::endl;
    }

private:
    DataPartPtr data_part_;
    CustomStorageMergeTreePtr storage_;
};
using DataPartStorageHolderPtr = std::shared_ptr<DataPartStorageHolder>;

class StorageMergeTreeFactory
{
public:
    static StorageMergeTreeFactory & instance();
    static void freeStorage(const StorageID & id, const String & snapshot_id = "");
    static CustomStorageMergeTreePtr
    getStorage(const StorageID& id, const String & snapshot_id, MergeTreeTable merge_tree_table, std::function<CustomStorageMergeTreePtr()> creator);
    static DataPartsVector getDataPartsByNames(const StorageID & id, const String & snapshot_id, std::unordered_set<String> part_name);
    static void init_cache_map()
    {
        auto config = MergeTreeConfig::loadFromContext(SerializedPlanParser::global_context);
        auto & storage_map_v = storage_map;
        if (!storage_map_v)
        {
            storage_map_v = std::make_unique<Poco::LRUCache<std::string, std::pair<CustomStorageMergeTreePtr, MergeTreeTable>>>(config.table_metadata_cache_max_count);
        }
        else
        {
            storage_map_v->clear();
        }
        auto & datapart_map_v = datapart_map;
        if (!datapart_map_v)
        {
            datapart_map_v = std::make_unique<Poco::LRUCache<std::string, std::shared_ptr<Poco::LRUCache<std::string, DataPartStorageHolderPtr>>>>(
                config.table_metadata_cache_max_count);
        }
        else
        {
            datapart_map_v->clear();
        }
    }
    static void clear()
    {
        if (storage_map) storage_map->clear();
        if (datapart_map) datapart_map->clear();
    }

    static String getTableName(const StorageID & id, const String & snapshot_id);

private:
    static std::unique_ptr<Poco::LRUCache<std::string, std::pair<CustomStorageMergeTreePtr, MergeTreeTable>>> storage_map;
    static std::unique_ptr<Poco::LRUCache<std::string, std::shared_ptr<Poco::LRUCache<std::string, DataPartStorageHolderPtr>>>> datapart_map;

    static std::recursive_mutex storage_map_mutex;
    static std::recursive_mutex datapart_mutex;
};

struct TempStorageFreer
{
    StorageID id;
    ~TempStorageFreer()
    {
        StorageMergeTreeFactory::instance().freeStorage(id);
    }
};
}
