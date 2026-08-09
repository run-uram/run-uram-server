#pragma once

#include <shared_mutex>
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
class SessionManager
{
private:
    /**
     * @brief Table of active sessions (User ID -> UserSession)
     */
    std::unordered_map<uint64_t, std::shared_ptr<UserSession>> m_session;

    mutable std::shared_mutex m_mutex;

public:
    SessionManager() = default;
    ~SessionManager() = default;

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    void AddSession(uint64_t user_id, std::shared_ptr<UserSession> session);
    void RemoveSession(uint64_t user_id);

    std::shared_ptr<UserSession> GetSession(uint64_t user_id) const;

    bool IsUserOnline(uint64_t user_id) const;

    /**
     * @brief Asynchronously broadcasts to connected users
     * @param bytes Message payload
     * @param exclude_user_id Optional user ID to exclude from the broadcast (e.g., the sender). 
     *                        Defaults to 0 (broadcast to everyone)
     */
    void Broadcast(const std::string& bytes, uint64_t exclude_user_id = 0);
};

} // namespace server::session