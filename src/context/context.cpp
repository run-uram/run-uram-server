#include "context/context.hpp"

namespace context
{

Context::Context(unsigned int io_threads, unsigned int work_threads)
    : m_io_context(io_threads)
    , m_work_context(work_threads)
    , m_signals(m_io_context.GetIOContext(), SIGINT, SIGTERM)
{
    InitSignals();
}

bool Context::EnableSSL()
{
    m_ssl_context = std::make_unique<SSLContext>();
    return m_ssl_context != nullptr;
}

void Context::InitSignals()
{
    m_signals.async_wait(
        [this](const sys::error_code&, int signal_number)
        {
            LOG_INFO("Signal received: ({}). Initiating graceful shutdown ...", signal_number);
            Stop();
            spdlog::shutdown();
        }
    );
}

void Context::Run()
{
    m_io_context.Run();
}

void Context::Stop()
{
    m_work_context.Stop();
    m_io_context.Stop();
}

} // namespace context
