#include "Logger.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <utility>

namespace Sick::Backend::System
{
    Logger& Logger::Get() noexcept
    {
        static Logger logger;
        return logger;
    }

    bool Logger::Initialize(Tasking::ThreadPool& pool, std::filesystem::path path) noexcept
    {
        try
        {
            {
                std::scoped_lock fileLock(m_FileMutex);
                std::ofstream output(path, std::ios::out | std::ios::trunc);
                if (!output)
                    return false;
            }

            std::scoped_lock stateLock(m_StateMutex);
            m_Pool = &pool;
            m_Path = std::move(path);
            m_Dropped.store(0, std::memory_order_release);
            m_Ready = true;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    void Logger::Shutdown() noexcept
    {
        std::scoped_lock lock(m_StateMutex);
        m_Ready = false;
        m_Pool = nullptr;
    }

    void Logger::Write(std::string_view message) noexcept
    {
        try
        {
            std::string copy{message};
            Tasking::ThreadPool* pool{};
            {
                std::scoped_lock lock(m_StateMutex);
                if (m_Ready)
                    pool = m_Pool;
            }

            if (!pool)
            {
                WriteImmediate(copy);
                return;
            }

            if (!pool->Submit([this, message = std::move(copy)]() {
                    WriteImmediate(message);
                }))
            {
                m_Dropped.fetch_add(1, std::memory_order_relaxed);
            }
        }
        catch (...)
        {
            m_Dropped.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void Logger::WriteImmediate(std::string_view message) noexcept
    {
        try
        {
            std::filesystem::path path;
            {
                std::scoped_lock lock(m_StateMutex);
                path = m_Path;
            }

            std::scoped_lock fileLock(m_FileMutex);
            std::cout << "[SickMenu] " << message << std::endl;
            if (!path.empty())
            {
                std::ofstream output(path, std::ios::out | std::ios::app);
                if (output)
                    output << "[SickMenu] " << message << '\n';
            }
        }
        catch (...)
        {
        }
    }

    bool Logger::Ready() const noexcept
    {
        std::scoped_lock lock(m_StateMutex);
        return m_Ready;
    }

    std::size_t Logger::Dropped() const noexcept
    {
        return m_Dropped.load(std::memory_order_acquire);
    }
}
