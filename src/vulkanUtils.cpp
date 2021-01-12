#include <iostream>

#include "vulkanUtils.h"

void panic (const char *msg) {
    std::cerr << "Panic! " << msg << std::endl;
    exit(-1);
}

void log (const char *msg) {
    std::cerr << "Info: " << msg << std::endl;
}
