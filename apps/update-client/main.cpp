#include <iostream>
#include <set>

#include "TimeUtils.hpp"
#include "Options.hpp"
#include "AssetEntry.hpp"

int main_downloader(Options &options);
int main_cleaner(Options &options);

int main(int argc, char **argv)
{
    Options options;

    if (!Options::parseOptions(argc, argv, options))
        return 0;

    AssetEntry::setDataPath(options.downloadPath);

    int code = 0;
    if ((options.mode & Options::Mode::DOWNLOADER) > 0)
        code = main_downloader(options);

    if (code == 0 && (options.mode & Options::Mode::CLEANER) > 0)
        code = main_cleaner(options);

    return code;
}
