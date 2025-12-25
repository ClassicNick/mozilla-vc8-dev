/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_CAMERACOMMON_H
#define DOM_CAMERA_CAMERACOMMON_H

#ifndef __func__
#ifdef __FUNCTION__
#define __func__ __FUNCTION__
#else
#define __func__ __FILE__
#endif
#endif

#ifndef NAN
#define NAN std::numeric_limits<double>::quiet_NaN()
#endif

#include "nsThreadUtils.h"
#include "nsIDOMCameraManager.h"

#define DOM_CAMERA_LOG( l, a )          \
  do {                                    \
    if ( DOM_CAMERA_LOG_LEVEL >= (l) ) {  \
      printf_stderr (a);        \
    }                                     \
  } while (0)

#define DOM_CAMERA_LOGA( a )        DOM_CAMERA_LOG( 0, a )

enum {
  DOM_CAMERA_LOG_NOTHING,
  DOM_CAMERA_LOG_ERROR,
  DOM_CAMERA_LOG_WARNING,
  DOM_CAMERA_LOG_INFO
};

#define DOM_CAMERA_LOGI( a )        DOM_CAMERA_LOG( DOM_CAMERA_LOG_INFO,  a )
#define DOM_CAMERA_LOGW( a )        DOM_CAMERA_LOG( DOM_CAMERA_LOG_WARNING, a )
#define DOM_CAMERA_LOGE( a )        DOM_CAMERA_LOG( DOM_CAMERA_LOG_ERROR, a )

class CameraErrorResult : public nsRunnable
{
public:
  CameraErrorResult(nsICameraErrorCallback* onError, const nsString& aErrorMsg)
    : mOnErrorCb(onError)
    , mErrorMsg(aErrorMsg)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnErrorCb) {
      mOnErrorCb->HandleEvent(mErrorMsg);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
  const nsString mErrorMsg;
};

#endif // DOM_CAMERA_CAMERACOMMON_H
