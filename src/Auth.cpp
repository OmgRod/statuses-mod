#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <argon/argon.hpp>
#include "LoadingOverlay.hpp"
#include "ServerListener.hpp"
#include "StatusManager.hpp"
#include "Auth.h"

using namespace geode::prelude;

LoadingOverlay* loading;

class $modify(AuthHook, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (GJAccountManager::get()->m_accountID == 0) return true;

        if (Mod::get()->getSavedValue<bool>("authenticated") == true &&
            StatusManager::get()->autoReconnect &&
            StatusManager::get()->isWSOpen == false) {
            ServerListener::connectAsync();
        } else {
            if (Mod::get()->getSavedValue<bool>("auth-info-popup-showed") != true) {
                std::thread openAuthPopup = std::thread([this]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                    Loader::get()->queueInMainThread([this]() {
                        this->showAuthPopup();
                    });
                });
                openAuthPopup.detach();
            }
        }
        return true;
    }

    void showAuthPopup() {
        createQuickPopup(
            "Welcome!",
            "Hello!\nI see you've just <cj>installed</c> the <cg>Statuses Mod</c>, would you like to go to your <co>profile</c> to <cy>authenticate</c>?\n<cb>(To authenticate you need to press the Status Icon on your profile)</c>",
            "Later", "OPEN",
            [](auto, bool btn2) {
                if (btn2) {
                    ProfilePage::create(GJAccountManager::get()->m_accountID, true)->show();
                }
            });
        Mod::get()->setSavedValue<bool>("auth-info-popup-showed", true);
    }
};

void Auth::authenticationStart() {
    auto GAM = GJAccountManager::get();

    if (GAM->m_accountID == 0) {
        FLAlertLayer::create("Oops!", "You need to <cg>log in</c> to use this mod.", "OK")->show();
        return;
    }

    loading = LoadingOverlay::create("Authenticating..");
    loading->show();

    auto res = argon::startAuth(
        [](Result<std::string> result) {
            if (!result) {
                Loader::get()->queueInMainThread([err = result.unwrapErr()] {
                    loading->fadeOut();
                    FLAlertLayer::create("Oops!", fmt::format("Authentication failed:\n<cr>{}</c>", err), "OK")->show();
                    StatusManager::get()->authFailed = true;
                });
                return;
            }

            auto token = result.unwrap();

            Loader::get()->queueInMainThread([token = std::move(token)] {
                Mod::get()->setSavedValue("authenticated", true);
                Mod::get()->setSavedValue("token", token);
                StatusManager::get()->authFailed = false;
                StatusManager::get()->autoReconnect = true;
                ServerListener::connectAsync();

                std::thread showSuccessThread = std::thread([]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                    Loader::get()->queueInMainThread([] {
                        loading->fadeOut();
                        ProfilePage::create(GJAccountManager::get()->m_accountID, true)->show();
                    });
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                    Loader::get()->queueInMainThread([] {
                        FLAlertLayer::create("Success!", "You've been successfully <cg>authorized</c>.", "OK")->show();
                    });
                });
                showSuccessThread.detach();
            });
        },
        [](argon::AuthProgress progress) {
            Loader::get()->queueInMainThread([progress] {
                loading->changeStatus(fmt::format("Authenticating: {}...", argon::authProgressToString(progress)).c_str());
            });
        });

    if (!res) {
        Loader::get()->queueInMainThread([err = res.unwrapErr()] {
            loading->fadeOut();
            FLAlertLayer::create("Oops!", fmt::format("Failed to start auth:\n<cr>{}</c>", err), "OK")->show();
        });
    }
}

Auth* Auth::create() {
    auto ret = new Auth();
    if (ret) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}