#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Event.hpp>

using namespace geode::prelude;

class NoclipATPanel;
static NoclipATPanel* g_currentPanel = nullptr;
static void toggleCurrentPanel();

static int clampPercent(int value) {
    if (value < 0) return 0;
    if (value > 100) return 100;
    return value;
}


static int getPercent() {
    return static_cast<int>(Mod::get()->getSettingValue<int64_t>("activation-percent"));
}

static void setPercent(int value) {
    Mod::get()->setSettingValue("activation-percent", clampPercent(value));
}

static bool getEnabled() {
    return Mod::get()->getSettingValue<bool>("enabled");
}

static void setEnabled(bool value) {
    Mod::get()->setSettingValue("enabled", value);
}

static bool getShowMessage() {
    return Mod::get()->getSettingValue<bool>("show-message");
}


class NoclipATPanel : public CCLayer {
protected:
    CCLayerColor* m_panel = nullptr;
    TextInput* m_percentInput = nullptr;
    CCLabelBMFont* m_enabledText = nullptr;

public:
    static NoclipATPanel* create() {
        auto ret = new NoclipATPanel();

        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;

        this->setContentSize({150.f, 90.f});
        this->buildUI();

        return true;
    }

    void buildUI() {
        this->removeAllChildrenWithCleanup(true);

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        this->addChild(menu, 10);

        auto panel = CCLayerColor::create(ccc4(18, 18, 18, 235), 118.f, 66.f);
        panel->setPosition({0.f, 28.f});
        panel->setVisible(false);
        this->addChild(panel, 5);
        m_panel = panel;

        auto topBar = CCLayerColor::create(ccc4(235, 45, 105, 255), 118.f, 3.f);
        topBar->setPosition({0.f, 63.f});
        panel->addChild(topBar);

        auto title = CCLabelBMFont::create("NOCLIP AT", "goldFont.fnt");
        title->setScale(0.30f);
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({6.f, 54.f});
        panel->addChild(title);

        auto atLabel = CCLabelBMFont::create("At", "bigFont.fnt");
        atLabel->setScale(0.30f);
        atLabel->setAnchorPoint({0.f, 0.5f});
        atLabel->setPosition({8.f, 35.f});
        panel->addChild(atLabel);

        auto enabledLabel = CCLabelBMFont::create("Enabled", "bigFont.fnt");
        enabledLabel->setScale(0.26f);
        enabledLabel->setAnchorPoint({0.f, 0.5f});
        enabledLabel->setPosition({8.f, 14.f});
        panel->addChild(enabledLabel);

        // Percent text box
        auto percentInput = TextInput::create(38.f, "0", "bigFont.fnt");
        percentInput->setPosition({77.f, 35.f});
        percentInput->setScale(0.72f);
        percentInput->setCommonFilter(CommonFilter::Uint);
        percentInput->setMaxCharCount(3);
        percentInput->setString(fmt::format("{}", getPercent()), false);
        percentInput->setCallback([this](std::string const& text) {
            if (text.empty()) return;

            try {
                int value = std::stoi(text);
                setPercent(value);
                this->refreshText();
            }
            catch (...) {}
        });

        panel->addChild(percentInput);
        m_percentInput = percentInput;

        // Clickable ON/OFF text
        auto enabledText = CCLabelBMFont::create("", "bigFont.fnt");
        enabledText->setScale(0.34f);

        auto enabledBtn = CCMenuItemSpriteExtra::create(
            enabledText,
            this,
            menu_selector(NoclipATPanel::onToggleEnabled)
        );
        enabledBtn->setPosition({92.f, 14.f});

        auto panelMenu = CCMenu::create();
        panelMenu->setPosition({0.f, 0.f});
        panelMenu->addChild(enabledBtn);
        panel->addChild(panelMenu, 20);

        m_enabledText = enabledText;

        this->refreshText();
    }

    void refreshText() {
        if (m_percentInput) {
            m_percentInput->setString(fmt::format("{}", getPercent()), false);
        }

        if (m_enabledText) {
            m_enabledText->setString(getEnabled() ? "ON" : "OFF");

            if (getEnabled()) {
                m_enabledText->setColor(ccc3(255, 80, 170));
            } else {
                m_enabledText->setColor(ccc3(255, 255, 255));
            }
        }
    }

    void togglePanel() {
        if (!m_panel) return;
        m_panel->setVisible(!m_panel->isVisible());
        this->refreshText();
    }

    void onTogglePanel(CCObject*) {
        this->togglePanel();
    }

    void onToggleEnabled(CCObject*) {
        setEnabled(!getEnabled());
        this->refreshText();
    }
};


static void refreshtextforenabled() {
    if (g_currentPanel) {
        g_currentPanel->refreshText();
    }
}

static void toggleCurrentPanel() {
    if (g_currentPanel) {
        g_currentPanel->togglePanel();
    }
}

// Keep ALL keybind handling here globally so you don't duplicate listeners
$on_game(Loaded) {
    listenForKeybindSettingPresses("open-menu", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) {
            toggleCurrentPanel();
        }
    });

    listenForKeybindSettingPresses("enable-keybind", [](Keybind const&, bool down, bool repeat, double) {
        if (down && !repeat) {
            setEnabled(!getEnabled());
            refreshtextforenabled();
        }
    });
}

class $modify(NoclipATMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto panel = NoclipATPanel::create();
        panel->setPosition({win.width - 180.f, 42.f});
        this->addChild(panel, 99999);

        // FIX: Assign this panel instance to the global pointer
        g_currentPanel = panel; 

        return true;
    }

    void onExit() {
        MenuLayer::onExit();
        // Clear global pointer when leaving MenuLayer
        if (g_currentPanel == this->getChildByTag(99999)) { // logic safety step
             g_currentPanel = nullptr;
        }
    }
};

class $modify(NoclipATPlayLayer, PlayLayer) {
    struct Fields {
        bool messageShown = false;
        NoclipATPanel* panel = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        m_fields->messageShown = false;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto panel = NoclipATPanel::create();
        panel->setPosition({win.width - 180.f, 42.f});
        this->addChild(panel, 99999);
        m_fields->panel = panel;

        // FIX: Assign this panel instance to the global pointer
        g_currentPanel = panel;

        return true;
    }

    void onExit() {
        PlayLayer::onExit();
        // Clear global pointer when exiting the level
        g_currentPanel = nullptr;
    }

    void resetLevel() {
        m_fields->messageShown = false;
        PlayLayer::resetLevel();
    }

    void update(float dt) {
        PlayLayer::update(dt);

        if (!getEnabled()) return;

        float currentPercent = this->getCurrentPercent();
        int at = getPercent();

        if (!m_fields->messageShown && currentPercent >= static_cast<float>(at)) {
            m_fields->messageShown = true;

            if (getShowMessage()) {
                Notification::create(
                    fmt::format("Noclip AT active at {}%", at),
                    NotificationIcon::Success
                )->show();
            }
        }
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (getEnabled()) {
            float currentPercent = this->getCurrentPercent();
            int at = getPercent();

            if (currentPercent >= static_cast<float>(at)) {
                return;
            }
        }

        PlayLayer::destroyPlayer(player, object);
    }
};
