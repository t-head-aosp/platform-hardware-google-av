/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_MEDIA_ECO_SESSION_H_
#define ANDROID_MEDIA_ECO_SESSION_H_

#include <android/media/eco/BnECOSession.h>
#include <android/media/eco/IECOServiceStatsProvider.h>
#include <binder/BinderService.h>

#include <condition_variable>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

#include "ECOData.h"
#include "ECOServiceInfoListener.h"
#include "ECOServiceStatsProvider.h"
#include "ECOUtils.h"

namespace android {
namespace media {
namespace eco {

using ::android::binder::Status;

/**
 * ECO Session.
 *
 * ECOSession is created by ECOService to manage an encoding session. Both the providers and
 * listeners should interact with ECO session after obtain it from ECOService. For ECOService 1.0,
 * it only supports resolution of up to 720P and only for camera recording use case. Also, it only
 * supports encoder as the provider and camera as listener.
 */
class ECOSession : public BinderService<ECOSession>,
                   public BnECOSession,
                   public virtual IBinder::DeathRecipient {
    friend class BinderService<ECOSession>;

public:
    virtual ~ECOSession();

    virtual Status addStatsProvider(const sp<IECOServiceStatsProvider>& provider,
                                    const ECOData& statsConfig, /*out*/ bool* status);

    virtual Status removeStatsProvider(const sp<IECOServiceStatsProvider>&, bool*);

    virtual Status addInfoListener(const sp<IECOServiceInfoListener>&,
                                   const ECOData& listenerConfig,
                                   /*out*/ bool* status);

    virtual Status removeInfoListener(const sp<IECOServiceInfoListener>&, bool*);

    virtual Status pushNewStats(const ECOData&, bool*);

    virtual Status getWidth(int32_t* _aidl_return);

    virtual Status getHeight(int32_t* _aidl_return);

    virtual Status getNumOfListeners(int32_t*);

    virtual Status getNumOfProviders(int32_t*);

    // IBinder::DeathRecipient implementation
    virtual void binderDied(const wp<IBinder>& who);

    // Grant permission to EcoSessionTest to run test.
    friend class EcoSessionTest;

    // Let ECOService create the session.
    friend class ECOService;

protected:
    static android::sp<ECOSession> createECOSession(int32_t width, int32_t height,
                                                    bool isCameraRecording);

private:
    // Only the  ECOService could create ECOSession.
    ECOSession(int32_t width, int32_t height, bool isCameraRecording);

    // Start the main thread for processing the stats and pushing info to listener.
    static void startThread(ECOSession* session);

    void run();

    bool processStats(const ECOData& stats);

    // Process the session stats received from provider.
    bool processSessionStats(const ECOData& stats);

    // Process the frame stats received from provider.
    bool processFrameStats(const ECOData& stats);

    std::atomic<bool> mStopThread;

    std::mutex mStatsQueueLock;
    std::deque<ECOData> mStatsQueue;  // GUARDED_BY(mStatsQueueLock)
    std::condition_variable mStatsQueueWaitCV;

    constexpr static int32_t ENCODER_MIN_QP = 0;
    constexpr static int32_t ENCODER_MAX_QP = 51;

    // Save the QP last reported to the listener. Init to be 0.
    int32_t mLastReportedQp;

    typedef struct QpRange {
        int32_t mQpBlocknessThreshold = 50;
        int32_t mQpChangeThreshold = 50;
    } QpCondition;
    QpCondition mListenerQpCondition;

    android::sp<IECOServiceInfoListener> mListener;
    android::sp<IECOServiceStatsProvider> mProvider;

    // Main thread for processing the events from provider.
    std::thread mThread;

    // Width of the encoding session in number of pixels.
    int32_t mWidth;

    // Height of the encoding session in number of pixels.
    int32_t mHeight;

    // Whether the encoding is for camera recording.
    bool mIsCameraRecording;

    // Encoder codec type of the encoding session. -1 means not available.
    int32_t mCodecType;

    // Encoder codec profile. -1 means not available.
    int32_t mCodecProfile;

    // Encoder codec level. -1 means not available.
    int32_t mCodecLevel;

    // Target bitrate in bits per second. This should be provided by the provider. -1 means not
    // available.
    int32_t mBitrateBps;

    // Key frame interval in number of frames. -1 means not available.
    int32_t mKeyFrameIntervalFrames;

    // Frame rate in frames per second. -1 means not available.
    float mFramerateFps;
};

}  // namespace eco
}  // namespace media
}  // namespace android

#endif  // ANDROID_MEDIA_ECO_SESSION_H_
