#pragma once

#include <shared_mutex>
#include <unordered_set>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include "common.hpp"

namespace server::session
{

class UserSession;

/**
 * @brief Manages active network user sessions
 */
class SessionManager : public std::enable_shared_from_this<SessionManager>
{
private:
    using SessionPtr = std::shared_ptr<session::UserSession>;
    using SessionRawPtr = UserSession*;

    // h3_parent_index (res=6) -> Sessions
    std::unordered_map<std::string, std::unordered_set<SessionPtr>> m_zone_subscribers;
    
    // user_id -> Sessions
    std::unordered_map<uint64_t, std::unordered_set<SessionPtr>> m_user_sessions;
    
    // Sessions -> h3_parent_index
    std::unordered_map<SessionRawPtr, std::unordered_set<std::string>> m_session_zones;

    mutable std::shared_mutex m_mutex;

public:
    SessionManager() = default;
    ~SessionManager() = default;

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    void AddSession(uint64_t user_id, SessionPtr session);
    void RemoveSession(uint64_t user_id, SessionRawPtr raw_ptr);

    void UpdateViewportSubscription(SessionPtr session, const std::vector<std::string>& new_h3_zones);

    void BroadcastToZone(const std::string& h3_zone, const std::string& payload);
    void SendToUser(uint64_t user_id, const std::string& payload);
};

} // namespace server::session