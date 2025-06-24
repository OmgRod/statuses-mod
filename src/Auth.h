#pragma once
#include <Geode/Geode.hpp>
#include <argon/argon.hpp>
#include "LoadingOverlay.hpp"

using namespace geode::prelude;

class Auth : public CCObject {
public:
    void authenticationStart();
    static Auth* create();

private:
    std::shared_ptr<LoadingOverlay> m_loading;

    void onAuthSuccess(const std::string& token);
    void onAuthFailure(const std::string& err);
    void onAuthProgress(argon::AuthProgress progress);
};
