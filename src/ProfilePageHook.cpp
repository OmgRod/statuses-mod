#include <Geode/Geode.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include "StatusIconNode.hpp"
#include "utils.hpp"
#include "StatusChangeLayer.hpp"
#include "StatusManager.hpp"
#include "Auth.h"

using namespace geode::prelude;

class $modify(MyProfilePage, ProfilePage) {
    struct Fields {
        CCSprite *statusSprite;
        bool statusSpriteAdded = false;
        int accID;
    };

    void onChangeStatus(CCObject* sender) {
        log::debug("MyProfilePage::onChangeStatus called");

        if (!Mod::get()->getSavedValue<bool>("authenticated") || StatusManager::get()->authFailed) {
            log::debug("Not authenticated or auth failed, starting auth");
            auto auth = Auth::create();
            if (auth) {
                log::debug("Auth object created, starting authentication");
                auth->authenticationStart();
            } else {
                log::warn("Failed to create Auth object");
            }
            log::debug("Closing profile page after auth start");
            this->onClose(sender);
            return;
        }

        auto btn = dynamic_cast<CCNode*>(sender);
        if (!btn) {
            log::warn("MyProfilePage::onChangeStatus - sender is not a CCNode");
            return;
        }
        log::debug("Sender casted to CCNode");

        auto menuItem = dynamic_cast<CCMenuItemSpriteExtra*>(sender);
        if (!menuItem) {
            log::warn("MyProfilePage::onChangeStatus - sender is not a CCMenuItemSpriteExtra");
            return;
        }
        log::debug("Sender casted to CCMenuItemSpriteExtra");

        auto scene = CCDirector::sharedDirector()->getRunningScene();
        if (!scene) {
            log::warn("MyProfilePage::onChangeStatus - no running scene found");
            return;
        }
        log::debug("Got running scene");

        auto pos = btn->convertToWorldSpace({0, 0});
        log::debug("Converted position to world space: ({}, {})", pos.x, pos.y);

        auto statusChangeLayer = StatusChangeLayer::create(pos, menuItem);
        if (!statusChangeLayer) {
            log::warn("MyProfilePage::onChangeStatus - failed to create StatusChangeLayer");
            return;
        }
        log::debug("Created StatusChangeLayer successfully");

        scene->addChild(statusChangeLayer);
        log::debug("StatusChangeLayer added to scene");
    }

    void onClose(CCObject* sender) {
        log::debug("MyProfilePage::onClose called");
        ProfilePage::onClose(sender);
    }

    void loadPageFromUserInfo(GJUserScore* info) {
        log::debug("MyProfilePage::loadPageFromUserInfo called for account ID: {}", info->m_accountID);

        ProfilePage::loadPageFromUserInfo(info);

        if (this->m_fields->statusSpriteAdded) {
            log::debug("Status sprite already added, skipping");
            return;
        }

        this->m_fields->accID = info->m_accountID;

        if (info->m_accountID != GJAccountManager::get()->m_accountID) {
            log::debug("Subscribing to status updates for account ID: {}", info->m_accountID);
            status_mod::subscribeToStatusUpdates(info->m_accountID);
        }

        if (this->m_ownProfile) {
            log::debug("Adding status sprite button for own profile");
            this->m_fields->statusSprite = CCSprite::createWithSpriteFrameName(status_mod::getStatusFrameNameFromType(StatusManager::get()->getCurrentStatus().type));
            this->m_fields->statusSprite->setScale(.75f);
            CCMenuItemSpriteExtra *changeStatusBtn = CCMenuItemSpriteExtra::create(this->m_fields->statusSprite, this, menu_selector(MyProfilePage::onChangeStatus));
            auto usernameMenu = static_cast<CCMenu*>(this->m_mainLayer->getChildByIDRecursive("username-menu"));
            usernameMenu->addChild(changeStatusBtn);
            usernameMenu->updateLayout();
            log::debug("Status sprite button added");
        } else {
            log::debug("Adding status icon for other user");
            auto statusIcon = StatusIconNode::create(info->m_accountID);
            statusIcon->setScale(.75f);
            auto usernameMenu = static_cast<CCMenu*>(this->m_mainLayer->getChildByIDRecursive("username-menu"));
            usernameMenu->addChild(statusIcon);
            usernameMenu->updateLayout();
            log::debug("Status icon added");
        }

        this->m_fields->statusSpriteAdded = true;
        log::debug("Marked status sprite as added");
    }
};
