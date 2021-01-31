#include <mu_stdlib.h>

#include <cstdio>

namespace details
{
	struct dependency_singleton
	{
		dependency_singleton()
		{
			printf("dependency_singleton()\n");
		}

		~dependency_singleton()
		{
			printf("~dependency_singleton()\n");
		}
	};

	struct base_singleton
	{
		base_singleton()
		{
			printf("base_singleton()\n");
		}

		virtual ~base_singleton()
		{
			printf("~base_singleton()\n");
		}

		virtual void ping() = 0;
	};

	struct derived_singleton : public base_singleton
	{
		derived_singleton()
		{
			printf("derived_singleton()\n");
		}

		virtual ~derived_singleton()
		{
			printf("~derived_singleton()\n");
		}

		virtual void ping()
		{
			printf("ping!\n");
		}
	};
} // namespace details

using dependency_singleton = mu::singleton<::details::dependency_singleton>;
using derived_singleton	   = mu::virtual_singleton<::details::base_singleton>;
MU_DEFINE_VIRTUAL_SINGLETON_DEPS(::details::base_singleton, ::details::derived_singleton, dependency_singleton);

int main(int, char**)
{
	derived_singleton()->ping();
	return 0;
}