// Copyright 2019 JasonYuchen (jasonyuchen@foxmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "statemachines.h"

OpenResult DiskKV::open(const dragonboat::DoneChan &done) noexcept
{
  return OpenResult();
}

uint64_t DiskKV::update(
  const dragonboat::Byte *data,
  size_t size,
  uint64_t index) noexcept
{
  return 0;
}

LookupResult DiskKV::lookup(
  const dragonboat::Byte *data,
  size_t size) const noexcept
{
  return LookupResult();
}

int DiskKV::sync() const noexcept
{
  return 0;
}

uint64_t DiskKV::getHash() const noexcept
{
  return 0;
}

PrepareSnapshotResult DiskKV::prepareSnapshot() const noexcept
{
  return PrepareSnapshotResult();
}

SnapshotResult DiskKV::saveSnapshot(
  const dragonboat::Byte *ctx,
  size_t size,
  dragonboat::SnapshotWriter *writer,
  const dragonboat::DoneChan &done) const noexcept
{
  return SnapshotResult();
}

int DiskKV::recoverFromSnapshot(
  dragonboat::SnapshotReader *reader,
  const dragonboat::DoneChan &done) noexcept
{
  return 0;
}

void DiskKV::freePrepareSnapshotResult(PrepareSnapshotResult r) noexcept
{
  delete[] r.result;
}

void DiskKV::freeLookupResult(LookupResult r) noexcept
{
  delete[] r.result;
}
