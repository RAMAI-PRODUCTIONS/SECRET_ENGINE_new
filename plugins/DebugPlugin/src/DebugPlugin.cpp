#include "DebugPlugin.h"

extern "C" {
    SecretEngine::IPlugin* CreateDebugPlugin() {
        return new SecretEngine::DebugPlugin();
    }
}
