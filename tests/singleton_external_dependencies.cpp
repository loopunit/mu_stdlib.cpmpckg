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

	struct basic_singleton
	{
		basic_singleton()
		{
			printf("basic_singleton()\n");
		}

		~basic_singleton()
		{
			printf("~basic_singleton()\n");
		}

		void ping()
		{
			printf("ping!\n");
		}
	};
} // namespace details

using dependency_singleton = mu::singleton<details::dependency_singleton>;
using exported_singleton   = mu::exported_singleton<mu::singleton<details::basic_singleton>>;

// Typically this would be in an implementation file within the library.
MU_EXPORT_SINGLETON_DEPS(exported_singleton, dependency_singleton);

int main(int, char**)
{
	exported_singleton()->ping();
	return 0;
}