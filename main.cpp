#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>


struct Tokens {
    std::string accessToken;
    std::string refreshToken;
    std::chrono::system_clock::time_point expiryTime;
};

class AuthManager {
private:
    Tokens tokens;
    std::mutex mutex;
    std::condition_variable cv;
    bool isRefreshing = false;

public:
    AuthManager() {
        login("user@example.com", "password123");
    }

    void login(const std::string& email, const std::string& password) {

        tokens.accessToken = "initial_access_token";
        tokens.refreshToken = "initial_refresh_token";
        tokens.expiryTime = std::chrono::system_clock::now() + std::chrono::seconds(10); // 10 saniyəlik token
        std::cout << "[Login] Token alınmış.\n";
    }

    bool isTokenExpired() {
        return std::chrono::system_clock::now() >= tokens.expiryTime;
    }


    void refreshToken() {
        std::unique_lock<std::mutex> lock(mutex);
        if (isRefreshing) {

            cv.wait(lock, [this] { return !isRefreshing; });
            return;
        }
        isRefreshing = true;

        std::cout << "[Refresh] Token yenilənir...\n";
        lock.unlock();


        std::this_thread::sleep_for(std::chrono::seconds(2));


        lock.lock();
        tokens.accessToken = "refreshed_token_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        tokens.expiryTime = std::chrono::system_clock::now() + std::chrono::seconds(10);
        isRefreshing = false;
        cv.notify_all();
        std::cout << "[Refresh] Token yeniləndi.\n";
    }

    std::string getAccessToken() {
        if (isTokenExpired()) {
            refreshToken();
        }
        return tokens.accessToken;
    }
};

class DataFetcher {
private:
    AuthManager& auth;

public:
    DataFetcher(AuthManager& authManager) : auth(authManager) {}

    void fetchData() {
        std::string token = auth.getAccessToken();
        std::cout << "[Fetch] Data çəkilir. Token: " << token << "\n";


        static int fetchCount = 0;
        fetchCount++;
        if (fetchCount % 3 == 0) {

            std::cout << "[Fetch] 401 səhv alındı. Token yeniləmək lazımdır.\n";
            auth.refreshToken(); // Yenilənir
            fetchData(); // Yenidən cəhd
        } else {

            std::cout << "[Fetch] Məlumat uğurla çəkildi.\n";
        }
    }
};

int main() {
    AuthManager auth;
    DataFetcher fetcher(auth);


    for (int i = 0; i < 5; ++i) {
        fetcher.fetchData();
        std::this_thread::sleep_for(std::chrono::seconds(3)); // Hər 3 saniyəlik aralıq
    }

    return 0;
}