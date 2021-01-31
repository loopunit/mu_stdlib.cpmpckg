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

using basic_singleton = mu::singleton<details::basic_singleton>;

int main(int, char**)
{
	basic_singleton()->ping();
	return 0;
}