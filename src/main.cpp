// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "icons.h"
#include "ui/calculatorwindow.h"

#include "base/i18n.h"
#include "ui/application.h"
#include "ui/icon.h"

#include <cstdio>
#include <string>

using namespace cutefish;

static int parseArguments(int argc, char** argv, bool* quit)
{
    *quit = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "-h" || argument == "--help" || argument == "-?") {
            std::printf("Usage: %s [options]\nCutefish Calculator\n\nOptions:\n"
                        "  -h, --help     Displays help on commandline options.\n"
                        "  -v, --version  Displays version information.\n",
                        argv[0]);
            *quit = true;
            return 0;
        }
        if (argument == "-v" || argument == "--version") {
            std::printf("cutefish-calculator %s\n", CALCULATOR_VERSION);
            *quit = true;
            return 0;
        }
        std::fprintf(stderr, "Unknown option '%s'.\n", argument.substr(argument.find_first_not_of('-')).c_str());
        *quit = true;
        return 1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    bool quit = false;
    const int status = parseArguments(argc, argv, &quit);
    if (quit)
        return status;

    ui::Application app(argc, argv);
    app.setApplicationId("cutefish-calculator");
    i18n::setDomain("cutefish-calculator");
    for (const auto& [name, svg] : embeddedIcons())
        ui::Icon::registerGlyph(std::string(name), std::string(svg));

    CalculatorWindow window;
    window.window().show();
    return app.run();
}
