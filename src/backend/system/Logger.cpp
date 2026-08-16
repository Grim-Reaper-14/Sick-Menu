#include "Logger.hpp"

#include <array>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

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
                m_File.open(path, std::ios::out | std::ios::trunc);
                if (!m_File)
                    return false;
            }

            {
                std::scoped_lock queueLock(m_QueueMutex);
                std::queue<std::string> empty;
                m_Messages.swap(empty);
            }

            std::scoped_lock stateLock(m_StateMutex);
            m_Pool = &pool;
            m_Path = std::move(path);
            m_Dropped.store(0, std::memory_order_release);
            m_DrainScheduled.store(false, std::memory_order_release);
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
        {
            std::scoped_lock lock(m_StateMutex);
            m_Ready = false;
            m_Pool = nullptr;
        }

        FlushQueued();
        m_DrainScheduled.store(false, std::memory_order_release);

        std::scoped_lock fileLock(m_FileMutex);
        if (m_File)
        {
            m_File.flush();
            m_File.close();
        }
    }

    void Logger::Write(std::string_view message) noexcept
    {
        try
        {
            Tasking::ThreadPool* pool{};
            {
                std::scoped_lock lock(m_StateMutex);
                if (m_Ready)
                    pool = m_Pool;
            }

            if (!pool)
            {
                WriteImmediate(message);
                return;
            }

            {
                std::scoped_lock lock(m_QueueMutex);
                if (m_Messages.size() >= MaxPending)
                {
                    m_Dropped.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
                m_Messages.emplace(message);
            }

            ScheduleDrain();
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
            std::scoped_lock fileLock(m_FileMutex);
            std::cout << "[SickMenu] " << message << '\n';
            std::cout.flush();
            if (m_File)
            {
                m_File << "[SickMenu] " << message << '\n';
                m_File.flush();
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

    std::size_t Logger::Pending() const noexcept
    {
        std::scoped_lock lock(m_QueueMutex);
        return m_Messages.size();
    }

    void Logger::ScheduleDrain() noexcept
    {
        if (m_DrainScheduled.exchange(true, std::memory_order_acq_rel))
            return;

        Tasking::ThreadPool* pool{};
        {
            std::scoped_lock lock(m_StateMutex);
            if (m_Ready)
                pool = m_Pool;
        }

        if (!pool || !pool->Submit([this]() { Drain(); }))
            m_DrainScheduled.store(false, std::memory_order_release);
    }

    void Logger::Drain() noexcept
    {
        try
        {
            for (std::size_t batchIndex = 0; batchIndex < MaxBatchesPerDrain; ++batchIndex)
            {
                std::vector<std::string> batch;
                batch.reserve(BatchSize);
                {
                    std::scoped_lock lock(m_QueueMutex);
                    while (!m_Messages.empty() && batch.size() < BatchSize)
                    {
                        batch.push_back(std::move(m_Messages.front()));
                        m_Messages.pop();
                    }
                }

                if (batch.empty())
                    break;

                std::scoped_lock fileLock(m_FileMutex);
                for (const auto& message : batch)
                {
                    std::cout << "[SickMenu] " << message << '\n';
                    if (m_File)
                        m_File << "[SickMenu] " << message << '\n';
                }
                if (m_File)
                    m_File.flush();
            }
        }
        catch (...)
        {
        }

        m_DrainScheduled.store(false, std::memory_order_release);
        if (Pending() != 0)
            ScheduleDrain();
    }

    void Logger::FlushQueued() noexcept
    {
        for (;;)
        {
            std::string message;
            {
                std::scoped_lock lock(m_QueueMutex);
                if (m_Messages.empty())
                    break;
                message = std::move(m_Messages.front());
                m_Messages.pop();
            }

            std::scoped_lock fileLock(m_FileMutex);
            std::cout << "[SickMenu] " << message << '\n';
            if (m_File)
                m_File << "[SickMenu] " << message << '\n';
        }

        std::scoped_lock fileLock(m_FileMutex);
        std::cout.flush();
        if (m_File)
            m_File.flush();
    }
}
