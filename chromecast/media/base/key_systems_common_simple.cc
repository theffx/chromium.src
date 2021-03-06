// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/base/key_systems_common.h"

namespace chromecast {
namespace media {

CastKeySystem GetPlatformKeySystemByName(const std::string& key_system_name) {
  return KEY_SYSTEM_NONE;
}

}  // namespace media
}  // namespace chromecast
