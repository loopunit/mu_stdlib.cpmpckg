#include <mu_stdlib.h>

#include <cstdio>

namespace details
{
	struct basic_singleton
	{
		basic_singleton() {}

		~basic_singleton() {}

		void ping() {}
	};
} // namespace details

using basic_singleton = mu::singleton<details::basic_singleton>;


int main(int, char**)
{
	return mu::leaf::try_handle_all(

		[&]() -> mu::leaf::result<int> {
			// The TryBlock goes here, we'll see it later
			basic_singleton()->ping();
			return 0;
		},
		// Error handlers below:
		[](mu::leaf::error_info const& unmatched) {
			//  "Unknown failure detected" << std::endl <<
			//  "Cryptic diagnostic information follows" << std::endl <<
			//  unmatched;
			return 6;
		});
}