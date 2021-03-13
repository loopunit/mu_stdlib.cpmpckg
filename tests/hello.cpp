#include <mu_stdlib.h>

#include <cstdio>

namespace details
{
	struct basic_singleton
	{
		basic_singleton() { }

		~basic_singleton() { }

		void ping() { }
	};
} // namespace details

using basic_singleton = mu::singleton<details::basic_singleton>;

int main(int argc, char* argv[])
{
	return mu::leaf::try_handle_all(

		[&]() -> mu::leaf::result<int>
		{
			basic_singleton()->ping();
			return 0;
		},

		[](mu::leaf::error_info const& unmatched)
		{
			//  "Unknown failure detected" << std::endl <<
			//  "Cryptic diagnostic information follows" << std::endl <<
			//  unmatched;
			return -1;
		});
}