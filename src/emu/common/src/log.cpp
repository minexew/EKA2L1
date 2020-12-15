/*
 * Copyright (c) 2018 EKA2L1 Team.
 * 
 * This file is part of EKA2L1 project 
 * (see bentokun.github.com/EKA2L1).
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <common/log.h>
#include <common/platform.h>

#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>

#include <iostream>
#include <memory>

#ifdef _MSC_VER
#include <spdlog/sinks/msvc_sink.h>
#elif EKA2L1_PLATFORM_ANDROID
#include "spdlog/sinks/android_sink.h"
#endif

#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace eka2l1 {
    bool already_setup = false;

    const char *log_class_to_string(const log_class cls) {
        if (cls >= LOG_CLASS_COUNT) {
            return nullptr;
        }

        #define LOGCLASS(name, short_nice_name, nice_name) short_nice_name,
        static const char *LOG_CLASS_NAME_ARRAYS[LOG_CLASS_COUNT] = {
            #include <common/logclass.def>
        };

        return LOG_CLASS_NAME_ARRAYS[static_cast<int>(cls)];
    }

    log_filterings::log_filterings() {
        std::fill(levels_, levels_ + LOG_CLASS_COUNT, spdlog::level::trace);
        levels_[SERVICE_EFSRV] = spdlog::level::off;
    }

    bool log_filterings::set_minimum_level(const log_class cls, const spdlog::level::level_enum level) {
        if (cls >= LOG_CLASS_COUNT) {
            return false;
        }

        levels_[static_cast<int>(cls)] = level;
        return true;
    }

    bool log_filterings::is_passed(const log_class cls, const spdlog::level::level_enum level) {
        return (cls < LOG_CLASS_COUNT) && (level >= levels_[static_cast<int>(cls)]);
    }

    namespace log {
        std::shared_ptr<spdlog::logger> spd_logger;
        std::unique_ptr<log_filterings> filterings;

        struct imgui_logger_sink : public spdlog::sinks::base_sink<std::mutex> {
            explicit imgui_logger_sink(std::shared_ptr<base_logger> _logger)
                : logger(_logger.get()) {}

            void set_force_clear(bool clear) {
                force_clear = clear;
            }

        private:
            base_logger *logger;
            bool force_clear = false;

        protected:
            void sink_it_(const spdlog::details::log_msg &msg) override {
                spdlog::memory_buf_t formatted;
                base_sink<std::mutex>::formatter_->format(msg, formatted);

                const std::string real_msg = fmt::to_string(formatted);
                logger->log(real_msg.c_str());

                if (force_clear) {
                    flush_();
                }
            }

            void flush_() override {
                //logger->clear();
            }
        };

        void setup_log(std::shared_ptr<base_logger> gui_logger) {
            const char *log_file_name = "EKA2L1.log";

            std::vector<spdlog::sink_ptr> sinks;

            if (gui_logger) {
                sinks.push_back(std::make_shared<imgui_logger_sink>(gui_logger));
            }

#if EKA2L1_PLATFORM(WIN32)
            DeleteFileA(log_file_name);
#else
            remove(log_file_name);
#endif

            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

            sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_name));

#ifdef _MSC_VER
            sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_st>());
#elif EKA2L1_PLATFORM(ANDROID)
            sinks.push_back(std::make_shared<spdlog::sinks::android_sink_mt>());
#endif

            spd_logger = std::make_unique<spdlog::logger>("EKA2L1 Logger", begin(sinks), end(sinks));
            spdlog::set_default_logger(spd_logger);

            spdlog::set_error_handler([](const std::string &msg) {
                std::cerr << "spdlog error: " << msg << std::endl;
            });

            spdlog::set_pattern("%L %^%v%$");
            spdlog::set_level(spdlog::level::trace);

            spd_logger->flush_on(spdlog::level::debug);

            // Setup the filterings
            filterings = std::make_unique<log_filterings>();
            already_setup = true;
        }
    }
}
