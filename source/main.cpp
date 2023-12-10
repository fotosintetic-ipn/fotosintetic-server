#include <crow.h>

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

#include <nlohmann/json.hpp>

#include <chrono>

#include "../include/dbschema.h"

int main(){
    auto config = std::make_shared<sqlpp::mysql::connection_config>();
    config -> database = "";
    config -> user = "";
    config -> password = "";
    config -> host = "";
    config -> debug = true;
    auto db = sqlpp::mysql::connection(config);

    crow::SimpleApp app;

    CROW_ROUTE(app, "/submit").methods(crow::HTTPMethod::POST)([&db](const crow::request& req){
        if(req.body == "") return crow::response(crow::status::BAD_REQUEST);
        auto body = nlohmann::json::parse(req.body);
        if(req.get_header_value("Authorization") == ""
        || !body.contains("heartRatePrecision")
        || !body.contains("spo2Precision")
        || !body.contains("heartRateArray")
        || !body.contains("spo2Array")
        || !body.contains("sent")
        || !body["heartRatePrecision"].is_number()
        || !body["spo2Precision"].is_number()
        || !body["heartRateArray"].is_array()
        || !body["spo2Array"].is_array()
        || !body["sent"].is_string()){
            return crow::response(crow::status::BAD_REQUEST);
        }

        std::string auth = req.get_header_value("Authorization").substr(6);
        std::string decoded_auth = crow::utility::base64decode(auth, auth.size());
        
        uint64_t found = decoded_auth.find(':');
        std::string username = decoded_auth.substr(0, found);
        std::string password = decoded_auth.substr(found + 1);
        
        auto u = oxim::Users{};
        auto rows = db(select(all_of(u)).from(u).where(u.username == username and u.password == password));

        if(rows.empty())
            return crow::response(crow::status::UNAUTHORIZED);

        int32_t user_id = rows.front().id;

        auto vs = oxim::VitalSigns{};
        db(insert_into(vs).set(vs.userId = user_id,
                               vs.heartrate = body["heartRateArray"].dump(),
                               vs.spo2 = body["spo2Array"].dump(),
                               vs.heartratePrecision = static_cast<double>(body["heartRatePrecision"]),
                               vs.spo2_precision = static_cast<double>(body["spo2Precision"]),
                               vs.received = static_cast<std::string>(body["sent"])));

        return crow::response(crow::status::OK);
    });

    CROW_ROUTE(app, "/create_account").methods(crow::HTTPMethod::POST)([&db](const crow::request& req){
        if(req.url_params.get("username") == nullptr
        || req.url_params.get("password") == nullptr){
            return crow::response(crow::status::BAD_REQUEST);
        }

        std::string username = req.url_params.get("username");
        std::string password = req.url_params.get("password");

        auto u = oxim::Users{};
        auto rows = db(select(all_of(u)).from(u).where(u.username == username));
        
        if(!rows.empty())
            return crow::response(crow::status::CONFLICT);
        
        db(insert_into(u).set(u.username = username,
                              u.password = password));

        return crow::response(crow::status::OK);
    });

    CROW_ROUTE(app, "/get_records").methods(crow::HTTPMethod::GET)([&db](const crow::request& req){
        if(req.get_header_value("Authorization") == ""
        || (req.url_params.get("before") == nullptr && req.url_params.get("after") == nullptr)
        || (req.url_params.get("before") != nullptr && req.url_params.get("after") != nullptr)){
            return crow::response(crow::status::BAD_REQUEST);
        }
        try{
            uint32_t aux;
            if(req.url_params.get("limit") != nullptr)
                aux = std::stoi(req.url_params.get("limit"));
            if(req.url_params.get("after") != nullptr)
                aux = std::stoi(req.url_params.get("after"));
            else
                aux = std::stoi(req.url_params.get("before"));
        }
        catch(std::invalid_argument& e){
            return crow::response(crow::status::BAD_REQUEST);
        }

        std::string auth = req.get_header_value("Authorization").substr(6);
        std::string decoded_auth = crow::utility::base64decode(auth, auth.size());
        
        uint64_t found = decoded_auth.find(':');
        std::string username = decoded_auth.substr(0, found);
        std::string password = decoded_auth.substr(found + 1);
        
        auto u = oxim::Users{};
        auto rows = db(select(all_of(u)).from(u).where(u.username == username and u.password == password));

        if(rows.empty())
            return crow::response(crow::status::UNAUTHORIZED);

        uint32_t id = rows.front().id;

        uint32_t limit;
        if(req.url_params.get("limit") == nullptr)
            limit = 100;
        else
            limit = std::stoi(req.url_params.get("limit"));
        limit = std::min(limit, (uint32_t)100);

        auto vs = oxim::VitalSigns{};
        nlohmann::json response_body;

        try{
            if(req.url_params.get("before") != nullptr){
                auto result = db(select(all_of(vs))
                                .from(vs)
                                .where(vs.userId == id and vs.id < std::stoi(req.url_params.get("before")))
                                .limit(limit)
                                .order_by(vs.id.desc()));
                int idx = 0;

                response_body["data"] = nlohmann::json::array();

                for(const auto& i: result){
                    response_body["data"][idx]["id"] = static_cast<uint32_t>(i.id);
                    response_body["data"][idx]["received"] = static_cast<std::string>(i.received);
                    
                    auto array = nlohmann::json::parse(static_cast<std::string>(i.heartrate));
                    response_body["data"][idx]["heartrate"] = nlohmann::json::array();
                    for(int it = 0; it != 10; it++)
                        response_body["data"][idx]["heartrate"][it] = array[it];
                    array = nlohmann::json::parse(static_cast<std::string>(i.spo2));
                    response_body["data"][idx]["spo2"] = nlohmann::json::array();
                    for(int it = 0; it != 10; it++)
                        response_body["data"][idx]["spo2"][it] = array[it];

                    response_body["data"][idx]["heartrate_precision"] = static_cast<double>(i.heartratePrecision);
                    response_body["data"][idx]["spo2_precision"] = static_cast<double>(i.spo2_precision);

                    idx++;
                }
                std::reverse(response_body["data"].begin(), response_body["data"].end());
            }
            else{
                auto result = db(select(all_of(vs))
                                .from(vs)
                                .where(vs.userId == id and vs.id > std::stoi(req.url_params.get("after")))
                                .limit(limit)
                                .order_by(vs.id.asc()));
                int idx = 0;

                response_body["data"] = nlohmann::json::array();

                for(const auto& i: result){
                    response_body["data"][idx]["id"] = static_cast<uint32_t>(i.id);
                    response_body["data"][idx]["received"] = static_cast<std::string>(i.received);
                    
                    auto array = nlohmann::json::parse(static_cast<std::string>(i.heartrate));
                    response_body["data"][idx]["heartrate"] = nlohmann::json::array();
                    for(int it = 0; it != 10; it++)
                        response_body["data"][idx]["heartrate"][it] = array[it];
                    response_body["data"][idx]["spo2"] = nlohmann::json::array();
                    array = nlohmann::json::parse(static_cast<std::string>(i.spo2));
                    for(int it = 0; it != 10; it++)
                        response_body["data"][idx]["spo2"][it] = array[it];

                    response_body["data"][idx]["heartrate_precision"] = static_cast<double>(i.heartratePrecision);
                    response_body["data"][idx]["spo2_precision"] = static_cast<double>(i.spo2_precision);

                    idx++;
                }   
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
        if(req.url_params.get("username") == nullptr
        || req.url_params.get("password") == nullptr){
            return crow::response(crow::status::BAD_REQUEST);
        }

        std::string username = req.url_params.get("username");
        std::string password = req.url_params.get("password");

        auto u = oxim::Users{};
        auto rows = db(select(all_of(u)).from(u).where(u.username == username and u.password == password));
        
        if(!rows.empty())
            return crow::response(crow::status::OK);
        return crow::response(crow::status::UNAUTHORIZED);
    });

    auto ret = app.port(18080).multithreaded().run_async();
}