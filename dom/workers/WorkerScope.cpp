/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WorkerScope.h"

#include "Location.h"
#include "Navigator.h"
#include "ScriptLoader.h"
#include "WorkerPrivate.h"

#include "jsapi.h"
#include "mozilla/dom/FunctionBinding.h"
#include "mozilla/dom/DedicatedWorkerGlobalScopeBinding.h"
#include "mozilla/dom/SharedWorkerGlobalScopeBinding.h"

#ifdef ANDROID
#include <android/log.h>
#endif

#include "RuntimeService.h" // For WorkersDumpEnabled().

#define UNWRAP_WORKER_OBJECT(Interface, cx, obj, value)                       \
  UnwrapObject<prototypes::id::Interface##_workers,                           \
    mozilla::dom::Interface##Binding_workers::NativeType>(cx, obj, value)

using namespace mozilla::dom;
USING_WORKERS_NAMESPACE

BEGIN_WORKERS_NAMESPACE

WorkerGlobalScope::WorkerGlobalScope(WorkerPrivate* aWorkerPrivate)
: mWorkerPrivate(aWorkerPrivate)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  SetIsDOMBinding();
}

WorkerGlobalScope::~WorkerGlobalScope()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(WorkerGlobalScope)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(WorkerGlobalScope,
                                                  nsDOMEventTargetHelper)
  tmp->mWorkerPrivate->AssertIsOnWorkerThread();
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(WorkerGlobalScope,
                                                nsDOMEventTargetHelper)
  tmp->mWorkerPrivate->AssertIsOnWorkerThread();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(WorkerGlobalScope,
                                               nsDOMEventTargetHelper)
  tmp->mWorkerPrivate->AssertIsOnWorkerThread();

  tmp->mWorkerPrivate->TraceTimeouts(aCallbacks, aClosure);
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_ADDREF_INHERITED(WorkerGlobalScope, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(WorkerGlobalScope, nsDOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WorkerGlobalScope)
  NS_INTERFACE_MAP_ENTRY(nsIGlobalObject)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

JSObject*
WorkerGlobalScope::WrapObject(JSContext* aCx, JS::HandleObject aScope)
{
  MOZ_CRASH("We should never get here!");
}

already_AddRefed<WorkerLocation>
WorkerGlobalScope::Location()
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  if (!mLocation) {
    WorkerPrivate::LocationInfo& info = mWorkerPrivate->GetLocationInfo();

    mLocation = WorkerLocation::Create(info);
    MOZ_ASSERT(mLocation);
  }

  nsRefPtr<WorkerLocation> location = mLocation;
  return location.forget();
}

already_AddRefed<WorkerNavigator>
WorkerGlobalScope::Navigator()
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  if (!mNavigator) {
    mNavigator = WorkerNavigator::Create();
    MOZ_ASSERT(mNavigator);
  }

  nsRefPtr<WorkerNavigator> navigator = mNavigator;
  return navigator.forget();
}

void
WorkerGlobalScope::Close(JSContext* aCx)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  mWorkerPrivate->CloseInternal(aCx);
}

OnErrorEventHandlerNonNull*
WorkerGlobalScope::GetOnerror()
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  nsEventListenerManager *elm = GetExistingListenerManager();
  return elm ? elm->GetOnErrorEventHandler() : nullptr;
}

void
WorkerGlobalScope::SetOnerror(OnErrorEventHandlerNonNull* aHandler)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  nsEventListenerManager *elm = GetOrCreateListenerManager();
  if (elm) {
    elm->SetEventHandler(aHandler);
  }
}

void
WorkerGlobalScope::ImportScripts(JSContext* aCx,
                                 const Sequence<nsString>& aScriptURLs,
                                 ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  scriptloader::Load(aCx, mWorkerPrivate, aScriptURLs, aRv);
}

int32_t
WorkerGlobalScope::SetTimeout(JSContext* aCx,
                              Function& aHandler,
                              const int32_t aTimeout,
                              const Sequence<JS::Value>& aArguments,
                              ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  return mWorkerPrivate->SetTimeout(aCx, &aHandler, EmptyString(), aTimeout,
                                    aArguments, false, aRv);
}

int32_t
WorkerGlobalScope::SetTimeout(const nsAString& aHandler,
                              const int32_t aTimeout,
                              ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  Sequence<JS::Value> dummy;
  return mWorkerPrivate->SetTimeout(GetCurrentThreadJSContext(), nullptr,
                                    aHandler, aTimeout, dummy, false, aRv);
}

void
WorkerGlobalScope::ClearTimeout(int32_t aHandle, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  mWorkerPrivate->ClearTimeout(aHandle);
}

int32_t
WorkerGlobalScope::SetInterval(JSContext* aCx,
                               Function& aHandler,
                               const Optional<int32_t>& aTimeout,
                               const Sequence<JS::Value>& aArguments,
                               ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int32_t timeout = aTimeout.WasPassed() ? aTimeout.Value() : 0;

  return mWorkerPrivate->SetTimeout(aCx, &aHandler, EmptyString(), timeout,
                                    aArguments, !!timeout, aRv);
}

int32_t
WorkerGlobalScope::SetInterval(const nsAString& aHandler,
                               const Optional<int32_t>& aTimeout,
                               ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  Sequence<JS::Value> dummy;

  int32_t timeout = aTimeout.WasPassed() ? aTimeout.Value() : 0;

  return mWorkerPrivate->SetTimeout(GetCurrentThreadJSContext(), nullptr,
                                    aHandler, timeout, dummy, !!timeout, aRv);
}

void
WorkerGlobalScope::ClearInterval(int32_t aHandle, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  mWorkerPrivate->ClearTimeout(aHandle);
}

void
WorkerGlobalScope::Atob(const nsAString& aAtob, nsAString& aOutput, ErrorResult& aRv) const
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  aRv = nsContentUtils::Atob(aAtob, aOutput);
}

void
WorkerGlobalScope::Btoa(const nsAString& aBtoa, nsAString& aOutput, ErrorResult& aRv) const
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  aRv = nsContentUtils::Btoa(aBtoa, aOutput);
}

void
WorkerGlobalScope::Dump(const Optional<nsAString>& aString) const
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  if (!aString.WasPassed()) {
    return;
  }

  RuntimeService* runtimeService = RuntimeService::GetService();
  MOZ_ASSERT(runtimeService);

  if (!runtimeService->WorkersDumpEnabled()) {
    return;
  }

  NS_ConvertUTF16toUTF8 str(aString.Value());

#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "Gecko", "%s", str.get());
#endif
  fputs(str.get(), stdout);
  fflush(stdout);
}

DedicatedWorkerGlobalScope::DedicatedWorkerGlobalScope(WorkerPrivate* aWorkerPrivate)
: WorkerGlobalScope(aWorkerPrivate)
{
}

/* static */ bool
DedicatedWorkerGlobalScope::Visible(JSContext* aCx, JSObject* aObj)
{
  DedicatedWorkerGlobalScope* self = nullptr;
  nsresult rv = UNWRAP_WORKER_OBJECT(DedicatedWorkerGlobalScope, aCx, aObj, self);
  return NS_SUCCEEDED(rv) && self;
}

JSObject*
DedicatedWorkerGlobalScope::WrapGlobalObject(JSContext* aCx,
                                             JS::CompartmentOptions& aOptions,
                                             JSPrincipals* aPrincipal)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(!mWorkerPrivate->IsSharedWorker());

  // We're wrapping the global, so the scope is undefined.
  JS::Rooted<JSObject*> scope(aCx);

  return DedicatedWorkerGlobalScopeBinding_workers::Wrap(aCx, scope, this,
                                                         this, aOptions,
                                                         aPrincipal);
}

void
DedicatedWorkerGlobalScope::PostMessage(JSContext* aCx,
                                        JS::Handle<JS::Value> aMessage,
                                        const Optional<Sequence<JS::Value>>& aTransferable,
                                        ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  mWorkerPrivate->PostMessageToParent(aCx, aMessage, aTransferable, aRv);
}

SharedWorkerGlobalScope::SharedWorkerGlobalScope(WorkerPrivate* aWorkerPrivate,
                                                 const nsString& aName)
: WorkerGlobalScope(aWorkerPrivate), mName(aName)
{
}

/* static */ bool
SharedWorkerGlobalScope::Visible(JSContext* aCx, JSObject* aObj)
{
  SharedWorkerGlobalScope* self = nullptr;
  nsresult rv = UNWRAP_WORKER_OBJECT(SharedWorkerGlobalScope, aCx, aObj, self);
  return NS_SUCCEEDED(rv) && self;
}

JSObject*
SharedWorkerGlobalScope::WrapGlobalObject(JSContext* aCx,
                                          JS::CompartmentOptions& aOptions,
                                          JSPrincipals* aPrincipal)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(mWorkerPrivate->IsSharedWorker());

  // We're wrapping the global, so the scope is undefined.
  JS::Rooted<JSObject*> scope(aCx);

  return SharedWorkerGlobalScopeBinding_workers::Wrap(aCx, scope, this, this,
                                                      aOptions, aPrincipal);
}

bool
GetterOnlyJSNative(JSContext* aCx, unsigned aArgc, JS::Value* aVp)
{
<<<<<<< HEAD
  if (!aVal.isObject()) {
    return false;
  }

  const JSClass* classPtr = JS_GetClass(&aVal.toObject());

  return classPtr == DedicatedWorkerGlobalScope::Class() ||
         classPtr == SharedWorkerGlobalScope::Class();
}

} /* anonymous namespace */

BEGIN_WORKERS_NAMESPACE

JSObject*
CreateGlobalScope(JSContext* aCx)
{
  using namespace mozilla::dom;

  WorkerPrivate* worker = GetWorkerPrivateFromContext(aCx);
  MOZ_ASSERT(worker);

  const JSClass* classPtr;
  if (worker->IsDedicatedWorker()) {
    classPtr = DedicatedWorkerGlobalScope::Class();
  } else if (worker->IsSharedWorker()) {
    classPtr = SharedWorkerGlobalScope::Class();
  } else {
    MOZ_CRASH("Bad type");
  }

  JS::CompartmentOptions options;
  if (worker->IsChromeWorker()) {
    options.setVersion(JSVERSION_LATEST);
  }

  JS::Rooted<JSObject*> global(aCx,
    JS_NewGlobalObject(aCx, classPtr, GetWorkerPrincipal(),
                       JS::DontFireOnNewGlobalHook, options));
  if (!global) {
    return nullptr;
  }

  JSAutoCompartment ac(aCx, global);

  // Make the private slots now so that all our instance checks succeed.
  if (worker->IsDedicatedWorker()) {
    if (!DedicatedWorkerGlobalScope::InitPrivate(aCx, global, worker)) {
      return nullptr;
    }
  } else if (worker->IsSharedWorker()) {
    if (!SharedWorkerGlobalScope::InitPrivate(aCx, global, worker)) {
      return nullptr;
    }
  } else {
    MOZ_CRASH("Bad type");
  }

  // Proto chain should be:
  //   global -> [Dedicated|Shared]WorkerGlobalScope
  //          -> WorkerGlobalScope
  //          -> EventTarget
  //          -> Object

  JS::Rooted<JSObject*> eventTargetProto(aCx,
    EventTargetBinding_workers::GetProtoObject(aCx, global));
  if (!eventTargetProto) {
    return nullptr;
  }

  JS::Rooted<JSObject*> scopeProto(aCx,
    WorkerGlobalScope::InitClass(aCx, global, eventTargetProto));
  if (!scopeProto) {
    return nullptr;
  }

  JS::Rooted<JSObject*> finalScopeProto(aCx);
  if (worker->IsDedicatedWorker()) {
    finalScopeProto = DedicatedWorkerGlobalScope::InitClass(aCx, global, scopeProto);
  } else if (worker->IsSharedWorker()) {
    finalScopeProto = SharedWorkerGlobalScope::InitClass(aCx, global, scopeProto);
  } else {
    MOZ_CRASH("Bad type");
  }

  if (!finalScopeProto) {
    return nullptr;
  }

  if (!JS_SetPrototype(aCx, global, finalScopeProto)) {
    return nullptr;
  }

  JS::Rooted<JSObject*> workerProto(aCx,
    worker::InitClass(aCx, global, eventTargetProto, false));
  if (!workerProto) {
    return nullptr;
  }

  if (worker->IsChromeWorker()) {
    if (!chromeworker::InitClass(aCx, global, workerProto, false) ||
        !DefineChromeWorkerFunctions(aCx, global) ||
        !DefineOSFileConstants(aCx, global)) {
      return nullptr;
    }
  }

  // Init other classes we care about.
  if (!events::InitClasses(aCx, global, false) ||
      !file::InitClasses(aCx, global)) {
    return nullptr;
  }

  // Init other paris-bindings.
  if (!DOMExceptionBinding::GetConstructorObject(aCx, global) ||
      !EventBinding::GetConstructorObject(aCx, global) ||
      !FileReaderSyncBinding_workers::GetConstructorObject(aCx, global) ||
      !ImageDataBinding::GetConstructorObject(aCx, global) ||
      !TextDecoderBinding::GetConstructorObject(aCx, global) ||
      !TextEncoderBinding::GetConstructorObject(aCx, global) ||
      !XMLHttpRequestBinding_workers::GetConstructorObject(aCx, global) ||
      !XMLHttpRequestUploadBinding_workers::GetConstructorObject(aCx, global) ||
      !URLBinding_workers::GetConstructorObject(aCx, global) ||
      !WorkerLocationBinding_workers::GetConstructorObject(aCx, global) ||
      !WorkerNavigatorBinding_workers::GetConstructorObject(aCx, global)) {
    return nullptr;
  }

  if (!JS_DefineProfilingFunctions(aCx, global)) {
    return nullptr;
  }

  JS_FireOnNewGlobalObject(aCx, global);

  return global;
=======
  JS_ReportErrorNumber(aCx, js_GetErrorMessage, nullptr, JSMSG_GETTER_ONLY);
  return false;
>>>>>>> 7afcc44
}

END_WORKERS_NAMESPACE
