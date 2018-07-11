/*
 * This file is part of the trojan project.
 * Trojan is an unidentifiable mechanism that helps you bypass GFW.
 * Copyright (C) 2018  GreaterFire
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "authenticator.h"
#include <cstdlib>
#include <stdexcept>
using namespace std;

Authenticator::Authenticator(const Config &config) {
    mysql_init(&con);
    Log::log_with_date_time("connecting to MySQL server " + config.mysql.server_addr + ':' + to_string(config.mysql.server_port), Log::INFO);
    if (mysql_real_connect(&con, config.mysql.server_addr.c_str(),
                                 config.mysql.username.c_str(),
                                 config.mysql.password.c_str(),
                                 config.mysql.database.c_str(),
                                 config.mysql.server_port, NULL, 0) == NULL) {
        throw runtime_error(mysql_error(&con));
    }
    my_bool reconnect = 1;
    mysql_options(&con, MYSQL_OPT_RECONNECT, &reconnect);
    Log::log_with_date_time("connected to MySQL server", Log::INFO);
}

bool Authenticator::auth(const string &password) {
    if (!is_valid_password(password)) {
        return false;
    }
    if (mysql_query(&con, ("SELECT quota, download + upload FROM users WHERE password = '" + password + '\'').c_str())) {
        Log::log_with_date_time(mysql_error(&con), Log::WARN);
        return false;
    }
    MYSQL_RES *res = mysql_store_result(&con);
    if (res == NULL) {
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    int64_t quota = atoll(row[0]);
    int64_t used = atoll(row[1]);
    mysql_free_result(res);
    return quota < 0 || used < quota;
}

void Authenticator::record(const std::string &password, uint64_t download, uint64_t upload) {
    if (!is_valid_password(password)) {
        return;
    }
    if (mysql_query(&con, ("UPDATE users SET download = download + " + to_string(download) + ", upload = upload + " + to_string(upload) + " WHERE password = '" + password + '\'').c_str())) {
        Log::log_with_date_time(mysql_error(&con), Log::WARN);
    }
}

bool Authenticator::is_valid_password(const std::string &password) {
    if (password.size() != PASSWORD_LENGTH) {
        return false;
    }
    for (size_t i = 0; i < PASSWORD_LENGTH; ++i) {
        if (!((password[i] >= '0' && password[i] <= '9') || (password[i] >= 'a' && password[i] <= 'f'))) {
            return false;
        }
    }
    return true;
}

Authenticator::~Authenticator() {
    mysql_close(&con);
}