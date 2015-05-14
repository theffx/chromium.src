// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_ANDROID_CRONET_UPLOAD_DATA_STREAM_DELEGATE_H_
#define COMPONENTS_CRONET_ANDROID_CRONET_UPLOAD_DATA_STREAM_DELEGATE_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/cronet/android/cronet_upload_data_stream.h"
#include "net/base/io_buffer.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace cronet {

// The Delegate holds onto a reference to the IOBuffer that is currently being
// written to in Java, so may not be deleted until any read operation in Java
// has completed.
//
// The Delegate is owned by the Java CronetUploadDataStream, and also owns a
// reference to it. The Delegate is only destroyed after the net::URLRequest
// destroys the C++ CronetUploadDataStream and the Java CronetUploadDataStream
// has no read operation pending, at which point it also releases its reference
// to the Java CronetUploadDataStream.
//
// Failures don't go through the delegate, but directly to the Java request
// object, since normally reads aren't allowed to fail during an upload.
class CronetUploadDataStreamDelegate : public CronetUploadDataStream::Delegate {
 public:
  CronetUploadDataStreamDelegate(JNIEnv* env, jobject jupload_data_stream);
  ~CronetUploadDataStreamDelegate() override;

  // CronetUploadDataStream::Delegate implementation.  Called on network thread.
  void InitializeOnNetworkThread(
      base::WeakPtr<CronetUploadDataStream> upload_data_stream) override;
  void Read(net::IOBuffer* buffer, int buf_len) override;
  void Rewind() override;
  void OnUploadDataStreamDestroyed() override;

  // Callbacks from Java, called on some Java thread.
  void OnReadSucceeded(JNIEnv* env,
                       jobject obj,
                       int bytes_read,
                       bool final_chunk);
  void OnRewindSucceeded(JNIEnv* env, jobject obj);

 private:
  // Initialized on construction, effectively constant.
  base::android::ScopedJavaGlobalRef<jobject> jupload_data_stream_;

  // These are initialized in InitializeOnNetworkThread, so are safe to access
  // during Java callbacks, which all happen after initialization.
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;
  base::WeakPtr<CronetUploadDataStream> upload_data_stream_;

  // Used to keep the read buffer alive until the callback from Java has been
  // received.
  scoped_refptr<net::IOBuffer> buffer_;

  DISALLOW_COPY_AND_ASSIGN(CronetUploadDataStreamDelegate);
};

// Explicitly register static JNI functions.
bool CronetUploadDataStreamDelegateRegisterJni(JNIEnv* env);

}  // namespace cronet

#endif  // COMPONENTS_CRONET_ANDROID_CRONET_UPLOAD_DATA_STREAM_DELEGATE_H_
