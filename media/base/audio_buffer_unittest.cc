// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "media/base/audio_buffer.h"
#include "media/base/audio_bus.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

template <class T>
static scoped_refptr<AudioBuffer> MakeInterleavedBuffer(
    SampleFormat format,
    int channels,
    T start,
    T increment,
    int frames,
    const base::TimeDelta start_time) {
  DCHECK(format == kSampleFormatU8 || format == kSampleFormatS16 ||
         format == kSampleFormatS32 || format == kSampleFormatF32);

  // Create a block of memory with values:
  //   start
  //   start + increment
  //   start + 2 * increment, ...
  // Since this is interleaved data, channel 0 data will be:
  //   start
  //   start + channels * increment
  //   start + 2 * channels * increment, ...
  int buffer_size = frames * channels * sizeof(T);
  scoped_ptr<uint8[]> memory(new uint8[buffer_size]);
  uint8* data[] = { memory.get() };
  T* buffer = reinterpret_cast<T*>(memory.get());
  for (int i = 0; i < frames * channels; ++i) {
    buffer[i] = start;
    start += increment;
  }
  // Duration is 1 second per frame (for simplicity).
  base::TimeDelta duration = base::TimeDelta::FromSeconds(frames);
  return AudioBuffer::CopyFrom(
      format, channels, frames, data, start_time, duration);
}

template <class T>
static scoped_refptr<AudioBuffer> MakePlanarBuffer(
    SampleFormat format,
    int channels,
    T start,
    T increment,
    int frames,
    const base::TimeDelta start_time) {
  DCHECK(format == kSampleFormatPlanarF32 || format == kSampleFormatPlanarS16);

  // Create multiple blocks of data, once for each channel.
  // Values in channel 0 will be:
  //   start
  //   start + increment
  //   start + 2 * increment, ...
  // Values in channel 1 will be:
  //   start + frames * increment
  //   start + (frames + 1) * increment
  //   start + (frames + 2) * increment, ...
  int buffer_size = frames * sizeof(T);
  scoped_ptr<uint8*[]> data(new uint8*[channels]);
  scoped_ptr<uint8[]> memory(new uint8[channels * buffer_size]);
  for (int i = 0; i < channels; ++i) {
    data.get()[i] = memory.get() + i * buffer_size;
    T* buffer = reinterpret_cast<T*>(data.get()[i]);
    for (int j = 0; j < frames; ++j) {
      buffer[j] = start;
      start += increment;
    }
  }
  // Duration is 1 second per frame (for simplicity).
  base::TimeDelta duration = base::TimeDelta::FromSeconds(frames);
  return AudioBuffer::CopyFrom(
      format, channels, frames, data.get(), start_time, duration);
}

static void VerifyResult(float* channel_data,
                         int frames,
                         float start,
                         float increment) {
  for (int i = 0; i < frames; ++i) {
    SCOPED_TRACE(base::StringPrintf(
        "i=%d/%d start=%f, increment=%f", i, frames, start, increment));
    ASSERT_EQ(channel_data[i], start);
    start += increment;
  }
}

TEST(AudioBufferTest, CopyFrom) {
  const int channels = 1;
  const int frames = 8;
  const base::TimeDelta start_time;
  scoped_refptr<AudioBuffer> buffer = MakeInterleavedBuffer<uint8>(
      kSampleFormatU8, channels, 1, 1, frames, start_time);
  EXPECT_EQ(frames, buffer->frame_count());
  EXPECT_EQ(buffer->timestamp(), start_time);
  EXPECT_EQ(buffer->duration().InSeconds(), frames);
  EXPECT_FALSE(buffer->end_of_stream());
}

TEST(AudioBufferTest, CreateEOSBuffer) {
  scoped_refptr<AudioBuffer> buffer = AudioBuffer::CreateEOSBuffer();
  EXPECT_TRUE(buffer->end_of_stream());
}

TEST(AudioBufferTest, FrameSize) {
  const uint8 kTestData[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                              15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                              27, 28, 29, 30, 31 };
  const base::TimeDelta kTimestampA = base::TimeDelta::FromMicroseconds(1337);
  const base::TimeDelta kTimestampB = base::TimeDelta::FromMicroseconds(1234);

  const uint8* const data[] = { kTestData };
  scoped_refptr<AudioBuffer> buffer = AudioBuffer::CopyFrom(
      kSampleFormatU8, 2, 16, data, kTimestampA, kTimestampB);
  EXPECT_EQ(16, buffer->frame_count());  // 2 channels of 8-bit data

  buffer = AudioBuffer::CopyFrom(
      kSampleFormatF32, 4, 2, data, kTimestampA, kTimestampB);
  EXPECT_EQ(2, buffer->frame_count());  // now 4 channels of 32-bit data
}

TEST(AudioBufferTest, ReadU8) {
  const int channels = 4;
  const int frames = 4;
  const base::TimeDelta start_time;
  scoped_refptr<AudioBuffer> buffer = MakeInterleavedBuffer<uint8>(
      kSampleFormatU8, channels, 128, 1, frames, start_time);

  // Read all 4 frames from the buffer. Data is interleaved, so ch[0] should be
  // 128, 132, 136, 140, other channels similar. However, values are converted
  // from [0, 255] to [-1.0, 1.0] with a bias of 128. Thus the first buffer
  // value should be 0.0, then 1/127, 2/127, etc.
  scoped_ptr<AudioBus> bus = AudioBus::Create(channels, 100);
  buffer->ReadFrames(frames, 0, 0, bus.get());
  VerifyResult(bus->channel(0), frames, 0.0f, 4.0f / 127.0f);
  VerifyResult(bus->channel(1), frames, 1.0f / 127.0f, 4.0f / 127.0f);
  VerifyResult(bus->channel(2), frames, 2.0f / 127.0f, 4.0f / 127.0f);
  VerifyResult(bus->channel(3), frames, 3.0f / 127.0f, 4.0f / 127.0f);
}

TEST(AudioBufferTest, ReadS16) {
  const int channels = 2;
  const int frames = 10;
  const base::TimeDelta start_time;
  scoped_refptr<AudioBuffer> buffer = MakeInterleavedBuffer<int16>(
      kSampleFormatS16, channels, 1, 1, frames, start_time);

  // Read 6 frames from the buffer. Data is interleaved, so ch[0] should be 1,
  // 3, 5, 7, 9, 11, and ch[1] should be 2, 4, 6, 8, 10, 12. Data is converted
  // to float from -1.0 to 1.0 based on int16 range.
  scoped_ptr<AudioBus> bus = AudioBus::Create(channels, 100);
  buffer->ReadFrames(6, 0, 0, bus.get());
  VerifyResult(bus->channel(0), 6, 1.0f / kint16max, 2.0f / kint16max);
  VerifyResult(bus->channel(1), 6, 2.0f / kint16max, 2.0f / kint16max);

  // Now read the same data one frame at a time.
  bus = AudioBus::Create(channels, 100);
  for (int i = 0; i < frames; ++i) {
    buffer->ReadFrames(1, i, i, bus.get());
  }
  VerifyResult(bus->channel(0), frames, 1.0f / kint16max, 2.0f / kint16max);
  VerifyResult(bus->channel(1), frames, 2.0f / kint16max, 2.0f / kint16max);
}

TEST(AudioBufferTest, ReadS32) {
  const int channels = 2;
  const int frames = 6;
  const base::TimeDelta start_time;
  scoped_refptr<AudioBuffer> buffer = MakeInterleavedBuffer<int32>(
      kSampleFormatS32, channels, 1, 1, frames, start_time);

  // Read 6 frames from the buffer. Data is interleaved, so ch[0] should be 1,
  // 3, 5, 7, 9, 11, and ch[1] should be 2, 4, 6, 8, 10, 12. Data is converted
  // to float from -1.0 to 1.0 based on int32 range.
  scoped_ptr<AudioBus> bus = AudioBus::Create(channels, 100);
  buffer->ReadFrames(frames, 0, 0, bus.get());
  VerifyResult(bus->channel(0), frames, 1.0f / kint32max, 2.0f / kint32max);
  VerifyResult(bus->channel(1), frames, 2.0f / kint32max, 2.0f / kint32max);

  // Now read 2 frames starting at frame offset 3. ch[0] should be 7, 9, and
  // ch[1] should be 8, 10.
  buffer->ReadFrames(2, 3, 0, bus.get());
  VerifyResult(bus->channel(0), 2, 7.0f / kint32max, 2.0f / kint32max);
  VerifyResult(bus->channel(1), 2, 8.0f / kint32max, 2.0f / kint32max);
}

TEST(AudioBufferTest, ReadF32) {
  const int channels = 2;
  const int frames = 20;
  const base::TimeDelta start_time;
  scoped_refptr<AudioBuffer> buffer = MakeInterleavedBuffer<float>(
      kSampleFormatF32, channels, 1.0f, 1.0f, frames, start_time);

  // Read first 10 frames from the buffer. F32 is interleaved, so ch[0] should
  // be 1, 3, 5, ... and ch[1] should be 2, 4, 6, ...
  scoped_ptr<AudioBus> bus = AudioBus::Create(channels, 100);
  buffer->ReadFrames(10, 0, 0, bus.get());
  VerifyResult(bus->channel(0), 10, 1.0f, 2.0f);
  VerifyResult(bus->channel(1), 10, 2.0f, 2.0f);

  // Read second 10 frames.
  bus = AudioBus::Create(channels, 100);
  buffer->ReadFrames(10, 10, 0, bus.get());
  VerifyResult(bus->channel(0), 10, 21.0f, 2.0f);
  VerifyResult(bus->channel(1), 10, 22.0f, 2.0f);
}

TEST(AudioBufferTest, ReadS16Planar) {
  const int channels = 2;
  const int frames = 20;
  const base::TimeDelta start_time;
  scoped_refptr<AudioBuffer> buffer = MakePlanarBuffer<int16>(
      kSampleFormatPlanarS16, channels, 1, 1, frames, start_time);

  // Read 6 frames from the buffer. Data is planar, so ch[0] should be 1, 2, 3,
  // 4, 5, 6, and ch[1] should be 21, 22, 23, 24, 25, 26. Data is converted to
  // float from -1.0 to 1.0 based on int16 range.
  scoped_ptr<AudioBus> bus = AudioBus::Create(channels, 100);
  buffer->ReadFrames(6, 0, 0, bus.get());
  VerifyResult(bus->channel(0), 6, 1.0f / kint16max, 1.0f / kint16max);
  VerifyResult(bus->channel(1), 6, 21.0f / kint16max, 1.0f / kint16max);

  // Read all the frames backwards, one by one. ch[0] should be 20, 19, ...
  bus = AudioBus::Create(channels, 100);
  for (int i = 0; i < frames; ++i) {
    buffer->ReadFrames(1, frames - i - 1, i, bus.get());
  }
  VerifyResult(bus->channel(0), frames, 20.0f / kint16max, -1.0f / kint16max);
  VerifyResult(bus->channel(1), frames, 40.0f / kint16max, -1.0f / kint16max);

  // Read 0 frames with different offsets. Existing data in AudioBus should be
  // unchanged.
  buffer->ReadFrames(0, 0, 0, bus.get());
  buffer->ReadFrames(0, 0, 10, bus.get());
  buffer->ReadFrames(0, 10, 0, bus.get());
  VerifyResult(bus->channel(0), frames, 20.0f / kint16max, -1.0f / kint16max);
  VerifyResult(bus->channel(1), frames, 40.0f / kint16max, -1.0f / kint16max);
}

TEST(AudioBufferTest, ReadF32Planar) {
  const int channels = 4;
  const int frames = 100;
  const base::TimeDelta start_time;
  scoped_refptr<AudioBuffer> buffer = MakePlanarBuffer<float>(
      kSampleFormatPlanarF32, channels, 1.0f, 1.0f, frames, start_time);

  // Read all 100 frames from the buffer. F32 is planar, so ch[0] should be 1,
  // 2, 3, 4, ..., ch[1] should be 101, 102, 103, ..., and so on for all 4
  // channels.
  scoped_ptr<AudioBus> bus = AudioBus::Create(channels, 100);
  buffer->ReadFrames(frames, 0, 0, bus.get());
  VerifyResult(bus->channel(0), frames, 1.0f, 1.0f);
  VerifyResult(bus->channel(1), frames, 101.0f, 1.0f);
  VerifyResult(bus->channel(2), frames, 201.0f, 1.0f);
  VerifyResult(bus->channel(3), frames, 301.0f, 1.0f);

  // Now read 20 frames from the middle of the buffer.
  bus = AudioBus::Create(channels, 100);
  buffer->ReadFrames(20, 50, 0, bus.get());
  VerifyResult(bus->channel(0), 20, 51.0f, 1.0f);
  VerifyResult(bus->channel(1), 20, 151.0f, 1.0f);
  VerifyResult(bus->channel(2), 20, 251.0f, 1.0f);
  VerifyResult(bus->channel(3), 20, 351.0f, 1.0f);
}

}  // namespace media
