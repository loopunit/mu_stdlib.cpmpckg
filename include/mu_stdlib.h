#pragma once

#include <boost/leaf.hpp>

#include <functional>
#include <atomic>
#include <array>
#include <cstdint>

namespace mu
{
	namespace leaf = boost::leaf;
}

namespace mu
{
	template<typename E>
	constexpr auto underlying_cast(E e) -> typename std::underlying_type<E>::type
	{
		return static_cast<typename std::underlying_type<E>::type>(e);
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
			T* operator->()
			{
				return s_instance;
			}

			const T* operator->() const
			{
				return s_instance;
			}

			T& operator*()
			{
				return *s_instance;
			}

			const T& operator*() const
			{
				return *s_instance;
			}

			static_root_singleton()
			{
				static bool static_init = []() -> bool {
					s_instance = new (&s_instance_memory[0]) T();
					std::atexit(destroy);
					return true;
				}();
			}

		protected:
			static void destroy()
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

			void push(cleanup_func f)
			{
				s_cleanup_funcs[s_cleanup_index.fetch_add(1)] = f;
			}

			~singleton_cleanup_list()
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
			static inline void update_dependencies_impl()
			{
				const typename T_ARG::type* t = T_ARG().get();
				if constexpr (sizeof...(T_ARGS) > 0)
				{
					update_dependencies_impl<T_ARGS...>();
				}
			}

		public:
			static inline void update()
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
			static inline T* create()
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
			static T* create();
		};
	} // namespace details

#define MU_DEFINE_VIRTUAL_SINGLETON(T, T_DERIVED)                                                                                                                                  \
	namespace mu                                                                                                                                                                   \
	{                                                                                                                                                                              \
		namespace details                                                                                                                                                          \
		{                                                                                                                                                                          \
			template<>                                                                                                                                                             \
			T* virtual_singleton_factory<T>::create()                                                                                                                              \
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
			T* virtual_singleton_factory<T>::create()                                                                                                                              \
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
			using factory = typename T_FACTORY;
			using type	  = typename T;

			T* operator->()
			{
				return s_instance;
			}

			const T* operator->() const
			{
				return s_instance;
			}

			T& operator*()
			{
				return *s_instance;
			}

			const T& operator*() const
			{
				return *s_instance;
			}

			T* get()
			{
				return s_instance;
			}

			const T* get() const
			{
				return s_instance;
			}

			singleton_base()
			{
				static bool static_init = []() -> bool {
					s_instance = factory::create();
					singleton_cleanup_root()->push([]() -> void {
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
		using singleton_type = typename T_SINGLETON;
		using type			 = typename singleton_type::type;

		type* operator->()
		{
			return s_instance;
		}

		const type* operator->() const
		{
			return s_instance;
		}

		type& operator*()
		{
			return *s_instance;
		}

		const type& operator*() const
		{
			return *s_instance;
		}

		type* get()
		{
			return s_instance;
		}

		const type* get() const
		{
			return s_instance;
		}

		exported_singleton()
		{
			static bool static_init = []() -> bool {
				s_instance = get_instance();
				return true;
			}();
		}

	private:
		static inline type* s_instance;
		static type*		get_instance();
	};

#define MU_EXPORT_SINGLETON(T)                                                                                                                                                     \
	template<>                                                                                                                                                                     \
	T::type* T::get_instance()                                                                                                                                                     \
	{                                                                                                                                                                              \
		return singleton_type().get();                                                                                                                                             \
	}                                                                                                                                                                              \
	/**/

#define MU_EXPORT_SINGLETON_DEPS(T, ...)                                                                                                                                           \
	template<>                                                                                                                                                                     \
	T::type* T::get_instance()                                                                                                                                                     \
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
			T* operator->()
			{
				return s_instance;
			}
			const T* operator->() const
			{
				return s_instance;
			}
			T& operator*()
			{
				return *s_instance;
			}
			const T& operator*() const
			{
				return *s_instance;
			}

			static_root_thread_local_singleton()
			{
				static bool static_init = []() -> bool {
					s_instance = new (&s_instance_memory[0]) T();
					std::atexit(destroy);
					return true;
				}();
			}

		protected:
			static void destroy()
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
			using type = typename T;
			T* operator->()
			{
				return s_instance;
			}

			const T* operator->() const
			{
				return s_instance;
			}

			T& operator*()
			{
				return *s_instance;
			}

			const T& operator*() const
			{
				return *s_instance;
			}

			T* get()
			{
				return s_instance;
			}

			const T* get() const
			{
				return s_instance;
			}

			thread_local_singleton_base()
			{
				static thread_local bool static_init = []() -> bool {
					s_instance = new T();
					thread_local_singleton_cleanup_root()->push([]() -> void {
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
		using singleton_type = typename T_SINGLETON;
		using type			 = typename singleton_type::type;

		type* operator->()
		{
			return s_instance;
		}

		const type* operator->() const
		{
			return s_instance;
		}

		type& operator*()
		{
			return *s_instance;
		}

		const type& operator*() const
		{
			return *s_instance;
		}

		type* get()
		{
			return s_instance;
		}

		const type* get() const
		{
			return s_instance;
		}

		exported_thread_local_singleton()
		{
			static thread_local bool static_init = []() -> bool {
				s_instance = get_instance();
				return true;
			}();
		}

	private:
		static inline thread_local type* s_instance;
		static type*					 get_instance();
	};

#define MU_EXPORT_THREAD_LOCAL_SINGLETON(T)                                                                                                                                        \
	template<>                                                                                                                                                                     \
	::mu::exported_thread_local_singleton<T>::type* ::mu::exported_thread_local_singleton<T>::get_instance()                                                                       \
	{                                                                                                                                                                              \
		return singleton_type().get();                                                                                                                                             \
	}                                                                                                                                                                              \
	/**/
} // namespace mu

namespace mu
{
	namespace time
	{
		int64_t performance_frequency() noexcept;
		int64_t get_now() noexcept;
		void	sleep(const int64_t milliseconds) noexcept;
		void	micro_sleep(const int64_t tx) noexcept;
		void	init();
		void	calibrate();
		void	set_high_resolution_timer() noexcept;
		void	release_high_resolution_timer();

		class moment
		{
			friend class run_time;
			friend class long_clock;

		protected:
			int64_t value;

			moment(const int64_t v) noexcept : value(v) {}

			moment(const long double v) noexcept : value(static_cast<int64_t>(v)) {}

			friend moment now() noexcept;

			template<typename T>
			friend moment seconds(const T&) noexcept;

			template<typename T>
			friend moment milliseconds(const T&) noexcept;

			template<typename T>
			friend moment microseconds(const T&) noexcept;

			template<typename T>
			friend moment nanoseconds(const T&) noexcept;

		public:
			moment() noexcept : value(0) {}

			inline moment& operator=(const moment& rhs) noexcept
			{
				value = rhs.value;
				return *this;
			}

			inline moment& operator+=(const moment& rhs) noexcept
			{
				value += rhs.value;
				return *this;
			}

			inline moment& operator-=(const moment& rhs) noexcept
			{
				value += rhs.value;
				return *this;
			}

			template<typename T>
			inline moment& operator*=(const T& rhs) noexcept
			{
				value = static_cast<int64_t>(static_cast<long double>(value) * static_cast<long double>(rhs));
				return *this;
			}

			template<typename T>
			inline moment& operator/=(const T& rhs) noexcept
			{
				value = static_cast<int64_t>(static_cast<long double>(value) / static_cast<long double>(rhs));
				return *this;
			}

			inline moment add(const moment& rhs) const noexcept
			{
				return moment{value + rhs.value};
			}

			inline moment sub(const moment& rhs) const noexcept
			{
				return moment{value - rhs.value};
			}

			template<typename T>
			inline moment div(const T& rhs) const noexcept
			{
				return moment{static_cast<long double>(value) / static_cast<long double>(rhs)};
			}

			template<typename T>
			inline moment mul(const T& rhs) const noexcept
			{
				return moment{static_cast<long double>(value) * static_cast<long double>(rhs)};
			}

			template<typename T>
			inline T as_seconds() const noexcept
			{
				return static_cast<T>(static_cast<long double>(value) / static_cast<long double>(performance_frequency()));
			}

			template<typename T>
			inline T as_milliseconds() const noexcept
			{
				return static_cast<T>(static_cast<long double>(value) / ((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull))));
			}

			template<typename T>
			inline T as_microseconds() const noexcept
			{
				return static_cast<T>(static_cast<long double>(value) / ((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull))));
			}

			template<typename T>
			inline T as_nanoseconds() const noexcept
			{
				return static_cast<T>(
					static_cast<long double>(value) / ((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull * 1000ull))));
			}

			template<typename T>
			inline T as_ticks() const noexcept
			{
				return static_cast<T>(value);
			}

			template<typename T>
			inline moment& set_seconds(const T& seconds) noexcept
			{
				value = static_cast<int64_t>((static_cast<long double>(performance_frequency())) * static_cast<long double>(seconds));
				return *this;
			}

			template<typename T>
			inline moment& set_milliseconds(const T& seconds) noexcept
			{
				value = static_cast<int64_t>((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull)) * static_cast<long double>(seconds));
				return *this;
			}

			template<typename T>
			inline moment& set_microseconds(const T& seconds) noexcept
			{
				value = static_cast<int64_t>((static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull)) * static_cast<long double>(seconds));
				return *this;
			}

			template<typename T>
			inline moment& set_nanoseconds(const T& seconds) noexcept
			{
				value = static_cast<int64_t>(
					(static_cast<long double>(performance_frequency()) / static_cast<long double>(1000ull * 1000ull * 1000ull)) * static_cast<long double>(seconds));
				return *this;
			}

			template<typename T>
			inline moment& set_ticks(const T& tx) noexcept
			{
				value = static_cast<int64_t>(tx);
				return *this;
			}

			inline moment& now() noexcept
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
			long_clock() noexcept : value(0) {}

			template<typename T>
			inline T as_seconds() const noexcept
			{
				return static_cast<T>(static_cast<long double>(value) / static_cast<long double>(1000ull));
			}

			template<typename T>
			inline T as_milliseconds() const noexcept
			{
				return static_cast<T>(static_cast<long double>(value));
			}

			template<typename T>
			inline T as_microseconds() const noexcept
			{
				return static_cast<T>(static_cast<long double>(value) * static_cast<long double>(1000ull));
			}

			void update() noexcept;
		};

		inline bool operator<(const moment& lhs, const moment& rhs) noexcept
		{
			return lhs.as_ticks<int64_t>() < rhs.as_ticks<int64_t>();
		}

		inline bool operator<=(const moment& lhs, const moment& rhs) noexcept
		{
			return lhs.as_ticks<int64_t>() <= rhs.as_ticks<int64_t>();
		}

		inline bool operator>(const moment& lhs, const moment& rhs) noexcept
		{
			return lhs.as_ticks<int64_t>() > rhs.as_ticks<int64_t>();
		}

		inline bool operator>=(const moment& lhs, const moment& rhs) noexcept
		{
			return lhs.as_ticks<int64_t>() >= rhs.as_ticks<int64_t>();
		}

		inline bool operator==(const moment& lhs, const moment& rhs) noexcept
		{
			return lhs.as_ticks<int64_t>() == rhs.as_ticks<int64_t>();
		}

		inline moment operator+(const moment& lhs, const moment& rhs) noexcept
		{
			return lhs.add(rhs);
		}

		inline moment operator-(const moment& lhs, const moment& rhs) noexcept
		{
			return lhs.sub(rhs);
		}

		template<typename T>
		inline moment operator/(const moment& lhs, const T& rhs) noexcept
		{
			return lhs.div(rhs);
		}

		template<typename T>
		inline moment operator*(const moment& lhs, const T& rhs) noexcept
		{
			return lhs.mul(rhs);
		}

		inline moment now() noexcept
		{
			moment t;
			return t.now();
		}

		template<typename T>
		inline moment seconds(const T& val) noexcept
		{
			moment t;
			return t.set_seconds(val);
		}

		template<typename T>
		inline moment milliseconds(const T& val) noexcept
		{
			moment t;
			return t.set_milliseconds(val);
		}

		template<typename T>
		inline moment microseconds(const T& val) noexcept
		{
			moment t;
			return t.set_microseconds(val);
		}

		template<typename T>
		inline moment nanoseconds(const T& val) noexcept
		{
			moment t;
			return t.set_nanoseconds(val);
		}

		template<typename T>
		inline moment ticks(const T& val) noexcept
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
			sleep(moment().set_seconds(seconds).as_milliseconds<int64_t>());
		}

		template<typename T>
		inline void sleepmilliseconds(const T& seconds) noexcept
		{
			sleep(moment().set_milliseconds(seconds).as_milliseconds<int64_t>());
		}

		template<typename T>
		inline void sleepmicroseconds(const T& seconds) noexcept
		{
			sleep(moment().set_microseconds(seconds).as_milliseconds<int64_t>());
		}

		template<typename T>
		inline void sleepticks(const T& tx) noexcept
		{
			sleep(moment().set_ticks(tx).as_milliseconds<int64_t>());
		}

		inline void sleep(const moment& t) noexcept
		{
			sleep(t.as_milliseconds<int64_t>());
		}

		inline void long_clock::update() noexcept
		{
			const moment n = now();
			const moment d = n - last_moment;
			last_moment	   = n;
			value += d.as_milliseconds<int64_t>();
		}
	} // namespace time
} // namespace mu

namespace mu
{

}