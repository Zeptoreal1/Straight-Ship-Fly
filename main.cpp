// Ship Straight Assist v1.1.0
// While you spam-click in ship mode, the ship's vertical speed is damped so it holds a
// straight line instead of bouncing. v1.1 adds a pause-menu GUI, presets, a soft ramp,
// hard flatten, cooldown, practice-only mode, player 2 control, an optional quick-toggle
// button and session stats. No on-screen text.
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <algorithm>
#include <chrono>
#include <deque>
#include <map>

using namespace geode::prelude;

// ============================================================ core logic
namespace {
    using Clock = std::chrono::steady_clock;

    std::deque<Clock::time_point> g_clicks;     // recent ship clicks
    Clock::time_point g_lastClick{};
    Clock::time_point g_activeSince{};
    Clock::time_point g_cooldownUntil{};
    bool   g_active = false;
    int    g_triggers = 0;                      // stats for this game session
    double g_assistedSeconds = 0.0;

    Ref<CCMenu> g_quickMenu;

    template <class T> T S(char const* key) { return Mod::get()->getSettingValue<T>(key); }

    double secondsSince(Clock::time_point t) {
        return std::chrono::duration<double>(Clock::now() - t).count();
    }

    bool allowedIn(PlayLayer* pl) {
        if (!pl || !S<bool>("enabled")) return false;
        if (S<bool>("practice-only") && !pl->m_isPracticeMode) return false;
        return true;
    }

    void activate() {
        if (g_active) return;
        g_active = true;
        g_activeSince = Clock::now();
        ++g_triggers;
    }

    void deactivate() {
        if (g_active) {
            g_assistedSeconds += secondsSince(g_activeSince);
            g_active = false;
            g_cooldownUntil = Clock::now() + std::chrono::milliseconds(static_cast<int>(S<double>("cooldown") * 1000.0));
        }
        g_clicks.clear();
    }

    void registerClick() {
        auto now = Clock::now();
        g_lastClick = now;
        if (now < g_cooldownUntil) return;
        g_clicks.push_back(now);

        double window = S<double>("click-window");
        while (!g_clicks.empty() && secondsSince(g_clicks.front()) > window) g_clicks.pop_front();
        if (static_cast<int64_t>(g_clicks.size()) >= S<int64_t>("click-count")) activate();
    }

    // how strongly to damp right now (0..1), including the optional soft ramp
    double effectiveStrength() {
        double s = S<double>("strength");
        double ramp = S<double>("ramp-time");
        if (ramp > 0.001) s *= std::clamp(secondsSince(g_activeSince) / ramp, 0.0, 1.0);
        return s;
    }

    // ---- settings helpers used by the GUI ----
    void setBool(char const* k, bool v) { Mod::get()->setSettingValue<bool>(k, v); }
    void setInt(char const* k, int64_t v) { Mod::get()->setSettingValue<int64_t>(k, v); }
    void setFloat(char const* k, double v) { Mod::get()->setSettingValue<double>(k, v); }

    struct Preset { char const* name; int clicks; double window, release, strength; };
    constexpr Preset kPresets[] = {
        {"Soft",     4, 0.5, 0.15, 0.3},
        {"Balanced", 3, 0.5, 0.20, 0.5},
        {"Strong",   3, 0.6, 0.30, 0.8},   // the original v1.0 behaviour
    };
}

// ============================================================ hooks (gameplay)
class $modify(AssistGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        if (!down || button != 1) return;
        auto* pl = PlayLayer::get();
        if (!allowedIn(pl)) return;
        auto* player = isPlayer1 ? m_player1 : m_player2;
        if (player && player->m_isShip) registerClick();
    }
};

class $modify(AssistPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);
        if (!g_active || !this->m_isShip) return;

        auto* pl = PlayLayer::get();
        if (!pl || (this != pl->m_player1 && this != pl->m_player2)) return;
        if (!allowedIn(pl)) { deactivate(); return; }
        if (this == pl->m_player2 && !S<bool>("affect-player2")) return;

        // switch off after the player stops spamming (player 1 drives the timer)
        if (this == pl->m_player1 && secondsSince(g_lastClick) > S<double>("release-time")) {
            deactivate();
            return;
        }

        if (S<bool>("hard-flatten")) this->m_yVelocity = 0.0;
        else this->m_yVelocity *= (1.0 - effectiveStrength());
    }
};

// ============================================================ settings GUI
namespace {
    struct TRow { char const* key; char const* label; };
    struct SRow { char const* key; char const* label; double mn, mx, step; bool isFloat; int decimals; };

    constexpr TRow kToggles[] = {
        {"enabled", "Enabled"},
        {"hard-flatten", "Hard flatten"},
        {"practice-only", "Practice only"},
        {"affect-player2", "Affect player 2"},
        {"quick-button", "Quick button"},
    };
    constexpr SRow kSteppers[] = {
        {"click-count", "Clicks", 2, 10, 1, false, 0},
        {"click-window", "Window (s)", 0.1, 2.0, 0.1, true, 1},
        {"release-time", "Stop delay (s)", 0.05, 1.5, 0.05, true, 2},
        {"strength", "Strength", 0.1, 1.0, 0.1, true, 1},
        {"ramp-time", "Soft ramp (s)", 0.0, 1.0, 0.1, true, 1},
        {"cooldown", "Cooldown (s)", 0.0, 1.0, 0.1, true, 1},
    };

    std::string fmtVal(SRow const& r) {
        if (r.isFloat) return fmt::format("{:.{}f}", S<double>(r.key), r.decimals);
        return std::to_string(S<int64_t>(r.key));
    }
}

class AssistPopup : public geode::Popup {
protected:
    std::map<std::string, CCMenuItemToggler*> m_togglers;
    std::map<std::string, CCLabelBMFont*> m_vals;
    CCLabelBMFont* m_status = nullptr;

    bool init() {
        if (!Popup::init(420.f, 300.f)) return false;
        this->setTitle("Ship Straight Assist");

        // presets row
        auto presetLabel = CCLabelBMFont::create("Presets", "bigFont.fnt");
        presetLabel->setScale(.4f);
        presetLabel->setAnchorPoint({0.f, .5f});
        presetLabel->setPosition({24.f, 242.f});
        m_mainLayer->addChild(presetLabel);
        for (int i = 0; i < 3; ++i) {
            auto spr = ButtonSprite::create(kPresets[i].name, 78, true, "bigFont.fnt", "GJ_button_04.png", 24.f, .5f);
            auto b = CCMenuItemSpriteExtra::create(spr, this, menu_selector(AssistPopup::onPreset));
            b->setTag(i);
            b->setPosition({150.f + i * 96.f, 242.f});
            m_buttonMenu->addChild(b);
        }

        // toggles (left column)
        float top = 202.f;
        for (int i = 0; i < (int)std::size(kToggles); ++i) {
            auto& r = kToggles[i];
            float y = top - i * 27.f;
            auto t = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(AssistPopup::onToggle), .55f);
            t->setID(r.key);
            t->toggle(S<bool>(r.key));
            t->setPosition({30.f, y});
            m_buttonMenu->addChild(t);
            m_togglers[r.key] = t;
            auto l = CCLabelBMFont::create(r.label, "bigFont.fnt");
            l->setScale(.4f);
            l->setAnchorPoint({0.f, .5f});
            l->setPosition({48.f, y});
            m_mainLayer->addChild(l);
        }

        // steppers (right column)
        for (int i = 0; i < (int)std::size(kSteppers); ++i) {
            auto& r = kSteppers[i];
            float y = top - i * 27.f;
            auto l = CCLabelBMFont::create(r.label, "bigFont.fnt");
            l->setScale(.34f);
            l->setAnchorPoint({0.f, .5f});
            l->setPosition({190.f, y});
            m_mainLayer->addChild(l);

            auto mk = [&](char const* txt, int dir, float x) {
                auto spr = ButtonSprite::create(txt, 20, true, "bigFont.fnt", "GJ_button_04.png", 20.f, .55f);
                auto b = CCMenuItemSpriteExtra::create(spr, this, menu_selector(AssistPopup::onStep));
                b->setID(r.key);
                b->setTag(dir);
                b->setPosition({x, y});
                m_buttonMenu->addChild(b);
            };
            mk("-", -1, 318.f);
            mk("+", 1, 392.f);

            auto v = CCLabelBMFont::create(fmtVal(r).c_str(), "goldFont.fnt");
            v->setScale(.5f);
            v->setPosition({355.f, y});
            m_mainLayer->addChild(v);
            m_vals[r.key] = v;
        }

        // footer: status + reset
        m_status = CCLabelBMFont::create("", "chatFont.fnt");
        m_status->setScale(.55f);
        m_status->setOpacity(190);
        m_status->setAnchorPoint({0.f, .5f});
        m_status->setPosition({24.f, 24.f});
        m_mainLayer->addChild(m_status);
        this->refreshStatus();

        auto resetSpr = ButtonSprite::create("Reset", 60, true, "bigFont.fnt", "GJ_button_06.png", 24.f, .45f);
        auto resetBtn = CCMenuItemSpriteExtra::create(resetSpr, this, menu_selector(AssistPopup::onReset));
        resetBtn->setPosition({375.f, 24.f});
        m_buttonMenu->addChild(resetBtn);
        return true;
    }

    void refreshStatus() {
        double total = g_assistedSeconds + (g_active ? secondsSince(g_activeSince) : 0.0);
        m_status->setString(fmt::format("Session: triggered {}x, assisted {:.1f}s", g_triggers, total).c_str());
    }

    void refreshAll() {
        for (auto& [key, t] : m_togglers) t->toggle(S<bool>(key.c_str()));
        for (auto& r : kSteppers) m_vals[r.key]->setString(fmtVal(r).c_str());
    }

    void onToggle(CCObject* s) {
        auto t = static_cast<CCMenuItemToggler*>(s);
        setBool(t->getID().c_str(), !t->isToggled());
    }

    void onStep(CCObject* s) {
        auto n = static_cast<CCNode*>(s);
        std::string key = n->getID();
        int dir = n->getTag();
        for (auto& r : kSteppers) {
            if (key != r.key) continue;
            if (r.isFloat) {
                double v = S<double>(r.key) + dir * r.step;
                v = std::round(v * 100.0) / 100.0;
                setFloat(r.key, std::clamp(v, r.mn, r.mx));
            } else {
                setInt(r.key, std::clamp<int64_t>(S<int64_t>(r.key) + dir * (int64_t)r.step, (int64_t)r.mn, (int64_t)r.mx));
            }
            m_vals[key]->setString(fmtVal(r).c_str());
        }
    }

    void onPreset(CCObject* s) {
        auto const& p = kPresets[static_cast<CCNode*>(s)->getTag()];
        setInt("click-count", p.clicks);
        setFloat("click-window", p.window);
        setFloat("release-time", p.release);
        setFloat("strength", p.strength);
        this->refreshAll();
    }

    void onReset(CCObject*) {
        Ref<AssistPopup> self = this;
        createQuickPopup("Reset settings", "Put <cy>all</c> Ship Straight Assist settings back to default?",
            "Cancel", "Reset", [self](FLAlertLayer*, bool yes) {
                if (!yes) return;
                auto const& p = kPresets[1]; // Balanced = default
                setInt("click-count", p.clicks);
                setFloat("click-window", p.window);
                setFloat("release-time", p.release);
                setFloat("strength", p.strength);
                setFloat("ramp-time", 0.0);
                setFloat("cooldown", 0.0);
                setBool("enabled", true);
                setBool("hard-flatten", false);
                setBool("practice-only", false);
                setBool("affect-player2", true);
                setBool("quick-button", false);
                self->refreshAll();
            });
    }

public:
    static AssistPopup* create() {
        auto ret = new AssistPopup();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }
};

// ============================================================ hooks (UI)
class $modify(AssistPlayLayer, PlayLayer) {
    struct Fields { ButtonSprite* quickSprite = nullptr; };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        auto win = CCDirector::get()->getWinSize();

        g_quickMenu = nullptr;
        if (S<bool>("quick-button")) {
            auto menu = CCMenu::create();
            menu->setPosition({0, 0});
            menu->setZOrder(100);
            auto spr = ButtonSprite::create("SA", 30, true, "bigFont.fnt", "GJ_button_04.png", 28.f, .55f);
            spr->setOpacity(S<bool>("enabled") ? 160 : 60);
            m_fields->quickSprite = spr;
            auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(AssistPlayLayer::onQuickToggle));
            btn->setPosition({30.f, 115.f});
            menu->addChild(btn);
            this->addChild(menu);
            g_quickMenu = menu;
        }
    }

    void onQuickToggle(CCObject*) {
        bool now = !S<bool>("enabled");
        setBool("enabled", now);
        if (!now) deactivate();
        if (m_fields->quickSprite) m_fields->quickSprite->setOpacity(now ? 160 : 60);
    }

    void pauseGame(bool unfocused) {
        if (g_quickMenu) g_quickMenu->setVisible(false);
        PlayLayer::pauseGame(unfocused);
    }

    void resume() {
        PlayLayer::resume();
        if (g_quickMenu) g_quickMenu->setVisible(true);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        deactivate();
    }

    void onQuit() {
        deactivate();
        g_quickMenu = nullptr;
        PlayLayer::onQuit();
    }
};

// Pause menu button: bottom-left, a bit above the corner, opens the GUI
class $modify(AssistPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto spr = ButtonSprite::create("Assist", 70, true, "goldFont.fnt", "GJ_button_01.png", 34.f, .8f);
        spr->setScale(static_cast<float>(S<double>("pause-button-scale")));
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(AssistPauseLayer::onAssist));
        auto menu = CCMenu::create();
        menu->setPosition({62.f, 78.f});
        menu->setZOrder(100);
        btn->setPosition({0, 0});
        menu->addChild(btn);
        this->addChild(menu);
    }

    void onAssist(CCObject*) {
        if (auto p = AssistPopup::create()) p->show();
    }
};
