#pragma once

#include <atomic>
#include <array>
#include <cstdint>
#include <future>
#include <functional>
#include <optional>
#include <bitset>

#ifndef SPDLOG_FMT_EXTERNAL
#define SPDLOG_FMT_EXTERNAL 1
#endif
#include <spdlog/spdlog.h>

#define SG_REQUIRE_NOEXCEPT_IN_CPP17
#include <scope_guard.hpp>

// TODO: leaf pulls in Windows.h via common.hpp, so we bypass that here to avoid the mess
// if something did pull in windows.h, this is fine. custom_formatmessage is just a wrapper.
#ifndef _WINDOWS_

#ifndef LPVOID
#define LPVOID void*
#define LPVOID_redefined
#endif
#ifndef LPCSTR
#define LPCSTR char*
#define LPCSTR_redefined
#endif

namespace mu
{
	auto custom_formatmessage(
		unsigned long dwFlags,
		const void*	  lpSource,
		unsigned long dwMessageId,
		unsigned long dwLanguageId,
		char*		  lpBuffer,
		unsigned long nSize,
		va_list*	  Arguments) noexcept -> unsigned long;
}

#define FormatMessageA(DWORD_dwFlags, LPCVOID_lpSource, DWORD_dwMessageId, DWORD_dwLanguageId, LPSTR_lpBuffer, DWORD_nSize, va_list_Arguments)                                     \
	mu::custom_formatmessage(DWORD_dwFlags, LPCVOID_lpSource, DWORD_dwMessageId, DWORD_dwLanguageId, LPSTR_lpBuffer, DWORD_nSize, va_list_Arguments)
#define _WINDOWS_
#define _WINDOWS_redefined			   true
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_FROM_SYSTEM	   0x00001000
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define MAKELANGID(p, s)			   ((((unsigned short)(s)) << 10) | (unsigned short)(p))
#define LANG_NEUTRAL				   0x00
#define SUBLANG_DEFAULT				   0x01
#define LPSTR						   char*
#else // #ifndef _WINDOWS_
#define _WINDOWS_redefined false
#endif // #else // #ifndef _WINDOWS_
#include <boost/leaf.hpp>
#if _WINDOWS_redefined
#ifdef LPVOID_redefined
#undef LPVOID
#undef LPVOID_redefined
#endif
#ifdef LPCSTR_redefined
#undef LPCSTR_redefined
#undef LPCSTR
#endif
#undef FormatMessageA
#undef _WINDOWS_
#undef _WINDOWS_redefined
#undef FORMAT_MESSAGE_ALLOCATE_BUFFER
#undef FORMAT_MESSAGE_FROM_SYSTEM
#undef FORMAT_MESSAGE_IGNORE_INSERTS
#undef MAKELANGID
#undef LANG_NEUTRAL
#undef SUBLANG_DEFAULT
#undef LPSTR
#endif // #if _WINDOWS_redefined

namespace mu
{
	template<typename E>
	constexpr auto underlying_cast(E e) noexcept -> typename std::underlying_type<E>::type
	{
		return static_cast<typename std::underlying_type<E>::type>(e);
	}
} // namespace mu

namespace mu
{
	namespace leaf = boost::leaf;

	struct common_error
	{
	};

	struct runtime_error
	{
		struct not_specified : common_error
		{
		};
	};
} // namespace mu

#define MU_LEAF_RETHROW(r)                                                                                                                                                         \
	auto&& BOOST_LEAF_TMP = r;                                                                                                                                                     \
	static_assert(::boost::leaf::is_result_type<typename std::decay<decltype(BOOST_LEAF_TMP)>::type>::value, "MU_LEAF_CHECK requires a result object (see is_result_type)");       \
	if (BOOST_LEAF_TMP)                                                                                                                                                            \
		;                                                                                                                                                                          \
	else                                                                                                                                                                           \
	{                                                                                                                                                                              \
		return BOOST_LEAF_TMP.error();                                                                                                                                             \
	}

#define MU_LEAF_ASSIGN(v, r)                                                                                                                                                       \
	auto&& BOOST_LEAF_TMP = r;                                                                                                                                                     \
	static_assert(                                                                                                                                                                 \
		::boost::leaf::is_result_type<std::decay<decltype(BOOST_LEAF_TMP)>::type>::value,                                                                                 \
		"MU_LEAF_ASSIGN and MU_LEAF_AUTO require a result object as the second argument (see is_result_type)");                                                                    \
	if (!BOOST_LEAF_TMP)                                                                                                                                                           \
	{                                                                                                                                                                              \
		return BOOST_LEAF_TMP.error();                                                                                                                                             \
	}                                                                                                                                                                              \
	v = std::forward<decltype(BOOST_LEAF_TMP)>(BOOST_LEAF_TMP).value()

#define MU_LEAF_PUSH_BACK(v, r)                                                                                                                                                    \
	auto&& BOOST_LEAF_TMP = r;                                                                                                                                                     \
	static_assert(                                                                                                                                                                 \
		::boost::leaf::is_result_type<std::decay<decltype(BOOST_LEAF_TMP)>::type>::value,                                                                                 \
		"MU_LEAF_ASSIGN and MU_LEAF_AUTO require a result object as the second argument (see is_result_type)");                                                                    \
	if (!BOOST_LEAF_TMP)                                                                                                                                                           \
	{                                                                                                                                                                              \
		return BOOST_LEAF_TMP.error();                                                                                                                                             \
	}                                                                                                                                                                              \
	v.push_back(std::forward<decltype(BOOST_LEAF_TMP)>(BOOST_LEAF_TMP).value())

#define MU_LEAF_EMPLACE_BACK(v, r)                                                                                                                                                 \
	auto&& BOOST_LEAF_TMP = r;                                                                                                                                                     \
	static_assert(                                                                                                                                                                 \
		::boost::leaf::is_result_type<std::decay<decltype(BOOST_LEAF_TMP)>::type>::value,                                                                                 \
		"MU_LEAF_ASSIGN and MU_LEAF_AUTO require a result object as the second argument (see is_result_type)");                                                                    \
	if (!BOOST_LEAF_TMP)                                                                                                                                                           \
	{                                                                                                                                                                              \
		return BOOST_LEAF_TMP.error();                                                                                                                                             \
	}                                                                                                                                                                              \
	v.emplace_back(std::forward<decltype(BOOST_LEAF_TMP)>(BOOST_LEAF_TMP).value())

#define MU_LEAF_AUTO(v, r) MU_LEAF_ASSIGN(auto v, r)

#define MU_LEAF_ASSIGN_THROW(v, r)                                                                                                                                                 \
	auto&& BOOST_LEAF_TMP = r;                                                                                                                                                     \
	static_assert(                                                                                                                                                                 \
		::boost::leaf::is_result_type<std::decay<decltype(BOOST_LEAF_TMP)>::type>::value,                                                                                 \
		"MU_LEAF_ASSIGN and MU_LEAF_AUTO require a result object as the second argument (see is_result_type)");                                                                    \
	if (!BOOST_LEAF_TMP)                                                                                                                                                           \
	{                                                                                                                                                                              \
		throw BOOST_LEAF_TMP.error();                                                                                                                                              \
	}                                                                                                                                                                              \
	v = std::forward<decltype(BOOST_LEAF_TMP)>(BOOST_LEAF_TMP).value()

#define MU_LEAF_AUTO_THROW(v, r) MU_LEAF_ASSIGN_THROW(auto v, r)

#define MU_LEAF_CHECK(r)                                                                                                                                                           \
	auto&& BOOST_LEAF_TMP = r;                                                                                                                                                     \
	static_assert(::boost::leaf::is_result_type<std::decay<decltype(BOOST_LEAF_TMP)>::type>::value, "MU_LEAF_CHECK requires a result object (see is_result_type)");       \
	if (BOOST_LEAF_TMP)                                                                                                                                                            \
		;                                                                                                                                                                          \
	else                                                                                                                                                                           \
	{                                                                                                                                                                              \
		return BOOST_LEAF_TMP.error();                                                                                                                                             \
	}

#define MU_LEAF_NEW_ERROR		::boost::leaf::leaf_detail::inject_loc{__FILE__, __LINE__, __FUNCTION__} + ::boost::leaf::new_error
#define MU_LEAF_EXCEPTION		::boost::leaf::leaf_detail::inject_loc{__FILE__, __LINE__, __FUNCTION__} + ::boost::leaf::exception
#define MU_LEAF_THROW_EXCEPTION ::boost::leaf::leaf_detail::throw_with_loc{__FILE__, __LINE__, __FUNCTION__} + ::boost::leaf::exception
#define MU_LEAF_LOG_ERROR(...)	((void)0)

namespace mu
{
	template<typename T>
	using optional_future = std::future<std::optional<T>>;

	template<typename T>
	inline auto future_is_ready(T const& f) noexcept -> bool
	{
		return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}
} // namespace mu

namespace mu
{
	namespace details
	{
		template<typename T>
		class static_root_singleton
		{
		public:
			auto operator->() noexcept -> T*
			{
				return s_instance;
			}

			auto operator->() const noexcept -> const T*
			{
				return s_instance;
			}

			auto operator*() noexcept -> T&
			{
				return *s_instance;
			}

			auto operator*() const noexcept -> const T&
			{
				return *s_instance;
			}

			static_root_singleton() noexcept
			{
				static bool static_init = []() -> bool
				{
					s_instance = new (&s_instance_memory[0]) T();
					std::atexit(destroy);
					return true;
				}();
			}

		protected:
			static void destroy() noexcept
			{
				if (s_instance)
				{
					s_instance->~T();
					s_instance = nullptr;
				}
			}

		private:
			static inline uint64_t s_instance_memory[1 + (sizeof(T) / sizeof(uint64_t))];
			static inline T*	   s_instance = nullptr;
		};

		template<size_t POOL_COUNT>
		class singleton_cleanup_list
		{
		public:
			using cleanup_func = std::function<void()>;

			void push(cleanup_func f) noexcept
			{
				s_cleanup_funcs[s_cleanup_index.fetch_add(1)] = f;
			}

			~singleton_cleanup_list() noexcept
			{
				for (auto i = s_cleanup_index.load(); i > 0; --i)
				{
					s_cleanup_funcs[i - 1]();
				}
				s_cleanup_index.store(0);
			}

		private:
			std::array<cleanup_func, POOL_COUNT> s_cleanup_funcs;
			std::atomic_size_t					 s_cleanup_index = 0;
		};

		using singleton_cleanup_root = details::static_root_singleton<details::singleton_cleanup_list<1024>>;

		template<typename... T_DEPS>
		struct singleton_dependencies
		{
		private:
			template<typename T_ARG, typename... T_ARGS>
			static inline void update_dependencies_impl() noexcept
			{
				const typename T_ARG::type* t = T_ARG().get();
				if constexpr (sizeof...(T_ARGS) > 0)
				{
					update_dependencies_impl<T_ARGS...>();
				}
			}

		public:
			static inline void update() noexcept
			{
				if constexpr (sizeof...(T_DEPS) > 0)
				{
					update_dependencies_impl<T_DEPS...>();
				}
			}
		};

		template<typename T, typename... T_DEPS>
		struct singleton_factory
		{
			static inline auto create() noexcept -> T*
			{
				if constexpr (sizeof...(T_DEPS) > 0)
				{
					singleton_dependencies<T_DEPS...>::update();
				}
				return new T();
			}
		};

		template<typename T>
		struct virtual_singleton_factory
		{
			static auto create() noexcept -> T*;
		};
	} // namespace details

#define MU_DEFINE_VIRTUAL_SINGLETON(T, T_DERIVED)                                                                                                                                  \
	namespace mu                                                                                                                                                                   \
	{                                                                                                                                                                              \
		namespace details                                                                                                                                                          \
		{                                                                                                                                                                          \
			template<>                                                                                                                                                             \
			auto virtual_singleton_factory<T>::create() noexcept -> T*                                                                                                             \
			{                                                                                                                                                                      \
				return new T_DERIVED;                                                                                                                                              \
			}                                                                                                                                                                      \
		}                                                                                                                                                                          \
	}                                                                                                                                                                              \
	/**/

#define MU_DEFINE_VIRTUAL_SINGLETON_DEPS(T, T_DERIVED, ...)                                                                                                                        \
	namespace mu                                                                                                                                                                   \
	{                                                                                                                                                                              \
		namespace details                                                                                                                                                          \
		{                                                                                                                                                                          \
			template<>                                                                                                                                                             \
			auto virtual_singleton_factory<T>::create() noexcept -> T*                                                                                                             \
			{                                                                                                                                                                      \
				singleton_dependencies<__VA_ARGS__>::update();                                                                                                                     \
				return new T_DERIVED;                                                                                                                                              \
			}                                                                                                                                                                      \
		}                                                                                                                                                                          \
	}                                                                                                                                                                              \
	/**/

	namespace details
	{
		template<typename T, typename T_FACTORY>
		class singleton_base
		{
		public:
			using factory = T_FACTORY;
			using type	  = T;

			auto operator->() noexcept -> T*
			{
				return s_instance;
			}

			auto operator->() const noexcept -> const T*
			{
				return s_instance;
			}

			auto operator*() noexcept -> T&
			{
				return *s_instance;
			}

			auto operator*() const noexcept -> const T&
			{
				return *s_instance;
			}

			auto get() noexcept -> T*
			{
				return s_instance;
			}

			auto get() const noexcept -> const T*
			{
				return s_instance;
			}

			singleton_base() noexcept
			{
				static bool static_init = []() -> bool
				{
					s_instance = factory::create();
					singleton_cleanup_root()->push(
						[]() -> void
						{
							delete s_instance;
							s_instance = nullptr;
						});
					return true;
				}();
			}

		private:
			static inline T* s_instance;
		};
	} // namespace details

	template<typename T, typename... T_DEPENDENCIES>
	using singleton = details::singleton_base<T, details::singleton_factory<T, T_DEPENDENCIES...>>;

	template<typename T, typename... T_DEPENDENCIES>
	using virtual_singleton = details::singleton_base<T, details::virtual_singleton_factory<T>>;

	template<typename T_SINGLETON>
	class exported_singleton
	{
	public:
		using singleton_type = T_SINGLETON;
		using type			 = typename singleton_type::type;

		auto operator->() noexcept -> type*
		{
			return s_instance;
		}

		auto operator->() const noexcept -> const type*
		{
			return s_instance;
		}

		auto operator*() noexcept -> type&
		{
			return *s_instance;
		}

		auto operator*() const noexcept -> const type&
		{
			return *s_instance;
		}

		auto get() noexcept -> type*
		{
			return s_instance;
		}

		auto get() const noexcept -> const type*
		{
			return s_instance;
		}

		exported_singleton() noexcept
		{
			static bool static_init = []() -> bool
			{
				s_instance = get_instance();
				return true;
			}();
		}

	private:
		static inline type* s_instance;
		static type*		get_instance() noexcept;
	};

#define MU_EXPORT_SINGLETON(T)                                                                                                                                                     \
	template<>                                                                                                                                                                     \
	auto T::get_instance() noexcept->T::type*                                                                                                                                      \
	{                                                                                                                                                                              \
		return singleton_type().get();                                                                                                                                             \
	}                                                                                                                                                                              \
	/**/

#define MU_EXPORT_SINGLETON_DEPS(T, ...)                                                                                                                                           \
	template<>                                                                                                                                                                     \
	auto T::get_instance() noexcept->T::type*                                                                                                                                      \
	{                                                                                                                                                                              \
		::mu::details::singleton_dependencies<__VA_ARGS__>::update();                                                                                                              \
		return singleton_type().get();                                                                                                                                             \
	}                                                                                                                                                                              \
	/**/

	namespace details
	{
		template<typename T>
		class static_root_thread_local_singleton
		{
		public:
			auto operator->() noexcept -> T*
			{
				return s_instance;
			}

			auto operator->() const noexcept -> const T*
			{
				return s_instance;
			}

			auto operator*() noexcept -> T&
			{
				return *s_instance;
			}

			auto operator*() const noexcept -> const T&
			{
				return *s_instance;
			}

			static_root_thread_local_singleton() noexcept
			{
				static bool static_init = []() -> bool
				{
					s_instance = new (&s_instance_memory[0]) T();
					std::atexit(destroy);
					return true;
				}();
			}

		protected:
			static void destroy() noexcept
			{
				if (s_instance)
				{
					s_instance->~T();
					s_instance = nullptr;
				}
			}

		private:
			static inline thread_local uint64_t s_instance_memory[1 + (sizeof(T) / sizeof(uint64_t))];
			static inline thread_local T*		s_instance = nullptr;
		};

		using thread_local_singleton_cleanup_root = static_root_thread_local_singleton<singleton_cleanup_list<1024>>;

		template<typename T, typename T_FACTORY>
		class thread_local_singleton_base
		{
		public:
			using type = T;
			auto operator->() noexcept -> T*
			{
				return s_instance;
			}

			auto operator->() const noexcept -> const T*
			{
				return s_instance;
			}

			auto operator*() noexcept -> T&
			{
				return *s_instance;
			}

			auto operator*() const noexcept -> const T&
			{
				return *s_instance;
			}

			auto get() noexcept -> T*
			{
				return s_instance;
			}

			auto get() const noexcept -> const T*
			{
				return s_instance;
			}

			thread_local_singleton_base() noexcept
			{
				static thread_local bool static_init = []() -> bool
				{
					s_instance = new T();
					thread_local_singleton_cleanup_root()->push(
						[]() -> void
						{
							delete s_instance;
							s_instance = nullptr;
						});
					return true;
				}();
			}

			static inline thread_local T* s_instance;
		};
	} // namespace details

	template<typename T, typename... T_DEPENDENCIES>
	using thread_local_singleton = details::thread_local_singleton_base<T, details::singleton_factory<T, T_DEPENDENCIES...>>;

	template<typename T, typename... T_DEPENDENCIES>
	using thread_local_virtual_singleton = details::thread_local_singleton_base<T, details::virtual_singleton_factory<T>>;

	template<typename T_SINGLETON>
	class exported_thread_local_singleton
	{
	public:
		using singleton_type = T_SINGLETON;
		using type			 = typename singleton_type::type;

		auto operator->() noexcept -> type*
		{
			return s_instance;
		}

		auto operator->() const noexcept -> const type*
		{
			return s_instance;
		}

		auto operator*() noexcept -> type&
		{
			return *s_instance;
		}

		auto operator*() const noexcept -> const type&
		{
			return *s_instance;
		}

		auto get() noexcept -> type*
		{
			return s_instance;
		}

		auto get() const noexcept -> const type*
		{
			return s_instance;
		}

		exported_thread_local_singleton() noexcept
		{
			static thread_local bool static_init = []() -> bool
			{
				s_instance = get_instance();
				return true;
			}();
		}

	private:
		static inline thread_local type* s_instance;
		static type*					 get_instance() noexcept;
	};

#define MU_EXPORT_THREAD_LOCAL_SINGLETON(T)                                                                                                                                        \
	template<>                                                                                                                                                                     \
	auto ::mu::exported_thread_local_singleton<T>::get_instance() noexcept->::mu::exported_thread_local_singleton<T>::type*                                                        \
	{                                                                                                                                                                              \
		return singleton_type().get();                                                                                                                                             \
	}                                                                                                                                                                              \
	/**/
} // namespace mu

namespace mu
{
	namespace time
	{
		auto performance_frequency() noexcept -> int64_t;
		auto get_now() noexcept -> int64_t;
		void sleep(const int64_t milliseconds) noexcept;
		void micro_sleep(const int64_t tx) noexcept;
		void init() noexcept;
		void calibrate() noexcept;
		void set_high_resolution_timer() noexcept;
		auto release_high_resolution_timer() noexcept -> leaf::result<void>;

		class moment
		{
			friend class run_time;
			friend class long_clock;

		protected:
			int64_t value;

			moment(const int64_t v) noexcept : value(v) { }

			moment(const long double v) noexcept : value(static_cast<int64_t>(v)) { }

			friend auto now() noexcept -> moment;

			template<typename T>
			friend auto seconds(const T&) noexcept -> moment;

			template<typename T>
			friend auto milliseconds(const T&) noexcept -> moment;

			template<typename T>
			friend auto microseconds(const T&) noexcept -> moment;

			template<typename T>
			friend auto nanoseconds(const T&) noexcept -> moment;

		public:
			moment() noexcept : value(0) { }

			inline auto operator=(const moment& rhs) noexcept -> moment&
			{
				value = rhs.value;
				return *this;
			}

			inline auto operator+=(const moment& rhs) noexcept -> moment&
			{
				value += rhs.value;
				return *this;
			}

			inline auto operator-=(const moment& rhs) noexcept -> moment&
			{
				value += rhs.value;
				return *this;
			}

			template<typename T>
			inline auto operator*=(const T& rhs) noexcept -> moment&
			{
				value = static_cast<int64_t>(static_cast<long double>(value) * static_cast<long double>(rhs));
				return *this;
			}

			template<typename T>
			inline auto operator/=(const T& rhs) noexcept -> moment&
			{
				value = static_cast<int64_t>(static_cast<long double>(value) / static_cast<long double>(rhs));
				return *this;
			}

			inline auto add(const moment& rhs) const noexcept -> moment
			{
				return moment{value + rhs.value};
			}

			inline auto sub(const moment& rhs) const noexcept -> moment
			{
				return moment{value - rhs.value};
			}

			template<typename T>
			inline auto div(const T& rhs) const noexcept -> moment
			{
				return moment{static_cast<long double>(value) / static_cast<long double>(rhs)};
			}

			template<typename T>
			inline auto mul(const T& rhs) const noexcept -> moment
			{
				return moment{static_cast<long double>(value) * static_cast<long double>(rhs)};
			}

			template<typename T>
			inline auto as_seconds() const noexcept -> T
			{
				return static_cast<T>(static_cast<long double>(value) / static_cast<long double>(performance_frequency()));
			}

			template<typename T>
			inline auto as_milliseconds() const noexcept -> T
			{
				return static_cast<T>(static_cast<long double>(value) / ((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull))));
			}

			template<typename T>
			inline auto as_microseconds() const noexcept -> T
			{
				return static_cast<T>(static_cast<long double>(value) / ((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull))));
			}

			template<typename T>
			inline auto as_nanoseconds() const noexcept -> T
			{
				return static_cast<T>(
					static_cast<long double>(value) / ((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull * 1000ull))));
			}

			template<typename T>
			inline auto as_ticks() const noexcept -> T
			{
				return static_cast<T>(value);
			}

			template<typename T>
			inline auto set_seconds(const T& seconds) noexcept -> moment&
			{
				value = static_cast<int64_t>((static_cast<long double>(performance_frequency())) * static_cast<long double>(seconds));
				return *this;
			}

			template<typename T>
			inline auto set_milliseconds(const T& seconds) noexcept -> moment&
			{
				value = static_cast<int64_t>((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull)) * static_cast<long double>(seconds));
				return *this;
			}

			template<typename T>
			inline auto set_microseconds(const T& seconds) noexcept -> moment&
			{
				value = static_cast<int64_t>((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull)) * static_cast<long double>(seconds));
				return *this;
			}

			template<typename T>
			inline auto set_nanoseconds(const T& seconds) noexcept -> moment&
			{
				value = static_cast<int64_t>(
					(static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull * 1000ull)) * static_cast<long double>(seconds));
				return *this;
			}

			template<typename T>
			inline auto set_ticks(const T& tx) noexcept -> moment&
			{
				value = static_cast<int64_t>(tx);
				return *this;
			}

			inline auto now() noexcept -> moment&
			{
				value = get_now();
				return *this;
			}
		};

		class long_clock
		{
			uint64_t value;
			moment	 last_moment;

		public:
			long_clock() noexcept : value(0) { }

			template<typename T>
			inline auto as_seconds() const noexcept -> T
			{
				return static_cast<T>(static_cast<long double>(value) / static_cast<long double>(1000ull));
			}

			template<typename T>
			inline auto as_milliseconds() const noexcept -> T
			{
				return static_cast<T>(static_cast<long double>(value));
			}

			template<typename T>
			inline auto as_microseconds() const noexcept -> T
			{
				return static_cast<T>(static_cast<long double>(value) * static_cast<long double>(1000ull));
			}

			void update() noexcept;
		};

		inline auto operator<(const moment& lhs, const moment& rhs) noexcept -> bool
		{
			return lhs.as_ticks<int64_t>() < rhs.as_ticks<int64_t>();
		}

		inline auto operator<=(const moment& lhs, const moment& rhs) noexcept -> bool
		{
			return lhs.as_ticks<int64_t>() <= rhs.as_ticks<int64_t>();
		}

		inline auto operator>(const moment& lhs, const moment& rhs) noexcept -> bool
		{
			return lhs.as_ticks<int64_t>() > rhs.as_ticks<int64_t>();
		}

		inline auto operator>=(const moment& lhs, const moment& rhs) noexcept -> bool
		{
			return lhs.as_ticks<int64_t>() >= rhs.as_ticks<int64_t>();
		}

		inline auto operator==(const moment& lhs, const moment& rhs) noexcept -> bool
		{
			return lhs.as_ticks<int64_t>() == rhs.as_ticks<int64_t>();
		}

		inline auto operator+(const moment& lhs, const moment& rhs) noexcept -> moment
		{
			return lhs.add(rhs);
		}

		inline auto operator-(const moment& lhs, const moment& rhs) noexcept -> moment
		{
			return lhs.sub(rhs);
		}

		template<typename T>
		inline auto operator/(const moment& lhs, const T& rhs) noexcept -> moment
		{
			return lhs.div(rhs);
		}

		template<typename T>
		inline auto operator*(const moment& lhs, const T& rhs) noexcept -> moment
		{
			return lhs.mul(rhs);
		}

		inline auto now() noexcept -> moment
		{
			moment t;
			return t.now();
		}

		template<typename T>
		inline auto seconds(const T& val) noexcept -> moment
		{
			moment t;
			return t.set_seconds(val);
		}

		template<typename T>
		inline auto milliseconds(const T& val) noexcept -> moment
		{
			moment t;
			return t.set_milliseconds(val);
		}

		template<typename T>
		inline auto microseconds(const T& val) noexcept -> moment
		{
			moment t;
			return t.set_microseconds(val);
		}

		template<typename T>
		inline auto nanoseconds(const T& val) noexcept -> moment
		{
			moment t;
			return t.set_nanoseconds(val);
		}

		template<typename T>
		inline auto ticks(const T& val) noexcept -> moment
		{
			moment t;
			return t.set_ticks(val);
		}

		template<typename T>
		inline void micro_sleep_seconds(const T& seconds) noexcept
		{
			micro_sleep(static_cast<int64_t>((static_cast<long double>(performance_frequency())) * static_cast<long double>(seconds)));
		}

		template<typename T>
		inline void micro_sleep_milliseconds(const T& seconds) noexcept
		{
			micro_sleep(static_cast<int64_t>((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull)) * static_cast<long double>(seconds)));
		}

		template<typename T>
		inline void micro_sleep_microseconds(const T& seconds) noexcept
		{
			micro_sleep(
				static_cast<int64_t>((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull)) * static_cast<long double>(seconds)));
		}

		template<typename T>
		inline void micro_sleep_ticks(const T& tx) noexcept
		{
			micro_sleep(static_cast<int64_t>(tx));
		}

		inline void micro_sleep(const moment& t) noexcept
		{
			micro_sleep(t.as_ticks<int64_t>());
		}

		template<typename T>
		inline void sleepseconds(const T& seconds) noexcept
		{
			sleep(moment().set_seconds(seconds).template as_milliseconds<int64_t>());
		}

		template<typename T>
		inline void sleepmilliseconds(const T& seconds) noexcept
		{
			sleep(moment().set_milliseconds(seconds).template as_milliseconds<int64_t>());
		}

		template<typename T>
		inline void sleepmicroseconds(const T& seconds) noexcept
		{
			sleep(moment().set_microseconds(seconds).template as_milliseconds<int64_t>());
		}

		template<typename T>
		inline void sleepticks(const T& tx) noexcept
		{
			sleep(moment().set_ticks(tx).template as_milliseconds<int64_t>());
		}

		inline void sleep(const moment& t) noexcept
		{
			sleep(t.template as_milliseconds<int64_t>());
		}

		inline void long_clock::update() noexcept
		{
			const moment n = now();
			const moment d = n - last_moment;
			last_moment	   = n;
			value += d.template as_milliseconds<int64_t>();
		}
	} // namespace time
} // namespace mu

namespace mu
{
	enum class messagebox_result : int
	{
		ok = 0,
		cancel,
		yes,
		no,
		quit,
		none,
		error
	};

	enum class messagebox_style : int
	{
		info = 0,
		warning,
		error,
		question
	};

	enum class messagebox_buttons : int
	{
		ok = 0,
		okcancel,
		yesno,
		quit
	};

	namespace details
	{
		template<typename T>
		struct future_helper
		{
			bool m_state = false;
			T	 m_future;

			using value_type = decltype(m_future.get());

			auto is_active() const noexcept -> bool
			{
				return m_state;
			}

			auto is_ready() const noexcept -> bool
			{
				if (m_state)
				{
					return future_is_ready(m_future);
				}
				return false;
			}

			auto acquire_value() noexcept -> value_type
			{
				m_state = false;
				return m_future.get();
			}

			auto get_value() noexcept -> std::optional<value_type>
			{
				if (is_ready())
				{
					return acquire_value();
				}
				return std::nullopt;
			}
		};
	} // namespace details

	namespace details
	{
		auto show_messagebox(const char* message, const char* title, messagebox_style style, messagebox_buttons buttons) noexcept -> std::future<messagebox_result>;
	}

	using messagebox_future = details::future_helper<decltype(details::show_messagebox("", "", messagebox_style(), messagebox_buttons()))>;
	inline auto show_messagebox(const char* message, const char* title, messagebox_style style, messagebox_buttons buttons) noexcept -> messagebox_future
	{
		return {true, details::show_messagebox(message, title, style, buttons)};
	}

	namespace details
	{
		auto show_file_open_dialog(std::string_view origin, std::string_view filter) noexcept -> optional_future<std::string>;
		auto show_file_open_multiple_dialog(std::string_view origin, std::string_view filter) noexcept -> optional_future<std::vector<std::string>>;
		auto show_file_save_dialog(std::string_view origin, std::string_view filter) noexcept -> optional_future<std::string>;
		auto show_path_dialog(std::string_view origin, std::string_view filter) noexcept -> optional_future<std::string>;
	} // namespace details

	using file_open_dialog_future = details::future_helper<decltype(details::show_file_open_dialog(std::string_view(), std::string_view()))>;
	inline auto show_file_open_dialog(std::string_view origin, std::string_view filter) noexcept -> file_open_dialog_future
	{
		return {true, details::show_file_open_dialog(origin, filter)};
	}

	using file_open_multiple_dialog_future = details::future_helper<decltype(details::show_file_open_multiple_dialog(std::string_view(), std::string_view()))>;
	inline auto show_file_open_multiple_dialog(std::string_view origin, std::string_view filter) noexcept -> file_open_multiple_dialog_future
	{
		return {true, details::show_file_open_multiple_dialog(origin, filter)};
	}

	using file_save_dialog_future = details::future_helper<decltype(details::show_file_save_dialog(std::string_view(), std::string_view()))>;
	inline auto show_file_save_dialog(std::string_view origin, std::string_view filter) noexcept -> file_save_dialog_future
	{
		return {true, details::show_file_save_dialog(origin, filter)};
	}

	using path_dialog_future = details::future_helper<decltype(details::show_path_dialog(std::string_view(), std::string_view()))>;
	inline auto show_path_dialog(std::string_view origin, std::string_view filter) noexcept -> path_dialog_future
	{
		return {true, details::show_path_dialog(origin, filter)};
	}
} // namespace mu

namespace backward
{
	class StackTrace;
}

namespace mu
{
	namespace debug
	{
		namespace details
		{
			struct logger_interface
			{
				logger_interface()			= default;
				virtual ~logger_interface() = default;

				virtual auto stdout_logger() noexcept -> std::shared_ptr<spdlog::logger> = 0;
				virtual auto stderr_logger() noexcept -> std::shared_ptr<spdlog::logger> = 0;
			};
		} // namespace details

		using logger = mu::exported_singleton<mu::virtual_singleton<details::logger_interface>>;

		void log_stack_trace(spdlog::logger& l, spdlog::level::level_enum lvl, unsigned int level_skip) noexcept;

	} // namespace debug

	static inline auto error_handlers = std::make_tuple(
		[](runtime_error::not_specified x, leaf::e_source_location sl)
		{
			debug::logger()->stderr_logger()->log(spdlog::level::err, "{0} :: {1} -> {2} : runtime_error :: not_specified", sl.line, sl.file, sl.function);
		},
		[](common_error x, leaf::e_source_location sl)
		{
			debug::logger()->stderr_logger()->log(spdlog::level::err, "{0} :: {1} -> {2} : common_error", sl.line, sl.file, sl.function);
		},
		[]
		{
			debug::logger()->stderr_logger()->log(spdlog::level::err, "???");
		});

	template<class TryBlock>
	constexpr inline auto try_handle(TryBlock&& try_block) -> typename std::decay<decltype(std::declval<TryBlock>()().value())>::type
	{
		return leaf::try_handle_all(try_block, error_handlers);
	}

} // namespace mu