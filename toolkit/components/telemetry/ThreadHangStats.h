/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_BackgroundHangTelemetry_h
#define mozilla_BackgroundHangTelemetry_h

#include "mozilla/Array.h"
#include "mozilla/Assertions.h"
#include "mozilla/Move.h"
#include "mozilla/Mutex.h"
#include "mozilla/PodOperations.h"
#include "mozilla/Vector.h"

#include "nsString.h"
#include "prinrval.h"

namespace mozilla {
namespace Telemetry {

static const size_t kTimeHistogramBuckets = 8 * sizeof(PRIntervalTime);

/* TimeHistogram is an efficient histogram that puts time durations into
   exponential (base 2) buckets; times are accepted in PRIntervalTime and
   stored in milliseconds. */
class TimeHistogram : public mozilla::Array<uint32_t, kTimeHistogramBuckets>
{
public:
  TimeHistogram()
  {
    mozilla::PodArrayZero(*this);
  }
  // Get minimum (inclusive) range of bucket in milliseconds
  uint32_t GetBucketMin(size_t aBucket) const {
    MOZ_ASSERT(aBucket < ArrayLength(*this));
    return (1u << aBucket) & ~1u; // Bucket 0 starts at 0, not 1
  }
  // Get maximum (inclusive) range of bucket in milliseconds
  uint32_t GetBucketMax(size_t aBucket) const {
    MOZ_ASSERT(aBucket < ArrayLength(*this));
    return (1u << (aBucket + 1u)) - 1u;
  }
  void Add(PRIntervalTime aTime);
};

/* HangStack stores an array of const char pointers,
   with optional internal storage for strings. */
class HangStack : public mozilla::Vector<const char*, 8>
{
private:
  typedef mozilla::Vector<const char*, 8> Base;

  // Stack entries can either be a static const char*
  // or a pointer to within this buffer.
  mozilla::Vector<char, 0> mBuffer;

public:
  HangStack() { }

  HangStack(mozilla::MoveRef<HangStack> aOther)
    : Base(aOther)
    , mBuffer(mozilla::OldMove(aOther->mBuffer))
  {
  }

  bool operator==(const HangStack& aOther) const {
    for (size_t i = 0; i < length(); i++) {
      if (!IsSameAsEntry(operator[](i), aOther[i])) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const HangStack& aOther) const {
    return !operator==(aOther);
  }

  void clear() {
    Base::clear();
    mBuffer.clear();
  }

  bool IsInBuffer(const char* aEntry) const {
    return aEntry >= mBuffer.begin() && aEntry < mBuffer.end();
  }

  bool IsSameAsEntry(const char* aEntry, const char* aOther) const {
    // If the entry came from the buffer, we need to compare its content;
    // otherwise we only need to compare its pointer.
    return IsInBuffer(aEntry) ? !strcmp(aEntry, aOther) : (aEntry == aOther);
  }

  size_t AvailableBufferSize() const {
    return mBuffer.capacity() - mBuffer.length();
  }

  bool EnsureBufferCapacity(size_t aCapacity) {
    // aCapacity is the minimal capacity and Vector may make the actual
    // capacity larger, in which case we want to use up all the space.
    return mBuffer.reserve(aCapacity) &&
           mBuffer.reserve(mBuffer.capacity());
  }

  const char* InfallibleAppendViaBuffer(const char* aText, size_t aLength);
  const char* AppendViaBuffer(const char* aText, size_t aLength);
};

/* A hang histogram consists of a stack associated with the
   hang, along with a time histogram of the hang times. */
class HangHistogram : public TimeHistogram
{
private:
  static uint32_t GetHash(const HangStack& aStack);

  HangStack mStack;
  // Use a hash to speed comparisons
  const uint32_t mHash;

public:
  HangHistogram(mozilla::MoveRef<HangStack> aStack)
    : mStack(aStack)
    , mHash(GetHash(mStack))
  {
  }
  HangHistogram(mozilla::MoveRef<HangHistogram> aOther)
    : TimeHistogram(aOther)
    , mStack(mozilla::OldMove(aOther->mStack))
    , mHash(mozilla::OldMove(aOther->mHash))
  {
  }
  bool operator==(const HangHistogram& aOther) const;
  bool operator!=(const HangHistogram& aOther) const
  {
    return !operator==(aOther);
  }
  const HangStack& GetStack() const {
    return OldMove(mStack);
  }
};

/* Thread hang stats consist of
 - thread name
 - time histogram of all task run times
 - hang histograms of individual hangs. */
class ThreadHangStats
{
private:
  nsAutoCString mName;

public:
  TimeHistogram mActivity;
  mozilla::Vector<HangHistogram, 4> mHangs;

  explicit ThreadHangStats(const char* aName)
    : mName(aName)
  {
  }
  ThreadHangStats(mozilla::MoveRef<ThreadHangStats> aOther)
    : mName(mozilla::OldMove(aOther->mName))
    , mActivity(mozilla::OldMove(aOther->mActivity))
    , mHangs(mozilla::OldMove(aOther->mHangs))
  {
  }
  const char* GetName() const {
    return mName.get();
  }
};

} // namespace Telemetry
} // namespace mozilla
#endif // mozilla_BackgroundHangTelemetry_h
