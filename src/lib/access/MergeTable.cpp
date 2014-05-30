// Copyright (c) 2012 Hasso-Plattner-Institut fuer Softwaresystemtechnik GmbH. All rights reserved.
#include "access/MergeTable.h"

#include "access/system/QueryParser.h"

#include "helper/checked_cast.h"
#include "storage/Store.h"
#include "storage/ColumnStoreMerger.h"

namespace hyrise {
namespace access {

namespace {
auto _ = QueryParser::registerPlanOperation<MergeTable>("MergeTable");
}

MergeTable::~MergeTable() {}

void MergeTable::executePlanOperation() {
  std::vector<storage::c_atable_ptr_t> tables;
  // Add all tables to the game
  for (auto& table : input.getTables()) {
    if (auto store = std::dynamic_pointer_cast<const storage::Store>(table)) {
      tables.push_back(store->getMainTable());
      tables.push_back(store->getDeltaTable());
    } else {
      tables.push_back(table);
    }
  }

  // Call the Merge
  storage::TableMerger merger(new storage::DefaultMergeStrategy(), new storage::SequentialHeapMerger());
  auto new_table = input.getTable(0)->copy_structure();

  // Switch the tables
  auto merged_tables = merger.mergeToTable(new_table, tables);
  const auto& result = std::make_shared<storage::Store>(new_table);

  output.add(result);
}

std::shared_ptr<PlanOperation> MergeTable::parse(const Json::Value& data) { return std::make_shared<MergeTable>(); }

const std::string MergeTable::vname() { return "MergeTable"; }

namespace {
auto _2 = QueryParser::registerPlanOperation<MergeStore>("MergeStore");
}

MergeStore::~MergeStore() {}

void MergeStore::executePlanOperation() {
  auto t = checked_pointer_cast<const storage::Store>(getInputTable());
  auto store = std::const_pointer_cast<storage::Store>(t);
  store->merge();
  addResult(store);
}

std::shared_ptr<PlanOperation> MergeStore::parse(const Json::Value& data) { return std::make_shared<MergeStore>(); }

namespace {
auto _3 = QueryParser::registerPlanOperation<MergeColumnStore>("MergeColumnStore");
}

MergeColumnStore::MergeColumnStore(bool forceFullIndexRebuild) : _forceFullIndexRebuild(forceFullIndexRebuild) {};

MergeColumnStore::~MergeColumnStore() {}

void MergeColumnStore::executePlanOperation() {
  auto t = checked_pointer_cast<const storage::Store>(getInputTable());
  auto store = std::const_pointer_cast<storage::Store>(t);
  storage::ColumnStoreMerger merger = storage::ColumnStoreMerger(store, _forceFullIndexRebuild);

  if (!store->isColumnStore())
    throw std::runtime_error("Column Store Merge applied to a table with row or hybrid layout");

  merger.merge();
  addResult(store);
}

std::shared_ptr<PlanOperation> MergeColumnStore::parse(const Json::Value& data) {
  if (data.isMember("force_full_index_rebuild"))
    return std::make_shared<MergeColumnStore>(data["force_full_index_rebuild"].asBool());

  return std::make_shared<MergeColumnStore>(false);
}

void MergeColumnStore::setForceFullIndexRebuild(bool force) { _forceFullIndexRebuild = force; }
}
}
