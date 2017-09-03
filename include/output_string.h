#pragma once


#include <sstream>


#define SIREN_OUTPUT_STRING(X) \
    (static_cast<std::stringstream &&>(std::ostringstream() << X).str())
