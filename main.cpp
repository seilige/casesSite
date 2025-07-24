#include <drogon/drogon.h>
#include <fstream>
#include <sstream>
#include <random>
#include <thread>
#include <chrono>
#include <json/json.h>
#include <unordered_map>
#include <set>
#include "database.cpp"

std::string getRandomItem(const std::vector<std::string> &items, std::set<std::string> &usedItems) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::vector<std::string> availableItems;

    // Выбираем только те предметы, которые еще не использованы
    for (const auto &item : items) {
        if (usedItems.find(item) == usedItems.end()) {
            availableItems.push_back(item);
        }
    }

    // Если все предметы уже использованы, сбрасываем список
    if (availableItems.empty()) {
        usedItems.clear();
        availableItems = items;
    }

    std::uniform_int_distribution<> dis(0, availableItems.size() - 1);
    std::string selectedItem = availableItems[dis(gen)];
    usedItems.insert(selectedItem); // Добавляем выбранный предмет в использованные
    return selectedItem;
}

static Database db("users.db");

int main() {
    std::filesystem::current_path("C:/Users/Acer/Desktop/main");
    // Хранилище использованных предметов для каждого кейса
    std::unordered_map<std::string, std::set<std::string>> usedItemsMap;
    // Включаем поддержку сессий
    drogon::app().enableSession(3600); // 20 минут

    // Замените существующий обработчик "/" на этот:
    drogon::app().registerHandler("/", [](const drogon::HttpRequestPtr &,
                                        std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
        std::ifstream file("views/auth.html");
        if (!file.is_open()) {
            auto resp = drogon::HttpResponse::newNotFoundResponse();
            callback(resp);
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);
        resp->setBody(buffer.str());
        callback(resp);
    });

drogon::app().registerHandler("/cases", [](const drogon::HttpRequestPtr &req,
                                           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    // Получаем сессию пользователя
    auto sessionPtr = req->session();
    
    // Проверяем существование сессии
    if (!sessionPtr) {
        auto resp = drogon::HttpResponse::newRedirectionResponse("/");
        callback(resp);
        return;
    }

    bool isLoggedIn = false;
    if (sessionPtr->find("logged_in")) {
        try {
            isLoggedIn = sessionPtr->get<bool>("logged_in");
        } catch (const std::exception& e) {
            std::cerr << "Error getting logged_in status: " << e.what() << std::endl;
            auto resp = drogon::HttpResponse::newRedirectionResponse("/");
            callback(resp);
            return;
        }
    } else {
        std::cerr << "logged_in not found in session" << std::endl;
        auto resp = drogon::HttpResponse::newRedirectionResponse("/");
        callback(resp);
        return;
    }

    // Если пользователь не авторизован, перенаправляем на главную страницу
    if (!isLoggedIn) {
        auto resp = drogon::HttpResponse::newRedirectionResponse("/");
        callback(resp);
        return;
    }
    
    std::string username;
    if (sessionPtr->find("username")) {
        try {
            username = sessionPtr->get<std::string>("username");
        } catch (const std::exception& e) {
            std::cerr << "Error getting username: " << e.what() << std::endl;
            sessionPtr->erase("logged_in");
            sessionPtr->erase("username");
            auto resp = drogon::HttpResponse::newRedirectionResponse("/");
            callback(resp);
            return;
        }
    } else {
        std::cerr << "username not found in session" << std::endl;
        sessionPtr->erase("logged_in");
        auto resp = drogon::HttpResponse::newRedirectionResponse("/");
        callback(resp);
        return;
    }
    // Проверяем, существует ли пользователь в базе данных
    if (!db.userExists(username)) {
        // Если пользователь не найден в БД, очищаем сессию
        sessionPtr->erase("logged_in");
        sessionPtr->erase("username");
        auto resp = drogon::HttpResponse::newRedirectionResponse("/");
        callback(resp);
        return;
    }
    
    // Загружаем HTML-файл со страницей кейсов
    std::ifstream file("views/cases.html");
    if (!file.is_open()) {
        // Если файл не найден, возвращаем 404
        auto resp = drogon::HttpResponse::newNotFoundResponse();
        callback(resp);
        return;
    }
    

    std::cout << "Session data: logged_in=" << sessionPtr->get<bool>("logged_in")
          << ", username=" << sessionPtr->get<std::string>("username") << std::endl;


    // Читаем содержимое файла
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    std::string htmlContent = buffer.str();
    
    // Получаем данные пользователя из базы данных
    auto userData = db.getUserData(username);
    
    // Заменяем плейсholders в HTML (если они есть)
    if (!userData.empty()) {
        size_t pos = htmlContent.find("{{username}}");
        if (pos != std::string::npos) {
            htmlContent.replace(pos, 12, userData["username"]);
        }
        
        pos = htmlContent.find("{{avatar}}");
        if (pos != std::string::npos) {
            htmlContent.replace(pos, 10, userData["avatar"]);
        }
    }
    
    // Создаем и отправляем ответ
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(htmlContent);
    
    // Устанавливаем заголовки для предотвращения кеширования
    resp->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    resp->addHeader("Pragma", "no-cache");
    resp->addHeader("Expires", "0");
    
    callback(resp);
});

// Обработчик для получения информации о текущем пользователе
drogon::app().registerHandler("/user_info", [](const drogon::HttpRequestPtr &req,
                                               std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto sessionPtr = req->session();
    
    // Проверяем авторизацию
    if (!sessionPtr) {
        Json::Value errorResponse;
        errorResponse["error"] = "Не авторизован";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorResponse);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }
    
    bool isLoggedIn = false;
    std::string username;
    
    try {
        isLoggedIn = sessionPtr->get<bool>("logged_in");
        username = sessionPtr->get<std::string>("username");
    } catch (const std::exception&) {
        Json::Value errorResponse;
        errorResponse["error"] = "Неверная сессия";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorResponse);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }
    
    if (!isLoggedIn || username.empty()) {
        Json::Value errorResponse;
        errorResponse["error"] = "Не авторизован";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(errorResponse);
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }
    
    // Получаем данные пользователя из базы данных
    auto userData = db.getUserData(username);
    auto userItems = db.getUserItems(username);
    
    Json::Value response;
    response["username"] = userData["username"];
    response["avatar"] = userData["avatar"];
    response["items"] = Json::arrayValue;
    
    for (const auto& item : userItems) {
        response["items"].append(item);
    }
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
});

drogon::app().registerHandler("/open_case", [&usedItemsMap](const drogon::HttpRequestPtr &req,
                                                          std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto sessionPtr = req->session();
    try {
        bool loggedIn = sessionPtr->get<bool>("logged_in");
        if (!loggedIn) {
            auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value());
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
            return;
        }
    } catch (const std::exception& e) {
        auto resp = drogon::HttpResponse::newHttpJsonResponse(Json::Value());
        resp->setStatusCode(drogon::k401Unauthorized);
        callback(resp);
        return;
    }
    
    auto caseId = req->getParameter("case_id");
    std::vector<std::string> items;

    if (caseId == "1") {
        items = {"/static/images/item1.jpg", "/static/images/item2.jpg", "/static/images/item3.jpg",
                 "/static/images/item4.jpg", "/static/images/item5.jpg", "/static/images/item6.jpg",
                 "/static/images/item7.jpg", "/static/images/item8.jpg", "/static/images/item9.jpg",
                 "/static/images/item10.jpg", "/static/images/item11.jpg", "/static/images/item12.jpg",
                 "/static/images/item13.jpg", "/static/images/item14.jpg", "/static/images/item15.jpg"};
    } else if (caseId == "2") {
        items = {"/static/images/itm1.jpg", "/static/images/itm2.jpg", "/static/images/itm3.jpg",
                 "/static/images/itm4.jpg", "/static/images/itm5.jpg", "/static/images/itm6.jpg",
                 "/static/images/itm7.jpg"};
    } else if (caseId == "3") {
        items = {"/static/images/it1.jpg", "/static/images/it2.jpg", "/static/images/it3.jpg"};
    } else {
        auto resp = drogon::HttpResponse::newNotFoundResponse();
        callback(resp);
        return;
    }

    auto &usedItems = usedItemsMap[caseId];

    Json::Value jsonResponse;
    jsonResponse["items"] = Json::arrayValue;
    for (const auto &item : items) {
        jsonResponse["items"].append(item);
    }

    jsonResponse["selectedItem"] = getRandomItem(items, usedItems);

    auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
    callback(resp);
});

drogon::app().registerHandler("/register", [](const drogon::HttpRequestPtr &req,
                                              std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    if (req->getMethod() != drogon::Post) {
        auto resp = drogon::HttpResponse::newNotFoundResponse();
        callback(resp);
        return;
    }

    auto username = req->getParameter("username");
    auto password = req->getParameter("password");
    auto avatar = req->getParameter("avatar");

    std::cout << "Registration attempt: username=" << username << ", password=" << password << ", avatar=" << avatar << std::endl;

    if (username.empty() || password.empty()) {
        std::cout << "Registration failed: empty username or password" << std::endl;

        Json::Value jsonResponse;
        jsonResponse["success"] = false;
        jsonResponse["message"] = "Пожалуйста, заполните все поля";

        auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        callback(resp);
        return;
    }

    Json::Value jsonResponse;
    if (db.registerUser(username, password, avatar.empty() ? "/static/images/default_avatar.jpg" : avatar)) {
        auto sessionPtr = req->session();
        sessionPtr->insert("username", username);
        sessionPtr->insert("logged_in", true);

        std::cout << "Session ID: " << sessionPtr->sessionId() << std::endl;
        std::cout << "Verifying session data..." << std::endl;
        try {
            bool testLoggedIn = sessionPtr->get<bool>("logged_in");
            std::string testUsername = sessionPtr->get<std::string>("username");
            std::cout << "Session verification successful: logged_in=" << testLoggedIn << ", username=" << testUsername << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Session verification failed: " << e.what() << std::endl;
        }

        std::cout << "Registration successful: username=" << username << std::endl;
        std::cout << "Session 'logged_in' set to true" << std::endl;

        jsonResponse["success"] = true;
        jsonResponse["message"] = "Регистрация прошла успешно!";
    } else {
        std::cout << "Registration failed: database error" << std::endl;

        jsonResponse["success"] = false;
        jsonResponse["message"] = "Ошибка при регистрации";
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
    callback(resp);
});

drogon::app().registerHandler("/login", [](const drogon::HttpRequestPtr &req,
                                           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    if (req->getMethod() != drogon::Post) {
        auto resp = drogon::HttpResponse::newNotFoundResponse();
        callback(resp);
        return;
    }

    auto username = req->getParameter("username");
    auto password = req->getParameter("password");

    std::cout << "Login attempt: username=" << username << ", password=" << password << std::endl;

    if (username.empty() || password.empty()) {
        std::cout << "Login failed: empty username or password" << std::endl;

        Json::Value jsonResponse;
        jsonResponse["success"] = false;
        jsonResponse["message"] = "Пожалуйста, заполните все поля";

        auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        callback(resp);
        return;
    }

    Json::Value jsonResponse;
    if (db.authenticateUser(username, password)) {
        auto sessionPtr = req->session();
        sessionPtr->insert("username", username);
        sessionPtr->insert("logged_in", true);

        std::cout << "Login successful: username=" << username << std::endl;
        std::cout << "Session 'logged_in' set to true" << std::endl;

        jsonResponse["success"] = true;
        jsonResponse["message"] = "Вход выполнен успешно!";
    } else {
        std::cout << "Login failed: invalid username or password" << std::endl;

        jsonResponse["success"] = false;
        jsonResponse["message"] = "Неверное имя пользователя или пароль";
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
    callback(resp);
});

// ДОБАВИТЬ ОБРАБОТЧИК ВЫХОДА:
drogon::app().registerHandler("/logout", [](const drogon::HttpRequestPtr &req,
                                          std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto sessionPtr = req->session();
    sessionPtr->erase("username");
    sessionPtr->erase("logged_in");
    
    auto resp = drogon::HttpResponse::newRedirectionResponse("/");
    callback(resp);
});
    drogon::app().addListener("0.0.0.0", 8080);
    drogon::app().run();
}