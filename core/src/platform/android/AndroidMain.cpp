#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/input.h>
#include <android/native_window.h>
#include <exception>
#include <SecretEngine/Core.h>
#include <SecretEngine/ICore.h>
#include <SecretEngine/ILogger.h>
#include <SecretEngine/IRenderer.h>

#define LOG_TAG "SecretEngine_Native"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Include the plugin header directly in platform code for convenience
#include "../../../../plugins/AndroidInput/src/InputPlugin.h"

static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (app->userData == nullptr) return 0;
    SecretEngine::ICore* core = static_cast<SecretEngine::ICore*>(app->userData);
    
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t actionCode = action & AMOTION_EVENT_ACTION_MASK;
        
        auto inputPlugin = core->GetCapability("input");
        if (!inputPlugin) return 1;
        auto input = static_cast<SecretEngine::AndroidInput*>(inputPlugin);

        // Get screen dimensions
        float screenWidth = 1080.0f;
        float screenHeight = 2400.0f;
        if (app->window) {
            screenWidth = (float)ANativeWindow_getWidth(app->window);
            screenHeight = (float)ANativeWindow_getHeight(app->window);
        }

        if (actionCode == AMOTION_EVENT_ACTION_MOVE) {
            size_t pointerCount = AMotionEvent_getPointerCount(event);
            for (size_t i = 0; i < pointerCount; i++) {
                float x = AMotionEvent_getX(event, i);
                float y = AMotionEvent_getY(event, i);
                int32_t pointerId = AMotionEvent_getPointerId(event, i);
                input->HandleTouch(action, x, y, pointerId, screenWidth, screenHeight);
            }
        } else {
            int32_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            float x = AMotionEvent_getX(event, pointerIndex);
            float y = AMotionEvent_getY(event, pointerIndex);
            int32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
            input->HandleTouch(action, x, y, pointerId, screenWidth, screenHeight);
        }
        
        return 1;
    }
    
    return 0;
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    if (app->userData == nullptr) {
        LOGE("handle_cmd: app->userData is nullptr!");
        return;
    }
    SecretEngine::ICore* core = static_cast<SecretEngine::ICore*>(app->userData);

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != nullptr) {
                LOGI("APP_CMD_INIT_WINDOW: Window is ready");
                if(core->GetLogger()) {
                    core->GetLogger()->LogInfo("AndroidMain", "Window Ready - Initializing Graphics...");
                }
                
                auto plugin = core->GetCapability("rendering");
                if (plugin) {
                    LOGI("Renderer plugin found, calling InitializeHardware...");
                    SecretEngine::IRenderer* renderer = static_cast<SecretEngine::IRenderer*>(plugin);
                    try {
                        renderer->InitializeHardware(app->window);
                        LOGI("InitializeHardware completed");
                    } catch (...) {
                        LOGE("CRITICAL: Exception during renderer->InitializeHardware()");
                        if(core->GetLogger()) {
                            core->GetLogger()->LogError("AndroidMain", "Renderer initialization failed - continuing without graphics");
                        }
                    }
                } else {
                    LOGE("WARNING: No rendering plugin found!");
                    if(core->GetLogger()) {
                        core->GetLogger()->LogWarning("AndroidMain", "No renderer available - running headless");
                    }
                }
            } else {
                LOGE("APP_CMD_INIT_WINDOW received but app->window is nullptr!");
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW: Window being terminated");
            if(core->GetLogger()) {
                core->GetLogger()->LogInfo("AndroidMain", "Window Terminated");
            }
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGI("APP_CMD_GAINED_FOCUS: App gained focus");
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("APP_CMD_LOST_FOCUS: App lost focus");
            break;
        case APP_CMD_START:
            LOGI("APP_CMD_START: App started");
            break;
        case APP_CMD_RESUME:
            LOGI("APP_CMD_RESUME: App resumed");
            break;
        case APP_CMD_PAUSE:
            LOGI("APP_CMD_PAUSE: App paused");
            break;
        case APP_CMD_STOP:
            LOGI("APP_CMD_STOP: App stopped");
            break;
    }
}

extern "C" {
    // Global android_app pointer for shader loading
    struct android_app* g_AndroidApp = nullptr;
    
    void android_main(struct android_app* state) {
        LOGI("========================================");
        LOGI("SecretEngine android_main() ENTRY POINT");
        LOGI("========================================");

        // 1. Safety check for state
        if (state == nullptr) {
            LOGE("FATAL: android_app state is nullptr!");
            return;
        }
        LOGI("✓ android_app state is valid");
        
        // Store global reference for shader loading
        g_AndroidApp = state;

        // 2. Get Engine Core
        SecretEngine::ICore* core = SecretEngine::GetEngineCore();
        if (core == nullptr) {
            LOGE("FATAL: SecretEngine::GetEngineCore() returned nullptr!");
            return;
        }
        LOGI("✓ Engine Core obtained successfully");

        state->userData = core;
        state->onAppCmd = handle_cmd;
        state->onInputEvent = handle_input; // Register touch input
        
        LOGI("✓ Event handlers registered");

        // 3. Initialize Core with Error Checking
        bool coreInitialized = false;
        try {
            LOGI("Initializing Engine Core...");
            core->Initialize();
            coreInitialized = true;
            LOGI("✓ Core Initialized successfully");
            if (core->GetLogger()) {
                core->GetLogger()->LogInfo("AndroidMain", "Core Initialized - App is running");
            }
        } catch (const std::exception& e) {
            LOGE("EXCEPTION during core->Initialize(): %s", e.what());
        } catch (...) {
            LOGE("UNKNOWN EXCEPTION during core->Initialize()");
        }

        if (!coreInitialized) {
            LOGE("FATAL: Core initialization failed - cannot continue");
            return;
        }

        // 4. PERSISTENT EVENT LOOP
        LOGI("========================================");
        LOGI("Entering Main Event Loop");
        LOGI("App should now be visible and running");
        LOGI("========================================");
        
        int frameCount = 0;
        while (true) {
            int events;
            struct android_poll_source* source;

            // Use -1 (wait) if no window to save battery, 0 to poll when active
            int timeout = (state->window != nullptr) ? 0 : -1;

            // Process all pending events (non-blocking when window is available)
            while ((ALooper_pollOnce(timeout, nullptr, &events, (void**)&source)) >= 0) {
                if (source != nullptr) {
                    source->process(state, source);
                }

                // Check for destroy request
                if (state->destroyRequested != 0) {
                    LOGI("========================================");
                    LOGI("Destroy requested - shutting down cleanly");
                    LOGI("========================================");
                    if (core->GetLogger()) {
                        core->GetLogger()->LogInfo("AndroidMain", "App closing normally");
                    }
                    core->Shutdown();
                    LOGI("✓ Shutdown complete");
                    return;
                }
                
                // After processing one event, break to render a frame
                // This ensures we don't get stuck processing events
                if (state->window != nullptr) {
                    break;
                }
            }

            // Update and render every frame when window is available
            if (state->window != nullptr) {
                try {
                    core->Update();
                    frameCount++;
                    
                    // Log every 300 frames (~5 seconds at 60fps) to confirm app is alive
                    if (frameCount % 300 == 0) {
                        LOGI("App running: %d frames rendered", frameCount);
                    }
                } catch (const std::exception& e) {
                    LOGE("Exception in core->Update(): %s", e.what());
                } catch (...) {
                    LOGE("Unknown exception in core->Update()");
                }
            }
        }
    }
}