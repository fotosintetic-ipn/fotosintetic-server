#include <crow.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <nlohmann/json.hpp>

#include <chrono>

#include "../include/dbschema.h"

int main(){
    auto config = std::make_shared<sqlpp::mysql::connection_config>();
    config -> database = "polih";
    config -> user = "pez";
    config -> password = "pez";
    config -> host = "localhost";
    config -> debug = true;
    auto db = sqlpp::mysql::connection(config);

    crow::SimpleApp app;

    CROW_ROUTE(app, "/submit").methods(crow::HTTPMethod::POST)([&db](const crow::request& req){
        if(req.body == "") return crow::response(crow::status::BAD_REQUEST);
        auto body = nlohmann::json::parse(req.body);
        if(req.get_header_value("Authorization") == ""
        || !body.contains("ph")
        || !body["ph"].is_array()){
            return crow::response(crow::status::BAD_REQUEST);
        }

        std::string auth = req.get_header_value("Authorization").substr(6);
        std::string decoded_auth = crow::utility::base64decode(auth, auth.size());
        
        uint64_t found = decoded_auth.find(':');
        std::string device_name = decoded_auth.substr(0, found);
        std::string password = decoded_auth.substr(found + 1);
        
        auto dev = polih::Devices{};
        auto rows = db(select(all_of(dev)).from(dev).where(dev.deviceName == device_name and dev.password == password));

        if(rows.empty())
            return crow::response(crow::status::UNAUTHORIZED);

        int32_t device_id = rows.front().id;

        auto data = polih::Data{};
        db(insert_into(data).set(data.deviceId = device_id,
                                 data.ph = body["ph"].dump()));

        return crow::response(crow::status::OK);
    });

    CROW_ROUTE(app, "/register_device").methods(crow::HTTPMethod::POST)([&db](const crow::request& req){
        if(req.url_params.get("device_name") == nullptr
        || req.url_params.get("password") == nullptr){
            return crow::response(crow::status::BAD_REQUEST);
        }

        std::string device_name = req.url_params.get("device_name");
        std::string password = req.url_params.get("password");

        auto dev = polih::Devices{};
        auto rows = db(select(all_of(dev)).from(dev).where(dev.deviceName == device_name));
        
        if(!rows.empty())
            return crow::response(crow::status::CONFLICT);
        
        db(insert_into(dev).set(dev.deviceName = device_name,
                                dev.password = password));

        return crow::response(crow::status::OK);
    });

    CROW_ROUTE(app, "/get_records").methods(crow::HTTPMethod::GET)([&db](const crow::request& req){
        if(req.get_header_value("Authorization") == ""){
            return crow::response(crow::status::BAD_REQUEST);
        }

        std::string auth = req.get_header_value("Authorization").substr(6);
        std::string decoded_auth = crow::utility::base64decode(auth, auth.size());
        
        uint64_t found = decoded_auth.find(':');
        std::string device_name = decoded_auth.substr(0, found);
        std::string password = decoded_auth.substr(found + 1);
        
        auto dev = polih::Devices{};
        auto rows = db(select(all_of(dev)).from(dev).where(dev.deviceName == device_name and dev.password == password));

        if(rows.empty())
            return crow::response(crow::status::UNAUTHORIZED);

        uint32_t id = rows.front().id;

        uint32_t limit;
        if(req.url_params.get("limit") == nullptr)
            limit = 100;
        else
            limit = std::stoi(req.url_params.get("limit"));
        limit = std::min(limit, (uint32_t)100);

        auto data = polih::Data{};
        nlohmann::json response_body;

        try{
            auto result = db(select(all_of(data))
                            .from(data)
                            .where(data.deviceId == id)
                            .limit(limit)
                            .order_by(data.entryId.asc()));
            int idx = 0;

            response_body["data"] = nlohmann::json::array();

            for(const auto& i: result){
                response_body["data"][idx]["entry_id"] = static_cast<uint32_t>(i.entryId);
                
                auto array = nlohmann::json::parse(static_cast<std::string>(i.ph));
                response_body["data"][idx]["ph"] = nlohmann::json::array();
                for(int it = 0; it != 10; it++)
                    response_body["data"][idx]["ph"][it] = array[it];

                idx++;
            }
        }
        catch(sqlpp::exception& e){
            response_body["data"] = nlohmann::json::array();
        }

        crow::response res;
        res.code = crow::status::OK;
        res.body = response_body.dump();

        return res;
    });

    CROW_ROUTE(app, "/validate_credentials").methods(crow::HTTPMethod::Get)([&db](const crow::request& req){
        if(req.url_params.get("device_name") == nullptr
        || req.url_params.get("password") == nullptr){
            return crow::response(crow::status::BAD_REQUEST);
        }

        std::string device_name = req.url_params.get("device_name");
        std::string password = req.url_params.get("password");

        auto dev = polih::Devices{};
        auto rows = db(select(all_of(dev)).from(dev).where(dev.deviceName == device_name and dev.password == password));
        
        if(!rows.empty())
            return crow::response(crow::status::OK);
        return crow::response(crow::status::UNAUTHORIZED);
    });

    auto ret = app.port(18080).multithreaded().run_async();
}