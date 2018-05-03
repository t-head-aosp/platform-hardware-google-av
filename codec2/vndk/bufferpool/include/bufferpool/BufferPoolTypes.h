/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_MEDIA_BUFFERPOOL_V1_0_BUFFERPOOLTYPES_H
#define ANDROID_HARDWARE_MEDIA_BUFFERPOOL_V1_0_BUFFERPOOLTYPES_H

#include <android/hardware/media/bufferpool/1.0/IAccessor.h>
#include <android/hardware/media/bufferpool/1.0/types.h>
#include <cutils/native_handle.h>
#include <fmq/MessageQueue.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace media {
namespace bufferpool {

struct BufferPoolData {
    // For local use, to specify a bufferpool (client connection) for buffers.
    // Return value from connect#IAccessor(android.hardware.media.bufferpool@1.0).
    // high 32 bit = pid of IAccessor : low 32 bit = sequence Id for connect
    int64_t mConnectionId;
    // BufferId
    uint32_t mId;
    // Accessor accosiated to the buffer.
    wp<V1_0::IAccessor> mAccessor;
    // mHandle is owned if not null, i.e., it will be closed and deleted in
    // ~BufferPoolData().
    native_handle_t *mHandle;

    BufferPoolData() : mConnectionId(0), mId(0), mAccessor(nullptr), mHandle(NULL) {}

    BufferPoolData(
            int64_t connectionId, uint32_t id,
            wp<V1_0::IAccessor> accessor,
            native_handle_t *handle)
            : mConnectionId(connectionId), mId(id), mAccessor(accessor), mHandle(handle) {}

    ~BufferPoolData() {
        if (mHandle) {
            native_handle_close(mHandle);
            native_handle_delete(mHandle);
        }
    }
};

namespace V1_0 {
namespace implementation {

using ::android::hardware::kSynchronizedReadWrite;

typedef uint32_t BufferId;
typedef uint64_t TransactionId;
typedef int64_t ConnectionId;

typedef android::hardware::MessageQueue<BufferStatusMessage, kSynchronizedReadWrite> BufferStatusQueue;
typedef BufferStatusQueue::Descriptor QueueDescriptor;

/**
 * Allocation wrapper class for buffer pool.
 */
struct BufferPoolAllocation {
    const native_handle_t *mHandle;

    const native_handle_t *handle() {
        return mHandle;
    }

    BufferPoolAllocation(const native_handle_t *handle) : mHandle(handle) {}

    ~BufferPoolAllocation() {};
};

/**
 * Allocator wrapper class for buffer pool.
 */
class BufferPoolAllocator {
public:

    /**
     * Allocate an allocation(buffer) for buffer pool.
     *
     * @param params    allocation parameters
     * @param alloc     created allocation
     *
     * @return OK when an allocation is created successfully.
     */
    virtual ResultStatus allocate(
            const std::vector<uint8_t> &params,
            std::shared_ptr<BufferPoolAllocation> *alloc) = 0;

    /**
     * Returns whether allocation parameters of an old allocation are
     * compatible with new allocation parameters.
     */
    virtual bool compatible(const std::vector<uint8_t> &newParams,
                            const std::vector<uint8_t> &oldParams) = 0;

protected:
    BufferPoolAllocator() = default;

    virtual ~BufferPoolAllocator() = default;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace bufferpool
}  // namespace media
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_MEDIA_BUFFERPOOL_V1_0_BUFFERPOOLTYPES_H
