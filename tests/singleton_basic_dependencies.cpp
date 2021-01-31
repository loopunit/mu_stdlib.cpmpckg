#include <mu_stdlib.h>

#include <cstdio>

namespace details
{
	struct root_singleton
	{
		root_singleton()
		{
			printf("root_singleton()\n");
		}

		~root_singleton()
		{
			printf("~root_singleton()\n");
		}
	};

	struct leaf_singleton
	{
		leaf_singleton()
		{
			printf("leaf_singleton()\n");
		}

		~leaf_singleton()
		{
			printf("~leaf_singleton()\n");
		}

		void ping()
		{
			printf("ping!\n");
		}
	};
} // namespace details

using root_singleton = mu::singleton<details::root_singleton>;
using leaf_singleton = mu::singleton<details::leaf_singleton, root_singleton>;

int main(int, char**)
{
	leaf_singleton()->ping();
	return 0;
}