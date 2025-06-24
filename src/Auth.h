#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class Auth : public CCObject {
public:
    void authenticationStart();
    static Auth* create();
};
