#include <mu_stdlib.h>

#include <cstdio>

namespace details
{
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

using exported_singleton = mu::exported_singleton<mu::singleton<details::basic_singleton>>;

MU_EXPORT_SINGLETON(exported_singleton); // Typically this would be in an implementation file within the library.

int main(int, char**)
{
	exported_singleton()->ping();
	return 0;
}