// Ship Straight Assist: while you spam-click in ship mode, the ship's vertical
// speed is damped so it holds a straight line instead of bouncing up and down.
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <deque>

using namespace geode::prelude;

namespace {
    using Clock = std::chrono::steady_clock;

    std::deque<Clock::time_point> g_clicks;   // recent ship clicks
    Clock::time_point g_lastClick{};
    bool g_active = false;

    template <class T> T S(char const* key) { return Mod::get()->getSettingValue<T>(key); }

    double secondsSince(Clock::time_point t) {
        return std::chrono::duration<double>(Clock::now() - t).count();
    }

    void reset() {
        g_clicks.clear();
        g_active = false;
    }

    // Called for every press. Turns the assist on once enough clicks land inside the window.
    void registerClick() {
        auto now = Clock::now();
        g_lastClick = now;
        g_clicks.push_back(now);

        double window = S<double>("click-window");
        while (!g_clicks.empty() && secondsSince(g_clicks.front()) > window) g_clicks.pop_front();

        if (static_cast<int64_t>(g_clicks.size()) >= S<int64_t>("click-count")) g_active = true;
    }
}

class $modify(AssistGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        if (!down || button != 1 || !S<bool>("enabled")) return;
        auto* player = isPlayer1 ? m_player1 : m_player2;
        if (player && player->m_isShip) registerClick();
    }
};

class $modify(AssistPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        if (!g_active || !S<bool>("enabled") || !this->m_isShip) return;
        auto* pl = PlayLayer::get();
        if (!pl || (this != pl->m_player1 && this != pl->m_player2)) return;

        // switch off after the player stops spamming
        if (secondsSince(g_lastClick) > S<double>("release-time")) {
            g_active = false;
            g_clicks.clear();
            return;
        }

        // damp vertical speed: strength 1.0 = dead flat, lower = gentler
        this->m_yVelocity *= (1.0 - S<double>("strength"));
    }
};

class $modify(AssistPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        reset();
    }
    void onQuit() {
        reset();
        PlayLayer::onQuit();
    }
};
