#include "server/session_manager.hpp"
#include "server/user_session.hpp"

namespace server::session
{

void SessionManager::AddSession(uint64_t user_id, std::shared_ptr<UserSession> session)
{
    std::unique_lock lock(m_mutex);
    m_session[user_id] = std::move(session);
}

void SessionManager::RemoveSession(uint64_t user_id)
{
    std::unique_lock lock(m_mutex);
    m_session.erase(user_id);
}

std::shared_ptr<UserSession> SessionManager::GetSession(uint64_t user_id) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_session.find(user_id);
    if (it != m_session.end())
    {
        return it->second;
    }

    return nullptr;
}

bool SessionManager::IsUserOnline(uint64_t user_id) const
{
    std::shared_lock lock(m_mutex);
    return m_session.find(user_id) != m_session.end();
}

void SessionManager::Broadcast(const std::string& bytes, uint64_t exclude_user_id)
{
    std::vector<std::shared_ptr<UserSession>> sessions_snapshot;

    {
        std::shared_lock lock(m_mutex);
        sessions_snapshot.reserve(m_session.size());
        for (const auto& [user_id, session] : m_session)
        {
            if (user_id != exclude_user_id)
            {
                sessions_snapshot.push_back(session);
            }
        }
    }

    for (auto& session : sessions_snapshot)
    {
        net::co_spawn(
            session->GetExecutor(),
            [session, bytes]() -> net::awaitable<void> {
                co_await session->SendAsync(bytes);
            },
            net::detached
        );
    }
}

} // namespace server::session