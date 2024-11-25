#include <rxcpp/rx.hpp>
#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>


const std::string devicePath = "/dev/input/event17";

class KeyboardTracker {
private:
    std::atomic<bool> running{true};
    std::ofstream logFile;

    rxcpp::subjects::subject<std::string> eventSubject;

    struct xkb_context* context;
    struct xkb_keymap* keymap;
    struct xkb_state* state;

    void logEvent(const std::string& message) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string timestamp = std::ctime(&now);
        timestamp.pop_back();
        std::string logMessage = timestamp + ": " + message;

        eventSubject.get_subscriber().on_next(logMessage);
    }

    std::string getKeySymbol(uint16_t code) {
        xkb_keysym_t keysym = xkb_state_key_get_one_sym(state, code + 8);
        char buffer[64];
        xkb_keysym_get_name(keysym, buffer, sizeof(buffer));
        return std::string(buffer);
    }

    void trackEvents() {
        int fd = open(devicePath.c_str(), O_RDONLY);
        if (fd == -1) {
            eventSubject.get_subscriber().on_error(
                std::make_exception_ptr(std::runtime_error("Ошибка: не удалось открыть устройство ввода.")));
            return;
        }

        logEvent("=== Новая сессия трекера ===");
        logEvent("Трекер запущен. Для выхода нажмите 'q'");

        struct input_event ev;
        while (running) {
            if (read(fd, &ev, sizeof(ev)) > 0 && ev.type == EV_KEY) {
                if (ev.value == 1) {
                    std::string keyName = getKeySymbol(ev.code);
                    logEvent("Нажата клавиша: " + keyName);

                    if (keyName == "q") {
                        logEvent("Обнаружена клавиша 'q'. Завершение работы...");
                        running = false;
                        break;
                    }
                }
            }
        }

        close(fd);
        eventSubject.get_subscriber().on_completed();
    }

public:
    KeyboardTracker() : logFile("keyboard_events.log", std::ios::app) {
        if (!logFile.is_open()) {
            throw std::runtime_error("Не удалось открыть лог-файл");
        }

        context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        if (!context) {
            throw std::runtime_error("Не удалось создать xkb_context");
        }

        keymap = xkb_keymap_new_from_names(context, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (!keymap) {
            throw std::runtime_error("Не удалось создать xkb_keymap");
        }

        state = xkb_state_new(keymap);
        if (!state) {
            throw std::runtime_error("Не удалось создать xkb_state");
        }
    }

    ~KeyboardTracker() {
        if (state) xkb_state_unref(state);
        if (keymap) xkb_keymap_unref(keymap);
        if (context) xkb_context_unref(context);

        if (logFile.is_open()) {
            logFile.close();
        }
    }

    rxcpp::observable<std::string> getObservable() {
        return eventSubject.get_observable();
    }

    void start() {
        std::thread trackingThread([this]() {
            trackEvents();
        });
        trackingThread.detach();
    }

    void stop() {
        running = false;
    }

    bool isRunning() const {
        return running;
    }
};

int main() {
    try {
        KeyboardTracker tracker;

        // Подписчик на события
        auto subscription = tracker.getObservable().subscribe(
            [&tracker](const std::string& event) {
                static std::ofstream logFile("keyboard_events.log", std::ios::app);
                if (!logFile.is_open()) {
                    std::cerr << "Ошибка: не удалось открыть лог-файл" << std::endl;
                    tracker.stop();
                }
                logFile << event << std::endl;
                std::cout << event << std::endl;
            },
            [](std::exception_ptr eptr) {
                try {
                    std::rethrow_exception(eptr);
                } catch (const std::exception& e) {
                    std::cerr << "Ошибка: " << e.what() << std::endl;
                }
            },
            []() {
                std::cout << "Трекер завершил работу." << std::endl;
            });

        tracker.start();

        while (tracker.isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        std::cerr << "Фатальная ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
