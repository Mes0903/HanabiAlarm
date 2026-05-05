#include "controllers/SchedulerService.h"

#include <cassert>
#include <string>

int main()
{
	const std::string url = "https://www.youtube.com/watch?v=xXMuBMhYXIM&list=PLku7p0RAD_yvD1wStCLyZ5iQqkuPAwz2E";

	assert(alarm::controller::SchedulerService::buildChromeLaunchArguments(url) == "--incognito \"" + url + "\"");
	assert(alarm::controller::SchedulerService::extractYoutubeUrlFromChromeArguments("--incognito \"" + url + "\"") == url);
	assert(alarm::controller::SchedulerService::extractYoutubeUrlFromChromeArguments(url) == url);

	return 0;
}
