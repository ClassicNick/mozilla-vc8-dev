/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GMPAudioHost.h"
#include "gmp-audio-samples.h"
#include "gmp-errors.h"

namespace mozilla {
namespace gmp {

GMPAudioSamplesImpl::GMPAudioSamplesImpl(GMPAudioFormat aFormat)
  : mFormat(aFormat)
  , mTimeStamp(0)
{
}

GMPAudioSamplesImpl::GMPAudioSamplesImpl(const GMPAudioEncodedSampleData& aData)
  : mFormat(kGMPAudioEncodedSamples)
  , mBuffer(aData.mData())
  , mTimeStamp(aData.mTimeStamp())
{
}

GMPAudioSamplesImpl::~GMPAudioSamplesImpl()
{
}

GMPAudioFormat
GMPAudioSamplesImpl::GetFormat()
{
  return mFormat;
}

void
GMPAudioSamplesImpl::Destroy()
{
  delete this;
}

GMPErr
GMPAudioSamplesImpl::SetBufferSize(uint32_t aSize)
{
  mBuffer.SetLength(aSize);
  return GMPNoErr;
}

uint32_t
GMPAudioSamplesImpl::Size()
{
  return mBuffer.Length();
}

void
GMPAudioSamplesImpl::SetTimeStamp(uint64_t aTimeStamp)
{
  mTimeStamp = aTimeStamp;
}

uint64_t
GMPAudioSamplesImpl::TimeStamp()
{
  return mTimeStamp;
}

const uint8_t*
GMPAudioSamplesImpl::Buffer() const
{
  return mBuffer.Elements();
}

uint8_t*
GMPAudioSamplesImpl::Buffer()
{
  return mBuffer.Elements();
}

void
GMPAudioSamplesImpl::RelinquishData(GMPAudioEncodedSampleData& aData)
{
  aData.mData() = mBuffer;
  aData.mTimeStamp() = TimeStamp();
}

GMPErr
GMPAudioHostImpl::CreateSamples(GMPAudioFormat aFormat,
                                GMPAudioSamples** aSamples)
{

  *aSamples = new GMPAudioSamplesImpl(aFormat);
  return GMPNoErr;
}

} // namespace gmp
} // namespace mozilla
