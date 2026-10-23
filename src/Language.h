// Language records and the filename/display-name choices exposed to the settings menu.
#pragma once
#include <string>
#include <vector>
#include "LangStrings.h"
struct Language {
    std::string file, name;
    std::vector<std::string> strings;
};
struct LanguageChoice {
    std::string file, name;
};
const char *languageString(int);
void loadLanguages(const char *);
std::vector<LanguageChoice> *languageChoices();
