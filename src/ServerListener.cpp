#include <ixwebsocket/IXWebSocket.h>
#include "ServerListener.hpp"
#include "StatusManager.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

std::unique_ptr<ix::WebSocket> ws = nullptr;

void ServerListener::connectAsync() {
    std::thread t(connect);
    t.detach();
}

void ServerListener::connect() {
    open();

    if (!ws) return;

    StatusManager::get()->isWSOpen = true;

    ws->setOnMessageCallback([](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            onMessageThreaded(msg->str);
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            geode::log::error("WebSocket Error: {}", msg->errorInfo.reason);
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            StatusManager::get()->isWSOpen = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            geode::Loader::get()->queueInMainThread([]() {
                if (StatusManager::get()->autoReconnect) {
                    ServerListener::connectAsync();
                }
                status_mod::clearStatusSubscriptions();
            });
        }
    });

    ws->start();

    // Send auth message
    matjson::Value object;
    matjson::Value dataObject;
    dataObject.set("token", geode::Mod::get()->getSavedValue<std::string>("token"));
    object.set("packet_type", "authorization");
    object.set("data", dataObject.dump());
    ws->send(object.dump());
}

void ServerListener::onMessageThreaded(std::string message) {
    matjson::Value object = matjson::parse(message).unwrap();
    if (!object.contains("packet_type") || !object.contains("data")) return;

    std::string packet_type = object["packet_type"].asString().unwrap();
    std::string packet_data = object["data"].asString().unwrap();

    if (packet_type == "auth_response") {
        matjson::Value data = matjson::parse(packet_data).unwrap();
        if (!data.contains("success")) return;

        if (data["success"].asBool().unwrap()) {
            matjson::Value object;
            matjson::Value dataObject;
            object.set("packet_type", "get_user_settings");
            object.set("data", dataObject.dump());
            ws->send(object.dump());
        } else {
            geode::log::error("WS Authentication failed!");
        }
    }

    if (packet_type == "ping") {
        matjson::Value object;
        matjson::Value dataObject;
        object.set("packet_type", "pong");
        object.set("data", dataObject.dump());
        ws->send(object.dump());
    }

    if (packet_type == "auth_failed") {
        Mod::get()->setSavedValue<bool>("authenticated", false);
        StatusManager::get()->isWSOpen = false;
        StatusManager::get()->autoReconnect = false;
        StatusManager::get()->autoRunAuth = true;
        StatusManager::get()->reset();
        Loader::get()->queueInMainThread([]() {
            createQuickPopup(
                "Uh Oh!",
                "You've been <cr>deauthorized</c> from <cg>Statuses</c>!\n"
                "Would you like to open your <co>profile</c> to <cy>authenticate</c> again?\n"
                "<cb>(To authenticate you need to press the Status Icon on your profile)</c>",
                "Later", "OPEN",
                [](auto, bool btn2) {
                    if (btn2) {
                        ProfilePage::create(GJAccountManager::get()->m_accountID, true)->show();
                    }
                });
        });
    }

    if (packet_type == "user_settings_response") {
        matjson::Value data = matjson::parse(packet_data).unwrap();
        status_mod::Status status;
        if (!data.contains("status")) return;
        status.type = status_mod::getStatusTypeFromString(data["status"].asString().unwrap().c_str());
        status.hasCustomStatus = data.contains("customStatus");
        if (status.hasCustomStatus) {
            status.customStatus = data["customStatus"].asString().unwrap().c_str();
            StatusManager::get()->setCurrentStatus(status.type, status.customStatus);
        } else {
            StatusManager::get()->setCurrentStatus(status.type);
        }
    }

    if (packet_type == "status_update") {
        matjson::Value data = matjson::parse(packet_data).unwrap();
        if (!data.contains("accID") || !data.contains("status")) return;
        int accountID = data["accID"].asInt().unwrap();
        status_mod::Status status;
        status.type = status_mod::getStatusTypeFromString(data["status"].asString().unwrap().c_str());
        status.hasCustomStatus = data.contains("customStatus");
        if (status.hasCustomStatus) {
            status.customStatus = data["customStatus"].asString().unwrap().c_str();
        }
        StatusManager::get()->updateUserStatus(accountID, status);
    }
}

void ServerListener::sendMessage(std::string message) {
    if (StatusManager::get()->isWSOpen && ws) {
        ws->send(message);
    }
}

void ServerListener::closeSocketAndDeauth() {
    if (StatusManager::get()->isWSOpen && ws) {
        ws->close();
    }

    if (Mod::get()->getSavedValue<bool>("authenticated") == true) {
        Mod::get()->setSavedValue<bool>("authenticated", false);
        Mod::get()->setSavedValue<std::string>("token", "");
    }

    StatusManager::get()->reset();
}

void ServerListener::open() {
    StatusManager::get()->isWSOpen = true;

    ws.reset(new ix::WebSocket());
    ws->setUrl(fmt::format("ws://ws.{}/", StatusManager::get()->domain));
    geode::log::info("Connecting to the server...");
}
