#include <nlohmann/json.hpp>
#include <MQTTClient.h>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdio>

#include "aiotek_mqtt.hpp"

class MQTTImplement {
  private:
    ::MQTTClient m_client;
    MQTTManager::MQTTConfig m_config;
    std::atomic<MQTTManager::State> m_state;

    static void connectionLost(void* context, char* cause)
    {
        auto* impl = static_cast<MQTTImplement*>(context);
        if (!impl) {
            return;
        }

        impl->m_state.store(MQTTManager::State::Disconnected);

        if (impl->errorCallback) {
            impl->errorCallback("Connection lost: " + std::string(cause ? cause : "Unknown"));
        }
        if (impl->disconnectCallback) {
            impl->disconnectCallback();
        }
    }
    static int messageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* message)
    {
        auto* impl = static_cast<MQTTImplement*>(context);
        if (!impl || !impl->messageCallback) {
            MQTTClient_freeMessage(&message);
            MQTTClient_free(topicName);
            return 1;
        }

        try {
            std::string topic(topicName, topicLen);

            std::string payload(static_cast<char*>(message->payload), message->payloadlen);
            nlohmann::json data = nlohmann::json::parse(payload);

            impl->messageCallback(topic, static_cast<std::string>(data));
        } catch (const std::exception& e) {
            if (impl->errorCallback) {
                impl->errorCallback("Message parsing error: " + std::string(e.what()));
            }
        }

        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }
    static void deliveryComplete(void* context, MQTTClient_deliveryToken dt)
    {
        auto* impl = static_cast<MQTTImplement*>(context);
        if (!impl || !impl->publishAckCb) {
            return;
        }

        impl->publishAckCb("", static_cast<int>(dt));
    }

  public:
    std::function<void()> connectCallback;
    std::function<void()> disconnectCallback;
    std::function<void(const std::string&)> errorCallback;
    std::function<void(const std::string&, const nlohmann::json&)> messageCallback;
    std::function<void(const std::string&, int)> publishAckCb;

    MQTTImplement() : m_client(nullptr), m_state(MQTTManager::State::Disconnected)
    {
    }
    ~MQTTImplement()
    {
        if (m_client) {
            if (m_state.load() == MQTTManager::State::Connected) {
                disconnect();
            }

            MQTTClient_destroy(&m_client);
        }
    }

    int32_t loadConfigFromFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << path << std::endl;
            return -1;
        }

        try {
            nlohmann::json j;
            file >> j;
            if (j.contains("MQTT")) {
                m_config = j.at("MQTT").get<MQTTManager::MQTTConfig>();
            } else {
                std::cerr << "Missing 'MQTT' field in config" << std::endl;
                return -2;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing config file: " << e.what() << std::endl;
            return -3;
        }

        return 0;
    }

    int32_t saveConfigToFile(const std::string& path) const
    {
        std::ofstream out(path);
        if (!out)
            return false;
        nlohmann::json j = m_config;
        out << j.dump(4);
        return true;
    }

    MQTTManager::MQTTConfig getConfig() const
    {
        nlohmann::json j = m_config;
        std::cout << j.dump(4) << std::endl;
        return m_config;
    }

    void setConfig(const MQTTManager::MQTTConfig& config)
    {
        m_config = config;
    }

    void disconnect()
    {
        if (m_state.load() == MQTTManager::State::Disconnected) {
            return;
        }

        m_state.store(MQTTManager::State::Disconnecting);

        if (m_client) {
            MQTTClient_disconnect(m_client, 1000);
        }

        m_state.store(MQTTManager::State::Disconnected);

        if (disconnectCallback) {
            disconnectCallback();
        }
    }

    int32_t connect()
    {
        int32_t ret = 0;
        MQTTManager::MQTTConfig config;
        std::string serverURI = "";

        if (m_state.load() == MQTTManager::State::Connected) {
            if (errorCallback) {
                errorCallback("Already connected");
                return -1;
            }
        }

        ret = loadConfigFromFile("./config/MQTT.json");
        if (ret != 0) {
            /* get default config */
            ret = loadConfigFromFile("./config/MQTT.json");
        }

        m_state.store(MQTTManager::State::Connecting);

        serverURI = "ssl://" + m_config.broker + ":" + std::to_string(m_config.port);
        MQTTClient_create(&m_client, serverURI.c_str(), m_config.clientId.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
        if (ret != MQTTCLIENT_SUCCESS) {
            m_state.store(MQTTManager::State::Disconnected);
            if (errorCallback) {
                errorCallback("Failed to create MQTT client: " + std::to_string(ret));
            }
            return ret;
        }

        MQTTClient_setCallbacks(m_client, this, connectionLost, messageArrived, deliveryComplete);
        MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        conn_opts.keepAliveInterval = m_config.keepAlive;
        conn_opts.cleansession = 1;
        conn_opts.username = m_config.username.c_str();
        conn_opts.password = m_config.password.c_str();

        MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
        ssl_opts.trustStore = m_config.caCertPath.empty() ? nullptr : m_config.caCertPath.c_str();
        ssl_opts.keyStore = m_config.clientCertPath.empty() ? nullptr : m_config.clientCertPath.c_str();
        ssl_opts.privateKey = m_config.clientKeyPath.empty() ? nullptr : m_config.clientKeyPath.c_str();
        conn_opts.ssl = &ssl_opts;

        MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
        if (!m_config.willTopic.empty() && !m_config.willMessage.empty()) {
            will_opts.topicName = m_config.willTopic.c_str();
            will_opts.message = m_config.willMessage.c_str();
            will_opts.qos = m_config.willQos;
            will_opts.retained = m_config.willRetained ? 1 : 0;
            conn_opts.will = &will_opts;
        }

        ret = MQTTClient_connect(m_client, &conn_opts);
        if (ret != MQTTCLIENT_SUCCESS) {
            m_state.store(MQTTManager::State::Disconnected);
            if (errorCallback) {
                errorCallback("Connection failed: " + std::to_string(ret));
            }

            return -1;
        }

        m_state.store(MQTTManager::State::Connected);
        if (connectCallback) {
            connectCallback();
        }

        return ret;
    }

    int32_t publish(const std::string& topic, const nlohmann::json& data)
    {
        if (m_state.load() != MQTTManager::State::Connected || !m_client) {
            return -1;
        }
        try {
            std::string payload = data.dump();
            MQTTClient_message pubmsg = MQTTClient_message_initializer;
            pubmsg.payload = const_cast<char*>(payload.c_str());
            pubmsg.payloadlen = static_cast<int>(payload.length());
            pubmsg.qos = 1;
            pubmsg.retained = 0;

            MQTTClient_deliveryToken token;
            int result = MQTTClient_publishMessage(m_client, topic.c_str(), &pubmsg, &token);

            if (result != MQTTCLIENT_SUCCESS) {
                return -1;
            }

            return 0;

        } catch (const std::exception& e) {
            if (errorCallback)
                errorCallback("Publish error: " + std::string(e.what()));
            return -1;
        }
    }

    int32_t subscribe(const std::string& topic)
    {
        if (m_state.load() != MQTTManager::State::Connected || !m_client) {
            if (errorCallback) {
                errorCallback("Subscribe failed: not connected or client not initialized");
            }
            return -1;
        }

        int result = MQTTClient_subscribe(m_client, topic.c_str(), 1);
        if (result != MQTTCLIENT_SUCCESS) {
            if (errorCallback) {
                errorCallback("Subscribe to topic '" + topic + "' failed! Error code: " + std::to_string(result));
                return -1;
            }
        }
        return 0;
    }

    bool isConnected() const
    {
        if (m_state.load() == MQTTManager::State::Connected) {
            return true;
        } else {
            return false;
        }
    }

    MQTTManager::State getState() const
    {
        return m_state.load();
    }
};

MQTTManager::MQTTManager() : pImplement(std::make_unique<MQTTImplement>())
{
}

MQTTManager::~MQTTManager() = default;

int32_t MQTTManager::connect()
{
    return pImplement->connect();
}

void MQTTManager::disconnect()
{
    return pImplement->disconnect();
}

int32_t MQTTManager::publish(const std::string& topic, const nlohmann::json& payload)
{
    return pImplement->publish(topic, payload);
}

int32_t MQTTManager::subscribe(const std::string& topic)
{
    return pImplement->subscribe(topic);
}

void MQTTManager::setConfig(const MQTTConfig& config)
{
    pImplement->setConfig(config);
}

MQTTManager::MQTTConfig MQTTManager::getConfig() const
{
    return pImplement->getConfig();
}

int32_t MQTTManager::loadConfigFromFile(const std::string& path)
{
    return pImplement->loadConfigFromFile(path);
}

int32_t MQTTManager::saveConfigToFile(const std::string& path) const
{
    return pImplement->saveConfigToFile(path);
}

void MQTTManager::onConnect(std::function<void()> callback)
{
    if (callback) {
        std::cout << "Register callback 'onConnect' for MQTT" << std::endl;
        pImplement->connectCallback = std::move(callback);
    }
}

void MQTTManager::onDisconnect(std::function<void()> callback)
{
    if (callback) {
        std::cout << "Register callback 'onDisconnect' for MQTT" << std::endl;
        pImplement->disconnectCallback = std::move(callback);
    }
}

void MQTTManager::onError(std::function<void(const std::string&)> callback)
{
    if (callback) {
        std::cout << "Register callback 'onError' for MQTT" << std::endl;
        pImplement->errorCallback = std::move(callback);
    }
}

void MQTTManager::onMessage(std::function<void(const std::string& topic, const std::string& data)> callback)
{
    if (callback) {
        std::cout << "Register callback 'onMessage' for MQTT" << std::endl;
        pImplement->messageCallback = std::move(callback);
    }
}

int32_t MQTTManager::send(const std::string& payload)
{
    std::cout << payload << std::endl;
    return 0;
}
int32_t MQTTManager::receive(std::string& payload)
{
    std::cout << payload << std::endl;
    return 0;
}

bool MQTTManager::isConnected()
{
    return pImplement->isConnected();
}

MQTTManager::State MQTTManager::getState()
{
    return pImplement->getState();
}