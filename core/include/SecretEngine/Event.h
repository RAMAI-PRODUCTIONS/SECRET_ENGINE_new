// SecretEngine
// Module: core
// Responsibility: Generic event system for callbacks
// Inspired by: Overload Engine's event system

#pragma once
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace SecretEngine {

/**
 * Generic event system that allows multiple listeners to subscribe to events.
 * Thread-safe for single-threaded usage (not thread-safe for multi-threaded).
 * 
 * Usage:
 *   Event<int, float> myEvent;
 *   auto id = myEvent.AddListener([](int a, float b) { ... });
 *   myEvent.Invoke(42, 3.14f);
 *   myEvent.RemoveListener(id);
 */
template<typename... Args>
class Event {
public:
    using Callback = std::function<void(Args...)>;
    using ListenerID = uint64_t;
    
    /**
     * Add a listener callback to this event
     * @param callback Function to call when event is invoked
     * @return Unique ID for this listener (use to remove later)
     */
    ListenerID AddListener(Callback callback) {
        ListenerID id = m_nextID++;
        m_listeners[id] = callback;
        return id;
    }
    
    /**
     * Remove a listener by ID
     * @param id Listener ID returned from AddListener
     * @return true if listener was found and removed
     */
    bool RemoveListener(ListenerID id) {
        return m_listeners.erase(id) > 0;
    }
    
    /**
     * Remove all listeners
     */
    void RemoveAllListeners() {
        m_listeners.clear();
    }
    
    /**
     * Get the number of active listeners
     */
    size_t GetListenerCount() const {
        return m_listeners.size();
    }
    
    /**
     * Invoke the event, calling all registered listeners
     * @param args Arguments to pass to each listener
     */
    void Invoke(Args... args) {
        // Copy listener map to allow listeners to modify the event during invocation
        auto listenersCopy = m_listeners;
        for (auto& [id, callback] : listenersCopy) {
            // Check if listener still exists (might have been removed during invocation)
            if (m_listeners.find(id) != m_listeners.end()) {
                callback(args...);
            }
        }
    }
    
    /**
     * Operator() overload for convenient invocation
     */
    void operator()(Args... args) {
        Invoke(args...);
    }
    
private:
    std::unordered_map<ListenerID, Callback> m_listeners;
    ListenerID m_nextID = 1;
};

} // namespace SecretEngine
