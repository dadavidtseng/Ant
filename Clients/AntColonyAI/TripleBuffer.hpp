#pragma once

//-----------------------------------------------------------------------------------------------
// TripleBuffer.hpp
//
// Header-only template for thread-safe triple-buffered data exchange between producer and
// consumer threads. Used for EXE<->AI thread communication.
//-----------------------------------------------------------------------------------------------

#include "Common.hpp"

template<typename T>
class TripleBuffer
{
public:
	TripleBuffer() = default;

	// Writer side
	T& GetWriteBuffer() { return m_buffers[m_writeIdx]; }
	void PublishWrite()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::swap(m_writeIdx, m_middleIdx);
		m_newDataAvailable = true;
		m_cv.notify_one();
	}

	// Reader side
	bool TrySwapRead()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_newDataAvailable)
			return false;
		std::swap(m_readIdx, m_middleIdx);
		m_newDataAvailable = false;
		return true;
	}

	const T& GetReadBuffer() const { return m_buffers[m_readIdx]; }

	// Signaling
	void WaitForNewData()
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv.wait(lock, [this] { return m_newDataAvailable || m_shutdown; });
	}

	void SignalShutdown()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_shutdown = true;
		m_cv.notify_all();
	}

	bool IsShutdown() const { return m_shutdown; }

private:
	T m_buffers[3];
	int m_writeIdx = 0;
	int m_middleIdx = 1;
	int m_readIdx = 2;
	bool m_newDataAvailable = false;
	bool m_shutdown = false;
	std::mutex m_mutex;
	std::condition_variable m_cv;
};
