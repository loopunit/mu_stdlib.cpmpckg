#include "mu_stdlib_internal.h"

#include <stdexcept>

#ifndef NOMINMAX
#define NOMINMAX
#endif // #ifndef NOMINMAX

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // #ifndef WIN32_LEAN_AND_MEAN

#include <objbase.h>
#include <timeapi.h>
#include <windows.h>

namespace mu
{
	unsigned long custom_formatmessage(
		unsigned long	dwFlags,
		const void*		lpSource,
		unsigned long	dwMessageId,
		unsigned long	dwLanguageId,
		char*			lpBuffer,
		unsigned long	nSize,
		va_list*		Arguments)
		{
			return ::FormatMessageA( dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments);
		}
		
}

namespace mu
{
	namespace time
	{
		namespace details
		{
			static int64_t get_perf_frequency() noexcept
			{
				LARGE_INTEGER perf_freq;
				QueryPerformanceFrequency(&perf_freq);
				return perf_freq.QuadPart;
			}

			int64_t			s_performance_frequency = get_perf_frequency();
			std::atomic_int s_hires_state{0};

		} // namespace details

		int64_t performance_frequency() noexcept
		{
			return details::s_performance_frequency;
		}

		void calibrate()
		{
			// TBD
		}

		void init()
		{
			details::s_performance_frequency = details::get_perf_frequency();
		}

		int64_t get_now() noexcept
		{
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);
			return now.QuadPart;
		}

		void sleep(const int64_t milliseconds) noexcept
		{
			::Sleep(static_cast<DWORD>(milliseconds));
		}

		void micro_sleep(const int64_t ticks) noexcept
		{
			LARGE_INTEGER current_moment;
			LARGE_INTEGER end_moment;

			QueryPerformanceCounter(&end_moment);
			end_moment.QuadPart += ticks;

			do
			{
				SwitchToThread();
				QueryPerformanceCounter(&current_moment);
			} while (current_moment.QuadPart < end_moment.QuadPart);
		}

		void set_high_resolution_timer() noexcept
		{
			const int prev_state = details::s_hires_state.fetch_add(1);
			if (prev_state == 0)
			{
				timeBeginPeriod(1);
			}
		}

		void release_high_resolution_timer()
		{
			const int prev_state = details::s_hires_state.fetch_sub(1);
			if (prev_state == 1)
			{
				timeEndPeriod(1);
			}
			else if (prev_state <= 0)
			{
				throw std::runtime_error("Unbalanced HighResolutionTimer reference count");
			}
		}
	} // namespace time

} // namespace mu