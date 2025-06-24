#include <Geode/Geode.hpp>
#include <Geode/modify/GJLevelScoreCell.hpp>
#include "utils.hpp"
#include "StatusIconNode.hpp"
#include "StatusManager.hpp"

using namespace geode::prelude;

class $modify(MyLevelScoreCell, GJLevelScoreCell) {
    void loadFromScore(GJUserScore * p0) {
        GJLevelScoreCell::loadFromScore(p0);
        if (p0->m_accountID != GJAccountManager::get()->m_accountID) {
            status_mod::subscribeToStatusUpdates(p0->m_accountID);
        }

        auto playerIcon = m_mainLayer->getChildByType<SimplePlayer>(0);

        auto statusIcon = StatusIconNode::create(p0->m_accountID);

        statusIcon->setScale(0.45f);

        if (playerIcon) {
            statusIcon->setPosition({13.f, -13.f});
            playerIcon->addChild(statusIcon);
        }

        statusIcon->setZOrder(1);
    }
};