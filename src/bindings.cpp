#include <emscripten/bind.h>
#include "filter.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(FilterModule) {
    class_<particle>("particle")
        .constructor<>()
        .property("x", &particle::x)
        .property("y", &particle::y)
        .property("z", &particle::z)
        .property("d", &particle::d)
        .property("w", &particle::w);

    class_<Filter>("Filter")
        .constructor<int, bool>()
        .function("get", &Filter::get)
        .function("set", &Filter::set)
        .function("getN", &Filter::getN)
        .function("setN", &Filter::setN)
        .function("getEstimate", &Filter::getEstimate)
        .function("estimateState", &Filter::estimateState);
}