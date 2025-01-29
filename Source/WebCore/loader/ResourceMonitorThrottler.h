/*
 * Copyright (C) 2025 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/CompletionHandler.h>
#include <wtf/ContinuousApproximateTime.h>
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/PriorityQueue.h>
#include <wtf/ThreadSafeWeakPtr.h>
#include <wtf/Vector.h>
#include <wtf/WorkQueue.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ResourceMonitorPersistence;

class ResourceMonitorThrottler final : public ThreadSafeRefCountedAndCanMakeThreadSafeWeakPtr<ResourceMonitorThrottler, WTF::DestructionThread::MainRunLoop> {
public:
    WEBCORE_EXPORT static Ref<ResourceMonitorThrottler> create();
    WEBCORE_EXPORT static Ref<ResourceMonitorThrottler> create(size_t count, Seconds duration, size_t maxHosts);
    WEBCORE_EXPORT static Ref<ResourceMonitorThrottler> create(const String& databasePath);
    WEBCORE_EXPORT static Ref<ResourceMonitorThrottler> create(const String& databasePath, size_t count, Seconds duration, size_t maxHosts);

    WEBCORE_EXPORT ~ResourceMonitorThrottler();

    WEBCORE_EXPORT void tryAccess(const String& host, ContinuousApproximateTime, CompletionHandler<void(bool)>&&);

    WEBCORE_EXPORT void setCountPerDuration(size_t, Seconds);

    static constexpr size_t defaultThrottleAccessCount = 5;
    static constexpr Seconds defaultThrottleDuration = 24_h;
    static constexpr size_t defaultMaxHosts = 100;

private:
    struct Config {
        size_t count;
        Seconds duration;
        size_t maxHosts;
    };

    class AccessThrottler final {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        AccessThrottler() = default;

        bool tryAccessAndUpdateHistory(ContinuousApproximateTime, const Config&);
        bool tryExpire(ContinuousApproximateTime, const Config&);
        ContinuousApproximateTime oldestAccessTime() const;
        ContinuousApproximateTime newestAccessTime() const { return m_newestAccessTime; }

    private:
        void removeExpired(ContinuousApproximateTime);

        PriorityQueue<ContinuousApproximateTime> m_accessTimes;
        ContinuousApproximateTime m_newestAccessTime { -ContinuousApproximateTime::infinity() };
    };

    class ThrottlerContainer final {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        ThrottlerContainer(String&& databasePath, size_t count, Seconds duration, size_t maxHosts);
        ~ThrottlerContainer();

        void openDatabase(String&& path);
        void closeDatabase();
        void recordAccess(const String& host, ContinuousApproximateTime);

        bool tryAccess(const String& host, ContinuousApproximateTime);

        void setCountPerDuration(size_t count, Seconds duration);

    private:
        AccessThrottler& throttlerForHost(const String& host);
        void removeExpiredThrottler();
        void removeOldestThrottler();
        void maintainHosts(ContinuousApproximateTime);

        Config m_config;
        HashMap<String, AccessThrottler> m_throttlersByHost;
        std::unique_ptr<ResourceMonitorPersistence> m_persistence;
    };

    // Shared WorkQueue is used to prevent race condition when delete and create Throttler for same database file.
    static Ref<WorkQueue> sharedWorkQueueSingleton()
    {
        static LazyNeverDestroyed<Ref<WorkQueue>> workQueue;
        static std::once_flag onceKey;
        std::call_once(onceKey, [] {
            workQueue.construct(WorkQueue::create("ResourceMonitorThrottler Work Queue"_s));
        });
        return workQueue.get();
    }

    ResourceMonitorThrottler(const String& databasePath, size_t count, Seconds duration, size_t maxHosts);

    std::unique_ptr<ThrottlerContainer> m_container;
};

}
