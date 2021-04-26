#include "mu_stdlib_internal.h"

#pragma warning(push)
#pragma warning(disable : 4507)
#include <backward.hpp>
#pragma warning(pop)

#include "debug_break.h"

#include <spdlog/sinks/stdout_sinks.h>

namespace mu
{
	namespace debug
	{
		void log_stack_trace(spdlog::logger& l, spdlog::level::level_enum lvl, unsigned int level_skip) noexcept
		try
		{
			backward::StackTrace stackTrace;
			stackTrace.load_here();

			backward::TraceResolver resolver;
			resolver.load_stacktrace(stackTrace);

			// Discard this level as well
			for (size_t i = level_skip + 1; i < stackTrace.size(); ++i)
			{
				backward::ResolvedTrace trace = resolver.resolve(stackTrace[i]);
				l.log(lvl, fmt::format("{0} {1} [{2}]", trace.object_filename, trace.object_function, trace.addr).c_str());
			}
		}
		catch (...)
		{
			// TODO: log error
			return;
		}

		void log_stack_trace(spdlog::logger& l, spdlog::level::level_enum lvl, backward::StackTrace& st, unsigned int level_skip) noexcept
		try
		{
			backward::TraceResolver tr;
			tr.load_stacktrace(st);

			for (size_t i = level_skip; i < st.size(); ++i)
			{
				backward::ResolvedTrace trace = tr.resolve(st[i]);
				l.log(lvl, "{0} {1} [{2}]", trace.object_filename, trace.object_function, trace.addr);
			}
		}
		catch (...)
		{
			// TODO: log error
			return;
		}

		namespace details
		{
			struct logger_impl : public logger_interface
			{
				backward::TraceResolver			m_resolver_ref; // Keep one of these around so the symbols load properly.
				std::shared_ptr<spdlog::logger> m_stderr_logger;
				std::shared_ptr<spdlog::logger> m_stdout_logger;

				static inline auto singleton() noexcept -> logger_impl*
				{
					return reinterpret_cast<logger_impl*>(logger().get());
				}

				logger_impl()
				{
					auto stderr_logger = std::make_shared<spdlog::logger>("stderr", std::make_shared<spdlog::sinks::stderr_sink_mt>());
					auto stdout_logger = std::make_shared<spdlog::logger>("stdout", std::make_shared<spdlog::sinks::stdout_sink_mt>());

					std::set_terminate(
						[]() noexcept
						{
							auto logger_impl = logger_impl::singleton();
							if (logger_impl)
							{
								if (auto logger_ref = logger_impl::singleton()->stderr_logger(); logger_ref)
								{
									backward::StackTrace st;
									st.load_here(64);

									logger_ref->log(spdlog::level::critical, "Unhandled exception");
									log_stack_trace(*logger_ref, spdlog::level::critical, st, 1);
									logger_ref->flush();
									logger_ref.reset();
								}

								// Rethrow and try to get some info out of it
								if (auto x = std::current_exception())
								{
									// TODO: concatenate with additional types
									auto terminate_error_handlers = error_handlers;

									leaf::try_handle_all(
										[&]() -> leaf::result<void>
										{
											std::rethrow_exception(x);
										},
										terminate_error_handlers);
								}

								std::_Exit(EXIT_FAILURE);
							}
						});

					m_stderr_logger = stderr_logger;
					m_stdout_logger = stdout_logger;
				}

				virtual ~logger_impl()
				{
					try
					{
						m_stderr_logger.reset();
						m_stdout_logger.reset();
					}
					catch (...)
					{
						// TODO: report error
						return;
					}
				}

				virtual auto stdout_logger() noexcept -> std::shared_ptr<spdlog::logger>
				{
					return m_stdout_logger;
				}

				virtual auto stderr_logger() noexcept -> std::shared_ptr<spdlog::logger>
				{
					return m_stderr_logger;
				}
			};

		} // namespace details
	}	  // namespace debug
} // namespace mu

MU_DEFINE_VIRTUAL_SINGLETON(mu::debug::details::logger_interface, mu::debug::details::logger_impl);
MU_EXPORT_SINGLETON(mu::debug::logger);

#ifdef _WINDOWS_
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
	// overriding the windows formatmessage handler
	auto custom_formatmessage(
		unsigned long dwFlags,
		const void*	  lpSource,
		unsigned long dwMessageId,
		unsigned long dwLanguageId,
		char*		  lpBuffer,
		unsigned long nSize,
		va_list*	  Arguments) noexcept -> unsigned long
	{
		return ::FormatMessageA(dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments);
	}

} // namespace mu

namespace mu
{
	namespace time
	{
		namespace details
		{
			static auto get_perf_frequency() noexcept -> int64_t
			{
				LARGE_INTEGER perf_freq;
				QueryPerformanceFrequency(&perf_freq);
				return perf_freq.QuadPart;
			}

			int64_t			s_performance_frequency = get_perf_frequency();
			std::atomic_int s_hires_state{0};

		} // namespace details

		auto performance_frequency() noexcept -> int64_t
		{
			return details::s_performance_frequency;
		}

		void calibrate() noexcept
		{
			// TBD
		}

		void init() noexcept
		{
			details::s_performance_frequency = details::get_perf_frequency();
		}

		auto get_now() noexcept -> int64_t
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
			}
			while (current_moment.QuadPart < end_moment.QuadPart);
		}

		void set_high_resolution_timer() noexcept
		{
			const int prev_state = details::s_hires_state.fetch_add(1);
			if (prev_state == 0)
			{
				timeBeginPeriod(1);
			}
		}

		auto release_high_resolution_timer() noexcept -> leaf::result<void>
		try
		{
			const int prev_state = details::s_hires_state.fetch_sub(1);
			if (prev_state == 1)
			{
				timeEndPeriod(1);
			}
			else if (prev_state <= 0)
			{
				//"Unbalanced HighResolutionTimer reference count"
				return MU_LEAF_NEW_ERROR(runtime_error::not_specified{});
			}
			return {};
		}
		catch (...)
		{
			return MU_LEAF_NEW_ERROR(runtime_error::not_specified{});
		}
	} // namespace time

} // namespace mu

// Implement some of the functions and types normally declared in recent Windows SDK.
#if !defined(_versionhelpers_H_INCLUDED_) && !defined(_INC_VERSIONHELPERS)
static BOOL IsWindowsVersionOrGreater(WORD major, WORD minor, WORD sp)
{
	OSVERSIONINFOEXW osvi = {sizeof(osvi), major, minor, 0, 0, {0}, sp, 0, 0, 0, 0};
	DWORD			 mask = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
	ULONGLONG		 cond = ::VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
	cond				  = ::VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
	cond				  = ::VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
	return ::VerifyVersionInfoW(&osvi, mask, cond);
}
#define IsWindows8Point1OrGreater() IsWindowsVersionOrGreater(HIBYTE(0x0602), LOBYTE(0x0602), 0) // _WIN32_WINNT_WINBLUE
#endif

#ifndef DPI_ENUMS_DECLARED
typedef enum
{
	PROCESS_DPI_UNAWARE			  = 0,
	PROCESS_SYSTEM_DPI_AWARE	  = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef enum
{
	MDT_EFFECTIVE_DPI = 0,
	MDT_ANGULAR_DPI	  = 1,
	MDT_RAW_DPI		  = 2,
	MDT_DEFAULT		  = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#endif
#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE (DPI_AWARENESS_CONTEXT) - 3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT) - 4
#endif
typedef HRESULT(WINAPI* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);						// Shcore.lib + dll, Windows 8.1+
typedef HRESULT(WINAPI* PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);		// Shcore.lib + dll, Windows 8.1+
typedef DPI_AWARENESS_CONTEXT(WINAPI* PFN_SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT); // User32.lib + dll, Windows 10 v1607+ (Creators Update)

#if defined(_MSC_VER) && !defined(NOGDI)
#pragma comment(lib, "gdi32") // Link with gdi32.lib for GetDeviceCaps()
#endif

namespace mu
{
	// Helper function to enable DPI awareness without setting up a manifest
	void enable_dpi_awareness() noexcept
	try
	{
		// if (IsWindows10OrGreater()) // This needs a manifest to succeed. Instead we try to grab the function pointer!
		{
			static HINSTANCE user32_dll = ::LoadLibraryA("user32.dll"); // Reference counted per-process
			if (PFN_SetThreadDpiAwarenessContext SetThreadDpiAwarenessContextFn = (PFN_SetThreadDpiAwarenessContext)::GetProcAddress(user32_dll, "SetThreadDpiAwarenessContext"))
			{
				SetThreadDpiAwarenessContextFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
				return;
			}
		}

		if (IsWindows8Point1OrGreater())
		{
			static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
			if (PFN_SetProcessDpiAwareness SetProcessDpiAwarenessFn = (PFN_SetProcessDpiAwareness)::GetProcAddress(shcore_dll, "SetProcessDpiAwareness"))
			{
				SetProcessDpiAwarenessFn(PROCESS_PER_MONITOR_DPI_AWARE);
				return;
			}
		}
#if _WIN32_WINNT >= 0x0600
		::SetProcessDPIAware();
#endif
	}
	catch (...)
	{
		// TODO: log error
	}

	auto get_dpi_scale_for_monitor(void* monitor) noexcept -> float
	try
	{
		UINT		xdpi = 96, ydpi = 96;
		static BOOL bIsWindows8Point1OrGreater = IsWindows8Point1OrGreater();
		if (bIsWindows8Point1OrGreater)
		{
			static HINSTANCE shcore_dll = ::LoadLibraryA("shcore.dll"); // Reference counted per-process
			if (PFN_GetDpiForMonitor GetDpiForMonitorFn = (PFN_GetDpiForMonitor)::GetProcAddress(shcore_dll, "GetDpiForMonitor"))
				GetDpiForMonitorFn((HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
		}
#ifndef NOGDI
		else
		{
			const HDC dc = ::GetDC(NULL);
			xdpi		 = ::GetDeviceCaps(dc, LOGPIXELSX);
			ydpi		 = ::GetDeviceCaps(dc, LOGPIXELSY);
			::ReleaseDC(NULL, dc);
		}
#endif
		assert(xdpi == ydpi); // Please contact me if you hit this assert!
		return xdpi / 96.0f;
	}
	catch (...)
	{
		return 1.0f;
	}

	auto get_dpi_scale_for_hwnd(void* hwnd) noexcept -> float
	try
	{
		HMONITOR monitor = ::MonitorFromWindow((HWND)hwnd, MONITOR_DEFAULTTONEAREST);
		return get_dpi_scale_for_monitor(monitor);
	}
	catch (...)
	{
		return 1.0f;
	}
} // namespace mu

#endif // #ifdef _WINDOWS_

#ifdef __APPLE__
#include <mach/mach_time.h>

namespace mu
{
	namespace time
	{
		namespace details
		{
			static auto get_perf_frequency() noexcept -> int64_t
			{
				mach_timebase_info_data_t mach_info;
				mach_timebase_info(&mach_info);
				return (mach_info.denom * 1000000000) / mach_info.numer;
			}

			int64_t			s_performance_frequency = get_perf_frequency();
			uint64_t		s_initial{0};
			std::atomic_int s_hires_state{0};

		} // namespace details

		auto performance_frequency() noexcept -> int64_t
		{
			return details::s_performance_frequency;
		}

		void calibrate() noexcept
		{
			// TBD
		}

		void init() noexcept
		{
			details::s_initial = mach_absolute_time();
		}

		auto get_now() noexcept -> int64_t
		{
			return mach_absolute_time() - details::s_initial;
		}

		void sleep(const int64_t milliseconds) noexcept
		{
			micro_sleep(((milliseconds * static_cast<long double>(performance_frequency()) * static_cast<long double>(1000ull))));
		}

		void micro_sleep(const int64_t ticks) noexcept
		{
			mach_wait_until(mach_absolute_time() + ticks);
		}

		void set_high_resolution_timer() noexcept
		{
			const int prev_state = details::s_hires_state.fetch_add(1);
			if (prev_state == 0)
			{
				// timeBeginPeriod(1);
			}
		}

		void release_high_resolution_timer()
		{
			const int prev_state = details::s_hires_state.fetch_sub(1);
			if (prev_state == 1)
			{
				// timeEndPeriod(1);
			}
			else if (prev_state <= 0)
			{
				throw std::runtime_error("Unbalanced HighResolutionTimer reference count");
			}
		}
	} // namespace time

} // namespace mu

namespace mu
{
	void enable_dpi_awareness() noexcept
	{
	}

	auto get_dpi_scale_for_monitor(void* monitor) noexcept -> float
	{
		return 1.0f;
	}

	auto get_dpi_scale_for_hwnd(void* hwnd) noexcept -> float
	{
		return 1.0f;
	}
}

#endif // #ifdef __APPLE__

#include <nfd.h>
#include <boxer/boxer.h>

namespace mu
{
	namespace details
	{
		static auto async_show_messagebox(std::string message, std::string title, messagebox_style style, messagebox_buttons buttons) -> messagebox_result
		{
			const auto converted_style = [style]()
			{
				switch (style)
				{
				case messagebox_style::info:
					return boxer::Style::Info;
				case messagebox_style::warning:
					return boxer::Style::Warning;
				case messagebox_style::error:
					return boxer::Style::Error;
				case messagebox_style::question:
				default:
					return boxer::Style::Question;
				};
			}();

			const auto converted_buttons = [buttons]()
			{
				switch (buttons)
				{
				case messagebox_buttons::ok:
					return boxer::Buttons::OK;
				case messagebox_buttons::okcancel:
					return boxer::Buttons::OKCancel;
				case messagebox_buttons::yesno:
					return boxer::Buttons::YesNo;
				case messagebox_buttons::quit:
				default:
					return boxer::Buttons::Quit;
				};
			}();

			switch (boxer::show(message.c_str(), title.c_str(), converted_style, converted_buttons))
			{
			case boxer::Selection::OK:
				return messagebox_result::ok;
			case boxer::Selection::Cancel:
				return messagebox_result::cancel;
			case boxer::Selection::Yes:
				return messagebox_result::yes;
			case boxer::Selection::No:
				return messagebox_result::no;
			case boxer::Selection::Quit:
				return messagebox_result::quit;
			case boxer::Selection::None:
				return messagebox_result::none;
			case boxer::Selection::Error:
			default:
				return messagebox_result::error;
			};
		}

		auto show_messagebox(const char* message, const char* title, messagebox_style style, messagebox_buttons buttons) noexcept -> std::future<messagebox_result>
		{
			return std::async(std::launch::async, async_show_messagebox, std::string(message), std::string(title), style, buttons);
		}
	} // namespace details

	namespace details
	{
		// indirect call required to copy the string views
		static auto async_file_open_dialog(std::string filter, std::string loc) noexcept -> std::optional<std::string>
		try
		{
			std::string result;
			char*		nfd_path = nullptr;

			auto nfd_result = NFD_OpenDialog(filter.data(), loc.data(), &nfd_path);
			if (nfd_result == NFD_OKAY)
			{
				result = nfd_path;
				::free(nfd_path);
				return result;
			}
			else if (nfd_result == NFD_CANCEL)
			{
				// nothing
			}
			else
			{
				// printf("Error: %s\n", NFD_GetError());
			}
			return std::nullopt;
		}
		catch (...)
		{
			return std::nullopt;
		}

		static auto async_file_open_multiple_dialog(std::string filter, std::string loc) noexcept -> std::optional<std::vector<std::string>>
		try
		{
			std::vector<std::string> results;
			nfdpathset_t			 path_set;

			auto nfd_result = NFD_OpenDialogMultiple(filter.data(), loc.data(), &path_set);

			if (nfd_result == NFD_OKAY)
			{
				try
				{
					const auto num_paths = NFD_PathSet_GetCount(&path_set);
					results.resize(num_paths);
					for (auto i = num_paths - 1; i >= 0; --i)
					{
						results[i] = NFD_PathSet_GetPath(&path_set, i);
					}
					return results;
				}
				catch (...)
				{
					NFD_PathSet_Free(&path_set);
				}
			}
			else if (nfd_result == NFD_CANCEL)
			{
				// nothing
			}
			else
			{
				//::imgui_app::error("%s", NFD_GetError());
				// printf("Error: );
			}
			return std::nullopt;
		}
		catch (...)
		{
			return std::nullopt;
		}

		static auto async_file_save_dialog(std::string filter, std::string loc) noexcept -> std::optional<std::string>
		try
		{
			std::string result;
			nfdchar_t*	nfd_path = nullptr;

			auto nfd_result = NFD_SaveDialog(filter.data(), loc.data(), &nfd_path);

			if (nfd_result == NFD_OKAY)
			{
				result = nfd_path;
				::free(nfd_path);
				return result;
			}
			else if (nfd_result == NFD_CANCEL)
			{
				// nothing
			}
			else
			{
				// printf("Error: %s\n", NFD_GetError());
			}
			return std::nullopt;
		}
		catch (...)
		{
			return std::nullopt;
		}

		static auto async_show_choose_path_dialog(std::string loc) noexcept -> std::optional<std::string>
		try
		{
			std::string result;
			nfdchar_t*	nfd_path = nullptr;

			auto nfd_result = NFD_OpenDirectoryDialog(nullptr, loc.data(), &nfd_path);

			if (nfd_result == NFD_OKAY)
			{
				result = nfd_path;
				::free(nfd_path);
				return result;
			}
			else if (nfd_result == NFD_CANCEL)
			{
				// nothing
			}
			else
			{
				// printf("Error: %s\n", NFD_GetError());
			}
			return std::nullopt;
		}
		catch (...)
		{
			return std::nullopt;
		}

		auto show_file_open_dialog(std::string_view origin, std::string_view filter) noexcept -> optional_future<std::string>
		{
			return std::async(std::launch::async, async_file_open_dialog, std::string(filter), std::string(origin));
		}

		auto show_file_open_multiple_dialog(std::string_view origin, std::string_view filter) noexcept -> optional_future<std::vector<std::string>>
		{
			return std::async(std::launch::async, async_file_open_multiple_dialog, std::string(filter), std::string(origin));
		}

		auto show_file_save_dialog(std::string_view origin, std::string_view filter) noexcept -> optional_future<std::string>
		{
			return std::async(std::launch::async, async_file_save_dialog, std::string(filter), std::string(origin));
		}

		auto show_path_dialog(std::string_view origin, std::string_view filter) noexcept -> optional_future<std::string>
		{
			return std::async(std::launch::async, async_show_choose_path_dialog, std::string(origin));
		}
	} // namespace details
} // namespace mu
