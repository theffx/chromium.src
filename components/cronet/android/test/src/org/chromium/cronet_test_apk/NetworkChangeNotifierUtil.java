// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.cronet_test_apk;

import org.chromium.base.JNINamespace;

/**
 * Wrapper class that contains utility methods to test network change notifier.
 */
@JNINamespace("cronet")
public final class NetworkChangeNotifierUtil {
    public static boolean isTestIPAddressObserverCalled() {
        return nativeIsTestIPAddressObserverCalled();
    }

    private static native boolean nativeIsTestIPAddressObserverCalled();
}
