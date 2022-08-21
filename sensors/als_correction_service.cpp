/*
 * Copyright (C) 2021 The LineageOS Project
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

#include <android-base/properties.h>
#include <binder/ProcessState.h>
#include <gui/LayerState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SyncScreenCaptureListener.h>
#include <sysutils/FrameworkCommand.h>
#include <sysutils/FrameworkListener.h>
#include <ui/DisplayState.h>

#include <cstdio>
#include <signal.h>
#include <time.h>
#include <unistd.h>

using android::base::GetProperty;
using android::base::SetProperty;
using android::gui::ScreenCaptureResults;
using android::ui::DisplayState;
using android::ui::PixelFormat;
using android::ui::Rotation;
using android::DisplayCaptureArgs;
using android::GraphicBuffer;
using android::IBinder;
using android::Rect;
using android::ScreenshotClient;
using android::sp;
using android::SurfaceComposerClient;
using android::SyncScreenCaptureListener;

static Rect screenshot_rect_0;
static Rect screenshot_rect_land_90;
static Rect screenshot_rect_180;
static Rect screenshot_rect_land_270;

class TakeScreenshotCommand : public FrameworkCommand {
  public:
    TakeScreenshotCommand() : FrameworkCommand("take_screenshot") {}
    ~TakeScreenshotCommand() override = default;

    int runCommand(SocketClient* cli, int /*argc*/, char **/*argv*/) {
        auto screenshot = takeScreenshot();
        cli->sendData(&screenshot, sizeof(screenshot_t));
        return 0;
    }

  private:
    struct screenshot_t {
        uint32_t r, g, b;
        nsecs_t timestamp;
    };

    // See frameworks/base/services/core/jni/com_android_server_display_DisplayControl.cpp and
    // frameworks/base/core/java/android/view/SurfaceControl.java
    static sp<IBinder> getInternalDisplayToken() {
        const auto displayIds = SurfaceComposerClient::getPhysicalDisplayIds();
        sp<IBinder> token = SurfaceComposerClient::getPhysicalDisplayToken(displayIds[0]);
        return token;
    }

    screenshot_t takeScreenshot() {
        DisplayCaptureArgs captureArgs;
        captureArgs.displayToken = getInternalDisplayToken();
        captureArgs.pixelFormat = PixelFormat::RGBA_8888;

        android::ui::DisplayState state;
        SurfaceComposerClient::getDisplayState(captureArgs.displayToken, &state);
        Rect screenshot_rect;
        switch (state.orientation) {
             case Rotation::Rotation90:  screenshot_rect = screenshot_rect_land_90;
                                         break;
             case Rotation::Rotation180: screenshot_rect = screenshot_rect_180;
                                         break;
             case Rotation::Rotation270: screenshot_rect = screenshot_rect_land_270;
                                         break;
             default:                    screenshot_rect = screenshot_rect_0;
                                         break;
        }

        static sp<GraphicBuffer> outBuffer = new GraphicBuffer(
        screenshot_rect.getWidth(), screenshot_rect.getHeight(),
        android::PIXEL_FORMAT_RGB_888,
        GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN);

        captureArgs.sourceCrop = screenshot_rect;
        captureArgs.width = screenshot_rect.getWidth();
        captureArgs.height = screenshot_rect.getHeight();
        captureArgs.useIdentityTransform = true;
        captureArgs.captureSecureLayers = true;

        sp<SyncScreenCaptureListener> captureListener = new SyncScreenCaptureListener();
        if (ScreenshotClient::captureDisplay(captureArgs, captureListener) == android::NO_ERROR) {
            ScreenCaptureResults captureResults = captureListener->waitForResults();
            ALOGV("Capture results received");
            if (captureResults.fenceResult.ok()) {
                outBuffer = captureResults.buffer;
            }
        }

        uint8_t *out;
        auto resultWidth = outBuffer->getWidth();
        auto resultHeight = outBuffer->getHeight();
        auto stride = outBuffer->getStride();

        outBuffer->lock(GraphicBuffer::USAGE_SW_READ_OFTEN, reinterpret_cast<void **>(&out));
        // we can sum this directly on linear light
        uint32_t rsum = 0, gsum = 0, bsum = 0;
        for (int y = 0; y < resultHeight; y++) {
            for (int x = 0; x < resultWidth; x++) {
                rsum += out[y * (stride * 4) + x * 4];
                gsum += out[y * (stride * 4) + x * 4 + 1];
                bsum += out[y * (stride * 4) + x * 4 + 2];
            }
        }
        uint32_t max = resultWidth * resultHeight;
        outBuffer->unlock();

        return { rsum / max, gsum / max, bsum / max, systemTime(SYSTEM_TIME_BOOTTIME) };
    }
};

class AlsCorrectionListener : public FrameworkListener {
  public:
    AlsCorrectionListener() : FrameworkListener("als_correction") {
        registerCmd(new TakeScreenshotCommand);
    }
};

int main() {
    int32_t left, top, right, bottom;
    int32_t ALS_RADIUS = 40;
    std::istringstream is(GetProperty("vendor.sensors.als_correction.grabrect", ""));

    is >> left >> top;
    if (left == 0) {
        ALOGE("No screenshot grab area config");
        return 0;
    }

    //TODO Reimplement rotation calculation
    screenshot_rect_0 = Rect(left - ALS_RADIUS, top - ALS_RADIUS, left + ALS_RADIUS, top + ALS_RADIUS);
    screenshot_rect_land_90 = Rect(top - ALS_RADIUS, 1080 - left - ALS_RADIUS, top + ALS_RADIUS, 1080 - left + ALS_RADIUS);
    screenshot_rect_180 = Rect(1080-left - ALS_RADIUS, 2400-top - ALS_RADIUS, 1080-left + ALS_RADIUS, 2400-top + ALS_RADIUS);
    screenshot_rect_land_270 = Rect(2400 - (top + ALS_RADIUS),left - ALS_RADIUS, 2400 - (top - ALS_RADIUS), left + ALS_RADIUS);

    ALOGI("Screenshot grab area rot=0: %d %d %d %d", screenshot_rect_0.left, screenshot_rect_0.right, screenshot_rect_0.top, screenshot_rect_0.bottom);
    ALOGI("Screenshot grab area rot_land=90: %d %d %d %d", screenshot_rect_land_90.left, screenshot_rect_land_90.right, screenshot_rect_land_90.top, screenshot_rect_land_90.bottom);
    ALOGI("Screenshot grab area rot=180: %d %d %d %d", screenshot_rect_180.left, screenshot_rect_180.right, screenshot_rect_180.top, screenshot_rect_180.bottom);
    ALOGI("Screenshot grab area rot_land_270: %d %d %d %d", screenshot_rect_land_270.left, screenshot_rect_land_270.right, screenshot_rect_land_270.top, screenshot_rect_land_270.bottom);

    auto listener = new AlsCorrectionListener();
    listener->startListener();

    while (true) {
        pause();
    }

    return 0;
}
