// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_IOS_IOS_SERIALIZED_NAVIGATION_DRIVER_H_
#define COMPONENTS_SESSIONS_IOS_IOS_SERIALIZED_NAVIGATION_DRIVER_H_

#include "components/sessions/core/serialized_navigation_driver.h"

template <typename T> struct DefaultSingletonTraits;

namespace sessions {

// Provides an iOS implementation of SerializedNavigationDriver that is backed
// by //ios/web classes.
class IOSSerializedNavigationDriver
    : public SerializedNavigationDriver {
 public:
  virtual ~IOSSerializedNavigationDriver();

  // Returns the singleton IOSSerializedNavigationDriver.  Almost all
  // callers should use SerializedNavigationDriver::Get() instead.
  static IOSSerializedNavigationDriver* GetInstance();

  // SerializedNavigationDriver implementation.
  int GetDefaultReferrerPolicy() const override;
  bool MapReferrerPolicyToOldValues(int referrer_policy,
                                    int* mapped_referrer_policy) const override;
  bool MapReferrerPolicyToNewValues(int referrer_policy,
                                    int* mapped_referrer_policy) const override;
  std::string GetSanitizedPageStateForPickle(
      const SerializedNavigationEntry* navigation) const override;
  void Sanitize(SerializedNavigationEntry* navigation) const override;
  std::string StripReferrerFromPageState(
      const std::string& page_state) const override;

 private:
  IOSSerializedNavigationDriver();
  friend struct DefaultSingletonTraits<IOSSerializedNavigationDriver>;
};

}  // namespace sessions

#endif  // COMPONENTS_SESSIONS_IOS_IOS_SERIALIZED_NAVIGATION_DRIVER_H_
