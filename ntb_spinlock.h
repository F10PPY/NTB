#ifndef NTB_SPINLOCK_H
#define NTB_SPINLOCK_H
#include <atomic>
namespace ntb
{
class Spinlock
{
public:
	inline void lock() noexcept
    {
		while (m_lock.test_and_set(std::memory_order_acquire))
		{
			cpuRelax();
		}
	}
	inline bool tryLock() noexcept
	{
		return !m_lock.test_and_set(std::memory_order_acquire);
	}
	inline void unlock() noexcept
	{
		m_lock.clear(std::memory_order_release);
	}
private:
	std::atomic_flag m_lock=ATOMIC_FLAG_INIT;
	inline void cpuRelax() noexcept
	{
#if defined(__arc__) || defined(__mips__) || defined(__arm__) || defined(__powerpc__)
		asm volatile("" ::: "memory");
#elif defined(__i386__) || defined(__x86_64__)
		asm volatile("rep; nop" ::: "memory");
#elif defined(__aarch64__)
		asm volatile("yield" ::: "memory");
#elif defined(__ia64__)
		asm volatile ("hint @pause" ::: "memory");

#elif defined(_WIN32)
#if defined(_MSC_VER) && _MSC_VER >= 1310 && ( defined(_M_ARM) )
		YieldProcessor();
#else
		_mm_pause();
#endif
#endif
	}
};
}//namespace end
#endif // NTB_SPINLOCK_H
