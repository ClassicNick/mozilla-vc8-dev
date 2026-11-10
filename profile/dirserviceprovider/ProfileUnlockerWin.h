/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_ProfileUnlockerWin_h
#define mozilla_ProfileUnlockerWin_h

#include <windows.h>
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
#include <restartmanager.h>
#else
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
// CCH_RM_MAX_APP_NAME - maximum character count of application friendly name
#define CCH_RM_MAX_APP_NAME 255
// CCH_RM_MAX_SVC_NAME - maximum character count of service short name
#define CCH_RM_MAX_SVC_NAME 63
// RM_SESSION_KEY_LEN - size in bytes of binary session key
#define RM_SESSION_KEY_LEN  sizeof(GUID)
// CCH_RM_SESSION_KEY - character count of text-encoded session key
#define CCH_RM_SESSION_KEY  RM_SESSION_KEY_LEN*2

typedef enum _RM_REBOOT_REASON {
    RmRebootReasonNone = 0x0,               // Reboot not required
    RmRebootReasonPermissionDenied = 0x1,   // Current user does not have
                                            // permission to shut down
                                            // one or more detected processes
    RmRebootReasonSessionMismatch = 0x2,    // One or more processes are
                                            // running in another TS session.
    RmRebootReasonCriticalProcess = 0x4,    // A critical process has been
                                            // detected
    RmRebootReasonCriticalService = 0x8,    // A critical service has been
                                            // detected
    RmRebootReasonDetectedSelf = 0x10       // The current process has been
                                            // detected
} RM_REBOOT_REASON;

typedef enum _RM_APP_TYPE {
    RmUnknownApp = 0,   // Application type cannot be classified in
                        // known categories
    RmMainWindow = 1,   // Application is a windows application that
                        // displays a top-level window
    RmOtherWindow = 2,  // Application is a windows app but does not
                        // display a top-level window
    RmService = 3,      // Application is an NT service
    RmExplorer = 4,     // Application is Explorer
    RmConsole = 5,      // Application is Console application
    RmCritical = 1000   // Application is critical system process
                        // where a reboot is required to restart
} RM_APP_TYPE;

typedef struct _RM_UNIQUE_PROCESS {
    DWORD dwProcessId;              // PID
    FILETIME ProcessStartTime;      // Process creation time
} RM_UNIQUE_PROCESS, *PRM_UNIQUE_PROCESS;

typedef struct _RM_PROCESS_INFO{
    RM_UNIQUE_PROCESS Process;      // Unique process identification
    WCHAR strAppName[CCH_RM_MAX_APP_NAME+1];    // Application friendly name
    WCHAR strServiceShortName[CCH_RM_MAX_SVC_NAME+1];   // Service short name,
                                                        // if applicable
    RM_APP_TYPE ApplicationType;    // Application type
    ULONG AppStatus;                // Bit mask of application status
    DWORD TSSessionId;              // Terminal Service session ID of 
                                    // process (-1 if n/a)
    BOOL bRestartable;              // Is application restartable?
} RM_PROCESS_INFO, *PRM_PROCESS_INFO;
#endif

#include "nsIProfileUnlocker.h"
#include "nsProfileStringTypes.h"
#include "nsWindowsHelpers.h"

namespace mozilla {

class ProfileUnlockerWin MOZ_FINAL : public nsIProfileUnlocker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROFILEUNLOCKER

  explicit ProfileUnlockerWin(const nsAString& aFileName);

  nsresult Init();

  DWORD StartSession(DWORD& aHandle);
  void EndSession(DWORD aHandle);

private:
  ~ProfileUnlockerWin();
  nsresult TryToTerminate(RM_UNIQUE_PROCESS& aProcess);

private:
  typedef DWORD (WINAPI *RMSTARTSESSION)(DWORD*, DWORD, WCHAR[]);
  typedef DWORD (WINAPI *RMREGISTERRESOURCES)(DWORD, UINT, LPCWSTR[], UINT,
                                              RM_UNIQUE_PROCESS[], UINT,
                                              LPCWSTR[]);
  typedef DWORD (WINAPI *RMGETLIST)(DWORD, UINT*, UINT*, RM_PROCESS_INFO[],
                                    LPDWORD);
  typedef DWORD (WINAPI *RMENDSESSION)(DWORD);
  typedef BOOL (WINAPI *QUERYFULLPROCESSIMAGENAME)(HANDLE, DWORD, LPWSTR, PDWORD);

private:
  nsModuleHandle            mRestartMgrModule;
  RMSTARTSESSION            mRmStartSession;
  RMREGISTERRESOURCES       mRmRegisterResources;
  RMGETLIST                 mRmGetList;
  RMENDSESSION              mRmEndSession;
  QUERYFULLPROCESSIMAGENAME mQueryFullProcessImageName;

  nsString                  mFileName;
};

} // namespace mozilla

#endif // mozilla_ProfileUnlockerWin_h

